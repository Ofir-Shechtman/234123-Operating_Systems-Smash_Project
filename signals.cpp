#include <iostream>
#include <signal.h>
#include <sstream>
#include "signals.h"
#include "Commands.h"
#include "system_functions.h"

using namespace std;

void ctrlZHandler(int sig_num) {
    cout << "smash: got ctrl-Z" << endl;
    auto &smash = SmallShell::getInstance();
    if (smash.fg_pid) {
        kill(smash.fg_pid, SIGSTOP);
        smash.fg_cmd->bg = true;
        auto &job_list = SmallShell::getInstance().jobs_list;
        auto job = job_list.getJobByPID(smash.fg_pid);
        if (job) { //was before in bg
            job->isStopped = true;
        } else {//never was in bg
            job_list.addJob(smash.fg_cmd, smash.fg_pid, true);
        }
        cout << "smash: process " << smash.fg_pid << " was stopped" << endl;
        smash.set_fg(0, nullptr);
    }
}

void ctrlCHandler(int sig_num) {
    cout << "smash: got ctrl-C"<<endl;
    auto& smash=SmallShell::getInstance();
    if(smash.fg_pid) {
        kill(smash.fg_pid, SIGKILL);
        cout << "smash: process " <<smash.fg_pid << " was killed" <<endl;
        smash.set_fg(0, nullptr);
    }
}

void alarmHandler(int sig_num) {
    auto& smash=SmallShell::getInstance();
    if(smash.min_time_job_pid == smash.fg_pid){
        cout << "smash: got an alarm"<<endl;
        cout <<"smash: "<< smash.fg_cmd->get_cmd_line() << " timed out!" <<endl;
        smash.jobs_list.removeTimedoutJob(-1);
    }
    else if(smash.min_time_job_pid) {
        auto job = smash.jobs_list.getJobByPID(smash.min_time_job_pid);
        if(!job) {
            smash.jobs_list.removeFinishedTimedjobs();
            smash.set_min_time_job_pid(0);
            return;
        }
        cout << "smash: got an alarm"<<endl;
        cout <<"smash: timeout "<<job->timer<<" "<< job->cmd->get_cmd_line() << " timed out!" <<endl;
        smash.jobs_list.removeTimedoutJob();
    }
}

