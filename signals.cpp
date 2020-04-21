#include <iostream>
#include <signal.h>
#include <sstream>
#include "signals.h"
#include "Commands.h"
#include "system_functions.h"

using namespace std;

void ctrlZHandler(int sig_num) {
    auto &smash = SmallShell::getInstance();
    if(smash.pid!=getpid()){
       // do_kill(getpid(), SIGSTOP);
        return;
    }
    cout << "smash: got ctrl-Z" << endl;
    auto &job = smash.fg_job;
    if(!job.pid)
        return;
    if (job.pid) {
        auto job_in_list = smash.jobs_list.getJobByPID(job.pid);
        if (job_in_list) { //was before in bg
            job_in_list->Kill(SIGSTOP);
        }
        else { //never was in bg
            smash.jobs_list.addJob(job);
            auto job_added = smash.jobs_list.getJobByPID(job.pid);
            job_added->Kill(SIGSTOP);
        }
        cout << "smash: process " << job.pid << " was stopped" << endl;
        smash.fg_job = JobEntry();
    }
}

void ctrlCHandler(int sig_num) {
    auto &smash = SmallShell::getInstance();
    if (smash.pid != getpid()) {
        do_kill(getpid(), SIGKILL);
        return;
    }
    cout << "smash: got ctrl-C"<<endl;
    auto &job = smash.fg_job;
    if(job.pid) {
        job.Kill(SIGKILL);
        cout << "smash: process " <<smash.fg_job.pid << " was killed" <<endl;
        smash.fg_job = JobEntry();
    }
}

void alarmHandler(int sig_num) {
    auto& smash=SmallShell::getInstance();
    if(smash.pid!=getpid())
        return;
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

