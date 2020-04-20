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
    auto &job = smash.fg_job;
    if (job.pid) {
        auto job_in_list = smash.jobs_list.getJobByPID(job.pid);
        if (job_in_list) { //was before in bg
            job_in_list->Kill(SIGSTOP);
        }
        else { //never was in bg
            job.Kill(SIGSTOP);
            smash.jobs_list.addJob(job);
        }
        cout << "smash: process " << job.pid << " was stopped" << endl;
        smash.fg_job = JobEntry();
    }
}

void ctrlCHandler(int sig_num) {
    cout << "smash: got ctrl-C"<<endl;
    auto& smash=SmallShell::getInstance();
    auto &job = smash.fg_job;
    if(job.pid) {
        job.Kill(SIGKILL);
        cout << "smash: process " <<smash.fg_job.pid << " was killed" <<endl;
        smash.fg_job = JobEntry();
    }
}

void alarmHandler(int sig_num) {
    auto& smash=SmallShell::getInstance();
    smash.jobs_list.removeTimedoutJobs();
    smash.jobs_list.reset_timer();
    /*
    if(smash.min_time_job_pid == smash.fg_pid){
        smash.jobs_list.removeTimedoutJob(-1);
    }
    else if(smash.min_time_job_pid) {
        auto job = smash.jobs_list.getJobByPID(smash.min_time_job_pid);
        if(!job) {
            smash.jobs_list.removeFinishedTimedjobs();
            smash.set_min_time_job_pid(0);
            return;
        }
        smash.jobs_list.removeTimedoutJob();
    }
     */
}

