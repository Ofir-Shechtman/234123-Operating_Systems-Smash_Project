#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"

using namespace std;

void ctrlZHandler(int sig_num) {
    cout << "smash: got ctrl-Z"<<endl;
    auto& smash=SmallShell::getInstance();
    if(smash.fg_pid) {
        kill(smash.fg_pid, SIGSTOP);
        smash.jobs_list.addJob(smash.fg_cmd, smash.fg_pid, true);
        cout << "smash: process " <<smash.fg_pid << " was stopped" <<endl;
        smash.fg_pid = 0;
    }
}

void ctrlCHandler(int sig_num) {
    cout << "smash: got ctrl-C"<<endl;
    auto& smash=SmallShell::getInstance();
    if(smash.fg_pid) {
        kill(smash.fg_pid, SIGKILL);
        cout << "smash: process " <<smash.fg_pid << " was killed" <<endl;
        smash.fg_pid = 0;
    }

}

void alarmHandler(int sig_num) {
  // TODO: Add your implementation
}

