#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <iostream>
#include <utility>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include <unistd.h>
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
    args[++i] = NULL;
  }
  return i;

  FUNC_EXIT()
}


vector<string> _parseCommandLine(const char* cmd_line){
    FUNC_ENTRY()

    vector<string> args;//COMMAND_MAX_ARGS
    //int i = 0;
    std::istringstream iss(_trim(string(cmd_line)).c_str());
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

SmallShell::SmallShell() : prompt("smash"), pid(getpid()), prev_dir(""){
// TODO: add your implementation
}

SmallShell::~SmallShell() {}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command * SmallShell::CreateCommand(const char* cmd_line) {
      vector<string> args=_parseCommandLine(cmd_line);
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
          return new QuitCommand(cmd_line);
      }
      else if(cmd_s.find("jobs") == 0) {
          return new JobsCommand(cmd_line, &smash.jobs_list);
      }
      else {
        return new ExternalCommand(cmd_line);
      }

    }

void SmallShell::executeCommand(const char *cmd_line) {
    bool bg_sign=_isBackgroundComamnd(cmd_line);
    char * cmd_line_no_bgsign = (char*)malloc(strlen(cmd_line) + 1);
    strcpy(cmd_line_no_bgsign, cmd_line);
    _removeBackgroundSign(cmd_line_no_bgsign);
    Command* cmd = nullptr;
    try {
        cmd = CreateCommand(cmd_line_no_bgsign);
        pid_t child_pid = fork();
        if(child_pid==0) {
            cmd->execute();
        }
        else{
            cmd->set_pid(child_pid);
            if(bg_sign)
                jobs_list.addJob(cmd, false);
            else {
                waitpid(child_pid, NULL, 0);
                delete cmd;
            }
        }
    }
    catch(ChangeDirCommand::TooManyArgs& tma){
        cout<<"smash error: cd: too many arguments"<<endl;
        delete cmd;
    }
    catch(ChangeDirCommand::NoOldPWD& nop){
        cout<<"smash error: cd: OLDPWD not set"<<endl;
        delete cmd;
    }
    catch(Command::Quit& quit) {
        delete cmd;
        throw quit;
    }//Please note that you must fork smash process for some commands (e.g., external commands....)
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

Command::Command(const char *cmd_line) : cmd_line(cmd_line), pid(-1){

}

string Command::getCommand() const {
    return _parseCommandLine(cmd_line.c_str())[0];
}

pid_t Command::get_pid() const {
    return pid;
}

void Command::set_pid(pid_t pid_in) {
    pid=pid_in;
}

void Command::Kill() const {
    kill(pid, 9);
    cout<<"signal number 9 was sent to pid " << pid << endl;
}

bool Command::isFinish() const {
    waitpid(pid, NULL ,WNOHANG);
    return false;
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
    if(cmd_line.empty())
        return;
    char cmd[cmd_line.size()+1];
    strcpy(cmd,cmd_line.c_str());
    const char* const argv[4] = {"/bin/bash", "-c", cmd, nullptr};
    execvp(argv[0], const_cast<char* const*>(argv));
}

QuitCommand::QuitCommand(const char *cmd_line, JobsList *jobs) : BuiltInCommand(cmd_line),to_kill(jobs){

}

void QuitCommand::execute() {
    throw Quit();
}

JobsList::JobEntry::JobEntry(Command *cmd, JobsList::JobId job_id, bool isStopped) :
    cmd(cmd), job_id(job_id), time_in(time(NULL)), isStopped(isStopped){

}

std::ostream &operator<<(std::ostream &os, const JobsList::JobEntry &job) {
    os << "[" << job.job_id << "]" << " " << job.cmd->getCommand() << " : ";
    os << job.cmd->get_pid() << " ";
    os << difftime(time(NULL), job.time_in) << " secs";
    if(job.isStopped)
        os << " (stopped)";
    return os;
}

void JobsList::addJob(Command *cmd, bool isStopped) {
    list.emplace_back(cmd, allocJobId(), isStopped);
}

void JobsList::printJobsList() const{
    for(auto& job : list){
        cout << job << endl;
    }

}

JobsList::JobId JobsList::allocJobId() const {
    return list.empty() ? 1 : list.back().job_id+1;
}

JobsCommand::JobsCommand(const char *cmd_line, JobsList *jobs):
    BuiltInCommand(cmd_line), jobs(jobs){}

void JobsCommand::execute() {
    jobs->printJobsList();

}

ChangePromptCommand::ChangePromptCommand(const char* cmd_line, vector<string> args) : BuiltInCommand(cmd_line) {
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

ChangeDirCommand::ChangeDirCommand(const char *cmd_line, vector<string> args) : BuiltInCommand(cmd_line){
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
