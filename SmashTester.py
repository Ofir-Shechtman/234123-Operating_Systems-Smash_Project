import sys
from smash_test import SmashRunner
from tempfile import mkstemp
from shutil import move, copymode
from os import fdopen, remove
import argparse


class SmashTester:
    def __init__(self, args):
        self.executable = args.executable
        self.tests = args.input
        self.output = args.output
        self.reset = args.reset
        self.valgrind = args.valgrind
        self.next = args.next
        self.pids = {}
        self.PID_SEQUENCE = 10000

    def run(self):
        if(len(self.tests)>1):
            for i, input in enumerate(self.tests):
                smash = SmashRunner(self.executable, input, self.next, self.output+"{}.out".format(i))
                smash.run()
                if self.reset:
                    self.pid_reset(self.output+"{}.out".format(i))
        else:
            if(self.output):
                smash = SmashRunner(self.executable, self.tests[0], self.next, self.output + ".out")
            else:
                smash = SmashRunner(self.executable, self.tests[0], self.next)
            smash.run()
            if self.reset:
                self.pid_reset(self.output + ".out")



    def get_pid(self, pid):
        if pid not in self.pids:
            self.pids[pid] = self.PID_SEQUENCE
            self.PID_SEQUENCE += 1
        return self.pids[pid]

    def pid_replace(self, line, idx):
        sline = line.split()
        badpid = sline[idx].strip(":")
        # if quitkill:
        #    s = str(pids[badpid]) + ":"
        #    return line.replace(sline[idx], s)
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
            # elif "sending SIGKILL signal to " in line:
            quitkill = True
        # elif quitkill:
        #    n = pid_replace(line, 0)
        return line

    def pid_reset(self, file_path):
        # Create temp file
        fh, abs_path = mkstemp()
        with fdopen(fh, 'w') as new_file:
            with open(file_path) as old_file:
                for line in old_file:
                    new_file.write(self.parse_line(line))
        # Copy the file permissions from the old file to the new file
        copymode(file_path, abs_path)
        # Remove original file
        remove(file_path)
        # Move new file
        move(abs_path, file_path)



def create_parser():
    parser = argparse.ArgumentParser(description='Tester for OS hw1 - smash')
    parser.add_argument('executable', metavar='executable_file', type=str,
                        help='path to the executable file')
    parser.add_argument('--valgrind', '-v', action='store_true',
                        help='run valgeind, not working yet')
    parser.add_argument('--reset', '-r', action='store_true',
                        help='reset PIDs, works only on -o mode')
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

