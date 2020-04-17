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

class Command {
// TODO: Add your data members
protected:
  const string cmd_line;
  const string command;
public:
  bool bg;
  const time_t time_in;
  explicit Command(const char* cmd_line, bool bg);
  virtual ~Command()= default;;
  virtual void execute() = 0;
  // TODO: Add your extra methods if needed
  string getCommand() const;
  string get_cmd_line() const;
};

class BuiltInCommand : public Command {
 public:
  explicit BuiltInCommand(const char* cmd_line);
  ~BuiltInCommand() override = default;
};

class ExternalCommand : public Command {
 public:
  explicit ExternalCommand(const char* cmd_line, bool bg);
  ~ExternalCommand() override = default;
  void execute() override;
};

class PipeCommand : public Command {
    Command* command1;
    Command* command2;
    string sign;
public:
      explicit PipeCommand(const char* cmd_line_c, vector<string> args, string sign, bool bg);
      ~PipeCommand() override = default;
      void execute() override;
};

class RedirectionCommand : public Command {
    Command* cmd_left;
    string output_file;
    string IO_type;
 public:
  explicit RedirectionCommand(const char* cmd_line, const string&  IO_type, bool bg);
  ~RedirectionCommand() override = default;
  void execute() override;
  //void prepare() override;
  //void cleanup() override;
  class RedirectionFailedOpen : public std::exception{};
  class RedirectionFailedWrite : public std::exception{};
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
// TODO: Add your data members
public:
    explicit ChangeDirCommand(const char* cmd_line, vector<string>& args);
    class TooManyArgs : public std::exception{};
    class NoOldPWD : public std::exception{};
    ~ChangeDirCommand() override = default;//TODO: free dirs
    void execute() override;
};


//class JobsList;

class Quit : public std::exception{};


class QuitCommand : public BuiltInCommand {
    vector<string> args;
public:
  QuitCommand(const char* cmd_line, vector<string>& args);
  ~QuitCommand() override = default;
  void execute() override;

};


class JobsList {
public:
    typedef unsigned int JobId;
private:
    class JobEntry {
      public:
        Command* cmd;
        pid_t pid;
        JobId job_id;
        bool isStopped;
        JobEntry(Command* cmd, pid_t pid, JobId job_id, bool isStopped);
        int Kill(int signal= SIGKILL);
        bool finish() const;
        ~JobEntry()= default;
        friend std::ostream& operator<<(std::ostream& os, const JobEntry& job);
  };
  std::vector<JobEntry> list;
  vector<JobsList::JobEntry*> last_stopped_jobs;
  JobId allocJobId() const;
public:
  JobsList()= default;
  ~JobsList();
  void addJob(Command* cmd, pid_t pid, bool isStopped = false);
  void printJobsList() const;
  void killAllJobs();
  void removeFinishedJobs();
  JobEntry * getJobByPID(pid_t);
  JobEntry * getJobByJobID(JobId);
  void removeJobById(JobId);
  JobEntry * getLastJob(const JobId* lastJobId);
  JobEntry *getLastStoppedJob();
  void removeFromStopped(JobId);
  void pushToStopped(JobEntry*);

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
    JobsList::JobId job_id;
    int sig_num;
public:
    KillCommand(const char *cmdLine1, vector<string> &args);
    ~KillCommand() override = default;
    class KillJobIDDoesntExist: public std::exception{
    public:
        JobsList::JobId job_id;
        explicit KillJobIDDoesntExist(JobsList::JobId job_id);
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
        JobsList::JobId job_id;
        explicit FGJobIDDoesntExist(JobsList::JobId job_id);
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
        JobsList::JobId job_id;
        explicit BGJobIDDoesntExist(JobsList::JobId job_id);
    };
    class BGJobAlreadyRunning: public std::exception{
    public:
        JobsList::JobId job_id;
        explicit BGJobAlreadyRunning(JobsList::JobId job_id);
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
  pid_t fg_pid;
  Command* fg_cmd;
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
  static void executeCommand(const char* cmd_line);
  string get_prompt() const;
  pid_t get_pid();
  void set_prompt(string input_prompt);
  string get_prev_dir() const;
  void set_prev_dir(string new_dir);
  void set_fg(pid_t fg_pid, Command* fg_cmd);
  // TODO: add extra methods as needed
};

#endif //SMASH_COMMAND_H_
