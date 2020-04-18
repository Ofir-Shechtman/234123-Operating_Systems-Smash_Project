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
        smash.fg_cmd->bg= true;
        auto& job_list=SmallShell::getInstance().jobs_list;
        auto job = job_list.getJobByPID(smash.fg_pid);
        if(job) { //was before in bg
            job->isStopped = true;
        }
        else {//never was in bg
            job_list.addJob(smash.fg_cmd, smash.fg_pid, true);
        }
        cout << "smash: process " <<smash.fg_pid << " was stopped" <<endl;
        //smash.set_fg(0, nullptr);
    }
}

void ctrlCHandler(int sig_num) {
    cout << "smash: got ctrl-C"<<endl;
    auto& smash=SmallShell::getInstance();
    if(smash.fg_pid) {
        kill(smash.fg_pid, SIGKILL);
        cout << "smash: process " <<smash.fg_pid << " was killed" <<endl;
        //smash.set_fg(0, nullptr);
    }
}

void alarmHandler(int sig_num) {
    cout << "smash: got an alarm"<<endl;
    auto& smash=SmallShell::getInstance();
    auto job = smash.jobs_list.getTimedoutJob();
    if((!job) && (smash.fg_pid)) {
        kill(smash.fg_pid, SIGKILL);
        cout <<"smash: "<< smash.fg_cmd->get_cmd_line() << " timed out!" <<endl;
    }
    else if(job){
        job->Kill(SIGKILL);
        cout <<"smash: "<< job->cmd->get_cmd_line() << " timed out!" <<endl;
    }
}

