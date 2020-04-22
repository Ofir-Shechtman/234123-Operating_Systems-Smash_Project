from subprocess import Popen, PIPE
from signal import SIGKILL, SIGTSTP, SIGINT
from errno import ETIME
import psutil
from time import sleep
import sys
from tempfile import mkstemp
from shutil import move, copymode
from os import fdopen, remove
import argparse
from os import path, strerror
from functools import wraps
import signal

RED =   "\x1b[1;31m"
GREEN = "\x1b[1;32m"
CYAN =  "\x1b[1;36m"
WHITE =  "\x1b[0;37m"
RETURN ="\x1b[0m"


def add_color(text, color):
    return color + text + RETURN


class TimeoutError(Exception):
    pass


def timeout(seconds=10, error_message=strerror(ETIME)):
    def decorator(func):
        def _handle_timeout(signum, frame):
            raise TimeoutError(error_message)

        def wrapper(*args, **kwargs):
            signal.signal(signal.SIGALRM, _handle_timeout)
            signal.setitimer(signal.ITIMER_REAL, seconds)  # used timer instead of alarm
            try:
                result = func(*args, **kwargs)
            finally:
                signal.alarm(0)
            return result

        return wraps(func)(wrapper)

    return decorator


def getProcess(name):
    smashes = []
    for p in psutil.process_iter(attrs=['name']):
        try:
            if name == p.info['name']:
                smashes.append(p)
        except (psutil.NoSuchProcess, psutil.AccessDenied, psutil.ZombieProcess):
            pass
    return smashes


class SmashRunner:
    def __init__(self, executable, test_path, NEXT="> \x1B[0m", color=True, valgrind=False, out_path=sys.stdout):
        self.test_file = open(test_path, "r")
        if isinstance(out_path, str):
            self.out_file = open(out_path, "w")
            self.err_file = self.out_file
        else:
            self.out_file = sys.stdout
            self.err_file = sys.stdout
        if valgrind:
            process_name = "memcheck-amd64-"
            executable = "valgrind --leak-check=full --show-leak-kinds=all --leak-resolution=med --track-origins=yes " \
                         "--vgdb=no " + executable
        else:
            process_name = path.basename(executable)
        self.shell_process = Popen(executable,
                                   stdout=PIPE,  # self.out_file,
                                   stdin=PIPE,
                                   stderr=PIPE,  # STDOUT,
                                   shell=True,
                                   universal_newlines=True)
        sleep(0.05)
        self.smash_process = None
        smashes = getProcess(process_name)
        if len(smashes) > 1:
            raise Exception("There is smash already running, try 'pkill -9 smash'")
        if not smashes:
            raise Exception("There no smash running")
        self.smash_process = smashes[0]
        self.NEXT = list(NEXT.encode().decode('unicode-escape'))
        self.NONE = [None] * len(self.NEXT)
        self.last = self.NONE[:]
        self.color = color

    def execute(self, command):
        color_command = command
        if self.color:
            color_command = add_color(command, WHITE)
        self.out_file.write(color_command)
        self.out_file.write('\n')
        self.out_file.flush()

        # time.sleep(0.1)

        self.shell_process.stdin.write(command)
        self.shell_process.stdin.write('\n')
        self.shell_process.stdin.flush()

        # if command.lstrip().startswith('sleep') and '&' not in command.split()[-1]:
        #   time.sleep(int(command.split()[1]) + 1)

    def signal(self, signum):
        sleep(0.05)
        self.smash_process.send_signal(signum)
        sleep(0.05)
        self.last = self.NONE[:]

    def close(self):
        self.test_file.close()
        self.out_file.close()
        try:
            self.signal(SIGKILL)
        except psutil.NoSuchProcess:
            pass
        try:
            self.shell_process.kill()
        except OSError:
            pass

    def push(self, c):
        size = len(self.NEXT) - 1
        for i in range(size):
            self.last[i] = self.last[i + 1]
        self.last[size] = c

    @timeout(0.01)
    def read_stdout(self):
        return self.shell_process.stdout.read(1)

    @timeout(0.01)
    def read_stderr(self):
        return self.shell_process.stderr.read(1)

    @timeout(1)
    def read(self):
        while True:
            out, err = None, None
            try:
                out = self.read_stdout()
            except TimeoutError:
                try:
                    err = self.read_stderr()
                except TimeoutError:
                    return
            if out:
                self.push(out)
                if self.color:
                    out = add_color(out, GREEN)
                self.out_file.write(out)  # '\x1b[1;37m' + out + '\x1b[0m'
                self.out_file.flush()

            if err:
                if self.color:
                    err = add_color(err, RED)
                self.err_file.write(err)
                sys.stderr.flush()

            if out == '' and self.shell_process.poll() is not None:
                return

    def read_and_wait(self):
        while self.last != self.NEXT:
            self.read()
            timeout(0.02)

    def run(self):
        lines = self.test_file.read().splitlines()
        for i, line in enumerate(lines):
            if line not in ["Ctrl-Z", "Ctrl-C"]:
                self.read_and_wait()
            else:
                self.read()
            if line.startswith("Ctrl-Z"):
                self.signal(SIGTSTP)
                continue
            elif line.startswith("Ctrl-C"):
                self.signal(SIGINT)
                continue
            else:

                self.last = self.NONE[:]
                self.execute(line)
        self.read()
        self.close()


class SmashTester:
    def __init__(self, args):
        self.executable = args.executable
        self.tests = args.input
        self.output = args.output
        self.reset = args.reset
        self.valgrind = args.valgrind
        self.next = args.next
        self.color = args.color or not self.output

    def run(self):
        if len(self.tests) > 1:
            for i, input_test in enumerate(self.tests):
                smash = SmashRunner(self.executable, input_test, self.next, self.color, self.valgrind,
                                    self.output + "{}.out".format(i))
                smash.run()
                if self.reset:
                    PidChanger(self.output + "{}.out".format(i)).pid_reset()
        else:
            if self.output:
                smash = SmashRunner(self.executable, self.tests[0], self.next, self.color, self.valgrind, self.output + ".out")
            else:
                smash = SmashRunner(self.executable, self.tests[0], self.next, self.color, self.valgrind)
            smash.run()
            if self.reset:
                PidChanger(self.output + ".out").pid_reset()


class PidChanger:
    def __init__(self, file_path):
        self.file_path = file_path
        self.pids = {}
        self.PID_SEQUENCE = 1000
        self.quit_kill = False

    def get_pid(self, pid):
        if pid not in self.pids:
            self.pids[pid] = self.PID_SEQUENCE
            self.PID_SEQUENCE += 1
        return self.pids[pid]

    def pid_replace(self, line, idx):
        sline = line.split()
        badpid = sline[idx].strip(":")
        if self.quit_kill:
            s = str(self.pids[badpid]) + ":"
            return line.replace(sline[idx], s)
        return line.replace(str(badpid), str(self.get_pid(badpid)))

    def parse_line(self, line):
        if " secs (stopped)" in line:
            line = self.pid_replace(line, -4)
        elif " secs" in line:
            line = self.pid_replace(line, -3)
        elif "was sent to pid" in line:
            line = self.pid_replace(line, -1)
        elif "smash pid" in line:
            line = self.pid_replace(line, -1)
        elif " : " in line:
            line = self.pid_replace(line, -1)
        elif " pts/" in line:
            line = self.pid_replace(line, 0)
        elif "was stopped" in line or "was killed" in line:
            line = self.pid_replace(line, -3)
        elif "sending SIGKILL signal to " in line:
            self.quit_kill = True
        elif self.quit_kill:
            line = self.pid_replace(line, 0)
        return line

    def pid_reset(self):
        # Create temp file
        fh, abs_path = mkstemp()
        with fdopen(fh, 'w') as new_file:
            with open(self.file_path) as old_file:
                for line in old_file:
                    new_file.write(self.parse_line(line))
        # Copy the file permissions from the old file to the new file
        copymode(self.file_path, abs_path)
        # Remove original file
        remove(self.file_path)
        # Move new file
        move(abs_path, self.file_path)


def create_parser():
    parser = argparse.ArgumentParser(description='Tester for OS hw1 - smash')
    parser.add_argument('executable', metavar='executable_file', type=str,
                        help='path to the executable file')
    parser.add_argument('--valgrind', '-v', action='store_true',
                        help='run valgrind, not working yet')
    parser.add_argument('--reset', '-r', action='store_true',
                        help='reset PIDs, works only on -o mode')
    parser.add_argument('--color', '-c', action='store_true',
                        help='add colors')
    parser.add_argument('--next', '-n', default="> ",
                        help='different template then "smash> "')
    parser.add_argument('input', metavar='input_file', type=str, nargs='+',
                        help='paths of input files')
    parser.add_argument('--output', '-o', nargs='?',
                        const='test_output',
                        help='if to write to a file [output name], no need extension.\nif omitted will print to stdout')

    return parser


class Arguments:
    pass


if __name__ == "__main__":
    args = Arguments()
    parser = create_parser()
    parser.parse_args(namespace=args)
    tester = SmashTester(args)
    tester.run()
