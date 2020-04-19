#include <cstring>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <fcntl.h>
#include "Commands.h"
#include "system_functions.h"

using namespace std;

const std::string WHITESPACE = " \n\r\t\f\v";

#if 0
#define FUNC_ENTRY()  \
  cerr << __PRETTY_FUNCTION__ << " --> " << endl;

#define FUNC_EXIT()  \
  cerr << __PRETTY_FUNCTION__ << " <-- " << endl;
#else
#define FUNC_ENTRY()
#define FUNC_EXIT()
#endif

#define DEBUG_PRINT cerr << "DEBUG: "

#define EXEC(path, arg) \
  execvp((path), (arg));

string _ltrim(const std::string& s)
{
  size_t start = s.find_first_not_of(WHITESPACE);
  return (start == std::string::npos) ? "" : s.substr(start);
}

string _rtrim(const std::string& s)
{
  size_t end = s.find_last_not_of(WHITESPACE);
  return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

string _trim(const std::string& s)
{
  return _rtrim(_ltrim(s));
}

int _parseCommandLine(const char* cmd_line, char** args) {
  FUNC_ENTRY()
  int i = 0;
  std::istringstream iss(_trim(string(cmd_line)).c_str());
  for(std::string s; iss >> s; ) {
    args[i] = (char*)malloc(s.length()+1);
    memset(args[i], 0, s.length()+1);
    strcpy(args[i], s.c_str());
    args[++i] = nullptr;
  }
  return i;
  FUNC_EXIT()
}

vector<string> _parseCommandLine(const char* cmd_line){
    FUNC_ENTRY()

    vector<string> args;//COMMAND_MAX_ARGS
    //int i = 0;
    std::istringstream iss(_trim(cmd_line));
    for(std::string s; iss >> s;) {
        args.push_back(s);
        //cout << "@" << args[i] << "@" << endl;
        //++i;
    }
    return args;

    FUNC_EXIT()
}

bool _isBackgroundComamnd(const char* cmd_line) {
  const string str(cmd_line);
  return str[str.find_last_not_of(WHITESPACE)] == '&';
}

void _removeBackgroundSign(char* cmd_line) {
  const string str(cmd_line);
  // find last character other than spaces
    std::string::size_type idx = str.find_last_not_of(WHITESPACE);
  // if all characters are spaces then return
  if (idx == string::npos) {
    return;
  }
  // if the command line does not end with & then return
  if (cmd_line[idx] != '&') {
    return;
  }
  // replace the & (background sign) with space and then remove all tailing spaces.
  cmd_line[idx] = ' ';
  // truncate the command line string up to the last non-space character
  cmd_line[str.find_last_not_of(WHITESPACE, idx) + 1] = 0;
}

// TODO: Add your implementation for classes in Commands.h

SmallShell::SmallShell() : prompt("smash"), pid(getpid()),
    prev_dir(""), min_time_job_pid(0),fg_timer(0), fg_pid(0), fg_cmd(nullptr){
}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command * SmallShell::CreateCommand(const char* cmd_line) {
      char cmd_line_no_bg[strlen(cmd_line)+1];
      strcpy(cmd_line_no_bg,cmd_line);
      _removeBackgroundSign(cmd_line_no_bg);
      vector<string> args=_parseCommandLine(cmd_line_no_bg);
      string cmd_s = string(cmd_line);

      if(cmd_s.find(">>") != string::npos) {
        return new RedirectionCommand(cmd_line_no_bg, ">>");
      }
      else if(cmd_s.find('>') != string::npos) {
            return new RedirectionCommand(cmd_line_no_bg, ">");
      }
      else if(cmd_s.find("|&") != string::npos) {
          return new PipeCommand(cmd_line_no_bg, "|&");
      }
      else if(cmd_s.find('|') != string::npos) {
          return new PipeCommand(cmd_line_no_bg, "|");
      }
      else if (cmd_s.find("timeout") == 0) {
            return new TimeoutCommand(cmd_line, args, _isBackgroundComamnd(cmd_line));
      }
      else if (cmd_s.find("chprompt") == 0) {
            return new ChangePromptCommand(cmd_line, args);
      }
      else if (cmd_s.find("showpid") == 0) {
          return new ShowPidCommand(cmd_line);
      }
      else if (cmd_s.find("pwd") == 0) {
        return new GetCurrDirCommand(cmd_line);
      }
      else if (cmd_s.find("cd") == 0) {
        return new ChangeDirCommand(cmd_line, args);
      }
      else if (cmd_s.find("kill") == 0) {
          return new KillCommand(cmd_line, args);
      }
      else if(cmd_s.find("quit") == 0) {
          return new QuitCommand(cmd_line, args);
      }
      else if(cmd_s.find("jobs") == 0) {
          return new JobsCommand(cmd_line);
      }
      else if(cmd_s.find("fg") == 0) {
          return new ForegroundCommand(cmd_line, args);
      }
      else if(cmd_s.find("bg") == 0) {
          return new BackgroundCommand(cmd_line, args);
      }
      else if(cmd_s.find("cp") == 0) {
          return new CopyCommand(cmd_line, args);
      }
      else {
        return new ExternalCommand(cmd_line, cmd_line_no_bg);
      }
}

void SmallShell::executeCommand(const char *cmd_line) {
   if(string(cmd_line).empty())
        return;

   Command* cmd= nullptr;
   try {
        cmd = CreateCommand(cmd_line);
        cmd->execute();
        if(!cmd->bg)
            delete cmd;
   }
   catch(Quit& quit) {
       delete cmd;
       throw quit;
   }
   catch(ChangeDirCommand::TooManyArgs& tma){
       cout<<"smash error: cd: too many arguments"<<endl;
       delete cmd;
   }
   catch(ChangeDirCommand::TooFewArgs& tfa){
       delete cmd;
   }
   catch(ChangeDirCommand::NoOldPWD& nop){
       cout<<"smash error: cd: OLDPWD not set"<<endl;
       delete cmd;
   }
   catch(KillCommand::KillInvalidArgs& kia){
       cout<<"smash error: kill: invalid arguments"<<endl;
       delete cmd;
   }
   catch(KillCommand::KillJobIDDoesntExist& kill_job_doesnt_exist){
       cout<<"smash error: kill: job-id "<<kill_job_doesnt_exist.job_id<<" does not exist"<<endl;
       delete cmd;
   }
   catch(ForegroundCommand::FGEmptyJobsList& fgejl){
       cout<<"smash error: fg: jobs list is empty"<<endl;
       delete cmd;
   }
   catch(ForegroundCommand::FGInvalidArgs& fia){
       cout<<"smash error: fg: invalid arguments"<<endl;
       delete cmd;
   }
   catch(ForegroundCommand::FGJobIDDoesntExist& fg_job_doesnt_exist) {
       cout << "smash error: fg: job-id " << fg_job_doesnt_exist.job_id << " does not exist" << endl;
       delete cmd;
   }
   catch(BackgroundCommand::BGJobIDDoesntExist& bg_job_doesnt_exist){
       cout<<"smash error: bg: job-id "<<bg_job_doesnt_exist.job_id<<" does not exist"<<endl;
       delete cmd;
   }
   catch(BackgroundCommand::BGJobAlreadyRunning& bg_job_already_running){
       cout<<"smash error: bg: job-id "<<bg_job_already_running.job_id<<" is already running in the background"<<endl;
       delete cmd;
   }
   catch(BackgroundCommand::BGNoStoppedJobs& bgnsj){
       cout<<"smash error: bg: there is no stopped jobs to resume"<<endl;
       delete cmd;
   }
   catch(BackgroundCommand::BGInvalidArgs& bia){
       cout<<"smash error: bg: invalid arguments"<<endl;
       delete cmd;
   }
}

string SmallShell::get_prompt() const {
    return prompt;
}

void SmallShell::set_prompt(string input_prompt) {
    prompt=std::move(input_prompt);

}

string SmallShell::get_prev_dir() const {
    return prev_dir;
}

void SmallShell::set_prev_dir(string new_dir) {
    prev_dir = std::move(new_dir);
}

pid_t SmallShell::get_pid() {
    return pid;
}

void SmallShell::set_fg(pid_t pid_in, Command *cmd, int timer) {
    fg_pid=pid_in;
    fg_cmd=cmd;
    fg_timer = timer;
}

void SmallShell::set_min_time_job_pid(pid_t pid) {
    min_time_job_pid = pid;
}

Command::Command(const char *cmd_line, bool bg) :
    cmd_line(cmd_line), bg(bg), time_in(time(nullptr)){
}

string Command::get_cmd_line() const {
    return cmd_line;
}

BuiltInCommand::BuiltInCommand(const char *cmd_line) : Command(cmd_line, _isBackgroundComamnd(cmd_line)) {
}

void GetCurrDirCommand::execute() {
    char* pwd=get_current_dir_name();
    cout<< pwd << endl;
    free(pwd);
}

ExternalCommand::ExternalCommand(const char *cmd_line, char* cmd_line_no_bg) :
    Command(cmd_line, _isBackgroundComamnd(cmd_line)),cmd_line_no_bg(cmd_line_no_bg)  {
}

void ExternalCommand::execute() {
     SmallShell& smash= SmallShell::getInstance();
     pid_t child_pid= do_fork();
     if(child_pid==0) {
            setpgrp();
            do_execvp(cmd_line_no_bg.c_str());

     }
     else{
         if(bg)
             SmallShell::getInstance().jobs_list.addJob(this, child_pid);
         else{
             smash.set_fg(child_pid, this);
             waitpid(child_pid, nullptr, WUNTRACED);
             smash.set_fg(0, nullptr);
         }
     }
}


QuitCommand::QuitCommand(const char *cmd_line, vector<string>& args)
        : BuiltInCommand(cmd_line), args(args){

}

void QuitCommand::execute() {
    if(args.size()>1 && args[1]=="kill")
        SmallShell::getInstance().jobs_list.killAllJobs();
    throw Quit();
}

JobsList::JobEntry::JobEntry(Command *cmd, pid_t pid_in, JobsList::JobId job_id, bool isStopped, int timer) :
    cmd(cmd), pid(pid_in), job_id(job_id), isStopped(isStopped), timer(timer){
}

std::ostream &operator<<(std::ostream &os, const JobsList::JobEntry &job) {
    os << "[" << job.job_id << "]" << " " << job.cmd->get_cmd_line() << " : ";
    os << job.pid << " ";
    os << difftime(time(nullptr), job.cmd->time_in) << " secs";
    if(job.isStopped)
        os << " (stopped)";
    return os;
}

int JobsList::JobEntry::Kill(int signal) {
    SmallShell& smash = SmallShell::getInstance();
    int res=do_kill(pid, signal);
    if(signal == SIGCONT) {
        isStopped = false;
        smash.jobs_list.removeFromStopped(job_id);
    }
    else if(signal == SIGSTOP) {
        isStopped = true;
        smash.jobs_list.pushToStopped(this);
    }
    return res;
}

bool JobsList::JobEntry::finish() const {
    waitpid(pid, nullptr ,WNOHANG); //voodoo for command not found with &
    return  waitpid(pid, nullptr ,WNOHANG)==-1 && !isStopped;
}

JobsList::JobEntry::~JobEntry() {
    if(finish())
        delete cmd;
}

void JobsList::addJob(Command *cmd, pid_t pid, bool isStopped, int timer) {
    removeFinishedJobs();
    list.emplace_back(cmd, pid,allocJobId(), isStopped, timer);
}

void JobsList::printJobsList() const{
    for(auto& job : list){
        cout << job << endl;
    }
}

JobsList::JobId JobsList::allocJobId() const {
    return list.empty() ? 1 : list.back().job_id+1;
}

void JobsList::killAllJobs() {
    removeFinishedJobs();
    cout<<"smash: sending SIGKILL signals to " << list.size() << " jobs:"<<endl;
    for(auto& job : list){
        cout<< job.pid << ": " << job.cmd->get_cmd_line() << endl;
        job.Kill();
    }
}

void JobsList::removeFinishedJobs() {
    for(auto job=list.begin(); job!=list.end();){
        if((*job).finish()) {
            list.erase(job);
        }
        else
            ++job;
    }
}

JobsList::JobEntry *JobsList::getJobByPID(pid_t pid) {
    removeFinishedJobs();
    for(auto& job : list){
        if(job.pid==pid)
            return &job;
    }
    return nullptr;
}

JobsList::JobEntry* JobsList::getJobByJobID(JobsList::JobId jobid) {
    removeFinishedJobs();
    for(auto& job : list){
        if(job.job_id==jobid)
            return &job;
    }
    return nullptr;
}


JobsList::JobEntry *JobsList::getLastJob(const JobsList::JobId* lastJobId) {
    removeFinishedJobs();
    if (!lastJobId)
        return &list.front();
    for (auto &job : list) {
        if (job.job_id == *lastJobId)
            return &job;
    }
    return nullptr;
}

JobsList::JobEntry *JobsList::getLastStoppedJob() {
    if(last_stopped_jobs.empty()) return nullptr;
    return last_stopped_jobs.front();
}

bool JobsList::empty() const {
    return list.empty();
}

void JobsList::removeFromStopped(JobId job_id) {
    for (auto job = last_stopped_jobs.begin(); job != last_stopped_jobs.end();) {
        if ((*job)->job_id == job_id)
            last_stopped_jobs.erase(job);
        else {
            ++job;
        }
    }
}

void JobsList::pushToStopped(JobEntry* job) {
    last_stopped_jobs.push_back(job);
}

void JobsList::removeTimedoutJob(JobId job_id) {
    SmallShell& smash = SmallShell::getInstance();
    for (auto job_id_it = timed_jobs.begin(); job_id_it != timed_jobs.end();) {
        if (*job_id_it == -1) {
            do_kill(smash.fg_pid,SIGKILL);
            timed_jobs.erase(job_id_it);
            if(smash.min_time_job_pid == smash.fg_pid) smash.set_min_time_job_pid(0);
            smash.set_fg(0,nullptr);
            continue;
        }
        auto job = getJobByJobID(*job_id_it);
        if(!job){
            timed_jobs.erase(job_id_it);
            continue;
        }
        if(smash.min_time_job_pid == job->pid) smash.set_min_time_job_pid(0);
        int time_left = job->timer - difftime(time(nullptr), job->cmd->time_in);
        if(time_left <= 0){
            job->Kill(SIGKILL);
            timed_jobs.erase(job_id_it);
            if(smash.min_time_job_pid == job->pid) smash.set_min_time_job_pid(0);
        }
        else{
            ++job_id_it;
        }
    }
    removeFinishedJobs();
    setAlarmTimer();
}

void JobsList::addTimedJob(JobId job_id) {
    timed_jobs.push_back(job_id);
    setAlarmTimer();
}

void JobsList::setAlarmTimer() {
    SmallShell& smash = SmallShell::getInstance();
    int min_time_left = MAXINT;
    int time_left = 0;
    pid_t min_job_pid = 0;
    for (auto &job_id : timed_jobs) {
        if(job_id == -1){
            time_left = smash.fg_timer - difftime(time(nullptr), smash.fg_cmd->time_in);
            if(time_left < min_time_left) {
                min_time_left = time_left;
                min_job_pid = smash.fg_pid;
            }
            continue;
        }
        auto job = getJobByJobID(job_id);
        if(!job) continue;
        time_left = job->timer - difftime(time(nullptr), job->cmd->time_in);
        if(time_left < min_time_left) {
            min_time_left = time_left;
            min_job_pid = job->pid;
        }
    }
    if(min_job_pid>0) {
        smash.set_min_time_job_pid(min_job_pid);
        alarm(min_time_left);
    }
    else{
        smash.set_min_time_job_pid(0);
        alarm(0);
    }
}

void JobsList::removeFinishedTimedjobs(JobId job_id) {
    bool remove_fg_flag = false;
    if(job_id == -1) remove_fg_flag = true;
    for (auto job_id_i = timed_jobs.begin(); job_id_i != timed_jobs.end();) {
        if(*job_id_i == -1){
            if(remove_fg_flag) timed_jobs.erase(job_id_i);
            continue;
        }
        auto job = getJobByJobID(*job_id_i);
        if (!job) {
            timed_jobs.erase(job_id_i);
        }
    }
    setAlarmTimer();
}

JobsCommand::JobsCommand(const char *cmd_line):
    BuiltInCommand(cmd_line){}

void JobsCommand::execute() {
    SmallShell::getInstance().jobs_list.removeFinishedJobs();
    SmallShell::getInstance().jobs_list.printJobsList();
}

ChangePromptCommand::ChangePromptCommand(const char* cmd_line, vector<string>& args) : BuiltInCommand(cmd_line) {
    if(args.size()>1) input_prompt = args[1];
    else input_prompt = "smash";
}

void ChangePromptCommand::execute() {
    SmallShell& smash = SmallShell::getInstance();
    smash.set_prompt(input_prompt);
}

void ShowPidCommand::execute() {
    SmallShell& smash = SmallShell::getInstance();
    cout<<"smash pid is "<<smash.get_pid()<<endl;
}

ChangeDirCommand::ChangeDirCommand(const char *cmd_line, vector<string>& args) : BuiltInCommand(cmd_line){
    SmallShell& smash = SmallShell::getInstance();
    if(args.size()>2) throw TooManyArgs();
    if(args.size()<2) throw TooFewArgs();
    if(args[1] == "-") {
        next_dir = smash.get_prev_dir();
        if(next_dir.empty()) throw NoOldPWD();
    }
    else next_dir =  args[1];
}

void ChangeDirCommand::execute() {
    SmallShell& smash = SmallShell::getInstance();
    string prev_dir = get_current_dir_name();
    do_chdir(next_dir.c_str());
    smash.set_prev_dir(prev_dir);
}

ForegroundCommand::ForegroundCommand(const char *cmd_line, vector<string>& args) :
        BuiltInCommand(cmd_line), args(args){}

ForegroundCommand::FGJobIDDoesntExist::FGJobIDDoesntExist(JobsList::JobId job_id): job_id(job_id){}

void ForegroundCommand::execute() {
    if(SmallShell::getInstance().jobs_list.empty()) throw FGEmptyJobsList();
    if(args.size()!= 2 && args.size() != 1) throw FGInvalidArgs();
    JobsList::JobId* job_id = nullptr;
    if(args.size() == 2) {
        try {
            JobsList::JobId id = stoi(args[1]);
            job_id = &id;
        }
        catch (std::invalid_argument const &e) {
            throw FGInvalidArgs();
        }
    }
    auto job= SmallShell::getInstance().jobs_list.getLastJob(job_id);
    if(job == nullptr) throw FGJobIDDoesntExist(*job_id);
    job->Kill(SIGCONT);
    job->cmd->bg=false;
    cout << job->cmd->get_cmd_line() << " : " << job->pid << endl;
    auto& smash=SmallShell::getInstance();
    smash.set_fg(job->pid, job->cmd);
    waitpid(job->pid, nullptr, WUNTRACED);
    smash.set_fg(0, nullptr);
}

BackgroundCommand::BackgroundCommand(const char *cmd_line, vector<string> &args):
        BuiltInCommand(cmd_line), args(args){}

BackgroundCommand::BGJobIDDoesntExist::BGJobIDDoesntExist(JobsList::JobId job_id): job_id(job_id){}

BackgroundCommand::BGJobAlreadyRunning::BGJobAlreadyRunning(JobsList::JobId job_id): job_id(job_id){}

void BackgroundCommand::execute() {
    SmallShell& smash = SmallShell::getInstance();
    if(args.size() != 1 && args.size() != 2) throw BGInvalidArgs();
    JobsList::JobId* job_id = nullptr;
    if(args.size() == 2) {
        try {
            JobsList::JobId id = stoi(args[1]);
            job_id = &id;}
        catch (std::invalid_argument const &e) {
            throw BGInvalidArgs();}
    }
    auto job = smash.jobs_list.getLastStoppedJob();
    if(args.size() == 1) {
        if (job == nullptr) throw BGNoStoppedJobs();
    }
    if(args.size() == 2){
        job = smash.jobs_list.getJobByJobID(*job_id);
        if(job == nullptr) throw BGJobIDDoesntExist(*job_id);
        if(!job->isStopped) throw BGJobAlreadyRunning(*job_id);
    }
    cout << job->cmd->get_cmd_line() << " : " << job->pid << endl;
    job->Kill(SIGCONT);
}

void CopyCommand::copy(const char* infile, const char* outfile) {

    const int BUFSIZE=4096;
    int infd, outfd;
    int n;
    char buf[BUFSIZE];

    int src_flags = O_RDONLY;
    int dest_flags = O_CREAT | O_WRONLY | O_TRUNC;

    infd = do_open(infile, src_flags);
    outfd = do_open(outfile, dest_flags);

    while ((n = do_read(infd, buf, BUFSIZE)) > 0)
        do_write(outfd, buf, n);

    do_close(infd);
    do_close(outfd);


}

CopyCommand::CopyCommand(const char *cmd_line, vector<string>& args) :
        BuiltInCommand(cmd_line), args(args){}

void CopyCommand::execute() {
    if(args.size()!=3) {
        cout << "smash error: fg: invalid arguments" << endl;
        return;
    }
    SmallShell& smash= SmallShell::getInstance();
    pid_t child_pid= do_fork();
    if(child_pid==0) {
        setpgrp();
        copy(args[1].c_str(), args[2].c_str());
        throw Quit();
    }
    else{
        if(bg)
            SmallShell::getInstance().jobs_list.addJob(this, child_pid);
        else{
            smash.set_fg(child_pid, this);
            waitpid(child_pid, nullptr, WUNTRACED);
            smash.set_fg(0, nullptr);
        }
    }
}

KillCommand::KillCommand(const char *cmdLine1, vector<string> &args) : BuiltInCommand(cmdLine1) {
    if(args.size() != 3 || args[1].substr(0,1) != "-") throw KillInvalidArgs();
    try {
        sig_num = stoi(args[1].substr(1));
        job_id = stoi(args[2]);
    }
    catch(std::invalid_argument const &e) {
        throw KillInvalidArgs();
    }
}

void KillCommand::execute() {
    SmallShell& smash= SmallShell::getInstance();
    auto job= smash.jobs_list.getJobByJobID(job_id);
    if (job == nullptr) throw KillJobIDDoesntExist(job_id);
    if(job->Kill(sig_num)==0) cout<<"signal number "<<sig_num<<" was sent to pid "<<job->pid<<endl;
}

KillCommand::KillJobIDDoesntExist::KillJobIDDoesntExist(JobsList::JobId job_id) :job_id(job_id){}

PipeCommand::PipeCommand(const char *cmd_line_c, string sign):
    Command(cmd_line_c, _isBackgroundComamnd(cmd_line_c)), sign(sign)
{
    string s_command1, s_command2;
    int sign_index = cmd_line.find(sign);
    s_command1 = cmd_line.substr(0,sign_index);
    s_command2 = cmd_line.substr(sign_index+sign.size()+1,cmd_line.size()-1);
    command1 = SmallShell::CreateCommand(s_command1.c_str());
    command2 = SmallShell::CreateCommand(s_command2.c_str());
    command1->bg=false;
    command2->bg=false;
}

void PipeCommand::execute() {
    SmallShell& smash = SmallShell::getInstance();
    int pipefd[2];
    do_pipe(pipefd);
    pid_t child_pid1= do_fork();
    if(child_pid1==0) {
        setpgrp();
        int fd = sign == "|" ? STDOUT_FILENO : STDERR_FILENO;
        do_dup2(pipefd[1],fd);
        do_close(pipefd[1]);
        do_close(pipefd[0]);
        command1->execute();
        throw Quit();
    } else {
        pid_t child_pid2 = do_fork();
        if (child_pid2 == 0) {
            setpgrp();
            do_dup2(pipefd[0], STDIN_FILENO);
            do_close(pipefd[0]);
            do_close(pipefd[1]);
            waitpid(child_pid1, nullptr, WUNTRACED);
            command2->execute();
            throw Quit();
        }
        else {
            do_close(pipefd[0]);
            do_close(pipefd[1]);
            if (bg) {
                smash.jobs_list.addJob(command1, child_pid1);
                smash.jobs_list.addJob(command2, child_pid2);
            } else {
                smash.set_fg(child_pid1, this);
                waitpid(child_pid1, nullptr, WUNTRACED);
                smash.set_fg(child_pid2, this);
                waitpid(child_pid2, nullptr, WUNTRACED);
                smash.set_fg(0, nullptr);
            }
        }

    }
}

RedirectionCommand::RedirectionCommand(const char *cmd_line_c,
                                       const string &IO_type) :
    Command(cmd_line_c,  _isBackgroundComamnd(cmd_line_c)), IO_type(IO_type){
    string sign= IO_type;
    int sign_loc=cmd_line.find(sign);
    cmd_left= SmallShell::CreateCommand(cmd_line.substr(0, sign_loc).c_str());
    output_file= cmd_line.substr(sign_loc+sign.size()+1,  string::npos);
}

void RedirectionCommand::execute() {
    int dest_flags=0;
    if(IO_type==">")
        dest_flags=O_CREAT | O_WRONLY | O_TRUNC;
    else if(IO_type==">>")
        dest_flags= O_CREAT | O_RDWR | O_APPEND;
    int outfd=do_open(output_file.c_str(), dest_flags);
    pid_t child_pid= do_fork();
    if(child_pid==0) {
        do_dup2(outfd, STDOUT_FILENO);
        cmd_left->execute();
        throw Quit();
    }
    else{
        auto& smash =SmallShell::getInstance();
        if(bg)
            smash.jobs_list.addJob(this, child_pid);
        else{
            smash.set_fg(child_pid, this);
            waitpid(child_pid, nullptr, WUNTRACED);
            smash.set_fg(0, nullptr);
        }
    }
    do_close(outfd);
}

TimeoutCommand::TimeoutCommand(const char *cmd_line, vector<string> args, bool bg) : Command(cmd_line, bg) {
    try {
        timer = stoi(args[1]);
    }
    catch (std::invalid_argument const &e) {
        throw TimerInvalidArgs();
    }
    string cmd_s = args[2];
    for(unsigned i = 3; i<args.size();i++){
        cmd_s += " " + args[i];
    }
    cmd1 = SmallShell::CreateCommand(cmd_s.c_str());
}

void TimeoutCommand::execute() {
    SmallShell& smash = SmallShell::getInstance();
    pid_t child_pid= do_fork();
    if(child_pid==0) {
        setpgrp();
        cmd1->execute();
        throw Quit();
    }
    else{
        if(bg){
            smash.jobs_list.addJob(cmd1,child_pid,false,timer);
            auto job = smash.jobs_list.getJobByPID(child_pid);
            smash.jobs_list.addTimedJob(job->job_id);
        }
        else{
            smash.set_fg(child_pid, this, timer);
            smash.jobs_list.addTimedJob(-1);
            waitpid(child_pid, nullptr, WUNTRACED);
            smash.jobs_list.removeFinishedTimedjobs(-1);
            smash.set_fg(0, nullptr);
        }
    }
    return;
}
