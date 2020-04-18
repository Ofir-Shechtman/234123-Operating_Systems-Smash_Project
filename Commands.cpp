#include <unistd.h>
#include <cstring>
#include <cstdio>
#include <iostream>
#include <utility>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <fcntl.h>
#include "Commands.h"
#include "signals.h"

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
    prev_dir(""), fg_pid(0), fg_cmd(nullptr){
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
      if(args.size()>=3){
          for(unsigned int i=1; i<args.size()-1; ++i){
              if(args[i]==">")
                  return new RedirectionCommand(cmd_line_no_bg, ">", _isBackgroundComamnd(cmd_line));
              else if(args[i]==">>")
                  return new RedirectionCommand(cmd_line_no_bg, ">>", _isBackgroundComamnd(cmd_line));
              else if(args[i]=="|")
                  return new PipeCommand(cmd_line_no_bg, args, "|", _isBackgroundComamnd(cmd_line));
              else if(args[i]=="|&")
                  return new PipeCommand(cmd_line_no_bg, args, "|&", _isBackgroundComamnd(cmd_line));
              else if(cmd_s.find("timeout") == 0)
                  return new TimeoutCommand(cmd_line, args, _isBackgroundComamnd(cmd_line));
          }
      }
      if (cmd_s.find("chprompt") == 0) {
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
        return new ExternalCommand(cmd_line_no_bg, _isBackgroundComamnd(cmd_line));
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

void SmallShell::set_fg(pid_t pid_in, Command *cmd) {
    fg_pid=pid_in;
    fg_cmd=cmd;
}

Command::Command(const char *cmd_line, bool bg) :
    cmd_line(cmd_line), command(_parseCommandLine(cmd_line)[0]), bg(bg), time_in(time(nullptr)){
}

string Command::getCommand() const {
    return command;
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

ExternalCommand::ExternalCommand(const char *cmd_line, bool bg) : Command(cmd_line, bg) {
}

void ExternalCommand::execute() {
     SmallShell& smash= SmallShell::getInstance();
     const char* const argv[4] = {"/bin/bash", "-c", cmd_line.c_str(), nullptr};
     pid_t child_pid= fork();
     if(child_pid<0)
        perror("smash error: fork failed");
     else if(child_pid==0) {
            setpgrp();
            execvp(argv[0], const_cast<char* const*>(argv));
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

JobsList::JobEntry::JobEntry(Command *cmd, pid_t pid_in, JobsList::JobId job_id, bool isStopped, bool isTimed, int duration) :
    cmd(cmd), pid(pid_in), job_id(job_id), isStopped(isStopped){
}

std::ostream &operator<<(std::ostream &os, const JobsList::JobEntry &job) {
    os << "[" << job.job_id << "]" << " " << job.cmd->getCommand() << " : ";
    os << job.pid << " ";
    os << difftime(time(nullptr), job.cmd->time_in) << " secs";
    if(job.isStopped)
        os << " (stopped)";
    return os;
}

JobsList::TimedJobEntry::TimedJobEntry(Command *cmd, pid_t pid, int duration) : cmd(cmd), pid(pid), duration(duration){
    timestamp = difftime(time(nullptr), cmd->time_in);
}

JobsList::TimedJobEntry::~TimedJobEntry() {
    delete cmd;
}

int JobsList::JobEntry::Kill(int signal) {
    SmallShell& smash = SmallShell::getInstance();
    if(kill(pid, signal)==0) {
        if(signal == SIGCONT) {
            isStopped = false;
            smash.jobs_list.removeFromStopped(job_id);
        }
        if(signal == SIGSTOP) {
            isStopped = true;
            smash.jobs_list.pushToStopped(this);
        }
        return 0;
    }
    else perror("smash error: kill failed");
    return -1;
}

bool JobsList::JobEntry::finish() const {
    waitpid(pid, nullptr ,WNOHANG); //voodoo for command not found with &
    return  waitpid(pid, nullptr ,WNOHANG)==-1 && !isStopped;
}

JobsList::JobEntry::~JobEntry() {
    delete cmd;
}

void JobsList::addJob(Command *cmd, pid_t pid, bool isStopped, bool isTimed, int duration) {
    removeFinishedJobs();
    list.emplace_back(cmd, pid,allocJobId(), isStopped, isTimed, duration);
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
        job.Kill();
    }
}

void JobsList::removeFinishedJobs() {
    for(auto job=list.begin(); job!=list.end();){
        if((*job).finish()) {
            list.erase(job);
        } else
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

JobsList::JobEntry *JobsList::getTimedoutJob() {
    removeFinishedJobs();
    for (auto &job : list) {
        if (job.isTimed && job.duration > difftime(time(nullptr), job.cmd->time_in))
            return &job;
    }
    return nullptr;
}

void JobsList::addTimedJob(Command *cmd, pid_t pid, int duration) {
    int i = 0;
    while(timed_jobs[i]>duration)
        i++;
        if(timed_jobs[i].)
    }

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
    if(chdir(next_dir.c_str()) == -1){
        perror("smash error: chdir failed");
    }
    else smash.set_prev_dir(prev_dir);
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
    mode_t dest_perms = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH; /*rw-rw-rw-*/

    try {
        infd = open(infile, src_flags);
        if (infd == -1) {
            throw CopyErrorFailedOpen();//"cannot open src ";
        }

        outfd = open(outfile, dest_flags, dest_perms);
        if (outfd == -1) {
            throw CopyErrorFailedOpen();//cannot open dest
        }

        while ((n = read(infd, buf, BUFSIZE)) > 0) {
            if (write(outfd, buf, n) != n)
                throw CopyError(); //"failed to write buf
        }

        if (n == -1)
            throw CopyError();//read() failed

        if (infd)
            close(infd);

        if (outfd)
            close(outfd);

    }
    catch(CopyError&) {
        if (infd)
            close(infd);

        if (outfd)
            close(outfd);
        perror("smash error: copy failed");
    }
    catch(CopyErrorFailedOpen&) {
        perror("smash error: copy failed");
    }
}

CopyCommand::CopyCommand(const char *cmd_line, vector<string>& args) :
        BuiltInCommand(cmd_line), args(args){}

void CopyCommand::execute() {
    if(args.size()!=3) {
        cout << "smash error: fg: invalid arguments" << endl;
        return;
    }
    SmallShell& smash= SmallShell::getInstance();
    pid_t child_pid= fork();
    if(child_pid<0)
        perror("smash error: fork failed");
    else if(child_pid==0) {
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

PipeCommand::PipeCommand(const char *cmd_line_c, vector<string> args, string sign, bool bg):
    Command(cmd_line_c, bg), sign(sign)
{
    string s_command1, s_command2;
    int sign_index = cmd_line.find(sign);
    s_command1 = cmd_line.substr(0,sign_index);
    if(sign == "|") s_command2 = cmd_line.substr(sign_index+2,cmd_line.size()-1);
    else if(sign == "|&") s_command2 = cmd_line.substr(sign_index+3,cmd_line.size()-1);
    command1 = SmallShell::CreateCommand(s_command1.c_str());
    command2 = SmallShell::CreateCommand(s_command2.c_str());
    command1->bg=false;
    command2->bg=false;
}

void PipeCommand::execute() {
    SmallShell& smash = SmallShell::getInstance();
    pid_t child_pid1= fork();
    int my_pipe[2];
    if(pipe(my_pipe)<0) perror("smash error: pipe failed");
    if(child_pid1<0) perror("smash error: fork failed");
    else if(child_pid1==0) {
        setpgrp();
        close(0);
        dup(my_pipe[0]);
        command1->execute();
        throw Quit();
    } else if(child_pid1>0) {
        pid_t child_pid2 = fork();
        if (child_pid2 < 0) perror("smash error: fork failed");
        else if (child_pid2 == 0) {
            setpgrp();
            if(sign == "|") close(1);
            if(sign == "|&") close(2);
            dup(my_pipe[1]);
            command2->execute();
            throw Quit();
        } else if (bg) {
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

RedirectionCommand::RedirectionCommand(const char *cmd_line_c,
                                       const string &IO_type,
                                       bool bg) :
    Command(cmd_line_c,  bg), IO_type(IO_type){
    string sign= " "+IO_type+" ";
    int sign_loc=cmd_line.find(sign);
    cmd_left= SmallShell::CreateCommand(cmd_line.substr(0, sign_loc).c_str());
    output_file= cmd_line.substr(sign_loc+sign.size(),  string::npos);
}

void RedirectionCommand::execute() {
    int outfd=-1;
    int dest_flags=0;
    if(IO_type==">") dest_flags=O_CREAT | O_WRONLY | O_TRUNC;
    else if(IO_type==">>") dest_flags= O_CREAT | O_RDWR | O_APPEND;
    mode_t dest_perms = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH; /*rw-rw-rw-*/
    outfd = open(output_file.c_str(), dest_flags, dest_perms);
    if (outfd == -1) perror("smash error: open failed");
    pid_t child_pid= fork();
    if(child_pid<0) perror("smash error: fork failed");
    else if(child_pid==0) {
        close(1);
        dup(outfd);
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
    if (outfd)
        close(outfd);
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
    pid_t child_pid= fork();
    if(child_pid<0) perror("smash error: fork failed");
    else if(child_pid==0) {
        setpgrp();
        cmd1->execute();
        throw Quit();
    }
    else{
        if(bg){
            smash.jobs_list.addJob(cmd1,child_pid);
            alarm(timer);
        }
        else{
            smash.set_fg(child_pid, this);
            alarm(timer);
            waitpid(child_pid, nullptr, WUNTRACED);
            alarm(0);
            smash.set_fg(0, nullptr);
        }
    }
}
