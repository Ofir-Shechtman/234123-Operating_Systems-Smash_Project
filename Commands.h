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
  const char *const cmd_line;
  //pid_t pid;
 public:
  bool bg;
  explicit Command(const char* cmd_line);
  virtual ~Command()= default;;
  virtual void execute() = 0;
  //virtual void prepare();
  //virtual void cleanup();
  // TODO: Add your extra methods if needed
  //bool isFinish() const;
  //void Kill() const;
  string getCommand() const;
  string get_cmd_line() const;
  //pid_t get_pid() const;
  //void set_pid(pid_t pid);
};

class BuiltInCommand : public Command {
 public:
  explicit BuiltInCommand(const char* cmd_line);
  virtual ~BuiltInCommand() {}
};

class ExternalCommand : public Command {
 public:
  explicit ExternalCommand(const char* cmd_line);
  virtual ~ExternalCommand() {}
  void execute() override;
};

class PipeCommand : public Command {
  // TODO: Add your data members
 public:
  PipeCommand(const char* cmd_line);
  virtual ~PipeCommand() {}
  void execute() override;
};

class RedirectionCommand : public Command {
 // TODO: Add your data members
 public:
  explicit RedirectionCommand(const char* cmd_line);
  virtual ~RedirectionCommand() {}
  void execute() override;
  //void prepare() override;
  //void cleanup() override;
};

class ChangePromptCommand : public BuiltInCommand {
    string input_prompt;
public:
    explicit ChangePromptCommand(const char* cmd_line, vector<string> args);
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
    explicit ChangeDirCommand(const char* cmd_line, vector<string> args);
    class TooManyArgs : public std::exception{};
    class NoOldPWD : public std::exception{};
    ~ChangeDirCommand() override = default;//TODO: free dirs
    void execute() override;
};


class JobsList;

class QuitCommand : public BuiltInCommand {
    JobsList* to_kill;
    vector<string> args;
public:
  QuitCommand(const char* cmd_line, JobsList* jobs, vector<string> args);
  virtual ~QuitCommand() {}
  void execute() override;
  class Quit : public std::exception{};
};

class CommandsHistory {
 protected:
  class CommandHistoryEntry {
	  // TODO: Add your data members
  };
 // TODO: Add your data members
 public:
  CommandsHistory();
  ~CommandsHistory() {}
  void addRecord(const char* cmd_line);
  void printHistory();
};

class HistoryCommand : public BuiltInCommand {
 // TODO: Add your data members
 public:
  HistoryCommand(const char* cmd_line, CommandsHistory* history);
  virtual ~HistoryCommand() {}
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
        time_t time_in;
        bool isStopped;
        JobEntry(Command* cmd, pid_t pid, JobId job_id, bool isStopped);
        void Kill();
        bool finish() const;
        ~JobEntry()= default;
        //JobEntry(JobEntry&)= delete;
        friend std::ostream& operator<<(std::ostream& os, const JobEntry& job);
  };
  std::vector<JobEntry> list;
  JobId allocJobId() const;
public:
  JobsList()= default;
  ~JobsList();
  void addJob(Command* cmd, pid_t pid, bool isStopped = false);
  void printJobsList() const;
  void killAllJobs();
  void removeFinishedJobs();
  JobEntry * getJobById(JobId);
  void removeJobById(JobId);
  JobEntry * getLastJob(JobId* lastJobId);
  JobEntry *getLastStoppedJob(JobId* jobId);
  bool empty() const;
  friend std::ostream& operator<<(std::ostream& os, const JobEntry& job);

};

class JobsCommand : public BuiltInCommand {
 // TODO: Add your data members
 JobsList* jobs;
 public:
  JobsCommand(const char* cmd_line, JobsList* jobs);
  virtual ~JobsCommand() {}
  void execute() override;
};

class KillCommand : public BuiltInCommand {
 // TODO: Add your data members
 public:
  KillCommand(const char* cmd_line);
  virtual ~KillCommand() {}
  void execute() override;
};

class ForegroundCommand : public BuiltInCommand {
    JobsList* jobs;
    vector<string> args;
 public:
    ForegroundCommand(const char* cmd_line, JobsList* jobs, vector<string> args);
    virtual ~ForegroundCommand()= default;
    void execute() override;
};

class BackgroundCommand : public BuiltInCommand {
    JobsList* jobs;
 public:
    BackgroundCommand(const char* cmd_line, JobsList* jobs);
    virtual ~BackgroundCommand() {}
    void execute() override;
};


// TODO: should it really inhirit from BuiltInCommand ?
class CopyCommand : public BuiltInCommand {
    const char* cmd_line;
    vector<string> args;
 public:
    CopyCommand(const char* cmd_line, vector<string>& args);
    virtual ~CopyCommand()= default;
    void execute() override;
    void copy(const char* infile, const char* outfile);
    class CopyError: public std::exception{};
    class CopyErrorFailedOpen: public std::exception{};
};

// TODO: add more classes if needed 
// maybe chprompt , timeout ?

class SmallShell {
public:
  // TODO: Add your data members
  string prompt;
  JobsList jobs_list;
  pid_t pid;
  string prev_dir;
  pid_t fg_pid;
  Command* fg_cmd;
  SmallShell();
 //public:
  Command *CreateCommand(const char* cmd_line);
  SmallShell(SmallShell const&)      = delete; // disable copy ctor
  void operator=(SmallShell const&)  = delete; // disable = operator
  static SmallShell& getInstance() // make SmallShell singleton
  {
    static SmallShell instance; // Guaranteed to be destroyed.
    // Instantiated on first use.
    return instance;
  }
  ~SmallShell();
  void executeCommand(const char* cmd_line);
  string get_prompt() const;
  pid_t get_pid();
  void set_prompt(string input_prompt);
  string get_prev_dir() const;
  void set_prev_dir(string new_dir);
  void set_fg(pid_t fg_pid, Command* fg_cmd);
  // TODO: add extra methods as needed
};

#endif //SMASH_COMMAND_H_
