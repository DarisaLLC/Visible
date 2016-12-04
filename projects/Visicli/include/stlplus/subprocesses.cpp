/*------------------------------------------------------------------------------

  Author:    Andy Rushton
  Copyright: (c) Andy Rushton, 2004
  License:   BSD License, see ../docs/license.html

  ------------------------------------------------------------------------------*/
#include "os_fixes.hpp"
#include "subprocesses.hpp"
#include "debug.hpp"
#include "fileio.hpp"
#include "file_system.hpp"
#include "string_utilities.hpp"
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#ifdef MSWINDOWS
#else
#include <signal.h>
#include <errno.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#endif

////////////////////////////////////////////////////////////////////////////////
// argument-vector related stuff

static void skip_white (const std::string& command, unsigned& i)
{
  while(i < command.size() && isspace(command[i]))
    i++;
}

// get_argument is the main function for breaking a string down into separate command arguments
// it mimics the way shells break down a command into an argv[] and unescapes the escaped characters on the way

#ifdef MSWINDOWS

// as far as I know, there is only double-quoting and no escape character in DOS
// so, how do you include a double-quote in an argument???

static std::string get_argument (const std::string& command, unsigned& i)
{
  std::string result;
  bool dquote = false;
  for ( ; i < command.size(); i++)
  {
    char ch = command[i];
    if (!dquote && isspace(ch)) break;
    if (dquote)
    {
      if (ch == '\"')
        dquote = false;
      else
        result += ch;
    }
    else if (ch == '\"')
      dquote = true;
    else
      result += ch;
  }
  return result;
}

#else

static std::string get_argument (const std::string& command, unsigned& i)
{
  std::string result;
  bool squote = false;
  bool dquote = false;
  bool escaped = false;
  for ( ; i < command.size(); i++)
  {
    char ch = command[i];
    if (!squote && !dquote && !escaped && isspace(ch)) break;
    if (escaped)
    {
      result += ch;
      escaped = false;
    }
    else if (squote)
    {
      if (ch == '\'')
        squote = false;
      else
        result += ch;
    }
    else if (ch == '\\')
      escaped = true;
    else if (dquote)
    {
      if (ch == '\"')
        dquote = false;
      else
        result += ch;
    }
    else if (ch == '\'')
      squote = true;
    else if (ch == '\"')
      dquote = true;
    else
      result += ch;
  }
  return result;
}

#endif

// this function performs the reverse of the above on a single argument
// it escapes special characters and quotes the argument if necessary ready for shell interpretation

#ifdef MSWINDOWS

static std::string make_argument (const std::string& arg)
{
  std::string result;
  bool needs_quotes = false;
  for (unsigned i = 0; i < arg.size(); i++)
  {
    switch (arg[i])
    {
      // set of whitespace characters that force quoting
    case ' ':
      result += arg[i];
      needs_quotes = true;
      break;
    default:
      result += arg[i];
      break;
    }
  }
  if (needs_quotes)
  {
    result.insert(result.begin(), '"');
    result += '"';
  }
  return result;
}

#else

static std::string make_argument (const std::string& arg)
{
  std::string result;
  bool needs_quotes = false;
  for (unsigned i = 0; i < arg.size(); i++)
  {
    switch (arg[i])
    {
      // set of characters requiring escapes
    case '\\': case '\'': case '\"': case '`': case '(': case ')': 
    case '&': case '|': case '<': case '>': case '*': case '?': case '!':
      result += '\\';
      result += arg[i];
      break;
      // set of whitespace characters that force quoting
    case ' ':
      result += arg[i];
      needs_quotes = true;
      break;
    default:
      result += arg[i];
      break;
    }
  }
  if (needs_quotes)
  {
    result.insert(result.begin(), '"');
    result += '"';
  }
  return result;
}

#endif

static char* copy_string (const char* str)
{
  char* result = new char[strlen(str)+1];
  strcpy(result,str);
  return result;
}

////////////////////////////////////////////////////////////////////////////////

arg_vector::arg_vector (void)
{
  v = 0;
}

arg_vector::arg_vector (const arg_vector& a)
{
  v = 0;
  *this = a;
}

arg_vector::arg_vector (char** a)
{
  v = 0;
  *this = a;
}

arg_vector::arg_vector (const std::string& command)
{
  v = 0;
  *this = command;
}

arg_vector::arg_vector (const char* command)
{
  v = 0;
  *this = command;
}

arg_vector::~arg_vector (void)
{
  clear();
}

arg_vector& arg_vector::operator = (const arg_vector& a)
{
  return *this = a.v;
}

arg_vector& arg_vector::operator = (char** a)
{
  clear();
  for (unsigned i = 0; a[i]; i++)
    operator += (a[i]);
  return *this;
}

arg_vector& arg_vector::operator = (const std::string& command)
{
  clear();
  for (unsigned i = 0; i < command.size(); )
  {
    std::string argument = get_argument(command, i);
    operator += (argument);
    skip_white(command, i);
  }
  return *this;
}

arg_vector& arg_vector::operator = (const char* command)
{
  return operator = (std::string(command));
}

arg_vector& arg_vector::operator += (const std::string& str)
{
  insert(size(), str);
  return *this;
}

arg_vector& arg_vector::operator -= (const std::string& str)
{
  insert(0, str);
  return *this;
}

void arg_vector::insert (unsigned index, const std::string& str)
{
  DEBUG_ASSERT(index <= size());
  // copy up to but not including index, then add the new argument, then copy the rest
  char** new_argv = new char*[size()+2];
  unsigned i = 0;
  for ( ; i < index; i++)
    new_argv[i] = copy_string(v[i]);
  new_argv[index] = copy_string(str.c_str());
  for ( ; i < size(); i++)
    new_argv[i+1] = copy_string(v[i]);
  new_argv[i+1] = 0;
  clear();
  v = new_argv;
}

void arg_vector::clear (unsigned index)
{
  DEBUG_ASSERT(index < size());
  // copy up to index, skip it, then copy the rest
  char** new_argv = new char*[size()];
  unsigned i = 0;
  for ( ; i < index; i++)
    new_argv[i] = copy_string(v[i]);
  i++;
  for ( ; i < size(); i++)
    new_argv[i-1] = copy_string(v[i]);
  new_argv[i-1] = 0;
  clear();
  v = new_argv;
}

void arg_vector::clear(void)
{
  if (v)
  {
    for (unsigned i = 0; v[i]; i++)
      delete[] v[i];
    delete[] v;
    v = 0;
  }
}

unsigned arg_vector::size (void) const
{
  unsigned i = 0;
  if (v) while (v[i]) i++;
  return i;
}

arg_vector::operator char** (void) const
{
  return v;
}

char** arg_vector::argv (void) const
{
  return v;
}

char* arg_vector::operator [] (unsigned index) const
{
  DEBUG_ASSERT(index < size());
  return v[index];
}

char* arg_vector::argv0 (void) const
{
  return operator [] (0);
}

std::string arg_vector::image (void) const
{
  std::string result;
  for (unsigned i = 0; i < size(); i++)
  {
    if (i) result += ' ';
    result += make_argument(v[i]);
  }
  return result;
}

otext& operator << (otext& t, const arg_vector& a)
{
  return t << a.image();
}

////////////////////////////////////////////////////////////////////////////////
// environment-vector
// Windows environment is a single string containing null-terminated name=value strings and the whole terminated by a null
// Unix environment is a null-terminated vector of pointers to null-terminated strings

#ifdef MSWINDOWS

static unsigned envp_size(const char* envp)
{
  unsigned size = 0;
  while (envp[size] || (size > 0 && envp[size-1])) size++;
  size++;
  return size;
}

static void envp_extract(std::string& name, std::string& value, const char* envp, unsigned& envi)
{
  name.erase();
  value.erase();
  if (!envp[envi]) return;
  // some special variables start with '=' so ensure at least one character in the name
  name += envp[envi++];
  while(envp[envi] != '=')
    name += envp[envi++];
  envi++;
  while(envp[envi])
    value += envp[envi++];
  envi++;
}

static void envp_append(const std::string& name, const std::string& value, char* envp, unsigned& envi)
{
  for (unsigned i = 0; i < name.size(); i++)
    envp[envi++] = name[i];
  envp[envi++] = '=';
  for (unsigned j = 0; j < value.size(); j++)
    envp[envi++] = value[j];
  envp[envi++] = '\0';
  envp[envi] = '\0';
}

static char* envp_copy(const char* envp)
{
  unsigned size = envp_size(envp);
  char* result = new char[size];
  result[0] = '\0';
  unsigned i = 0;
  unsigned j = 0;
  while(envp[i])
  {
    std::string name;
    std::string value;
    envp_extract(name, value, envp, i);
    envp_append(name, value, result, j);
  }
  return result;
}

static void envp_clear(char*& envp)
{
  if (envp)
  {
    delete[] envp;
    envp = 0;
  }
}

#else

static unsigned envp_size(char* const* envp)
{
  unsigned size = 0;
  while(envp[size]) size++;
  size++;
  return size;
}

static void envp_extract(std::string& name, std::string& value, char* const* envp, unsigned& envi)
{
  name = "";
  value = "";
  if (!envp[envi]) return;
  unsigned i = 0;
  while(envp[envi][i] != '=')
    name += envp[envi][i++];
  i++;
  while(envp[envi][i])
    value += envp[envi][i++];
  envi++;
}

static void envp_append(const std::string& name, const std::string& value, char** envp, unsigned& envi)
{
  std::string entry = name + "=" + value;
  envp[envi] = copy_string(entry.c_str());
  envi++;
  envp[envi] = 0;
}

static char** envp_copy(char* const* envp)
{
  unsigned size = envp_size(envp);
  char** result = new char*[size];
  unsigned i = 0;
  unsigned j = 0;
  while(envp[i])
  {
    std::string name;
    std::string value;
    envp_extract(name, value, envp, i);
    envp_append(name, value, result, j);
  }
  return result;
}

static void envp_clear(char**& envp)
{
  if (envp)
  {
    for (unsigned i = 0; envp[i]; i++)
      delete[] envp[i];
    delete[] envp;
    envp = 0;
  }
}

#endif

#ifdef MSWINDOWS

env_vector::env_vector(void)
{
  char* env = (char*)GetEnvironmentStrings();
  v = envp_copy(env);
  FreeEnvironmentStrings(env);
}

#else

extern char** environ;

env_vector::env_vector (void)
{
  v = envp_copy(environ);
}

#endif

env_vector::env_vector (const env_vector& a)
{
  v = 0;
  *this = a;
}

env_vector::~env_vector (void)
{
  clear();
}

env_vector& env_vector::operator = (const env_vector& a)
{
  clear();
  v = envp_copy(a.v);
  return *this;
}

void env_vector::clear(void)
{
  envp_clear(v);
}

#ifdef MSWINDOWS

void env_vector::add(const std::string& name, const std::string& value)
{
  // the trick is to add the value in alphabetic order
  // this is done by copying the existing environment string to a new string, inserting the new value when a name greater than it is found
  unsigned size = envp_size(v);
  unsigned new_size = size + name.size() + value.size() + 2;
  char* new_v = new char[new_size];
  new_v[0] = '\0';
  // now envp_extract each name=value pair and chk the ordering
  bool added = false;
  unsigned i = 0;
  unsigned j = 0;
  while(v[i])
  {
    std::string current_name;
    std::string current_value;
    envp_extract(current_name, current_value, v, i);
    // Note: Windows variables are case-insensitive!
    if (lowercase(name) == lowercase(current_name))
    {
      // replace an existing value
      envp_append(name, value, new_v, j);
    }
    else if (!added && lowercase(name) < lowercase(current_name))
    {
      // add the new value first, then the existing one
      envp_append(name, value, new_v, j);
      envp_append(current_name, current_value, new_v, j);
      added = true;
    }
    else
    {
      // just add the existing value
      envp_append(current_name, current_value, new_v, j);
    }
  }
  if (!added)
    envp_append(name, value, new_v, j);
  envp_clear(v);
  v = new_v;
}

#else

void env_vector::add(const std::string& name, const std::string& value)
{
  // the trick is to add the value in alphabetic order
  // this is done by copying the existing environment vector and inserting the new value when a name greater than it is found
  unsigned size = envp_size(v);
  unsigned new_size = size + 1;
  char** new_v = new char*[new_size];
  new_v[0] = 0;
  // now extract each name=value pair and chk the ordering
  bool added = false;
  unsigned i = 0;
  unsigned j = 0;
  while(v[i])
  {
    std::string current_name;
    std::string current_value;
    envp_extract(current_name, current_value, v, i);
    // Note: Unix variables are case-sensitive
    if (name == current_name)
    {
      // replace an existing value
      envp_append(name, value, new_v, j);
    }
    else if (!added && name < current_name)
    {
      // add the new value first, then the existing one
      envp_append(name, value, new_v, j);
      envp_append(current_name, current_value, new_v, j);
      added = true;
    }
    else
    {
      // just add the existing value
      envp_append(current_name, current_value, new_v, j);
    }
  }
  if (!added)
    envp_append(name, value, new_v, j);
  envp_clear(v);
  v = new_v;
}

#endif

#ifdef MSWINDOWS

bool env_vector::remove (const std::string& name)
{
  bool result = false;
  // this is done by copying the existing environment string to a new string, but excluding the specified name
  unsigned size = envp_size(v);
  char* new_v = new char[size];
  new_v[0] = '\0';
  unsigned i = 0;
  unsigned j = 0;
  while(v[i])
  {
    std::string current_name;
    std::string current_value;
    envp_extract(current_name, current_value, v, i);
    if (lowercase(name) == lowercase(current_name))
      result = true;
    else
      envp_append(current_name, current_value, new_v, j);
  }
  envp_clear(v);
  v = new_v;
  return result;
}

#else

bool env_vector::remove (const std::string& name)
{
  bool result = false;
  // this is done by copying the existing environment vector, but excluding the specified name
  unsigned size = envp_size(v);
  char** new_v = new char*[size];
  new_v[0] = 0;
  unsigned i = 0;
  unsigned j = 0;
  while(v[i])
  {
    std::string current_name;
    std::string current_value;
    envp_extract(current_name, current_value, v, i);
    if (name == current_name)
      result = true;
    else
      envp_append(current_name, current_value, new_v, j);
  }
  envp_clear(v);
  v = new_v;
  return result;
}

#endif

std::string env_vector::operator [] (const std::string& name) const
{
  return get(name);
}

#ifdef MSWINDOWS

std::string env_vector::get (const std::string& name) const
{
  unsigned i = 0;
  while(v[i])
  {
    std::string current_name;
    std::string current_value;
    envp_extract(current_name, current_value, v, i);
    if (lowercase(name) == lowercase(current_name))
      return current_value;
  }
  return std::string();
}

#else

std::string env_vector::get (const std::string& name) const
{
  unsigned i = 0;
  while(v[i])
  {
    std::string current_name;
    std::string current_value;
    envp_extract(current_name, current_value, v, i);
    if (name == current_name)
      return current_value;
  }
  return std::string();
}

#endif

#ifdef MSWINDOWS

unsigned env_vector::size (void) const
{
  unsigned i = 0;
  unsigned offset = 0;
  while(v[offset])
  {
    std::string current_name;
    std::string current_value;
    envp_extract(current_name, current_value, v, offset);
    i++;
  }
  return i;
}

#else

unsigned env_vector::size (void) const
{
  unsigned i = 0;
  while(v[i])
    i++;
  return i;
}

#endif

std::pair<std::string,std::string> env_vector::operator [] (unsigned index) const
{
  return get(index);
}

std::pair<std::string,std::string> env_vector::get (unsigned index) const
{
  unsigned j = 0;
  for (unsigned i = 0; i < index; i++)
  {
    std::string current_name;
    std::string current_value;
    envp_extract(current_name, current_value, v, j);
  }
  std::string name;
  std::string value;
  envp_extract(name, value, v, j);
  return std::make_pair(name,value);
}

ENVIRON_TYPE env_vector::envp (void) const
{
  return v;
}

otext& operator << (otext& t, const env_vector& a)
{
  for (unsigned i = 0; i < a.size(); i++)
  {
    std::pair<std::string,std::string> current = a.get(i);
    t << current.first << "=" << current.second << endl;
  }
  return t;
}

////////////////////////////////////////////////////////////////////////////////
// Synchronous subprocess
// Win32 implementation mostly cribbed from MSDN examples and then made (much) more readable
// Unix implementation mostly from man pages and bitter experience
////////////////////////////////////////////////////////////////////////////////

#ifdef MSWINDOWS

subprocess::subprocess(void)
{
  pid.hProcess = 0;
  child_in = 0;
  child_out = 0;
  child_err = 0;
  err = 0;
  status = 0;
}

#else

subprocess::subprocess(void)
{
  pid = -1;
  child_in = -1;
  child_out = -1;
  child_err = -1;
  err = 0;
  status = 0;
}

#endif

#ifdef MSWINDOWS

subprocess::~subprocess(void)
{
  if (pid.hProcess != 0)
  {
    close_stdin();
    close_stdout();
    close_stderr();
    kill();
    WaitForSingleObject(pid.hProcess, INFINITE);
    CloseHandle(pid.hThread);
    CloseHandle(pid.hProcess);
  }
}

#else

subprocess::~subprocess(void)
{
  if (pid != -1)
  {
    close_stdin();
    close_stdout();
    close_stderr();
    kill();
    for (;;)
    {
      int wait_status = 0;
      int wait_ret_val = waitpid(pid, &wait_status, 0);
      if (wait_ret_val != -1 || errno != EINTR) break;
    }
  }
}

#endif

void subprocess::add_variable(const std::string& name, const std::string& value)
{
  env.add(name, value);
}

bool subprocess::remove_variable(const std::string& name)
{
  return env.remove(name);
}

#ifdef MSWINDOWS

bool subprocess::spawn(const std::string& path, const arg_vector& argv,
                       bool connect_stdin, bool connect_stdout, bool connect_stderr)
{
  bool result = true;
  // first create the pipes to be used to connect to the child stdin/out/err
  // If no pipes requested, then connect to the parent stdin/out/err
  // for some reason you have to create a pipe handle, then duplicate it
  // This is not well explained in MSDN but seems to work
  PIPE_TYPE parent_stdin = 0;
  if (!connect_stdin)
    parent_stdin = GetStdHandle(STD_INPUT_HANDLE);
  else
  {
    PIPE_TYPE tmp = 0;
    SECURITY_ATTRIBUTES inherit_handles = {sizeof(SECURITY_ATTRIBUTES), 0, TRUE};
    CreatePipe(&parent_stdin, &tmp, &inherit_handles, 0);
    DuplicateHandle(GetCurrentProcess(), tmp, GetCurrentProcess(), &child_in, 0, FALSE, DUPLICATE_CLOSE_SOURCE | DUPLICATE_SAME_ACCESS);
  }

  PIPE_TYPE parent_stdout = 0;
  if (!connect_stdout)
    parent_stdout = GetStdHandle(STD_OUTPUT_HANDLE);
  else
  {
    PIPE_TYPE tmp = 0;
    SECURITY_ATTRIBUTES inherit_handles = {sizeof(SECURITY_ATTRIBUTES), 0, TRUE};
    CreatePipe(&tmp, &parent_stdout, &inherit_handles, 0);
    DuplicateHandle(GetCurrentProcess(), tmp, GetCurrentProcess(), &child_out, 0, FALSE, DUPLICATE_CLOSE_SOURCE | DUPLICATE_SAME_ACCESS);
  }

  PIPE_TYPE parent_stderr = 0;
  if (!connect_stderr)
    parent_stderr = GetStdHandle(STD_ERROR_HANDLE);
  else
  {
    PIPE_TYPE tmp = 0;
    SECURITY_ATTRIBUTES inherit_handles = {sizeof(SECURITY_ATTRIBUTES), 0, TRUE};
    CreatePipe(&tmp, &parent_stderr, &inherit_handles, 0);
    DuplicateHandle(GetCurrentProcess(), tmp, GetCurrentProcess(), &child_err, 0, FALSE, DUPLICATE_CLOSE_SOURCE | DUPLICATE_SAME_ACCESS);
  }

  // Now create the subprocess
  // The horrible trick of creating a console window and hiding it seems to be required for the pipes to work
  // Note that the child will inherit a copy of the pipe handles
  STARTUPINFO startup = {sizeof(STARTUPINFO),0,0,0,0,0,0,0,0,0,0,STARTF_USESTDHANDLES|STARTF_USESHOWWINDOW,SW_HIDE,0,0,
                         parent_stdin,parent_stdout,parent_stderr};
  bool created = CreateProcess(path.c_str(),(char*)argv.image().c_str(),0,0,TRUE,0,env.envp(),0,&startup,&pid) != 0;
  // close the parent copy of the pipe handles so that the pipes will be closed when the child releases them
  if (connect_stdin) CloseHandle(parent_stdin);
  if (connect_stdout) CloseHandle(parent_stdout);
  if (connect_stderr) CloseHandle(parent_stderr);
  if (!created)
  {
    err = GetLastError();
    close_stdin();
    close_stdout();
    close_stderr();
    result = false;
  }
  else
  {
    // The child process is now running so call the user's callback
    // The convention is that the callback can return false, in which case this will kill the child (if its still running)
    if (!callback())
    {
      result = false;
      kill();
    }
    close_stdin();
    close_stdout();
    close_stderr();
    // wait for the child to finish
    // TODO - kill the child if a timeout happens
    WaitForSingleObject(pid.hProcess, INFINITE);
    DWORD exit_status = 0;
    if (!GetExitCodeProcess(pid.hProcess, &exit_status))
    {
      err = GetLastError();
      result = false;
    }
    else if (exit_status != 0)
      result = false;
    status = (int)exit_status;
    CloseHandle(pid.hThread);
    CloseHandle(pid.hProcess);
  }
  pid.hProcess = 0;
  return result;
}

#else

bool subprocess::spawn(const std::string& path, const arg_vector& argv,
                       bool connect_stdin, bool connect_stdout, bool connect_stderr)
{
  bool result = true;
  // first create the pipes to be used to connect to the child stdin/out/err

  int stdin_pipe [2] = {-1, -1};
  if (connect_stdin)
    pipe(stdin_pipe);

  int stdout_pipe [2] = {-1, -1};
  if (connect_stdout)
    pipe(stdout_pipe);

  int stderr_pipe [2] = {-1, -1};
  if (connect_stderr)
    pipe(stderr_pipe);

  // now create the subprocess
  // In Unix, this is done by forking (creating two copies of the parent), then overwriting the child copy using exec
  pid = ::fork();
  switch(pid)
  {
  case -1:   // failed to fork
    err = errno;
    if (connect_stdin)
    {
      ::close(stdin_pipe[0]);
      ::close(stdin_pipe[1]);
    }
    if (connect_stdout)
    {
      ::close(stdout_pipe[0]);
      ::close(stdout_pipe[1]);
    }
    if (connect_stderr)
    {
      ::close(stderr_pipe[0]);
      ::close(stderr_pipe[1]);
    }
    result = false;
    break;
  case 0:  // in child;
  {
    // for each pipe, close the end of the duplicated pipe that is being used by the parent
    // and connect the child's end of the pipe to the appropriate standard I/O device
    if (connect_stdin)
    {
      ::close(stdin_pipe[1]);
      dup2(stdin_pipe[0],STDIN_FILENO);
    }
    if (connect_stdout)
    {
      ::close(stdout_pipe[0]);
      dup2(stdout_pipe[1],STDOUT_FILENO);
    }
    if (connect_stderr)
    {
      ::close(stderr_pipe[0]);
      dup2(stderr_pipe[1],STDERR_FILENO);
    }
    execve(path.c_str(), argv.argv(), env.envp());
    // will only ever get here if the exec() failed completely - *must* now exit the child process
    // by using errno, the parent has some chance of diagnosing the cause of the problem
    exit(errno);
  }
  break;
  default:  // in parent
  {
    // for each pipe, close the end of the duplicated pipe that is being used by the child
    // and connect the parent's end of the pipe to the class members so that they are visible to the parent() callback
    if (connect_stdin)
    {
      ::close(stdin_pipe[0]);
      child_in = stdin_pipe[1];
    }
    if (connect_stdout)
    {
      ::close(stdout_pipe[1]);
      child_out = stdout_pipe[0];
    }
    if (connect_stderr)
    {
      ::close(stderr_pipe[1]);
      child_err = stderr_pipe[0];
    }
    // call the user's callback
    if (!callback())
    {
      result = false;
      kill();
    }
    // close the pipes and wait for the child to finish
    // wait exits on a signal which may be the child signalling its exit or may be an interrupt
    close_stdin();
    close_stdout();
    close_stderr();
    int wait_status = 0;
    for (;;)
    {
      int wait_ret_val = waitpid(pid, &wait_status, 0);
      if (wait_ret_val != -1 || errno != EINTR) break;
    }
    // establish whether an error occurred
    if (WIFSIGNALED(wait_status))
    {
      // set_error(errno);
      status = WTERMSIG(wait_status);
      result = false;
    }
    else if (WIFEXITED(wait_status))
    {
      status = WEXITSTATUS(wait_status);
      if (status != 0)
        result = false;
    }
    pid = -1;
  }
  break;
  }
  return result;
}

#endif

bool subprocess::spawn(const std::string& command_line,
                       bool connect_stdin, bool connect_stdout, bool connect_stderr)
{
  arg_vector arguments = command_line;
  if (arguments.size() == 0) return false;
  std::string path = path_lookup(arguments.argv0());
  if (path.empty()) return false;
  return spawn(path, arguments, connect_stdin, connect_stdout, connect_stderr);
}

bool subprocess::callback(void)
{
  return true;
}

#ifdef MSWINDOWS

bool subprocess::kill (void)
{
  if (!pid.hProcess) return false;
  close_stdin();
  close_stdout();
  close_stderr();
  if (!TerminateProcess(pid.hProcess, (UINT)-1))
  {
    err = GetLastError();
    return false;
  }
  return true;
}

#else

bool subprocess::kill (void)
{
  if (pid == -1) return false;
  close_stdin();
  close_stdout();
  close_stderr();
  if (::kill(pid, SIGINT) == -1)
  {
    err = errno;
    return false;
  }
  return true;
}

#endif

#ifdef MSWINDOWS

int subprocess::write_stdin (std::string& buffer)
{
  if (child_in == 0) return -1;
  // do a blocking write of the whole buffer
  DWORD bytes = 0;
  if (!WriteFile(child_in, buffer.c_str(), (DWORD)buffer.size(), &bytes, 0))
  {
    err = GetLastError();
    close_stdin();
    return -1;
  }
  // now discard that part of the buffer that was written
  if (bytes > 0)
    buffer.erase(0, bytes);
  return bytes;
}

#else

int subprocess::write_stdin (std::string& buffer)
{
  if (child_in == -1) return -1;
  // do a blocking write of the whole buffer
  int bytes = write(child_in, buffer.c_str(), buffer.size());
  if (bytes == -1)
  {
    err = errno;
    close_stdin();
    return -1;
  }
  // now discard that part of the buffer that was written
  if (bytes > 0)
    buffer.erase(0, bytes);
  return bytes;
}

#endif

#ifdef MSWINDOWS

int subprocess::read_stdout (std::string& buffer)
{
  if (child_out == 0) return -1;
  DWORD bytes = 0;
  DWORD buffer_size = 256;
  char* tmp = new char[buffer_size];
  if (!ReadFile(child_out, tmp, buffer_size, &bytes, 0))
  {
    if (GetLastError() != ERROR_BROKEN_PIPE)
      err = GetLastError();
    close_stdout();
    delete[] tmp;
    return -1;
  }
  if (bytes == 0)
  {
    // EOF
    close_stdout();
    delete[] tmp;
    return -1;
  }
  buffer.append(tmp, bytes);
  delete[] tmp;
  return (int)bytes;
}

#else

int subprocess::read_stdout (std::string& buffer)
{
  if (child_out == -1) return -1;
  int buffer_size = 256;
  char* tmp = new char[buffer_size];
  int bytes = read(child_out, tmp, buffer_size);
  if (bytes == -1)
  {
    err = errno;
    close_stdout();
    delete[] tmp;
    return -1;
  }
  if (bytes == 0)
  {
    // EOF
    close_stdout();
    delete[] tmp;
    return -1;
  }
  buffer.append(tmp, bytes);
  delete[] tmp;
  return bytes;
}

#endif

#ifdef MSWINDOWS

int subprocess::read_stderr(std::string& buffer)
{
  if (child_err == 0) return -1;
  DWORD bytes = 0;
  DWORD buffer_size = 256;
  char* tmp = new char[buffer_size];
  if (!ReadFile(child_err, tmp, buffer_size, &bytes, 0))
  {
    if (GetLastError() != ERROR_BROKEN_PIPE)
      err = GetLastError();
    close_stderr();
    delete[] tmp;
    return -1;
  }
  if (bytes == 0)
  {
    // EOF
    close_stderr();
    delete[] tmp;
    return -1;
  }
  buffer.append(tmp, bytes);
  delete[] tmp;
  return (int)bytes;
}

#else

int subprocess::read_stderr (std::string& buffer)
{
  if (child_err == -1) return -1;
  int buffer_size = 256;
  char* tmp = new char[buffer_size];
  int bytes = read(child_err, tmp, buffer_size);
  if (bytes == -1)
  {
    err = errno;
    close_stderr();
    delete[] tmp;
    return -1;
  }
  if (bytes == 0)
  {
    // EOF
    close_stderr();
    delete[] tmp;
    return -1;
  }
  buffer.append(tmp, bytes);
  delete[] tmp;
  return bytes;
}

#endif

#ifdef MSWINDOWS

void subprocess::close_stdin (void)
{
  if (child_in)
  {
    CloseHandle(child_in);
    child_in = 0;
  }
}

#else

void subprocess::close_stdin (void)
{
  if (child_in != -1)
  {
    ::close(child_in);
    child_in = -1;
  }
}

#endif

#ifdef MSWINDOWS

void subprocess::close_stdout (void)
{
  if (child_out)
  {
    CloseHandle(child_out);
    child_out = 0;
  }
}

#else

void subprocess::close_stdout (void)
{
  if (child_out != -1)
  {
    ::close(child_out);
    child_out = -1;
  }
}

#endif

#ifdef MSWINDOWS

void subprocess::close_stderr (void)
{
  if (child_err)
  {
    CloseHandle(child_err);
    child_err = 0;
  }
}

#else

void subprocess::close_stderr (void)
{
  if (child_err != -1)
  {
    ::close(child_err);
    child_err = -1;
  }
}

#endif

bool subprocess::error(void) const
{
  return err != 0;
}

int subprocess::error_number(void) const
{
  return err;
}

#ifdef MSWINDOWS

std::string subprocess::error_text(void) const
{
  if (err == 0) return std::string();
  char* message;
  FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS,
                0,
                err,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                (LPTSTR)&message,
                0,0);
  std::string result = message;
  LocalFree(message);
  return result;
}

#else

std::string subprocess::error_text(void) const
{
  if (err == 0) return std::string();
  char* text = strerror(err);
  if (text) return std::string(text);
  return "error number " + to_string(err);
}

#endif

int subprocess::exit_status(void) const
{
  return status;
}

////////////////////////////////////////////////////////////////////////////////
// backtick subprocess and operations

backtick_subprocess::backtick_subprocess(void) : subprocess()
{
}

bool backtick_subprocess::callback(void)
{
  for (;;)
  {
    std::string buffer;
    int read_size = read_stdout(buffer);
    if (read_size < 0) break;
    device << buffer;
  }
  return !error();
}

bool backtick_subprocess::spawn(const std::string& path, const arg_vector& argv)
{
  return subprocess::spawn(path, argv, false, true, false);
}

bool backtick_subprocess::spawn(const std::string& command_line)
{
  return subprocess::spawn(command_line, false, true, false);
}

const std::vector<std::string>& backtick_subprocess::text(void) const
{
  return device.get_vector();
}

std::vector<std::string>& backtick_subprocess::text(void)
{
  return device.get_vector();
}

std::vector<std::string> backtick(const std::string& path, const arg_vector& argv)
{
  backtick_subprocess sub;
  sub.spawn(path, argv);
  return sub.text();
}

std::vector<std::string> backtick(const std::string& command_line)
{
  backtick_subprocess sub;
  sub.spawn(command_line);
  return sub.text();
}

////////////////////////////////////////////////////////////////////////////////
// Asynchronous subprocess

#ifdef MSWINDOWS

async_subprocess::async_subprocess(void)
{
  pid.hProcess = 0;
  child_in = 0;
  child_out = 0;
  child_err = 0;
  err = 0;
  status = 0;
}

#else

async_subprocess::async_subprocess(void)
{
  pid = -1;
  child_in = -1;
  child_out = -1;
  child_err = -1;
  err = 0;
  status = 0;
}

#endif

#ifdef MSWINDOWS

async_subprocess::~async_subprocess(void)
{
  if (pid.hProcess != 0)
  {
    close_stdin();
    close_stdout();
    close_stderr();
    kill();
    WaitForSingleObject(pid.hProcess, INFINITE);
    CloseHandle(pid.hThread);
    CloseHandle(pid.hProcess);
  }
}

#else

async_subprocess::~async_subprocess(void)
{
  if (pid != -1)
  {
    close_stdin();
    close_stdout();
    close_stderr();
    kill();
    for (;;)
    {
      int wait_status = 0;
      int wait_ret_val = waitpid(pid, &wait_status, 0);
      if (wait_ret_val != -1 || errno != EINTR) break;
    }
  }
}

#endif

void async_subprocess::set_error(int e)
{
  err = e;
}

void async_subprocess::add_variable(const std::string& name, const std::string& value)
{
  env.add(name, value);
}

bool async_subprocess::remove_variable(const std::string& name)
{
  return env.remove(name);
}

#ifdef MSWINDOWS

bool async_subprocess::spawn(const std::string& path, const arg_vector& argv,
                             bool connect_stdin, bool connect_stdout, bool connect_stderr)
{
  bool result = true;
  // first create the pipes to be used to connect to the child stdin/out/err
  // If no pipes requested, then connect to the parent stdin/out/err
  // for some reason you have to create a pipe handle, then duplicate it
  // This is not well explained in MSDN but seems to work
  PIPE_TYPE parent_stdin = 0;
  if (!connect_stdin)
    parent_stdin = GetStdHandle(STD_INPUT_HANDLE);
  else
  {
    PIPE_TYPE tmp = 0;
    SECURITY_ATTRIBUTES inherit_handles = {sizeof(SECURITY_ATTRIBUTES), 0, TRUE};
    CreatePipe(&parent_stdin, &tmp, &inherit_handles, 0);
    DuplicateHandle(GetCurrentProcess(), tmp, GetCurrentProcess(), &child_in, 0, FALSE, DUPLICATE_CLOSE_SOURCE | DUPLICATE_SAME_ACCESS);
  }

  PIPE_TYPE parent_stdout = 0;
  if (!connect_stdout)
    parent_stdout = GetStdHandle(STD_OUTPUT_HANDLE);
  else
  {
    PIPE_TYPE tmp = 0;
    SECURITY_ATTRIBUTES inherit_handles = {sizeof(SECURITY_ATTRIBUTES), 0, TRUE};
    CreatePipe(&tmp, &parent_stdout, &inherit_handles, 0);
    DuplicateHandle(GetCurrentProcess(), tmp, GetCurrentProcess(), &child_out, 0, FALSE, DUPLICATE_CLOSE_SOURCE | DUPLICATE_SAME_ACCESS);
  }

  PIPE_TYPE parent_stderr = 0;
  if (!connect_stderr)
    parent_stderr = GetStdHandle(STD_ERROR_HANDLE);
  else
  {
    PIPE_TYPE tmp = 0;
    SECURITY_ATTRIBUTES inherit_handles = {sizeof(SECURITY_ATTRIBUTES), 0, TRUE};
    CreatePipe(&tmp, &parent_stderr, &inherit_handles, 0);
    DuplicateHandle(GetCurrentProcess(), tmp, GetCurrentProcess(), &child_err, 0, FALSE, DUPLICATE_CLOSE_SOURCE | DUPLICATE_SAME_ACCESS);
  }

  // Now create the subprocess
  // The horrible trick of creating a console window and hiding it seems to be required for the pipes to work
  // Note that the child will inherit a copy of the pipe handles
  STARTUPINFO startup = {sizeof(STARTUPINFO),0,0,0,0,0,0,0,0,0,0,STARTF_USESTDHANDLES|STARTF_USESHOWWINDOW,SW_HIDE,0,0,
                         parent_stdin,parent_stdout,parent_stderr};
  bool created = CreateProcess(path.c_str(),(char*)argv.image().c_str(),0,0,TRUE,0,env.envp(),0,&startup,&pid) != 0;
  // close the parent copy of the pipe handles so that the pipes will be closed when the child releases them
  if (connect_stdin) CloseHandle(parent_stdin);
  if (connect_stdout) CloseHandle(parent_stdout);
  if (connect_stderr) CloseHandle(parent_stderr);
  if (!created)
  {
    set_error(GetLastError());
    close_stdin();
    close_stdout();
    close_stderr();
    result = false;
  }
  return result;
}

#else

bool async_subprocess::spawn(const std::string& path, const arg_vector& argv,
                             bool connect_stdin, bool connect_stdout, bool connect_stderr)
{
  bool result = true;
  // first create the pipes to be used to connect to the child stdin/out/err

  int stdin_pipe [2] = {-1, -1};
  if (connect_stdin)
    pipe(stdin_pipe);

  int stdout_pipe [2] = {-1, -1};
  if (connect_stdout)
    pipe(stdout_pipe);

  int stderr_pipe [2] = {-1, -1};
  if (connect_stderr)
    pipe(stderr_pipe);

  // now create the subprocess
  // In Unix, this is done by forking (creating two copies of the parent), then overwriting the child copy using exec
  pid = ::fork();
  switch(pid)
  {
  case -1:   // failed to fork
    set_error(errno);
    if (connect_stdin)
    {
      ::close(stdin_pipe[0]);
      ::close(stdin_pipe[1]);
    }
    if (connect_stdout)
    {
      ::close(stdout_pipe[0]);
      ::close(stdout_pipe[1]);
    }
    if (connect_stderr)
    {
      ::close(stderr_pipe[0]);
      ::close(stderr_pipe[1]);
    }
    result = false;
    break;
  case 0:  // in child;
  {
    // for each pipe, close the end of the duplicated pipe that is being used by the parent
    // and connect the child's end of the pipe to the appropriate standard I/O device
    if (connect_stdin)
    {
      ::close(stdin_pipe[1]);
      dup2(stdin_pipe[0],STDIN_FILENO);
    }
    if (connect_stdout)
    {
      ::close(stdout_pipe[0]);
      dup2(stdout_pipe[1],STDOUT_FILENO);
    }
    if (connect_stderr)
    {
      ::close(stderr_pipe[0]);
      dup2(stderr_pipe[1],STDERR_FILENO);
    }
    execve(path.c_str(), argv.argv(), env.envp());
    // will only ever get here if the exec() failed completely - *must* now exit the child process
    // by using errno, the parent has some chance of diagnosing the cause of the problem
    exit(errno);
  }
  break;
  default:  // in parent
  {
    // for each pipe, close the end of the duplicated pipe that is being used by the child
    // and connect the parent's end of the pipe to the class members so that they are visible to the parent() callback
    if (connect_stdin)
    {
      ::close(stdin_pipe[0]);
      child_in = stdin_pipe[1];
      if (fcntl(child_in, F_SETFL, O_NONBLOCK) == -1)
      {
        set_error(errno);
        result = false;
      }
    }
    if (connect_stdout)
    {
      ::close(stdout_pipe[1]);
      child_out = stdout_pipe[0];
      if (fcntl(child_out, F_SETFL, O_NONBLOCK) == -1)
      {
        set_error(errno);
        result = false;
      }
    }
    if (connect_stderr)
    {
      ::close(stderr_pipe[1]);
      child_err = stderr_pipe[0];
      if (fcntl(child_err, F_SETFL, O_NONBLOCK) == -1)
      {
        set_error(errno);
        result = false;
      }
    }
  }
  break;
  }
  return result;
}

#endif

bool async_subprocess::spawn(const std::string& command_line,
                             bool connect_stdin, bool connect_stdout, bool connect_stderr)
{
  arg_vector arguments = command_line;
  if (arguments.size() == 0) return false;
  std::string path = path_lookup(arguments.argv0());
  if (path.empty()) return false;
  return spawn(path, arguments, connect_stdin, connect_stdout, connect_stderr);
}

bool async_subprocess::callback(void)
{
  return true;
}

#ifdef MSWINDOWS

bool async_subprocess::tick(void)
{
  bool result = true;
  if (!callback())
    kill();
  DWORD exit_status = 0;
  if (!GetExitCodeProcess(pid.hProcess, &exit_status))
  {
    set_error(GetLastError());
    result = false;
  }
  else if (exit_status != STILL_ACTIVE)
  {
    CloseHandle(pid.hThread);
    CloseHandle(pid.hProcess);
    pid.hProcess = 0;
    result = false;
  }
  status = (int)exit_status;
  return result;
}

#else

bool async_subprocess::tick(void)
{
  bool result = true;
  if (!callback())
    kill();
  int wait_status = 0;
  int wait_ret_val = waitpid(pid, &wait_status, WNOHANG);
  if (wait_ret_val == -1 && errno != EINTR)
  {
    set_error(errno);
    result = false;
  }
  else if (wait_ret_val != 0)
  {
    // the only states that indicate a terminated child are WIFSIGNALLED and WIFEXITED
    if (WIFSIGNALED(wait_status))
    {
      // set_error(errno);
      status = WTERMSIG(wait_status);
      result = false;
    }
    else if (WIFEXITED(wait_status))
    {
      // child has exited
      status = WEXITSTATUS(wait_status);
      result = false;
    }
  }
  if (!result)
    pid = -1;
  return result;
}

#endif

#ifdef MSWINDOWS

bool async_subprocess::kill(void)
{
  if (!pid.hProcess) return false;
  close_stdin();
  close_stdout();
  close_stderr();
  if (!TerminateProcess(pid.hProcess, (UINT)-1))
  {
    set_error(GetLastError());
    return false;
  }
  return true;
}

#else

bool async_subprocess::kill(void)
{
  if (pid == -1) return false;
  close_stdin();
  close_stdout();
  close_stderr();
  if (::kill(pid, SIGINT) == -1)
  {
    set_error(errno);
    return false;
  }
  return true;
}

#endif

#ifdef MSWINDOWS

int async_subprocess::write_stdin (std::string& buffer)
{
  if (child_in == 0) return -1;
  // there doesn't seem to be a way of doing non-blocking writes under Windows
  DWORD bytes = 0;
  if (!WriteFile(child_in, buffer.c_str(), (DWORD)buffer.size(), &bytes, 0))
  {
    set_error(GetLastError());
    close_stdin();
    return -1;
  }
  // now discard that part of the buffer that was written
  if (bytes > 0)
    buffer.erase(0, bytes);
  return (int)bytes;
}

#else

int async_subprocess::write_stdin (std::string& buffer)
{
  if (child_in == -1) return -1;
  // relies on the pipe being non-blocking
  // This does block under Windows
  int bytes = write(child_in, buffer.c_str(), buffer.size());
  if (bytes == -1 && errno == EAGAIN)
  {
    // not ready
    return 0;
  }
  if (bytes == -1)
  {
    // error on write - close the pipe and give up
    set_error(errno);
    close_stdin();
    return -1;
  }
  // successful write
  // now discard that part of the buffer that was written
  if (bytes > 0)
    buffer.erase(0, bytes);
  return bytes;
}

#endif

#ifdef MSWINDOWS

int async_subprocess::read_stdout (std::string& buffer)
{
  if (child_out == 0) return -1;
  // peek at the buffer to see how much data there is in the first place
  DWORD buffer_size = 0;
  if (!PeekNamedPipe(child_out, 0, 0, 0, &buffer_size, 0))
  {
    if (GetLastError() != ERROR_BROKEN_PIPE)
      set_error(GetLastError());
    close_stdout();
    return -1;
  }
  if (buffer_size == 0) return 0;
  DWORD bytes = 0;
  char* tmp = new char[buffer_size];
  if (!ReadFile(child_out, tmp, buffer_size, &bytes, 0))
  {
    set_error(GetLastError());
    close_stdout();
    delete[] tmp;
    return -1;
  }
  if (bytes == 0)
  {
    // EOF
    close_stdout();
    delete[] tmp;
    return -1;
  }
  buffer.append(tmp, bytes);
  delete[] tmp;
  return (int)bytes;
}

#else

int async_subprocess::read_stdout (std::string& buffer)
{
  if (child_out == -1) return -1;
  // rely on the pipe being non-blocking
  int buffer_size = 256;
  char* tmp = new char[buffer_size];
  int bytes = read(child_out, tmp, buffer_size);
  if (bytes == -1 && errno == EAGAIN)
  {
    // not ready
    delete[] tmp;
    return 0;
  }
  if (bytes == -1)
  {
    // error
    set_error(errno);
    close_stdout();
    delete[] tmp;
    return -1;
  }
  if (bytes == 0)
  {
    // EOF
    close_stdout();
    delete[] tmp;
    return -1;
  }
  // successful read
  buffer.append(tmp, bytes);
  delete[] tmp;
  return bytes;
}

#endif

#ifdef MSWINDOWS

int async_subprocess::read_stderr (std::string& buffer)
{
  if (child_err == 0) return -1;
  // peek at the buffer to see how much data there is in the first place
  DWORD buffer_size = 0;
  if (!PeekNamedPipe(child_err, 0, 0, 0, &buffer_size, 0))
  {
    if (GetLastError() != ERROR_BROKEN_PIPE)
      set_error(GetLastError());
    close_stderr();
    return -1;
  }
  if (buffer_size == 0) return 0;
  DWORD bytes = 0;
  char* tmp = new char[buffer_size];
  if (!ReadFile(child_err, tmp, buffer_size, &bytes, 0))
  {
    set_error(GetLastError());
    close_stderr();
    delete[] tmp;
    return -1;
  }
  if (bytes == 0)
  {
    // EOF
    close_stderr();
    delete[] tmp;
    return -1;
  }
  buffer.append(tmp, bytes);
  delete[] tmp;
  return (int)bytes;
}

#else

int async_subprocess::read_stderr (std::string& buffer)
{
  if (child_err == -1) return -1;
  // rely on the pipe being non-blocking
  int buffer_size = 256;
  char* tmp = new char[buffer_size];
  int bytes = read(child_err, tmp, buffer_size);
  if (bytes == -1 && errno == EAGAIN)
  {
    // not ready
    delete[] tmp;
    return 0;
  }
  if (bytes == -1)
  {
    // error
    set_error(errno);
    close_stderr();
    delete[] tmp;
    return -1;
  }
  if (bytes == 0)
  {
    // EOF
    close_stderr();
    delete[] tmp;
    return -1;
  }
  // successful read
  buffer.append(tmp, bytes);
  delete[] tmp;
  return bytes;
}

#endif

#ifdef MSWINDOWS

void async_subprocess::close_stdin (void)
{
  if (child_in)
  {
    CloseHandle(child_in);
    child_in = 0;
  }
}

#else

void async_subprocess::close_stdin (void)
{
  if (child_in != -1)
  {
    ::close(child_in);
    child_in = -1;
  }
}

#endif

#ifdef MSWINDOWS

void async_subprocess::close_stdout (void)
{
  if (child_out)
  {
    CloseHandle(child_out);
    child_out = 0;
  }
}

#else

void async_subprocess::close_stdout (void)
{
  if (child_out != -1)
  {
    ::close(child_out);
    child_out = -1;
  }
}

#endif

#ifdef MSWINDOWS

void async_subprocess::close_stderr (void)
{
  if (child_err)
  {
    CloseHandle(child_err);
    child_err = 0;
  }
}

#else

void async_subprocess::close_stderr (void)
{
  if (child_err != -1)
  {
    ::close(child_err);
    child_err = -1;
  }
}

#endif

bool async_subprocess::error(void) const
{
  return err != 0;
}

int async_subprocess::error_number(void) const
{
  return err;
}

#ifdef MSWINDOWS

std::string async_subprocess::error_text(void) const
{
  if (err == 0) return std::string();
  char* message;
  FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS,
                0,
                err,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                (LPTSTR)&message,
                0,0);
  std::string result = message;
  LocalFree(message);
  return result;
}

#else

std::string async_subprocess::error_text(void) const
{
  if (err == 0) return std::string();
  char* text = strerror(err);
  if (text) return std::string(text);
  return "error number " + to_string(err);
}

#endif

int async_subprocess::exit_status(void) const
{
  return status;
}

////////////////////////////////////////////////////////////////////////////////
