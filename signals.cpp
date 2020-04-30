#include "signals.h"


using namespace std;

void ctrlZHandler(int sig_num) {
    auto &smash = SmallShell::getInstance();
    if(smash.pid!=getpid()){
        return;
    }
    cout << "smash: got ctrl-Z" << endl;
    auto &job = smash.fg_job;
    if(!job.pid)
        return;
    if (job.pid) {
        auto job_in_list = smash.jobs_list.getJobByPID(job.pid);
        if (job_in_list) { //was before in bg
        }
        else { //never was in bg
            smash.jobs_list.addJob(job);
            job_in_list=smash.jobs_list.getJobByPID(job.pid);;
        }
        job_in_list->Kill(SIGSTOP);
        cout << "smash: process " << job.pid << " was stopped" << endl;
        smash.fg_job=JobEntry();//replace without delete
    }
}

void ctrlCHandler(int sig_num) {
    auto &smash = SmallShell::getInstance();
    cout << "smash: got ctrl-C"<<endl;
    auto &job = smash.fg_job;
    auto job_in_list = smash.jobs_list.getJobByPID(job.pid);
    if(job.pid) {
        job.Kill(SIGKILL);
        cout << "smash: process " <<smash.fg_job.pid << " was killed" <<endl;
        if (job_in_list)  //was before in bg
            smash.fg_job=JobEntry();//replace without delete
        else
            smash.replace_fg_and_wait(JobEntry());
    }
}

void alarmHandler(int sig_num) {
    cout << "smash: got an alarm" << endl;
    auto& smash=SmallShell::getInstance();
    smash.jobs_list.removeTimeoutJobs();
    smash.jobs_list.reset_timer();
}
