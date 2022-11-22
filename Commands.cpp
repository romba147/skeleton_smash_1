#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include "Commands.h"
#include <fcntl.h>


#define MAX_ARGS 20
#define MIN_ARGS 2
#define LINUX_MAX_PATH_LENGTH 4096
#define FAIL -1
#define NO_JOB_ID -1

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

bool _is_number (char* string)
{
  for (int i =0; string[i] !='\0';i++)
  {
    if(!isdigit(string[i]))
    {
      return false;
    }
  }
  return true;
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
  else if (firstWord.compare("jobs") == 0)
  {
    return new JobsCommand(cmd_line, jobs_list);
  }
  else {
    return new ExternalCommand(cmd_line);
  }
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


////// Simple Built In Commands ////////

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
  __pid_t pid = getpid();
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


JobsList::JobsList() : jobs_list({}), jobs_counter(0){};

JobsList::JobEntry::JobEntry(int job_id, __pid_t job_pid, time_t entred_list_time, char** process_args, bool is_background): job_id(job_id),
                             job_pid(job_pid),entered_list_time(entered_list_time),process_args(process_args),is_background(is_background),is_stopped(false){};

JobsList::JobEntry* JobsList::getJobById(int jobId) 
{
  for(int i = 0; i < jobs_list.size(); i++)
  {
    if(jobs_list[i].job_id == jobId)
    {
      return &jobs_list[i];
    }
  }
  return nullptr;
}

JobsList::JobEntry* JobsList::getLastJob(int* lastJobId)
{
  int last_job_id = NO_JOB_ID;
  for(int i = 0; i < jobs_list.size(); i++)
  {
    if(jobs_list[i].job_id > last_job_id)
    {
      last_job_id = jobs_list[i].job_id;
    }
  }
  *lastJobId = last_job_id;
  JobEntry* last_job_entry = getJobById(last_job_id);
  return last_job_entry;
}



//TODO add cmd line to job entery
void JobsList::addJob(const char* cmd_line,__pid_t job_pid, bool isStopped)
{
  int args_num = 0;
  char** args_for_job = PrepareArgs(cmd_line, &args_num);
  jobs_list.push_back(JobEntry(jobs_counter, job_pid, time(nullptr), args_for_job, _isBackgroundComamnd(cmd_line)));
  //need to free args for job
  jobs_counter += 1;
}

void JobsList::killAllJobs(int* killed)
{
  this->removeFinishedJobs();
  *killed = jobs_list.size();
  for(int i = 0; i < jobs_list.size(); i++)
  {
    kill(jobs_list[i].job_pid, SIGKILL);
  }
  
}

void JobsList::removeFinishedJobs()
{

}

void JobsList::printJobsList()
{
  for(int i = 0; i < jobs_list.size(); i++)
  {
    time_t elapsed_time = difftime(time(nullptr), jobs_list[i].entered_list_time);
    if(jobs_list[i].is_stopped == true)
    {
      cout << "[" << jobs_list[i].job_id << "] " << jobs_list[i].process_args[0] << " : ";
      cout << jobs_list[i].job_pid << " " << elapsed_time << " " << "(stopped)" << endl;
    }
    else
    {
      cout << "[" << jobs_list[i].job_id << "] " << jobs_list[i].process_args[0] << " : ";
      cout << jobs_list[i].job_pid << " " << elapsed_time << " " << endl;      
    }
  }
}

void JobsList::removeJobById(int jobId)
{

}

JobsList::JobEntry * JobsList::getLastStoppedJob(int *jobId)
{
  
}

//Jobs command

JobsCommand::JobsCommand(const char* cmd_line, JobsList jobs_list) : BuiltInCommand(cmd_line), jobs_list(jobs_list) {}

void JobsCommand::execute() {
  jobs_list.removeFinishedJobs();
  jobs_list.printJobsList();
}

//// External Commandes ////

ExternalCommand::ExternalCommand(const char* cmd_line) : Command(cmd_line){};

void ExternalCommand::execute()
{
  
  std::string command_line = _trim(string(cmd_line));
  bool is_special_command = false;
  for (int i =i ;cmd_line[i] != '\0' ; i++)
  {
    if (cmd_line[i] == '?' || cmd_line[i] == '*')
    {
      is_special_command == true;
      break;
    }
  }

  char new_command [LINUX_MAX_PATH_LENGTH];
  strcpy(new_command,command_line.c_str());

  bool background_command = _isBackgroundComamnd(cmd_line);
  if(background_command)
  {
    _removeBackgroundSign(new_command);
  }  
  //TODO prepare argumants
  char* binbash = "/bin/bash";
  char* special_args[] =  {binbash,"-c",new_command,nullptr};

  int arg_num = 0;
  char** args = PrepareArgs(cmd_line , &arg_num);
  
  __pid_t process_pid = fork();
  if (process_pid ==0)
  {
    if (setpgrp() == FAIL)
    {
      perror("smash: setpgrp failed");
      return;
    }
    if (is_special_command)
    {
      if (execv(binbash,special_args) == FAIL)
      {
        perror("smash: external special process failed");
        return;
      }
    }
    else
    {
      if (execvp(args[0],args) == FAIL)
      {
        perror("smash: external process failed");
        return;
      }
    }
  }
  else
  {
    SmallShell &smash = SmallShell::getInstance();
    if(background_command) 
    {
    smash.jobs_list.addJob(new_command,process_pid,false);
    FreeArgs(args,arg_num);
    }
    else
    {
      int status;
      if(waitpid(process_pid,&status,WUNTRACED) == FAIL)
      {
        perror("smash: wait failed");
      }
    }
  }
   FreeArgs(args,arg_num);
}


//// Foreground Command/////
ForegroundCommand::ForegroundCommand(const char* cmd_line, JobsList* jobs):BuiltInCommand(cmd_line) , jobs_list(jobs){};


void ForegroundCommand::execute()
{
  int arg_num = 0;
  char** args = PrepareArgs(cmd_line , &arg_num);
  if ((arg_num  > 2) || !_is_number(args[1] ))
  {
    cout << "smash error: fg:invalid arguments"<<endl;
    FreeArgs(args,arg_num);
    return;
  }
  int last_job_id;
  JobsList::JobEntry* req_job;

  if (arg_num ==2)
  {
    req_job = jobs_list->getJobById(stoi(args[1]));
    if (!req_job)
    {
      cout << "smash error: fg:job-id "<<args[1]<<" does not exist"<<endl;
      FreeArgs(args,arg_num);
      return;
    }
  }
  else
  {
    int job_id;
    req_job = jobs_list->getLastJob(&job_id);
    if (!req_job)
    {
      cout << "smash error: fg:jobs list is empty"<<endl;
      FreeArgs(args,arg_num);
      return;
    }
  }

  if (req_job->is_stopped)
  {
    kill(req_job->job_pid,SIGCONT);
  }
  //TODO : print cmd line after adding cmd line to entery
  //cout<<req_job.

  jobs_list->removeJobById(req_job->job_id);
  int process_status;
  waitpid(req_job->job_pid,&process_status,WUNTRACED);
  FreeArgs(args,arg_num);
}

//// Background Commaned//////
BackgroundCommand::BackgroundCommand(const char* cmd_line, JobsList* jobs):BuiltInCommand(cmd_line) , jobs_list(jobs){};

void BackgroundCommand::execute()
{
  int arg_num = 0;
  char** args = PrepareArgs(cmd_line , &arg_num);
  if ((arg_num  > 2) || !_is_number(args[1] ))
  {
    cout << "smash error: bg:invalid arguments"<<endl;
    FreeArgs(args,arg_num);
    return;
  }
  int last_job_id;
  JobsList::JobEntry* req_job;

  if (arg_num ==2)
  {
    req_job = jobs_list->getJobById(stoi(args[1]));
    if (!req_job)
    {
      cout << "smash error: bg:job-id "<<args[1]<<" does not exist"<<endl;
      FreeArgs(args,arg_num);
      return;
    }
  }
  else
  {
    int job_id;
    req_job = jobs_list->getLastStoppedJob(&job_id);
    if (!req_job)
    {
      cout << "smash error: bg:there is no stopped jobs to resume"<<endl;
      FreeArgs(args,arg_num);
      return;
    }
  }

  if (!req_job->is_stopped)
  {
      cout << "smash error: bg:job-id "<<args[1]<<" is already running in the background"<<endl;
      FreeArgs(args,arg_num);
      return;
  }

  kill(req_job->job_pid,SIGCONT);
  //TODO : print cmd line after adding cmd line to entery
  //cout<<req_job.

  FreeArgs(args,arg_num);
}


//// Quit /////

QuitCommand::QuitCommand(const char* cmd_line, JobsList* jobs) : BuiltInCommand(cmd_line) , jobs_list(jobs){};

void QuitCommand::execute()
{
  int arg_num = 0;
  char** args = PrepareArgs(cmd_line , &arg_num);
  string killfalg = "kill";
  if (killfalg.compare(args[1]) == 0)
  {
    int killed;
    jobs_list.killAllJobs(&killed);

    cout << "sending SIGKILL signal to " <<killed<<" jobs:" <<endl;
  }

}

