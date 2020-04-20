#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <vector>
#include <ctime>
using std::string;
using std::cout;
using std::vector;
#define COMMAND_ARGS_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)
#define HISTORY_MAX_RECORDS (50)
#define MAXINT 2147483647



class Command {
// TODO: Add your data members
protected:
  const string cmd_line;
public:
  bool bg;
  explicit Command(const char* cmd_line, bool bg);
  virtual ~Command()= default;;
  virtual void execute() = 0;
  //string getCommand() const;
  string get_cmd_line() const;
};

typedef int JobId;
struct JobEntry {
    Command* cmd;
    pid_t pid;
    int timer;
    JobId job_id;
    time_t time_in;
    bool isStopped;
    bool isDead;
    explicit JobEntry(Command* cmd= nullptr, pid_t pid=0, int timer=0);
    int Kill(int signal= SIGKILL);
    bool is_finish();
    ~JobEntry()= default;
    int run_time() const;
    int time_left() const;
    void timeout();
    friend std::ostream& operator<<(std::ostream& os, const JobEntry& job);
};

class BuiltInCommand : public Command {
 public:
  explicit BuiltInCommand(const char* cmd_line);
  ~BuiltInCommand() override = default;
};

class ExternalCommand : public Command {
 public:
    explicit ExternalCommand(const char *cmd_line);
  ~ExternalCommand() override = default;
  void execute() override;
};



class PipeCommand : public Command {
    Command* command1;
    Command* command2;
    string sign;
public:
      explicit PipeCommand(const char* cmd_line_c, string sign);
      ~PipeCommand() override;
      void execute() override;
};

class RedirectionCommand : public Command {
    Command* cmd_left;
    string output_file;
    string IO_type;
 public:
  explicit RedirectionCommand(const char* cmd_line, const string&  IO_type);
  ~RedirectionCommand() override;
  void execute() override;
};

class TimeoutCommand : public Command {
    int timer;
    Command* cmd1;
public:
    explicit TimeoutCommand(const char* cmd_line, vector<string> args);
    ~TimeoutCommand() override;
    class TimerInvalidArgs: public std::exception{};
    void execute() override;
};

class ChangePromptCommand : public BuiltInCommand {
    string input_prompt;
public:
    explicit ChangePromptCommand(const char* cmd_line, vector<string>& args);
    ~ChangePromptCommand() override = default;
    void execute() override;
};

class ShowPidCommand : public BuiltInCommand {
public:
    explicit ShowPidCommand(const char* cmd_line): BuiltInCommand(cmd_line){};
    ~ShowPidCommand() override = default;
    void execute() override;
};

class GetCurrDirCommand : public BuiltInCommand {
public:
    explicit GetCurrDirCommand(const char* cmd_line): BuiltInCommand(cmd_line){};
    ~GetCurrDirCommand() override = default;
    void execute() override;
};

class ChangeDirCommand : public BuiltInCommand {
    string next_dir;
public:
    explicit ChangeDirCommand(const char* cmd_line, vector<string>& args);
    class TooManyArgs : public std::exception{};
    class TooFewArgs : public std::exception{};
    class NoOldPWD : public std::exception{};
    ~ChangeDirCommand() override = default;
    void execute() override;
};



class QuitCommand : public BuiltInCommand {
    vector<string> args;
public:
  QuitCommand(const char* cmd_line, vector<string>& args);
  ~QuitCommand() override = default;
  void execute() override;

};


class JobsList {
public:
private:

  std::vector<JobEntry> list;
  vector<JobId> last_stopped_jobs;
  vector<JobId> timed_jobs;
  JobId allocJobId() const;
public:
  JobsList()= default;
  ~JobsList()= default;
  void addJob(JobEntry);
  void printJobsList() const;
  void killAllJobs();
  void removeFinishedJobs();
  JobEntry * getJobByPID(pid_t);
  JobEntry * getJobByJobID(JobId);
  //void removeJobById(JobId);
  JobEntry * getLastJob(const JobId* lastJobId);
  JobEntry *getLastStoppedJob();
  void pushToStopped(JobId);
  void removeFromStopped(JobId);
  void remove(vector<JobEntry>::iterator it);
/*
  void addTimedJob(JobId);
  void removeTimedoutJob(JobId job_id = 0);
  void removeFinishedTimedjobs(JobId job_id = 0);
  void setAlarmTimer();*/

  void removeTimedoutJobs();
  void reset_timer(int new_timer=0);

  bool empty() const;
  friend std::ostream& operator<<(std::ostream& os, const JobEntry& job);

};

class JobsCommand : public BuiltInCommand {
 // TODO: Add your data members
 public:
  explicit JobsCommand(const char* cmd_line);
  ~JobsCommand() override = default;
  void execute() override;
};

class KillCommand : public BuiltInCommand {
    JobId job_id;
    int sig_num;
public:
    KillCommand(const char *cmdLine1, vector<string> &args);
    ~KillCommand() override = default;
    class KillJobIDDoesntExist: public std::exception{
    public:
        JobId job_id;
        explicit KillJobIDDoesntExist(JobId job_id);
    };
    class KillInvalidArgs: public std::exception{};
    void execute() override;
};

class ForegroundCommand : public BuiltInCommand {
    vector<string> args;
 public:
    ForegroundCommand(const char* cmd_line, vector<string>& args);
    ~ForegroundCommand() override = default;
    class FGJobIDDoesntExist: public std::exception{
    public:
        JobId job_id;
        explicit FGJobIDDoesntExist(JobId job_id);
    };
    class FGEmptyJobsList: public std::exception{};
    class FGInvalidArgs: public std::exception{};
    void execute() override;
};

class BackgroundCommand : public BuiltInCommand {
    vector<string> args;
public:
    BackgroundCommand(const char* cmd_line, vector<string>& args);
    ~BackgroundCommand() override = default;
    class BGJobIDDoesntExist: public std::exception{
    public:
        JobId job_id;
        explicit BGJobIDDoesntExist(JobId job_id);
    };
    class BGJobAlreadyRunning: public std::exception{
    public:
        JobId job_id;
        explicit BGJobAlreadyRunning(JobId job_id);
    };
    class BGNoStoppedJobs: public std::exception{};
    class BGInvalidArgs: public std::exception{};
    void execute() override;
};


// TODO: should it really inhirit from BuiltInCommand ?
class CopyCommand : public BuiltInCommand {
    const char* cmd_line{};
    vector<string> args;
 public:
    CopyCommand(const char* cmd_line, vector<string>& args);
    ~CopyCommand() override = default;
    void execute() override;
    static void copy(const char* infile, const char* outfile);
    class CopyError: public std::exception{};
    class CopyErrorFailedOpen: public std::exception{};
};

// TODO: add more classes if needed 
// maybe chprompt , timeout ?

class SmallShell {
public:
  // TODO: Add your data members
  string prompt;
  pid_t pid;
  string prev_dir;
  //pid_t min_time_job_pid;
  JobEntry fg_job;
  //int fg_timer;
  //pid_t fg_pid;
  //Command* fg_cmd;
  JobsList jobs_list;
  SmallShell();
 //public:
  static Command *CreateCommand(const char* cmd_line);
  SmallShell(SmallShell const&)      = delete; // disable copy ctor
  void operator=(SmallShell const&)  = delete; // disable = operator
  static SmallShell& getInstance() // make SmallShell singleton
  {
    static SmallShell instance; // Guaranteed to be destroyed.
    // Instantiated on first use.
    return instance;
  }
  ~SmallShell()= default;
  void executeCommand(const char* cmd_line);
  string get_prompt() const;
  pid_t get_pid();
  void set_prompt(string input_prompt);
  string get_prev_dir() const;
  void set_prev_dir(string new_dir);
  void replace_fg_and_wait(JobEntry job);
  //void set_min_time_job_pid(pid_t pid);
  // TODO: add extra methods as needed
};

#endif //SMASH_COMMAND_H_
