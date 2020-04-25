#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <vector>
#include <ctime>
#include <cstring>
#include <iostream>
#include <vector>
#include <sstream>
#include <fcntl.h>
#include <csignal>
#include "system_functions.h"

using std::string;
using std::cout;
using std::vector;
#define COMMAND_ARGS_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)
#define HISTORY_MAX_RECORDS (50)
#define MAXINT 2147483647

class Quit : public std::exception{};
class Continue : public std::exception{};


class Command {
protected:
  const string cmd_line;
public:
  bool bg;
  int create_time;
  explicit Command(const char* cmd_line, bool bg);
  virtual ~Command()= default;;
  virtual void execute() = 0;
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
    int time_in_list() const;
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
    string s_command1;
    string s_command2;
    string sign;
public:
      explicit PipeCommand(const char* cmd_line_c, const string& sign);
      ~PipeCommand() override = default;;
      void execute() override;
};

class RedirectionCommand : public Command {
    string cmd_left_line;
    string output_file;
    string IO_type;
 public:
  explicit RedirectionCommand(const char* cmd_line, const string&  IO_type);
  ~RedirectionCommand() override= default;
  void execute() override;
  void execute_built_in();
  void execute_external();
};

class TimeoutCommand : public Command {
    int timer;
    string cmd_s;
public:
    explicit TimeoutCommand(const char* cmd_line, vector<string> args);
    ~TimeoutCommand() override= default;
    class TimerInvalidArgs: public Continue{};
    void execute() override;
    void execute_built_in();
    void execute_external();
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
    class TooManyArgs : public Continue{};
    class TooFewArgs : public Continue{};
    class NoOldPWD : public Continue{};
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
  JobId allocJobId() const;
public:
  JobsList()= default;
  ~JobsList()= default;
  void addJob(JobEntry);
  void printJobsList() const;
  void killAllJobs();
  void killAllStopedJobs();
  void removeFinishedJobs();
  void deleteAll();
  JobEntry * getJobByPID(pid_t);
  JobEntry * getJobByJobID(JobId);
  JobEntry * getLastJob(const JobId* lastJobId= nullptr);
  JobEntry *getLastStoppedJob();
  void pushToStopped(JobId);
  void removeFromStopped(JobId);

  void removeTimeoutJobs();
  void reset_timer(int new_timer=0);

  bool empty() const;
  friend std::ostream& operator<<(std::ostream& os, const JobEntry& job);

};

class JobsCommand : public BuiltInCommand {
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
    class KillJobIDDoesntExist: public Continue{
    public:
        JobId job_id;
        explicit KillJobIDDoesntExist(JobId job_id);
    };
    class KillInvalidArgs: public Continue{};
    void execute() override;
};

class ForegroundCommand : public BuiltInCommand {
    vector<string> args;
 public:
    ForegroundCommand(const char* cmd_line, vector<string>& args);
    ~ForegroundCommand() override = default;
    class FGJobIDDoesntExist: public Continue{
    public:
        JobId job_id;
        explicit FGJobIDDoesntExist(JobId job_id);
    };
    class FGEmptyJobsList: public Continue{};
    class FGInvalidArgs: public Continue{};
    void execute() override;
};

class BackgroundCommand : public BuiltInCommand {
    vector<string> args;
public:
    BackgroundCommand(const char* cmd_line, vector<string>& args);
    ~BackgroundCommand() override = default;
    void execute() override;
    class BGJobIDDoesntExist: public Continue{
    public:
        JobId job_id;
        explicit BGJobIDDoesntExist(JobId job_id);
    };
    class BGJobAlreadyRunning: public Continue{
    public:
        JobId job_id;
        explicit BGJobAlreadyRunning(JobId job_id);
    };
    class BGNoStoppedJobs: public Continue{};
    class BGInvalidArgs: public Continue{};
};


class CopyCommand : public BuiltInCommand {
    vector<string> args;
 public:
    CopyCommand(const char* cmd_line, vector<string>& args);
    ~CopyCommand() override = default;
    void execute() override;
    static void copy(const char* infile, const char* outfile);
};


class SmallShell {
public:
  string prompt;
  pid_t pid;
  string prev_dir;
  JobEntry fg_job;
  JobsList jobs_list;
  SmallShell();
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
  void replace_fg_and_wait(JobEntry job = JobEntry());
  bool is_smash() const;
};

#endif //SMASH_COMMAND_H_
