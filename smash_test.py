from subprocess import Popen, PIPE
from signal import SIGKILL, SIGTSTP, SIGINT
from errno import ETIME
import psutil
import sys
from time import sleep
from os import path, strerror
from functools import wraps
import signal

#NEXT = ['>', ' ', '\x1B', '[', '0', 'm']

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
    def __init__(self, executable, test_path, NEXT="> \x1B[0m", out_path=sys.stdout):
        self.test_file = open(test_path, "r")
        if (isinstance(out_path, str)):
            self.out_file = open(out_path, "w")
            self.err_file = self.out_file
        else:
            self.out_file = sys.stdout
            self.err_file = sys.stdout
        self.shell_process = Popen(executable,
                                   stdout=PIPE,  # self.out_file,
                                   stdin=PIPE,
                                   stderr=PIPE,  # STDOUT,
                                   shell=True,
                                   universal_newlines=True)
        sleep(0.05)
        self.smash_process = None
        smashes = getProcess(path.basename(executable))
        if len(smashes) > 1:
            raise Exception("There is smash already running, use 'pkill -9 smash'")
        if not smashes:
            raise Exception("There no smash running")
        self.smash_process = smashes[0]
        self.NEXT = list(NEXT.encode().decode('unicode-escape'))
        self.NONE = [None] * len(self.NEXT)
        self.last = self.NONE[:]

    def execute(self, command):
        self.out_file.write('\x1b[1;32m' + command + '\x1b[0m')
        self.out_file.write('\n')
        self.out_file.flush()

        # time.sleep(0.1)

        self.shell_process.stdin.write(command)
        self.shell_process.stdin.write('\n')
        self.shell_process.stdin.flush()

        # if command.lstrip().startswith('sleep') and '&' not in command.split()[-1]:
        #   time.sleep(int(command.split()[1]) + 1)

    def signal(self, signal_type):
        sleep(0.05)
        self.smash_process.send_signal(signal_type)
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
            except TimeoutError as e1:
                try:
                    err = self.read_stderr()
                except TimeoutError as e2:
                    return
            if out:
                self.push(out)
                self.out_file.write(out)  # '\x1b[1;37m' + out + '\x1b[0m'
                self.out_file.flush()

            if err:
                self.err_file.write('\x1b[1;31m' + err + '\x1b[0m')
                sys.stderr.flush()

            if out == '' and self.shell_process.poll() is not None:
                return

    def read_and_wait(self):
        while self.last != self.NEXT:
            self.read()
            timeout(0.02)

    def run(self):
        lines = self.test_file.read().splitlines()
        lines_num = len(lines)
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


if __name__ == "__main__":
    smash = SmashRunner(*sys.argv[1:4])
    smash.run()
