#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
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

    vector<string> args(COMMAND_MAX_ARGS);
    int i = 0;
    std::istringstream iss(_trim(string(cmd_line)).c_str());
    for(std::string s; iss >> s;) {
        args[i] = s;
        cout << "@" << args[i] << "@" << endl;
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

SmallShell::SmallShell() : prompt("smash"){
// TODO: add your implementation
}

SmallShell::~SmallShell() {}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command * SmallShell::CreateCommand(const char* cmd_line) {
	// For example:
  auto args=_parseCommandLine(cmd_line);
  string cmd_s = string(cmd_line);
  if (cmd_s.find("pwd") == 0) {
    return new GetCurrDirCommand(cmd_line);
  }
  else if(cmd_s.find("quit") == 0) {
      return new QuitCommand(cmd_line);
  }
  else {
    return new ExternalCommand(cmd_line);
  }

}

void SmallShell::executeCommand(const char *cmd_line) {
  // TODO: Add your implementation here
   Command* cmd = CreateCommand(cmd_line);
   try {
       cmd->execute();
       delete cmd;
   }
   catch(Command::Quit& quit) {
       delete cmd;
       throw quit;
   }
   //Please note that you must fork smash process for some commands (e.g., external commands....)
}

string SmallShell::get_prompt() const {
    return prompt;
}

void SmallShell::set_prompt(string input_prompt) {
    prompt=input_prompt;

}

Command::Command(const char *cmd_line) : cmd_line(cmd_line){

}

BuiltInCommand::BuiltInCommand(const char *cmd_line) : Command(cmd_line) {

}


GetCurrDirCommand::GetCurrDirCommand(const char *cmd_line) : BuiltInCommand(
        cmd_line) {

}

void GetCurrDirCommand::execute() {
    auto pwd=get_current_dir_name();
    cout<< pwd << endl;
    free(pwd);

}

ExternalCommand::ExternalCommand(const char *cmd_line) : Command(cmd_line) {

}

void ExternalCommand::execute() {
    if(string(cmd_line).empty())
        return;

}

QuitCommand::QuitCommand(const char *cmd_line, JobsList *jobs) : BuiltInCommand(cmd_line),to_kill(jobs){

}

void QuitCommand::execute() {
    throw Quit();
}