#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <iostream>
#include <utility>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <fcntl.h>
#include "Commands.h"

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
      SmallShell& smash= SmallShell::getInstance();

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
      else if(cmd_s.find("quit") == 0) {
          return new QuitCommand(cmd_line, &smash.jobs_list, args);
      }
      else if(cmd_s.find("jobs") == 0) {
          return new JobsCommand(cmd_line, &smash.jobs_list);
      }
      else if(cmd_s.find("fg") == 0) {
          return new ForegroundCommand(cmd_line, &smash.jobs_list, args);
      }
      else if(cmd_s.find("cp") == 0) {
          return new CopyCommand(cmd_line, args);
      }
      else {
        return new ExternalCommand(cmd_line_no_bg);
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
   catch(ChangeDirCommand::TooManyArgs& tma){
       cout<<"smash error: cd: too many arguments"<<endl;
       delete cmd;
   }
   catch(ChangeDirCommand::NoOldPWD& nop){
       cout<<"smash error: cd: OLDPWD not set"<<endl;
       delete cmd;
   }
   catch(QuitCommand::Quit& quit) {
       delete cmd;
       throw quit;
   }
   //Please note that you must fork smash process for some commands (e.g., external commands....)
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

Command::Command(const char *cmd_line) :
    cmd_line(cmd_line), bg(_isBackgroundComamnd(cmd_line)){

}

string Command::getCommand() const {
    return _parseCommandLine(cmd_line)[0];
}

string Command::get_cmd_line() const {
    return cmd_line;
}

BuiltInCommand::BuiltInCommand(const char *cmd_line) : Command(cmd_line) {

}

void GetCurrDirCommand::execute() {
    char* pwd=get_current_dir_name();
    cout<< pwd << endl;
    free(pwd);
}

ExternalCommand::ExternalCommand(const char *cmd_line) : Command(cmd_line) {

}

void ExternalCommand::execute() {
     SmallShell& smash= SmallShell::getInstance();
     const char* const argv[4] = {"/bin/bash", "-c", cmd_line, nullptr};
     pid_t child_pid= fork();
     if(child_pid==0) {
            setpgrp();
            execvp(argv[0], const_cast<char* const*>(argv));
     }
     else{
         if(bg)
             smash.jobs_list.addJob(this, child_pid);
         else{
             smash.set_fg(child_pid, this);
             waitpid(child_pid, nullptr, WUNTRACED);
             smash.set_fg(0, nullptr);
         }
     }
}


QuitCommand::QuitCommand(const char *cmd_line, JobsList *jobs, vector<string>& args)
        : BuiltInCommand(cmd_line),to_kill(jobs), args(args){

}

void QuitCommand::execute() {
    if(args.size()>1 && args[1]=="kill")
        to_kill->killAllJobs();
    throw Quit();
}

JobsList::JobEntry::JobEntry(Command *cmd, pid_t pid_in, JobsList::JobId job_id, bool isStopped) :
    cmd(cmd), pid(pid_in), job_id(job_id), time_in(time(nullptr)), isStopped(isStopped){

}

std::ostream &operator<<(std::ostream &os, const JobsList::JobEntry &job) {
    os << "[" << job.job_id << "]" << " " << job.cmd->getCommand() << " : ";
    os << job.pid << " ";
    os << difftime(time(nullptr), job.time_in) << " secs";
    if(job.isStopped)
        os << " (stopped)";
    return os;
}

void JobsList::JobEntry::Kill() {
    kill(pid, SIGKILL);
    delete cmd;
    cmd= nullptr;
}


bool JobsList::JobEntry::finish() const {
    //cout<<"waitpid("<<pid<<") = "<<waitpid(pid, nullptr ,WNOHANG)<<endl;
    return waitpid(pid, nullptr ,WNOHANG)==-1;
}

void JobsList::addJob(Command *cmd, pid_t pid, bool isStopped) {
    list.emplace_back(JobEntry(cmd, pid,allocJobId(), isStopped));
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

JobsList::JobEntry *JobsList::getJobById(JobsList::JobId job_id_in) {
    for(auto& job : list){
        if(job.job_id==job_id_in)
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

JobsList::JobEntry *JobsList::getLastStoppedJob(const JobsList::JobId* jobId) {
    removeFinishedJobs();
    JobEntry* last_job = nullptr;
    for(auto& job : list){
        if(job.job_id==*jobId)
            return &job;
        if(job.isStopped)
            last_job=&job;
    }
    return last_job;
}

JobsList::~JobsList() {
    for(auto j: list)
        delete j.cmd;

}

bool JobsList::empty() const {
    return list.empty();
}


JobsCommand::JobsCommand(const char *cmd_line, JobsList *jobs):
    BuiltInCommand(cmd_line), jobs(jobs){}

void JobsCommand::execute() {
    jobs->removeFinishedJobs();
    jobs->printJobsList();

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

ForegroundCommand::ForegroundCommand(const char *cmd_line, JobsList *jobs, vector<string>& args) :
        BuiltInCommand(cmd_line), jobs(jobs), args(args){}

void ForegroundCommand::execute() {
    if(jobs->empty()) {
        cout << "smash error: fg: job list is empty" << endl;
        return;
    }
    JobsList::JobId* job_id = nullptr;
    bool valid_job_id = true;
    try {
        if(args.size()>1) {
            JobsList::JobId id = stoi(args[1]);
            job_id = &id;
        }
    }
    catch(std::invalid_argument const &e) {
        valid_job_id = false;
    }
    if(args.size()>2 || !valid_job_id){
        cout<<"smash error: fg: invalid arguments"<<endl;
        return;
    }
    auto job= jobs->getLastJob(job_id);
    if(!job) {
        cout << "smash error: fg: job-id " << *job_id << " does not exist"
             << endl;
        return;
    }
    kill(job->pid,SIGCONT);
    cout << job->cmd->get_cmd_line() << " : " << job->pid << endl;
    auto& smash=SmallShell::getInstance();
    smash.fg_pid=job->pid;
    waitpid(job->pid, nullptr, WUNTRACED);
    smash.fg_pid=0;
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
    if(child_pid==0) {
        setpgrp();
        copy(args[1].c_str(), args[2].c_str());
        throw QuitCommand::Quit();
    }
    else{
        if(bg)
            smash.jobs_list.addJob(this, child_pid);
        else{
            smash.set_fg(child_pid, this);
            waitpid(child_pid, nullptr, WUNTRACED);
            smash.set_fg(0, nullptr);
        }
    }
}
