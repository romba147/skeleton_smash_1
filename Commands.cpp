#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include "Commands.h"

#define MAX_ARGS 20
#define MIN_ARGS 2
#define LINUX_MAX_PATH_LENGTH 4096

using namespace std;
const std::string WHITESPACE = " \n\r\t\f\v";

#if 0
#define FUNC_ENTRY()  \
  cout << __PRETTY_FUNCTION__ << " --> " << endl;

#define FUNC_EXIT()  \
  cout << __PRETTY_FUNCTION__ << " <-- " << endl;
#else
#define FUNC_ENTRY()
#define FUNC_EXIT()
#endif

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

static char** PrepareArgs (const char* cmd_line, int* arg_num)
{
  char** args = (char**) malloc(sizeof(char *)*MAX_ARGS); 
  if (args == nullptr) return nullptr;
  for (int i =0; i<MAX_ARGS; i++) {
    args[i] = nullptr;
  }
  *arg_num = _parseCommandLine(cmd_line,args);
  return args;
}

void FreeArgs (char** args , int arg_num)
{
  for (int i = 0; i<arg_num; i++)
  {
    free(args[i]);
  }
  free (args);
}

bool _isBackgroundComamnd(const char* cmd_line) {
  const string str(cmd_line);
  return str[str.find_last_not_of(WHITESPACE)] == '&';
}

void _removeBackgroundSign(char* cmd_line) {
  const string str(cmd_line);
  // find last character other than spaces
  unsigned int idx = str.find_last_not_of(WHITESPACE);
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

SmallShell::SmallShell() : line_prompt("smash"),last_pwd(nullptr){};

SmallShell::~SmallShell() {
// TODO: add your implementation
}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command * SmallShell::CreateCommand(const char* cmd_line) {
	// For example:

  string cmd_s = _trim(string(cmd_line));
  string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));

 /* if (firstWord.compare("pwd") == 0) {
    return new GetCurrDirCommand(cmd_line);
  }
  else if (firstWord.compare("showpid") == 0) {
    return new ShowPidCommand(cmd_line);
 }
 */
  if (firstWord.compare("chprompt") == 0) {
    return new ChpromptCommand(cmd_line);
  }
  else if (firstWord.compare("showpid") == 0)
  {
    return new ShowPidCommand(cmd_line);
  }
  else if(firstWord.compare("pwd") == 0)
  {
    return new GetCurrDirCommand(cmd_line);
  }
  else if (firstWord.compare("cd") == 0)
  {
    return new ChangeDirCommand(cmd_line,&last_pwd);
  }
  /*
  else {
    return new ExternalCommand(cmd_line);
  }
  */
  return nullptr;
  
}

void SmallShell::executeCommand(const char *cmd_line) {
  // TODO: Add your implementation here
  // for example:
   Command* cmd = CreateCommand(cmd_line);
   cmd->execute();
  // Please note that you must fork smash process for some commands (e.g., external commands....)
}

/////// Command Constructor //////

Command::Command (const char* cmd) :cmd_line(cmd){};

BuiltInCommand::BuiltInCommand(const char* cmd_line) : Command(cmd_line) {};


////// Built In Commands ////////

// Change Prompt

ChpromptCommand::ChpromptCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {};

void ChpromptCommand::execute() {
  int arg_num = 0;
  char** args = PrepareArgs(cmd_line , &arg_num);
  std::string new_prompt;
  if (arg_num < MIN_ARGS) new_prompt = "smash";
  else new_prompt = args[1];

  SmallShell & shell = SmallShell::getInstance();
  shell.line_prompt = new_prompt;
  FreeArgs(args,arg_num);
}

// Show PID

ShowPidCommand::ShowPidCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {};

void ShowPidCommand::execute() {
  pid_t pid = getpid();
  cout<<"smash pid is "<< pid << endl;
}

// PWD 

GetCurrDirCommand::GetCurrDirCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {};

void GetCurrDirCommand::execute() {
  char* path_buffer = (char *) malloc(sizeof(char) * LINUX_MAX_PATH_LENGTH);
  if(!path_buffer) return;
  cout << getcwd(path_buffer, LINUX_MAX_PATH_LENGTH) << endl;
  free(path_buffer);
}

// CD

ChangeDirCommand::ChangeDirCommand(const char* cmd_line, char** plastPwd) : BuiltInCommand(cmd_line), plastPwd(plastPwd) {};

void ChangeDirCommand::execute() {
  char* plast_pwd_buffer = (char *) malloc(sizeof(char) * LINUX_MAX_PATH_LENGTH);
  int arg_num = 0;
  char** args = PrepareArgs(cmd_line , &arg_num);
  string special_char = "-";
  if (arg_num > 2)
  {
    cout << "smash error: cd: too many arguments" << endl;
  }
  else if((special_char.compare(args[1]) == 0) && !(plastPwd))
  {
    cout << "smash error: cd: OLDPWD not set" << endl;
  }

  else 
  {
    getcwd(plast_pwd_buffer, LINUX_MAX_PATH_LENGTH);
    if (special_char.compare(args[1]) == 0)
    {
      chdir(*plastPwd);
    }
    else
    {
      chdir(args[1]);
    }
    if (*plastPwd){
      free(*plastPwd);
    }
    *plastPwd = plast_pwd_buffer;
  }
FreeArgs(args, arg_num);
}


//// Jobs List ////


JobsList::JobsList(vector<JobEntry> jobs_list, int jobs_counter) : jobs_list({}), jobs_counter(0){};

JobsList::JobsList::JobEntry(int job_id, pid_t job_pid, time_t entered_list_time, char** process_args, bool is_background) : 
                              job_id(job_id), job_pid(job_pid) , entered_list_time(entered_list_time), process_args(process_args), is_background(is_background)
                              is_stopped(false) {}

//// External Commandes ////


//
