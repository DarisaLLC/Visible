#ifndef SUBPROCESSES_HPP
#define SUBPROCESSES_HPP
/*----------------------------------------------------------------------------

  Author:    Andy Rushton
  Copyright: (c) Andy Rushton, 2004
  License:   BSD License, see ../docs/license.html

  Platform-independent wrapper around the very platform-specific handling of
  subprocesses. Uses the C++ convention that all resources must be contained in
  an object so that when a subprocess object goes out of scope the subprocess
  itself gets closed down.

  ------------------------------------------------------------------------------*/
#include "os_fixes.hpp"
#ifdef MSWINDOWS
#include <windows.h>
#endif
#include "textio.hpp"
#include "string_vectorio.hpp"
#include <vector>
#include <string>
#include <map>

////////////////////////////////////////////////////////////////////////////////
// Argument vector class
// allows manipulation of argv-like vectors
// includes splitting of command lines into argvectors as per the shell
// (removing quotes) and the reverse conversion (adding quotes where necessary)

class arg_vector
{
private:
  char** v;

public:
  // create an empty vector
  arg_vector (void);

  // copy constructor (yes it copies)
  arg_vector (const arg_vector&);

  // construct from an argv
  arg_vector (char**);

  // construct from a command-line string
  // includes de-quoting of values
  arg_vector (const std::string&);
  arg_vector (const char*);

  ~arg_vector (void);

  // assignment operators are compatible with the constructors
  arg_vector& operator = (const arg_vector&);
  arg_vector& operator = (char**);
  arg_vector& operator = (const std::string&);
  arg_vector& operator = (const char*);

  // add an argument to the vector
  arg_vector& operator += (const std::string&);
  arg_vector& operator -= (const std::string&);

  // insert/clear an argument at a certain index
  // adding is like the other array classes - it moves the current item at index
  // up one (and all subsequent values) to make room
  void insert (unsigned index, const std::string&);
  void clear (unsigned index);
  void clear (void);

  // number of values in the vector (including argv[0], the command itself
  unsigned size (void) const;

  // type conversion to the argv type
  operator char** (void) const;
  // function-based version of the above for people who don't like type conversions
  char** argv (void) const;

  // access individual values in the vector
  char* operator [] (unsigned index) const;

  // special-case access of the command name (e.g. to do path lookup on the command)
  char* argv0 (void) const;

  // get the command-line string represented by this vector
  // includes escaping of special characters and quoting
  std::string image (void) const;

  // text output, prints the vector using the image() format
  friend otext& operator << (otext&, const arg_vector&);
};

// redefine friends for gcc v4.1
otext& operator << (otext&, const arg_vector&);

////////////////////////////////////////////////////////////////////////////////
// Environment class
// Allows manipulation of an environment vector
// This is typically used to create an environment to be used by a subprocess
// It does NOT modify the environment of the current process

#ifdef MSWINDOWS
#define ENVIRON_TYPE char*
#else
#define ENVIRON_TYPE char**
#endif

class env_vector
{
private:
  ENVIRON_TYPE v;

public:
  // create an env_vector vector from the current process
  env_vector (void);
  env_vector (const env_vector&);
  ~env_vector (void);

  env_vector& operator = (const env_vector&);

  void clear (void);

  // manipulate the env_vector by adding or removing variables
  // adding a name that already exists replaces its value
  void add (const std::string& name, const std::string& value);
  bool remove (const std::string& name);

  // get the value associated with a name
  // the first uses an indexed notation (e.g. env["PATH"] )
  // the second is a function based form (e.g. env.get("PATH"))
  std::string operator [] (const std::string& name) const;
  std::string get (const std::string& name) const;

  // number of name=value pairs in the env_vector
  unsigned size (void) const;

  // get the name=value pairs by index (in the range 0 to size()-1)
  std::pair<std::string,std::string> operator [] (unsigned index) const;
  std::pair<std::string,std::string> get (unsigned index) const;

  // access the env_vector as an envp type - used for passing to subprocesses
  ENVIRON_TYPE envp (void) const;

  // prints the env_vector in the same form as the shell env command
  friend otext& operator << (otext&, const env_vector&);
};

// redefine friends for gcc v4.1
otext& operator << (otext&, const env_vector&);

////////////////////////////////////////////////////////////////////////////////

#ifdef MSWINDOWS
#define PID_TYPE PROCESS_INFORMATION
#define PIPE_TYPE Qt::HANDLE
#else
#define PID_TYPE int
#define PIPE_TYPE int
#endif

////////////////////////////////////////////////////////////////////////////////
// Synchronous subprocess

class subprocess
{
private:
  PID_TYPE pid;
  PIPE_TYPE child_in;
  PIPE_TYPE child_out;
  PIPE_TYPE child_err;
  env_vector env;
  int err;
  int status;

public:
  subprocess(void);
  virtual ~subprocess(void);

  void add_variable(const std::string& name, const std::string& value);
  bool remove_variable(const std::string& name);

  bool spawn(const std::string& path, const arg_vector& argv,
             bool connect_stdin = false, bool connect_stdout = false, bool connect_stderr = false);
  bool spawn(const std::string& command_line,
             bool connect_stdin = false, bool connect_stdout = false, bool connect_stderr = false);

  virtual bool callback(void);
  bool kill(void);

  int write_stdin(std::string& buffer);
  int read_stdout(std::string& buffer);
  int read_stderr(std::string& buffer);

  void close_stdin(void);
  void close_stdout(void);
  void close_stderr(void);

  bool error(void) const;
  int error_number(void) const;
  std::string error_text(void) const;

  int exit_status(void) const;
};

////////////////////////////////////////////////////////////////////////////////
// Preconfigured subprocess which executes a command and captures its output

class backtick_subprocess : public subprocess
{
private:
  osvtext device;
public:
  backtick_subprocess(void);
  virtual bool callback(void);
  bool spawn(const std::string& path, const arg_vector& argv);
  bool spawn(const std::string& command_line);
  const std::vector<std::string>& text(void) const;
  std::vector<std::string>& text(void);
};

std::vector<std::string> backtick(const std::string& path, const arg_vector& argv);
std::vector<std::string> backtick(const std::string& command_line);

////////////////////////////////////////////////////////////////////////////////
// Asynchronous subprocess

class async_subprocess
{
private:
  PID_TYPE pid;
  PIPE_TYPE child_in;
  PIPE_TYPE child_out;
  PIPE_TYPE child_err;
  env_vector env;
  int err;
  int status;
  void set_error(int);

public:
  async_subprocess(void);
  virtual ~async_subprocess(void);

  void add_variable(const std::string& name, const std::string& value);
  bool remove_variable(const std::string& name);

  bool spawn(const std::string& path, const arg_vector& argv,
             bool connect_stdin = false, bool connect_stdout = false, bool connect_stderr = false);
  bool spawn(const std::string& command_line,
             bool connect_stdin = false, bool connect_stdout = false, bool connect_stderr = false);

  virtual bool callback(void);
  bool tick(void);
  bool kill(void);

  int write_stdin(std::string& buffer);
  int read_stdout(std::string& buffer);
  int read_stderr(std::string& buffer);

  void close_stdin(void);
  void close_stdout(void);
  void close_stderr(void);

  bool error(void) const;
  int error_number(void) const;
  std::string error_text(void) const;

  int exit_status(void) const;
};

////////////////////////////////////////////////////////////////////////////////
#endif
