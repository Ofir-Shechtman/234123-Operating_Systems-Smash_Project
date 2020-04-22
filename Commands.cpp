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
    std::istringstream iss(_trim(cmd_line));
    for(std::string s; iss >> s;) {
        args.push_back(s);
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

string _remove_bg_sign(const string& with){
    char without[with.size()+1];
    strcpy(without,with.c_str());
    _removeBackgroundSign(without);
    return without;
}


SmallShell::SmallShell() : prompt("smash"), pid(getpid()),
    prev_dir(""), fg_job(JobEntry()){
}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command * SmallShell::CreateCommand(const char* cmd_line) {
      const string cmd_line_no_bg = _remove_bg_sign(cmd_line);
      vector<string> args=_parseCommandLine(cmd_line_no_bg.c_str());
      string cmd_s = string(cmd_line);
      if (args.empty())
          return new ExternalCommand(cmd_line);

      else if(cmd_s.find(">>") != string::npos) {
        return new RedirectionCommand(cmd_line, ">>");
      }
      else if(cmd_s.find('>') != string::npos) {
            return new RedirectionCommand(cmd_line, ">");
      }
      else if(cmd_s.find("|&") != string::npos) {
          return new PipeCommand(cmd_line, "|&");
      }
      else if(cmd_s.find('|') != string::npos) {
          return new PipeCommand(cmd_line, "|");
      }
      else if (args[0] == "timeout") {
            return new TimeoutCommand(cmd_line, args);
      }
      else if (args[0] == "chprompt") {
            return new ChangePromptCommand(cmd_line, args);
      }
      else if (args[0] == "showpid") {
          return new ShowPidCommand(cmd_line);
      }
      else if (args[0] == "pwd") {
        return new GetCurrDirCommand(cmd_line);
      }
      else if (args[0] == "cd") {
        return new ChangeDirCommand(cmd_line, args);
      }
      else if (args[0] == "kill") {
          return new KillCommand(cmd_line, args);
      }
      else if(args[0] == "quit") {
          return new QuitCommand(cmd_line, args);
      }
      else if(args[0] == "jobs") {
          return new JobsCommand(cmd_line);
      }
      else if(args[0] == "fg") {
          return new ForegroundCommand(cmd_line, args);
      }
      else if(args[0] == "bg") {
          return new BackgroundCommand(cmd_line, args);
      }
      else if(args[0] == "cp") {
          return new CopyCommand(cmd_line, args);
      }
      else {
        return new ExternalCommand(cmd_line);
      }
}

void SmallShell::executeCommand(const char *cmd_line) {
   Command* cmd= nullptr;
   try {
       try {
           cmd = CreateCommand(cmd_line);
           cmd->execute();
       }
       catch (Quit &quit) {
           jobs_list.killAllStopedJobs();
           jobs_list.deleteAll();
           replace_fg_and_wait(JobEntry(nullptr));
           delete cmd;
           throw;
       }
       catch (ChangeDirCommand::TooManyArgs &tma) {
           cerr << "smash error: cd: too many arguments" << endl;
           throw;
       }
       catch (ChangeDirCommand::TooFewArgs &tfa) {
           throw;
       }
       catch (ChangeDirCommand::NoOldPWD &nop) {
           cerr << "smash error: cd: OLDPWD not set" << endl;
           throw;
       }
       catch (KillCommand::KillInvalidArgs &kia) {
           cerr << "smash error: kill: invalid arguments" << endl;
           throw;
       }
       catch (KillCommand::KillJobIDDoesntExist &kill_job_doesnt_exist) {
           cerr << "smash error: kill: job-id " << kill_job_doesnt_exist.job_id
                << " does not exist" << endl;
           throw;
       }
       catch (ForegroundCommand::FGEmptyJobsList &fgejl) {
           cerr << "smash error: fg: jobs list is empty" << endl;
           throw;
       }
       catch (ForegroundCommand::FGInvalidArgs &fia) {
           cerr << "smash error: fg: invalid arguments" << endl;
           throw;
       }
       catch (ForegroundCommand::FGJobIDDoesntExist &fg_job_doesnt_exist) {
           cerr << "smash error: fg: job-id " << fg_job_doesnt_exist.job_id
                << " does not exist" << endl;
           throw;
       }
       catch (BackgroundCommand::BGJobIDDoesntExist &bg_job_doesnt_exist) {
           cerr << "smash error: bg: job-id " << bg_job_doesnt_exist.job_id
                << " does not exist" << endl;
           throw;
       }
       catch (BackgroundCommand::BGJobAlreadyRunning &bg_job_already_running) {
           cerr << "smash error: bg: job-id " << bg_job_already_running.job_id
                << " is already running in the background" << endl;
           throw;
       }
       catch (BackgroundCommand::BGNoStoppedJobs &bgnsj) {
           cerr << "smash error: bg: there is no stopped jobs to resume"
                << endl;
           throw;
       }
       catch (BackgroundCommand::BGInvalidArgs &bia) {
           cerr << "smash error: bg: invalid arguments" << endl;
           throw;
       }
   }
   catch(Continue& e) {
       replace_fg_and_wait(JobEntry(cmd));
       throw;
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

void SmallShell::replace_fg_and_wait(JobEntry job) {
    auto& shell_fg_job = fg_job;
    delete shell_fg_job.cmd;
    shell_fg_job = JobEntry(job);
    do_waitpid(shell_fg_job.pid, nullptr, WUNTRACED);
    shell_fg_job.pid=0;

}

Command::Command(const char *cmd_line, bool bg) :
    cmd_line(cmd_line), bg(bg){
}

string Command::get_cmd_line() const {
    return cmd_line;
}

BuiltInCommand::BuiltInCommand(const char *cmd_line) : Command(cmd_line, _isBackgroundComamnd(cmd_line)) {
}

void GetCurrDirCommand::execute() {
    SmallShell& smash= SmallShell::getInstance();
    char* pwd=get_current_dir_name();
    cout<< pwd << endl;
    free(pwd);
    smash.replace_fg_and_wait(JobEntry(this));
}

ExternalCommand::ExternalCommand(const char *cmd_line) :
    Command(cmd_line, _isBackgroundComamnd(cmd_line))  {
}

void ExternalCommand::execute() {
     SmallShell& smash= SmallShell::getInstance();
    const string cmd_line_no_bg = _remove_bg_sign(cmd_line);
     pid_t child_pid= do_fork();
     if(child_pid==0) {
         do_execvp(cmd_line_no_bg.c_str());
     }
     else{
         if(bg)
             smash.jobs_list.addJob(JobEntry(this, child_pid));
         else{
             smash.replace_fg_and_wait(JobEntry(this, child_pid));
         }
     }
}


QuitCommand::QuitCommand(const char *cmd_line, vector<string>& args)
        : BuiltInCommand(cmd_line), args(args){

}

void QuitCommand::execute() {
    auto& smash =SmallShell::getInstance();
    auto& jobs_list=smash.jobs_list;
    if(args.size()>1 && args[1]=="kill")
        jobs_list.killAllJobs();
    //                 jobs_list.deleteAll();
    //NOW INSIDE QUIT! smash.replace_fg_and_wait(JobEntry(this));
    throw Quit();
}

JobEntry::JobEntry(Command *cmd, pid_t pid_in, int timer) :
    cmd(cmd), pid(pid_in), timer(timer), job_id(0), time_in(time(nullptr)),
    isStopped(false), isDead(false) {
}

std::ostream &operator<<(std::ostream &os, const JobEntry &job) {
    os << "[" << job.job_id << "]" << " " << job.cmd->get_cmd_line() << " : ";
    os << job.pid << " ";
    os << job.run_time() << " secs";
    if(job.isStopped)
        os << " (stopped)";
    return os;
}

int JobEntry::run_time() const {
    return difftime(time(nullptr), time_in);
}

int JobEntry::time_left() const {
    return this->timer-run_time();
}

int JobEntry::Kill(int signal)  {
    SmallShell& smash = SmallShell::getInstance();
    if(signal == SIGCONT) {
        isStopped = false;
        cmd->bg = false;
        smash.jobs_list.removeFromStopped(job_id);
    }
    else if(signal == SIGSTOP) {
        isStopped = true;
        cmd->bg = true;
        time_in = time(nullptr);
        smash.jobs_list.pushToStopped(this->job_id);
    }
    else if(signal == SIGKILL){
        smash.jobs_list.removeFromStopped(job_id);
    }
    int res=do_kill(pid, signal);
    return res;
}

bool JobEntry::is_finish()  {
    if(!isDead) {
        pid_t res=do_waitpid(pid, nullptr, WNOHANG);
        isDead = res == pid || res==-1;
    }
    return  isDead;
}

void JobEntry::timeout() {
    cout << "smash: got an alarm" << endl;
    cout << "smash: " << cmd->get_cmd_line() << " timed out!"
         << endl;
    Kill();
}


void JobsList::addJob(JobEntry job) {
    removeFinishedJobs();
    job.job_id = allocJobId();
    list.push_back(job);
}

void JobsList::printJobsList() const{
    for(auto& job : list){
        cout << job << endl;
    }
}

JobId JobsList::allocJobId() const {
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

void JobsList::killAllStopedJobs() {
    removeFinishedJobs();
    for(auto& job_id : last_stopped_jobs){
        auto job=getJobByJobID(job_id);
        job->Kill();
    }
}


void JobsList::removeFinishedJobs() {
    for(auto job=list.begin(); job!=list.end();) {
        if (job->is_finish()) {
            delete job->cmd;
            removeFromStopped(job->job_id);//if stopped
            list.erase(job);
        } else
            ++job;
    }
}

JobEntry *JobsList::getJobByPID(pid_t pid) {
    removeFinishedJobs();
    for(auto& job : list){
        if(job.pid==pid)
            return &job;
    }
    return nullptr;
}

JobEntry* JobsList::getJobByJobID(JobId jobid) {
    removeFinishedJobs();
    for(auto& job : list){
        if(job.job_id==jobid)
            return &job;
    }
    return nullptr;
}


JobEntry *JobsList::getLastJob(const JobId* lastJobId) {
    if(list.empty())
        return nullptr;
    if (!lastJobId)
        return &list.back();
    for (auto &job : list) {
        if (job.job_id == *lastJobId)
            return &job;
    }
    return nullptr;
}

JobEntry *JobsList::getLastStoppedJob() {
    if(last_stopped_jobs.empty()) return nullptr;
    auto job = getJobByJobID(last_stopped_jobs.front());
    return job;
}

bool JobsList::empty() const {
    return list.empty();
}

void JobsList::removeFromStopped(JobId job_id) {
    for (auto job_id_it = last_stopped_jobs.begin(); job_id_it != last_stopped_jobs.end();) {
        if (*job_id_it == job_id)
            last_stopped_jobs.erase(job_id_it);
        else {
            ++job_id_it;
        }
    }
}

void JobsList::pushToStopped(JobId job_id) {
    last_stopped_jobs.push_back(job_id);
}

void JobsList::removeTimeoutJobs(){
    removeFinishedJobs();
    for (auto & job : list) {
        if (job.timer && job.run_time() >= job.timer && !job.is_finish())
            job.timeout();
    }
    removeFinishedJobs();

    JobEntry &fg_job = SmallShell::getInstance().fg_job;
    if (fg_job.cmd && fg_job.timer && fg_job.run_time() >= fg_job.timer && !fg_job.is_finish()) {
        fg_job.timeout();
        fg_job = JobEntry();
    }
}


void JobsList::reset_timer(int new_timer) {
    int timer=0;
    for (auto & job : list) {
        if (job.timer && (!timer || job.time_left()<timer) ){
            timer=job.time_left();
        }
    }
    auto& fg_job = SmallShell::getInstance().fg_job;
    if(fg_job.timer && (!timer || fg_job.time_left()<timer))
        timer=fg_job.time_left();

    if(new_timer && (!timer || new_timer<timer))
        timer=new_timer;
    if(timer)
        alarm(timer);

}



JobsCommand::JobsCommand(const char *cmd_line):
    BuiltInCommand(cmd_line){}

void JobsCommand::execute() {
    auto &smash = SmallShell::getInstance();
    smash.jobs_list.removeFinishedJobs();
    smash.jobs_list.printJobsList();
    smash.replace_fg_and_wait(JobEntry(this));
}

ChangePromptCommand::ChangePromptCommand(const char* cmd_line, vector<string>& args) : BuiltInCommand(cmd_line) {
    if(args.size()>1) input_prompt = args[1];
    else input_prompt = "smash";
}

void ChangePromptCommand::execute() {
    SmallShell& smash = SmallShell::getInstance();
    smash.set_prompt(input_prompt);
    smash.replace_fg_and_wait(JobEntry(this));
}

void ShowPidCommand::execute() {
    SmallShell& smash = SmallShell::getInstance();
    cout<<"smash pid is "<<smash.get_pid()<<endl;
    smash.replace_fg_and_wait(JobEntry(this));
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
    char* prev_dir = get_current_dir_name();
    try{
        do_chdir(next_dir.c_str());
    }
    catch(Continue& e){
        free(prev_dir);
        throw;
    }
    smash.set_prev_dir(prev_dir);
    free(prev_dir);
    smash.replace_fg_and_wait(JobEntry(this));
}

ForegroundCommand::ForegroundCommand(const char *cmd_line, vector<string>& args) :
        BuiltInCommand(cmd_line), args(args){}

ForegroundCommand::FGJobIDDoesntExist::FGJobIDDoesntExist(JobId job_id): job_id(job_id){}

void ForegroundCommand::execute() {
    auto& smash = SmallShell::getInstance();
    smash.jobs_list.removeFinishedJobs();
    if(smash.jobs_list.empty())
        throw FGEmptyJobsList();
    if(args.size()!= 2 && args.size() != 1) throw FGInvalidArgs();
    JobId* job_id = nullptr;
    if(args.size() == 2) {
        try {
            JobId id = do_stoi(args[1]);
            job_id = &id;
        }
        catch (std::invalid_argument const &e) {
            throw FGInvalidArgs();
        }
    }
    auto job= smash.jobs_list.getLastJob(job_id);
    if(job == nullptr) throw FGJobIDDoesntExist(*job_id);
    job->Kill(SIGCONT);
    cout << job->cmd->get_cmd_line() << " : " << job->pid << endl;
    smash.replace_fg_and_wait(JobEntry(nullptr, job->pid)); //job->cmd will delete from joblist
}

BackgroundCommand::BackgroundCommand(const char *cmd_line, vector<string> &args):
        BuiltInCommand(cmd_line), args(args){}

BackgroundCommand::BGJobIDDoesntExist::BGJobIDDoesntExist(JobId job_id): job_id(job_id){}
BackgroundCommand::BGJobAlreadyRunning::BGJobAlreadyRunning(JobId job_id): job_id(job_id){}

void BackgroundCommand::execute() {
    SmallShell& smash = SmallShell::getInstance();
    if(args.size() != 1 && args.size() != 2) throw BGInvalidArgs();
    JobId* job_id = nullptr;
    if(args.size() == 2) {
        try {
            JobId id = do_stoi(args[1]);
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
    smash.replace_fg_and_wait(JobEntry(this));
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
        copy(args[1].c_str(), args[2].c_str());
        throw Quit();
    }
    else{
        if(bg)
            SmallShell::getInstance().jobs_list.addJob(JobEntry(this, child_pid));
        else{
            smash.replace_fg_and_wait(JobEntry(this, child_pid));
        }
    }
}

KillCommand::KillCommand(const char *cmdLine1, vector<string> &args) : BuiltInCommand(cmdLine1) {
    if(args.size() != 3 || args[1].substr(0,1) != "-") throw KillInvalidArgs();
    try {
        sig_num = do_stoi(args[1].substr(1));
        job_id = do_stoi(args[2]);
    }
    catch(std::invalid_argument const &e) {
        throw KillInvalidArgs();
    }
    if(sig_num < 1 || sig_num > 31)
        throw KillInvalidArgs();
}

void KillCommand::execute() {
    SmallShell& smash= SmallShell::getInstance();
    auto job= smash.jobs_list.getJobByJobID(job_id);
    if (job == nullptr) throw KillJobIDDoesntExist(job_id);
    if(job->Kill(sig_num)==0) cout<<"signal number "<<sig_num<<" was sent to pid "<<job->pid<<endl;
    smash.replace_fg_and_wait(JobEntry(this));
}

KillCommand::KillJobIDDoesntExist::KillJobIDDoesntExist(JobId job_id) :job_id(job_id){}

PipeCommand::PipeCommand(const char *cmd_line_c, const string& sign):
    Command(cmd_line_c, _isBackgroundComamnd(cmd_line_c)), sign(sign)
{
    int sign_index = cmd_line.find(sign);
    string cmd_line_no_bg= _remove_bg_sign(cmd_line);
    s_command1 = cmd_line_no_bg.substr(0,sign_index);
    s_command2 = cmd_line_no_bg.substr(sign_index+sign.size(),string::npos);
}

void PipeCommand::execute() {
    SmallShell &smash = SmallShell::getInstance();
    pid_t pipe_pid = do_fork();
    if (pipe_pid == 0) {
        int pipefd[2];
        do_pipe(pipefd);
        pid_t child_pid1 = do_fork();
        if (child_pid1 == 0) {
            int fd = sign == "|" ? STDOUT_FILENO : STDERR_FILENO;
            do_dup2(pipefd[1], fd);
            do_close(pipefd[1]);
            do_close(pipefd[0]);
            auto command1 = smash.CreateCommand(s_command1.c_str());
            command1->bg=false;
            command1->execute();
            throw Quit();
        } else {
            pid_t child_pid2 = do_fork();
            if (child_pid2 == 0) {
                do_dup2(pipefd[0], STDIN_FILENO);
                do_close(pipefd[0]);
                do_close(pipefd[1]);
                do_waitpid(child_pid1, nullptr, WUNTRACED);
                auto command2 = smash.CreateCommand(s_command2.c_str());
                command2->bg=false;
                command2->execute();
                throw Quit();
            } else {
                do_close(pipefd[0]);
                do_close(pipefd[1]);
            }
            do_waitpid(child_pid1, nullptr, WUNTRACED);
            do_waitpid(child_pid2, nullptr, WUNTRACED);
            throw Quit();
        }
    }

    if (bg) {
        smash.jobs_list.addJob(JobEntry(this, pipe_pid));
    } else {
        smash.replace_fg_and_wait(JobEntry(this, pipe_pid));
    }
}

RedirectionCommand::RedirectionCommand(const char *cmd_line_c,
                                       const string &IO_type) :
    Command(cmd_line_c,  _isBackgroundComamnd(cmd_line_c)), IO_type(IO_type){
    string cmd_line_no_bg_str= _remove_bg_sign(cmd_line);
    int sign_loc=cmd_line_no_bg_str.find(IO_type);
    cmd_left_line=cmd_line_no_bg_str.substr(0, sign_loc);
    output_file= cmd_line_no_bg_str.substr(sign_loc+IO_type.size()+1,  string::npos);
}

void RedirectionCommand::execute() {
    auto& smash =SmallShell::getInstance();
    unsigned int dest_flags=0;
    if(IO_type==">")
        dest_flags=O_CREAT | O_WRONLY | O_TRUNC;
    else if(IO_type==">>")
        dest_flags= O_CREAT | O_RDWR | O_APPEND;
    int outfd=do_open(output_file.c_str(), dest_flags);
    pid_t child_pid = do_fork();
    if (child_pid == 0) {
        do_dup2(outfd, STDOUT_FILENO);
        auto cmd_left= smash.CreateCommand(cmd_left_line.c_str());
        cmd_left->execute();
        throw Quit();
    }

    else{
        if(bg)
            smash.jobs_list.addJob(JobEntry(this, child_pid));
        else{
            smash.replace_fg_and_wait(JobEntry(this, child_pid));

        }
    }
    do_close(outfd);
}

TimeoutCommand::TimeoutCommand(const char *cmd_line_in, vector<string> args) : Command(cmd_line_in, _isBackgroundComamnd(cmd_line_in)) {
    if (args.size()<3)
        throw TimerInvalidArgs();
    try {
        timer = do_stoi(args[1]);
    }
    catch (std::invalid_argument const &e) {
        throw TimerInvalidArgs();
    }
    if (timer<0){
        throw TimerInvalidArgs();
    }
    string cmd_line_no_bg = _remove_bg_sign(cmd_line);
    cmd_s = cmd_line_no_bg.substr(cmd_line_no_bg.find(args[2]), string::npos);
}

void TimeoutCommand::execute() {
    SmallShell& smash = SmallShell::getInstance();
    pid_t child_pid= do_fork();
    if(child_pid==0) {
        auto cmd = smash.CreateCommand(cmd_s.c_str());
        cmd->execute();
        throw Quit();
    }
    else{
        smash.jobs_list.reset_timer(timer);
        if(bg){
            smash.jobs_list.addJob(JobEntry(this,child_pid,timer));
        }
        else{
            smash.replace_fg_and_wait(JobEntry(this,child_pid,timer));
        }
    }
}

void JobsList::deleteAll() {
    for(auto job:list)
        delete job.cmd;
}
