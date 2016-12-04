/*------------------------------------------------------------------------------

  Author:    Andy Rushton
  Copyright: (c) Andy Rushton, 2004
  License:   BSD License, see ../docs/license.html

------------------------------------------------------------------------------*/
#include "error_handler.hpp"
#include "fileio.hpp"
#include "file_system.hpp"
#include "string_utilities.hpp"
#include "debug.hpp"
#include <ctype.h>

////////////////////////////////////////////////////////////////////////////////

static char* information_id = "INFORMATION";
static char* context_id = "CONTEXT";
static char* supplement_id = "SUPPLEMENT";
static char* warning_id = "WARNING";
static char* error_id = "ERROR";
static char* fatal_id = "FATAL";
static char* position_id = "POSITION";

static char* default_information_format = "@0";
static char* default_context_format = "context: @0";
static char* default_supplement_format = "supplement: @0";
static char* default_warning_format = "warning: @0";
static char* default_error_format = "error: @0";
static char* default_fatal_format = "FATAL: @0";
static char* default_position_format = "\"@1\" (@2,@3) : @0";

////////////////////////////////////////////////////////////////////////////////
// position class

error_position::error_position(void) :
  m_filename(std::string()), m_line(0), m_column(0) 
{
}

error_position::error_position(const std::string& filename, unsigned line, unsigned column) :
  m_filename(filename), m_line(line), m_column(column)
{
}

error_position::~error_position(void)
{
}

const std::string& error_position::filename(void) const
{
  return m_filename;
}

unsigned error_position::line(void) const
{
  return m_line;
}

unsigned error_position::column(void) const
{
  return m_column;
}

error_position error_position::operator + (unsigned offset) const
{
  error_position result(*this);
  result += offset;
  return result;
}

error_position& error_position::operator += (unsigned offset)
{
  m_column += offset;
  return *this;
}

bool error_position::empty(void) const
{
  return m_filename.empty();
}

bool error_position::valid(void) const
{
  return !empty();
}

std::vector<std::string> error_position::show(void) const
{
  std::vector<std::string> result;
  if (valid())
  {
    // skip row-1 lines of the file and print out the resultant line
    iftext source(filename());
    std::string current_line;
    for (unsigned i = 0; i < line(); i++)
      if (source.error() || !source.getline(current_line))
        break;
    result.push_back(current_line);
    // now put an up-arrow at the appropriate column
    // preserve any tabs in the original line
    result.push_back(std::string());
    for (unsigned j = 0; j < column(); j++)
    {
      if (j < current_line.size() && current_line[j] == '\t')
        result.back() += '\t';
      else
        result.back() += ' ';
    }
    result.back() += '^';
  }
  return result;
}

std::string to_string(const error_position& where)
{
  return "{" + filespec_to_relative_path(where.filename()) + ":" + to_string(where.line()) + ":" + to_string(where.column()) + "}";
}

otext& operator << (otext& output, const error_position& where)
{
  return output << to_string(where);
}

////////////////////////////////////////////////////////////////////////////////
// exceptions

// read_error

error_handler_read_error::error_handler_read_error(const error_position& position, const std::string& reason) :
  std::runtime_error(to_string(position) + std::string(": Error handler read error - ") + reason), m_position(position)
{
}

error_handler_read_error::~error_handler_read_error(void) throw()
{
}

const error_position& error_handler_read_error::where(void) const
{
  return m_position;
}

// format error

error_handler_format_error::error_handler_format_error(const std::string& format, unsigned offset) :
  std::runtime_error(std::string("Error handler formatting error in \"") + format + std::string("\"")),
  m_position("",0,0), m_format(format), m_offset(offset)
{
}

error_handler_format_error::error_handler_format_error(const error_position& pos, const std::string& format, unsigned offset) : 
  std::runtime_error(to_string(pos) + std::string(": Error handler formatting error in \"") + format + std::string("\"")), 
  m_position(pos), m_format(format), m_offset(offset)
{
}

error_handler_format_error::~error_handler_format_error(void) throw()
{
}

const error_position& error_handler_format_error::where(void) const
{
  return m_position;
}

const std::string& error_handler_format_error::format(void) const
{
  return m_format;
}

unsigned error_handler_format_error::offset(void) const
{
  return m_offset;
}

// id_error

error_handler_id_error::error_handler_id_error(const std::string& id) :
  std::runtime_error(std::string("Error handler message ID not found: ") + id), m_id(id)
{
}

error_handler_id_error::~error_handler_id_error(void) throw()
{
}

const std::string& error_handler_id_error::id(void) const
{
  return m_id;
}

// limit_error

error_handler_limit_error::error_handler_limit_error(unsigned limit) : 
  std::runtime_error(std::string("Error handler limit error: ") + to_string(limit) + std::string(" reached")), m_limit(limit)
{
}

error_handler_limit_error::~error_handler_limit_error(void) throw()
{
}

unsigned error_handler_limit_error::limit(void) const
{
  return m_limit;
}

// fatal_error

error_handler_fatal_error::error_handler_fatal_error(const std::string& id) : 
  std::runtime_error(std::string("Fatal error: ") + id), m_id(id)
{
}

error_handler_fatal_error::~error_handler_fatal_error(void) throw()
{
}

const std::string& error_handler_fatal_error::id(void) const
{
  return m_id;
}

////////////////////////////////////////////////////////////////////////////////

class message
{
private:
  unsigned m_index;    // index into message files
  unsigned m_line;     // line
  unsigned m_column;   // column
  std::string m_text;  // text

public:
  message(unsigned index = (unsigned)-1,unsigned line = 0,unsigned column = 0,
          const std::string& text = "") :
    m_index(index),m_line(line),m_column(column),m_text(text) {}
  message(const std::string& text) :
    m_index((unsigned)-1),m_line(0),m_column(0),m_text(text) {}
  ~message(void) {}

  unsigned index(void) const {return m_index;}
  unsigned line(void) const {return m_line;}
  unsigned column(void) const {return m_column;}
  const std::string& text(void) const {return m_text;}
};

////////////////////////////////////////////////////////////////////////////////

class pending_message
{
private:
  error_position m_position;
  std::string m_id;
  std::vector<std::string> m_args;
public:
  pending_message(const error_position& position, const std::string& id, const std::vector<std::string>& args) :
    m_position(position), m_id(id), m_args(args) {}
  pending_message(const std::string& id, const std::vector<std::string>& args) :
    m_position(error_position()), m_id(id), m_args(args) {}
  ~pending_message(void) {}

  const error_position& position(void) const {return m_position;}
  const std::string& id(void) const {return m_id;}
  const std::vector<std::string>& args(void) const {return m_args;}
};

////////////////////////////////////////////////////////////////////////////////

class error_handler_base_body
{
public:
  std::vector<std::string> m_files;          // message files in the order they were added
  std::map<std::string,message> m_messages;  // messages stored as id:message pairs
  bool m_show;                               // show source
  std::list<pending_message> m_context;      // context message stack
  std::list<pending_message> m_supplement;   // supplementary message stack

public:
  error_handler_base_body(void)
    {
      // preload with default formats
      add_message(information_id,default_information_format);
      add_message(warning_id,default_warning_format);
      add_message(error_id,default_error_format);
      add_message(fatal_id,default_fatal_format);
      add_message(position_id,default_position_format);
      add_message(context_id,default_context_format);
      add_message(supplement_id,default_supplement_format);
    }

  ~error_handler_base_body(void)
    {
    }

  void set_show(bool show)
    {
      m_show = show;
    }

  void add_message_file(const std::string& file)
    throw(error_handler_read_error)
    {
      if (!file_exists(file)) throw error_handler_read_error(error_position(file,0,0), "file not found");
      m_files.push_back(file);
      iftext input(file);
      std::string line;
      unsigned l = 0;
      while(input.getline(line))
      {
        l++;
        if (line.size() > 0 && isalpha(line[0]))
        {
          // Get the id field which starts with an alphabetic and contains alphanumerics and underscore
          std::string id;
          unsigned i = 0;
          for (; i < line.size(); i++)
          {
            if (isalnum(line[i]) || line[i] == '_')
              id += line[i];
            else
              break;
          }
          // chk this ID is unique within the files - however it is legal to override a hard-coded message
          if (m_messages.find(id) != m_messages.end() && m_messages.find(id)->second.index() != (unsigned)-1)
            throw error_handler_read_error(error_position(file,l,i), std::string("repeated ID ") + id);
          // chk for at least one whitespace
          if (i >= line.size() || !isspace(line[i]))
            throw error_handler_read_error(error_position(file,l,i), "Missing whitespace");
          // skip whitespace
          for (; i < line.size(); i++)
          {
            if (!isspace(line[i]))
              break;
          }
          // now get the text part and add the message to the message map
          std::string text = line.substr(i, line.size()-i);
          m_messages[id] = message(m_files.size()-1, l, i, text);
        }
      }
    }

  void add_message(const std::string& id, const std::string& text)
    throw()
    {
      m_messages[id] = message((unsigned)-1, 0, 0, text);
    }

  void push_supplement(const error_position& position,
                       const std::string& message_id,
                       const std::vector<std::string>& args)
    {
      m_supplement.push_back(pending_message(position,message_id,args));
    }

  void push_context(const error_position& position,
                       const std::string& message_id,
                       const std::vector<std::string>& args)
    {
      m_context.push_back(pending_message(position,message_id,args));
    }

  void pop_context(unsigned depth)
    {
      while (depth < m_context.size())
        m_context.pop_back();
    }

  unsigned context_depth(void) const
    {
      return m_context.size();
    }

  std::vector<std::string> format_report(const error_position& position,
                                         const std::string& message_id,
                                         const std::string& status_id,
                                         const std::vector<std::string>& args)
    throw(error_handler_id_error,error_handler_format_error)
    {
      // gathers everything together into a full multi-line report
      std::vector<std::string> result;
      // the first part of the report is the status message (info/warning/error/fatal)
      result.push_back(format_id(position, message_id, status_id, args));
      // now append any supplemental messages that have been stacked
      // these are printed in FIFO order i.e. the order that they were added to the handler
      for (std::list<pending_message>::iterator j = m_supplement.begin(); j != m_supplement.end(); j++)
        result.push_back(format_id(j->position(),j->id(),supplement_id,j->args()));
      // now discard any supplementary messages because they only persist until they are printed once
      m_supplement.clear();
      // now append any context messages that have been stacked
      // these are printed in LIFO order i.e. closest context first
      for (std::list<pending_message>::reverse_iterator i = m_context.rbegin(); i != m_context.rend(); i++)
        result.push_back(format_id(i->position(),i->id(),context_id,i->args()));
      return result;
    }

  std::string format_id(const error_position& position,
                        const std::string& message_id,
                        const std::string& status_id,
                        const std::vector<std::string>& args)
    throw(error_handler_id_error,error_handler_format_error)
    {
      // This function creates a fully-formatted single-line message from a
      // combination of the position format and the status format plus the message
      // ID and its arguments. There are up to three levels of substitution
      // required to do this.

      // get the status format from the status_id
      std::map<std::string,message>::iterator status_found = m_messages.find(status_id);
      if (status_found == m_messages.end()) throw error_handler_id_error(status_id);

      // similarly get the message format
      std::map<std::string,message>::iterator message_found = m_messages.find(message_id);
      if (message_found == m_messages.end()) throw error_handler_id_error(message_id);

      // format the message contents
      std::string message_text = format_message(message_found->second, args);

      // now format the message in the status string (informational, warning etc).
      std::vector<std::string> status_args;
      status_args.push_back(message_text);
      std::string result = format_message(status_found->second, status_args);

      // finally, if the message is positional, format the status message in the positional string
      if (position.valid())
      {
        // get the position format from the message set
        std::map<std::string,message>::iterator position_found = m_messages.find(position_id);
        if (position_found == m_messages.end()) throw error_handler_id_error(position_id);

        // now format the message
        std::vector<std::string> position_args;
        position_args.push_back(result);
        position_args.push_back(filespec_to_relative_path(position.filename()));
        position_args.push_back(to_string(position.line()));
        position_args.push_back(to_string(position.column()));
        result = format_message(position_found->second, position_args);
        // add the source file text and position if that option is on

        if (m_show)
        {
          std::vector<std::string> show = position.show();
          for (unsigned i = 0; i < show.size(); i++)
          {
            result += std::string("\n");
            result += show[i];
          }
        }
      }
      return result;
    }

  std::string format_message(const message& mess,
                             const std::vector<std::string>& args) 
    throw(error_handler_format_error)
    {
      // this function creates a formatted string from the stored message text and
      // the arguments. Most of the work is done in format_string. However, if a
      // formatting error is found, this function uses extra information stored in
      // the message data structure to improve the reporting of the error
      try
      {
        // This is the basic string formatter which performs parameter substitution
        // into a message. Parameter placeholders are in the form @0, @1 etc, where
        // the number is the index of the argument in the args vector. At present,
        // no field codes are supported as in printf formats Algorithm: scan the
        // input string and transfer it into the result. When an @nnn appears,
        // substitute the relevant argument from the args vector. Throw an exception
        // if its out of range. Also convert C-style escaped characters of the form
        // \n.
        std::string format = mess.text();
        std::string result;
        for (unsigned i = 0; i < format.size(); )
        {
          if (format[i] == '@')
          {
            // an argument substitution has been found - now find the argument number
            if (i+1 == format.size()) throw error_handler_format_error(format, i);
            i++;
            // chk for @@ which is an escaped form of '@'
            if (format[i] == '@')
            {
              result += '@';
              i++;
            }
            else
            {
              // there must be at least one digit in the substitution number
              if (!isdigit(format[i])) throw error_handler_format_error(format,i);
              unsigned a = 0;
              for (; i < format.size() && isdigit(format[i]); i++)
              {
                a *= 10;
                a += (format[i] - '0');
              }
              // chk for an argument request out of the range of the set of arguments
              if (a >= args.size()) throw error_handler_format_error(format,i-1);
              result += args[a];
            }
          }
          else if (format[i] == '\\')
          {
            // an escaped character has been found
            if (i+1 == format.size()) throw error_handler_format_error(format, i);
            i++;
            // do the special ones first, then all the others just strip off the \ and leave the following characters
            switch(format[i])
            {
            case '\\':
              result += '\\';
              break;
            case 't':
              result += '\t';
              break;
            case 'n':
              result += '\n';
              break;
            case 'r':
              result += '\r';
              break;
            case 'a':
              result += '\a';
              break;
            default:
              result += format[i];
              break;
            }
            i++;
          }
          else
          {
            // plain text found - just append to the result
            result += format[i++];
          }
        }
        return result;
      }
      catch(error_handler_format_error& exception)
      {
        // if the message came from a message file, improve the error reporting by adding file positional information
        // Also adjust the position from the start of the text (stored in the message field) to the column of the error
        if (mess.index() != (unsigned)-1)
          throw error_handler_format_error(error_position(m_files[mess.index()],
                                                          mess.line(),
                                                          mess.column()+exception.offset()),
                                           exception.format(),
                                           exception.offset());
        else
          throw exception;
      }
    }
};

////////////////////////////////////////////////////////////////////////////////
// context subclass

error_context_body::error_context_body(error_handler_base& handler) :
  m_depth(0), m_handler(0)
{
  set(handler);
}

error_context_body::~error_context_body(void)
{
  pop();
}

void error_context_body::set(error_handler_base& handler)
{
  m_handler = &handler;
  m_depth = m_handler->context_depth();
}

void error_context_body::pop(void)
{
  if (m_handler)
    m_handler->pop_context(m_depth);
}

////////////////////////////////////////////////////////////////////////////////

error_handler_base::error_handler_base(bool show)
  throw() :
  m_base_body(new error_handler_base_body)
{
  m_base_body->set_show(show);
}

error_handler_base::error_handler_base(const std::string& file, bool show)
  throw(error_handler_read_error) :
  m_base_body(new error_handler_base_body)
{
  m_base_body->set_show(show);
  add_message_file(file);
}

error_handler_base::error_handler_base(const std::vector<std::string>& files, bool show)
  throw(error_handler_read_error) :
  m_base_body(new error_handler_base_body)
{
  m_base_body->set_show(show);
  add_message_files(files);
}

error_handler_base::~error_handler_base(void)
  throw()
{
}

////////////////////////////////////////////////////////////////////////////////

void error_handler_base::add_message_file(const std::string& file)
  throw(error_handler_read_error)
{
  m_base_body->add_message_file(file);
}

void error_handler_base::add_message_files(const std::vector<std::string>& files)
  throw(error_handler_read_error)
{
  for (unsigned i = 0; i < files.size(); i++)
    add_message_file(files[i]);
}

void error_handler_base::add_message(const std::string& id, const std::string& text)
  throw()
{
  m_base_body->add_message(id,text);
}

////////////////////////////////////////////////////////////////////////////////

void error_handler_base::set_information_format(const std::string& format) throw()
{
  add_message(information_id, format);
}

void error_handler_base::set_warning_format(const std::string& format) throw()
{
  add_message(warning_id, format);
}

void error_handler_base::set_error_format(const std::string& format) throw()
{
  add_message(error_id, format);
}

void error_handler_base::set_fatal_format(const std::string& format) throw()
{
  add_message(fatal_id, format);
}

void error_handler_base::set_position_format(const std::string& format) throw()
{
  add_message(position_id, format);
}

void error_handler_base::set_context_format(const std::string& format) throw()
{
  add_message(context_id, format);
}

void error_handler_base::set_supplement_format(const std::string& format) throw()
{
  add_message(supplement_id, format);
}

////////////////////////////////////////////////////////////////////////////////

void error_handler_base::show_position(void)
  throw()
{
  m_base_body->set_show(true);
}

void error_handler_base::hide_position(void)
  throw()
{
  m_base_body->set_show(false);
}

////////////////////////////////////////////////////////////////////////////////
// information messages

std::vector<std::string> error_handler_base::information_message(const std::string& id,
                                                                 const std::vector<std::string>& args)
  throw(error_handler_id_error,error_handler_format_error)
{
  return information_message(error_position(), id, args);
}

std::vector<std::string> error_handler_base::information_message(const std::string& id)
  throw(error_handler_id_error,error_handler_format_error)
{
  std::vector<std::string> args;
  return information_message(id, args);
}

std::vector<std::string> error_handler_base::information_message(const std::string& id,
                                                                 const std::string& arg1)
  throw(error_handler_id_error,error_handler_format_error)
{
  std::vector<std::string> args;
  args.push_back(arg1);
  return information_message(id, args);
}

std::vector<std::string> error_handler_base::information_message(const std::string& id,
                                                                 const std::string& arg1, const std::string& arg2)
  throw(error_handler_id_error,error_handler_format_error)
{
  std::vector<std::string> args;
  args.push_back(arg1);
  args.push_back(arg2);
  return information_message(id, args);
}

std::vector<std::string> error_handler_base::information_message(const std::string& id,
                                                                 const std::string& arg1, const std::string& arg2, const std::string& arg3)
  throw(error_handler_id_error,error_handler_format_error)
{
  std::vector<std::string> args;
  args.push_back(arg1);
  args.push_back(arg2);
  args.push_back(arg3);
  return information_message(id, args);
}

std::vector<std::string> error_handler_base::information_message(const error_position& position,
                                                                 const std::string& id,
                                                                 const std::vector<std::string>& args)
  throw(error_handler_id_error,error_handler_format_error)
{
  return m_base_body->format_report(position, id, information_id, args);
}

std::vector<std::string> error_handler_base::information_message(const error_position& position,
                                                                 const std::string& id)
  throw(error_handler_id_error,error_handler_format_error)
{
  std::vector<std::string> args;
  return information_message(position, id, args);
}

std::vector<std::string> error_handler_base::information_message(const error_position& position,
                                                                 const std::string& id,
                                                                 const std::string& arg1)
  throw(error_handler_id_error,error_handler_format_error)
{
  std::vector<std::string> args;
  args.push_back(arg1);
  return information_message(position, id, args);
}

std::vector<std::string> error_handler_base::information_message(const error_position& position,
                                                                 const std::string& id,
                                                                 const std::string& arg1, const std::string& arg2)
  throw(error_handler_id_error,error_handler_format_error)
{
  std::vector<std::string> args;
  args.push_back(arg1);
  args.push_back(arg2);
  return information_message(position, id, args);
}

std::vector<std::string> error_handler_base::information_message(const error_position& position,
                                                                 const std::string& id,
                                                                 const std::string& arg1, const std::string& arg2, const std::string& arg3)
  throw(error_handler_id_error,error_handler_format_error)
{
  std::vector<std::string> args;
  args.push_back(arg1);
  args.push_back(arg2);
  args.push_back(arg3);
  return information_message(position, id, args);
}

////////////////////////////////////////////////////////////////////////////////
// warning messages

std::vector<std::string> error_handler_base::warning_message(const std::string& id,
                                                             const std::vector<std::string>& args)
  throw(error_handler_id_error,error_handler_format_error)
{
  return warning_message(error_position(), id, args);
}

std::vector<std::string> error_handler_base::warning_message(const std::string& id)
  throw(error_handler_id_error,error_handler_format_error)
{
  std::vector<std::string> args;
  return warning_message(id, args);
}

std::vector<std::string> error_handler_base::warning_message(const std::string& id,
                                                             const std::string& arg1)
  throw(error_handler_id_error,error_handler_format_error)
{
  std::vector<std::string> args;
  args.push_back(arg1);
  return warning_message(id, args);
}

std::vector<std::string> error_handler_base::warning_message(const std::string& id,
                                                             const std::string& arg1, const std::string& arg2)
  throw(error_handler_id_error,error_handler_format_error)
{
  std::vector<std::string> args;
  args.push_back(arg1);
  args.push_back(arg2);
  return warning_message(id, args);
}

std::vector<std::string> error_handler_base::warning_message(const std::string& id,
                                                             const std::string& arg1, const std::string& arg2, const std::string& arg3)
  throw(error_handler_id_error,error_handler_format_error)
{
  std::vector<std::string> args;
  args.push_back(arg1);
  args.push_back(arg2);
  args.push_back(arg3);
  return warning_message(id, args);
}

std::vector<std::string> error_handler_base::warning_message(const error_position& position,
                                                             const std::string& id,
                                                             const std::vector<std::string>& args)
  throw(error_handler_id_error,error_handler_format_error)
{
  return m_base_body->format_report(position, id, warning_id, args);
}

std::vector<std::string> error_handler_base::warning_message(const error_position& position,
                                                             const std::string& id)
  throw(error_handler_id_error,error_handler_format_error)
{
  std::vector<std::string> args;
  return warning_message(position, id, args);
}

std::vector<std::string> error_handler_base::warning_message(const error_position& position,
                                                             const std::string& id,
                                                             const std::string& arg1)
  throw(error_handler_id_error,error_handler_format_error)
{
  std::vector<std::string> args;
  args.push_back(arg1);
  return warning_message(position, id, args);
}

std::vector<std::string> error_handler_base::warning_message(const error_position& position,
                                                             const std::string& id,
                                                             const std::string& arg1, const std::string& arg2)
  throw(error_handler_id_error,error_handler_format_error)
{
  std::vector<std::string> args;
  args.push_back(arg1);
  args.push_back(arg2);
  return warning_message(position, id, args);
}

std::vector<std::string> error_handler_base::warning_message(const error_position& position,
                                                             const std::string& id,
                                                             const std::string& arg1, const std::string& arg2, const std::string& arg3)
  throw(error_handler_id_error,error_handler_format_error)
{
  std::vector<std::string> args;
  args.push_back(arg1);
  args.push_back(arg2);
  args.push_back(arg3);
  return warning_message(position, id, args);
}

////////////////////////////////////////////////////////////////////////////////
// error messages

std::vector<std::string> error_handler_base::error_message(const std::string& id,
                                                           const std::vector<std::string>& args)
  throw(error_handler_id_error,error_handler_format_error)
{
  return error_message(error_position(), id, args);
}

std::vector<std::string> error_handler_base::error_message(const std::string& id)
  throw(error_handler_id_error,error_handler_format_error)
{
  std::vector<std::string> args;
  return error_message(id, args);
}

std::vector<std::string> error_handler_base::error_message(const std::string& id,
                                                           const std::string& arg1)
  throw(error_handler_id_error,error_handler_format_error)
{
  std::vector<std::string> args;
  args.push_back(arg1);
  return error_message(id, args);
}

std::vector<std::string> error_handler_base::error_message(const std::string& id,
                                                           const std::string& arg1, const std::string& arg2)
  throw(error_handler_id_error,error_handler_format_error)
{
  std::vector<std::string> args;
  args.push_back(arg1);
  args.push_back(arg2);
  return error_message(id, args);
}

std::vector<std::string> error_handler_base::error_message(const std::string& id,
                                                           const std::string& arg1, const std::string& arg2, const std::string& arg3)
  throw(error_handler_id_error,error_handler_format_error)
{
  std::vector<std::string> args;
  args.push_back(arg1);
  args.push_back(arg2);
  args.push_back(arg3);
  return error_message(id, args);
}

std::vector<std::string> error_handler_base::error_message(const error_position& position,
                                                           const std::string& id,
                                                           const std::vector<std::string>& args)
  throw(error_handler_id_error,error_handler_format_error)
{
  return m_base_body->format_report(position, id, error_id, args);
}

std::vector<std::string> error_handler_base::error_message(const error_position& position,
                                                           const std::string& id)
  throw(error_handler_id_error,error_handler_format_error)
{
  std::vector<std::string> args;
  return error_message(position, id, args);
}

std::vector<std::string> error_handler_base::error_message(const error_position& position,
                                                           const std::string& id,
                                                           const std::string& arg1)
  throw(error_handler_id_error,error_handler_format_error)
{
  std::vector<std::string> args;
  args.push_back(arg1);
  return error_message(position, id, args);
}

std::vector<std::string> error_handler_base::error_message(const error_position& position,
                                                           const std::string& id,
                                                           const std::string& arg1, const std::string& arg2)
  throw(error_handler_id_error,error_handler_format_error)
{
  std::vector<std::string> args;
  args.push_back(arg1);
  args.push_back(arg2);
  return error_message(position, id, args);
}

std::vector<std::string> error_handler_base::error_message(const error_position& position,
                                                           const std::string& id,
                                                           const std::string& arg1, const std::string& arg2, const std::string& arg3)
  throw(error_handler_id_error,error_handler_format_error)
{
  std::vector<std::string> args;
  args.push_back(arg1);
  args.push_back(arg2);
  args.push_back(arg3);
  return error_message(position, id, args);
}

////////////////////////////////////////////////////////////////////////////////
// fatal messages

std::vector<std::string> error_handler_base::fatal_message(const std::string& id,
                                                           const std::vector<std::string>& args)
  throw(error_handler_id_error,error_handler_format_error)
{
  return fatal_message(error_position(), id, args);
}

std::vector<std::string> error_handler_base::fatal_message(const std::string& id)
  throw(error_handler_id_error,error_handler_format_error)
{
  std::vector<std::string> args;
  return fatal_message(id, args);
}

std::vector<std::string> error_handler_base::fatal_message(const std::string& id,
                                                           const std::string& arg1)
  throw(error_handler_id_error,error_handler_format_error)
{
  std::vector<std::string> args;
  args.push_back(arg1);
  return fatal_message(id, args);
}

std::vector<std::string> error_handler_base::fatal_message(const std::string& id,
                                                           const std::string& arg1, const std::string& arg2)
  throw(error_handler_id_error,error_handler_format_error)
{
  std::vector<std::string> args;
  args.push_back(arg1);
  args.push_back(arg2);
  return fatal_message(id, args);
}

std::vector<std::string> error_handler_base::fatal_message(const std::string& id,
                                                           const std::string& arg1, const std::string& arg2, const std::string& arg3)
  throw(error_handler_id_error,error_handler_format_error)
{
  std::vector<std::string> args;
  args.push_back(arg1);
  args.push_back(arg2);
  args.push_back(arg3);
  return fatal_message(id, args);
}

std::vector<std::string> error_handler_base::fatal_message(const error_position& position,
                                                           const std::string& id,
                                                           const std::vector<std::string>& args)
  throw(error_handler_id_error,error_handler_format_error)
{
  return m_base_body->format_report(position, id, fatal_id, args);
}

std::vector<std::string> error_handler_base::fatal_message(const error_position& position,
                                                           const std::string& id)
  throw(error_handler_id_error,error_handler_format_error)
{
  std::vector<std::string> args;
  return fatal_message(position, id, args);
}

std::vector<std::string> error_handler_base::fatal_message(const error_position& position,
                                                           const std::string& id,
                                                           const std::string& arg1)
  throw(error_handler_id_error,error_handler_format_error)
{
  std::vector<std::string> args;
  args.push_back(arg1);
  return fatal_message(position, id, args);
}

std::vector<std::string> error_handler_base::fatal_message(const error_position& position,
                                                           const std::string& id,
                                                           const std::string& arg1, const std::string& arg2)
  throw(error_handler_id_error,error_handler_format_error)
{
  std::vector<std::string> args;
  args.push_back(arg1);
  args.push_back(arg2);
  return fatal_message(position, id, args);
}

std::vector<std::string> error_handler_base::fatal_message(const error_position& position,
                                                           const std::string& id,
                                                           const std::string& arg1, const std::string& arg2, const std::string& arg3)
  throw(error_handler_id_error,error_handler_format_error)
{
  std::vector<std::string> args;
  args.push_back(arg1);
  args.push_back(arg2);
  args.push_back(arg3);
  return fatal_message(position, id, args);
}

////////////////////////////////////////////////////////////////////////////////
// supplemental messages

void error_handler_base::push_supplement(const std::string& id,
                                         const std::vector<std::string>& args)
  throw(error_handler_id_error,error_handler_format_error)
{
  push_supplement(error_position(), id, args);
}

void error_handler_base::push_supplement(const std::string& id)
  throw(error_handler_id_error,error_handler_format_error)
{
  std::vector<std::string> args;
  push_supplement(id, args);
}

void error_handler_base::push_supplement(const std::string& id,
                                         const std::string& arg1)
  throw(error_handler_id_error,error_handler_format_error)
{
  std::vector<std::string> args;
  args.push_back(arg1);
  push_supplement(id, args);
}

void error_handler_base::push_supplement(const std::string& id,
                                         const std::string& arg1, const std::string& arg2)
  throw(error_handler_id_error,error_handler_format_error)
{
  std::vector<std::string> args;
  args.push_back(arg1);
  args.push_back(arg2);
  push_supplement(id, args);
}

void error_handler_base::push_supplement(const std::string& id,
                                         const std::string& arg1, const std::string& arg2, const std::string& arg3)
  throw(error_handler_id_error,error_handler_format_error)
{
  std::vector<std::string> args;
  args.push_back(arg1);
  args.push_back(arg2);
  args.push_back(arg3);
  push_supplement(id, args);
}

void error_handler_base::push_supplement(const error_position& position,
                                         const std::string& id,
                                         const std::vector<std::string>& args)
  throw(error_handler_id_error,error_handler_format_error)
{
  m_base_body->push_supplement(position, id, args);
}

void error_handler_base::push_supplement(const error_position& position,
                                         const std::string& id)
  throw(error_handler_id_error,error_handler_format_error)
{
  std::vector<std::string> args;
  push_supplement(position, id, args);
}

void error_handler_base::push_supplement(const error_position& position,
                                         const std::string& id,
                                         const std::string& arg1)
  throw(error_handler_id_error,error_handler_format_error)
{
  std::vector<std::string> args;
  args.push_back(arg1);
  push_supplement(position, id, args);
}

void error_handler_base::push_supplement(const error_position& position,
                                         const std::string& id,
                                         const std::string& arg1, const std::string& arg2)
  throw(error_handler_id_error,error_handler_format_error)
{
  std::vector<std::string> args;
  args.push_back(arg1);
  args.push_back(arg2);
  push_supplement(position, id, args);
}

void error_handler_base::push_supplement(const error_position& position,
                                         const std::string& id,
                                         const std::string& arg1, const std::string& arg2, const std::string& arg3)
  throw(error_handler_id_error,error_handler_format_error)
{
  std::vector<std::string> args;
  args.push_back(arg1);
  args.push_back(arg2);
  args.push_back(arg3);
  push_supplement(position, id, args);
}

////////////////////////////////////////////////////////////////////////////////
// context

void error_handler_base::push_context (const std::string& id,
                                       const std::vector<std::string>& args)
  throw(error_handler_id_error,error_handler_format_error)
{
  push_context(error_position(), id, args);
}

void error_handler_base::push_context (const std::string& id)
  throw(error_handler_id_error,error_handler_format_error)
{
  std::vector<std::string> args;
  push_context(id, args);
}

void error_handler_base::push_context (const std::string& id,
                                       const std::string& arg1)
  throw(error_handler_id_error,error_handler_format_error)
{
  std::vector<std::string> args;
  args.push_back(arg1);
  push_context(id, args);
}

void error_handler_base::push_context (const std::string& id,
                                       const std::string& arg1, const std::string& arg2)
  throw(error_handler_id_error,error_handler_format_error)
{
  std::vector<std::string> args;
  args.push_back(arg1);
  args.push_back(arg2);
  push_context(id, args);
}

void error_handler_base::push_context (const std::string& id,
                                       const std::string& arg1, const std::string& arg2, const std::string& arg3)
  throw(error_handler_id_error,error_handler_format_error)
{
  std::vector<std::string> args;
  args.push_back(arg1);
  args.push_back(arg2);
  args.push_back(arg3);
  push_context(id, args);
}

void error_handler_base::push_context (const error_position& position,
                                       const std::string& id,
                                       const std::vector<std::string>& args)
  throw(error_handler_id_error,error_handler_format_error)
{
  m_base_body->push_context(position, id, args);
}

void error_handler_base::push_context (const error_position& position,
                                       const std::string& id)
  throw(error_handler_id_error,error_handler_format_error)
{
  std::vector<std::string> args;
  push_context(position, id, args);
}

void error_handler_base::push_context (const error_position& position,
                                       const std::string& id,
                                       const std::string& arg1)
  throw(error_handler_id_error,error_handler_format_error)
{
  std::vector<std::string> args;
  args.push_back(arg1);
  push_context(position, id, args);
}

void error_handler_base::push_context (const error_position& position,
                                       const std::string& id,
                                       const std::string& arg1, const std::string& arg2)
  throw(error_handler_id_error,error_handler_format_error)
{
  std::vector<std::string> args;
  args.push_back(arg1);
  args.push_back(arg2);
  push_context(position, id, args);
}

void error_handler_base::push_context (const error_position& position,
                                       const std::string& id,
                                       const std::string& arg1, const std::string& arg2, const std::string& arg3)
  throw(error_handler_id_error,error_handler_format_error)
{
  std::vector<std::string> args;
  args.push_back(arg1);
  args.push_back(arg2);
  args.push_back(arg3);
  push_context(position, id, args);
}

void error_handler_base::pop_context(void) throw()
{
  m_base_body->pop_context(m_base_body->context_depth()-1);
}

void error_handler_base::pop_context(unsigned depth) throw()
{
  m_base_body->pop_context(depth);
}

unsigned error_handler_base::context_depth(void) const
  throw()
{
  return m_base_body->context_depth();
}

error_context error_handler_base::auto_push_context(const std::string& id, const std::vector<std::string>& args)
  throw(error_handler_id_error,error_handler_format_error)
{
  return auto_push_context(error_position(), id, args);
}

error_context error_handler_base::auto_push_context(const std::string& id)                                                 
  throw(error_handler_id_error,error_handler_format_error)
{
  std::vector<std::string> args;
  return auto_push_context(id, args);
}

error_context error_handler_base::auto_push_context(const std::string& id, const std::string& arg1)                         
  throw(error_handler_id_error,error_handler_format_error)
{
  std::vector<std::string> args;
  args.push_back(arg1);
  return auto_push_context(id, args);
}

error_context error_handler_base::auto_push_context (const std::string& id,
                                                     const std::string& arg1,
                                                     const std::string& arg2) 
  throw(error_handler_id_error,error_handler_format_error)
{
  std::vector<std::string> args;
  args.push_back(arg1);
  args.push_back(arg2);
  return auto_push_context(id, args);
}

error_context error_handler_base::auto_push_context(const std::string& id,
                                                    const std::string& arg1,
                                                    const std::string& arg2,
                                                    const std::string& arg3) 
  throw(error_handler_id_error,error_handler_format_error)
{
  std::vector<std::string> args;
  args.push_back(arg1);
  args.push_back(arg2);
  args.push_back(arg3);
  return auto_push_context(id, args);
}

error_context error_handler_base::auto_push_context(const error_position& position,
                                                    const std::string& id,
                                                    const std::vector<std::string>& args)            
  throw(error_handler_id_error,error_handler_format_error)
{
  error_context result(new error_context_body(*this));
  m_base_body->push_context(position, id, args);
  return result;
}

error_context error_handler_base::auto_push_context(const error_position& position,
                                                    const std::string& id)             
  throw(error_handler_id_error,error_handler_format_error)
{
  std::vector<std::string> args;
  return auto_push_context(position, id, args);
}

error_context error_handler_base::auto_push_context(const error_position& position,
                                                    const std::string& id,
                                                    const std::string& arg1)                         
  throw(error_handler_id_error,error_handler_format_error)
{
  std::vector<std::string> args;
  args.push_back(arg1);
  return auto_push_context(position, id, args);
}

error_context error_handler_base::auto_push_context(const error_position& position,
                                                    const std::string& id,
                                                    const std::string& arg1,
                                                    const std::string& arg2) 
  throw(error_handler_id_error,error_handler_format_error)
{
  std::vector<std::string> args;
  args.push_back(arg1);
  args.push_back(arg2);
  return auto_push_context(position, id, args);
}

error_context error_handler_base::auto_push_context(const error_position& position,
                                                    const std::string& id,
                                                    const std::string& arg1,
                                                    const std::string& arg2,
                                                    const std::string& arg3) 
  throw(error_handler_id_error,error_handler_format_error)
{
  std::vector<std::string> args;
  args.push_back(arg1);
  args.push_back(arg2);
  args.push_back(arg3);
  return auto_push_context(position, id, args);
}

////////////////////////////////////////////////////////////////////////////////
// textio-based derivative uses the above base class to generate messages then uses textio to print them
////////////////////////////////////////////////////////////////////////////////

class error_handler_body
{
private:
  otext m_device;                                // TextIO output device
  unsigned m_limit;                              // error limit
  unsigned m_count;                              // error count
public:
  error_handler_body(otext& device, unsigned limit) :
    m_device(device), m_limit(limit), m_count(0)
    {
    }
  ~error_handler_body(void) {}

  otext& device(void) {return m_device;}
  unsigned limit(void) const {return m_limit;}
  void set_limit(unsigned limit) {m_limit = limit;}
  unsigned count(void) const {return m_count;}
  void set_count(unsigned count) {m_count = count;}
  void increment(void) {++m_count;}
  bool limit_reached(void) const {return m_limit > 0 && m_count >= m_limit;}
};

////////////////////////////////////////////////////////////////////////////////

error_handler::error_handler(otext& device, unsigned limit, bool show)
  throw() :
  error_handler_base(show), m_body(new error_handler_body(device, limit))
{
}

error_handler::error_handler(otext& device, const std::string& message_file, unsigned limit, bool show) 
  throw(error_handler_read_error) :
  error_handler_base(message_file,show), m_body(new error_handler_body(device, limit))
{
}

error_handler::error_handler(otext& device, const std::vector<std::string>& message_files, unsigned limit, bool show) 
  throw(error_handler_read_error) :
  error_handler_base(message_files,show), m_body(new error_handler_body(device, limit))
{
}

error_handler::~error_handler(void)
  throw()
{
  m_body->device().flush();
}

////////////////////////////////////////////////////////////////////////////////
// error count

void error_handler::set_error_limit(unsigned error_limit)
  throw()
{
  m_body->set_limit(error_limit);
}

unsigned error_handler::error_limit(void) const
  throw()
{
  return m_body->limit();
}

void error_handler::reset_error_count(void)
  throw()
{
  m_body->set_count(0);
}

unsigned error_handler::error_count(void) const
  throw()
{
  return m_body->count();
}

otext& error_handler::device(void)
{
  return m_body->device();
}

////////////////////////////////////////////////////////////////////////////////
// information messages

bool error_handler::information(const std::string& id,
                                const std::vector<std::string>& args)
  throw(error_handler_id_error,error_handler_format_error)
{
  device() << information_message(id, args) << flush;
  return true;
}

bool error_handler::information(const std::string& id)
  throw(error_handler_id_error,error_handler_format_error)
{
  device() << information_message(id) << flush;
  return true;
}

bool error_handler::information(const std::string& id,
                                const std::string& arg1)
  throw(error_handler_id_error,error_handler_format_error)
{
  device() << information_message(id, arg1) << flush;
  return true;
}

bool error_handler::information(const std::string& id,
                                const std::string& arg1, const std::string& arg2)
  throw(error_handler_id_error,error_handler_format_error)
{
  device() << information_message(id, arg1, arg2) << flush;
  return true;
}

bool error_handler::information(const std::string& id,
                                const std::string& arg1, const std::string& arg2, const std::string& arg3)
  throw(error_handler_id_error,error_handler_format_error)
{
  device() << information_message(id, arg1, arg2, arg3) << flush;
  return true;
}

bool error_handler::information(const error_position& position,
                                const std::string& id,
                                const std::vector<std::string>& args)
  throw(error_handler_id_error,error_handler_format_error)
{
  device() << information_message(position, id, args) << flush;
  return true;
}

bool error_handler::information(const error_position& position,
                                const std::string& id)
  throw(error_handler_id_error,error_handler_format_error)
{
  device() << information_message(position, id) << flush;
  return true;
}

bool error_handler::information(const error_position& position,
                                const std::string& id,
                                const std::string& arg1)
  throw(error_handler_id_error,error_handler_format_error)
{
  device() << information_message(position, id, arg1) << flush;
  return true;
}

bool error_handler::information(const error_position& position,
                                const std::string& id,
                                const std::string& arg1, const std::string& arg2)
  throw(error_handler_id_error,error_handler_format_error)
{
  device() << information_message(position, id, arg1, arg2) << flush;
  return true;
}

bool error_handler::information(const error_position& position,
                                const std::string& id,
                                const std::string& arg1, const std::string& arg2, const std::string& arg3)
  throw(error_handler_id_error,error_handler_format_error)
{
  device() << information_message(position, id, arg1, arg2, arg3) << flush;
  return true;
}

////////////////////////////////////////////////////////////////////////////////
// warning messages

bool error_handler::warning(const std::string& id,
                            const std::vector<std::string>& args)
  throw(error_handler_id_error,error_handler_format_error)
{
  device() << warning_message(id, args) << flush;
  return true;
}

bool error_handler::warning(const std::string& id)
  throw(error_handler_id_error,error_handler_format_error)
{
  device() << warning_message(id) << flush;
  return true;
}

bool error_handler::warning(const std::string& id,
                            const std::string& arg1)
  throw(error_handler_id_error,error_handler_format_error)
{
  device() << warning_message(id, arg1) << flush;
  return true;
}

bool error_handler::warning(const std::string& id,
                            const std::string& arg1, const std::string& arg2)
  throw(error_handler_id_error,error_handler_format_error)
{
  device() << warning_message(id, arg1, arg2) << flush;
  return true;
}

bool error_handler::warning(const std::string& id,
                            const std::string& arg1, const std::string& arg2, const std::string& arg3)
  throw(error_handler_id_error,error_handler_format_error)
{
  device() << warning_message(id, arg1, arg2, arg3) << flush;
  return true;
}

bool error_handler::warning(const error_position& position,
                            const std::string& id,
                            const std::vector<std::string>& args)
  throw(error_handler_id_error,error_handler_format_error)
{
  device() << warning_message(position, id, args) << flush;
  return true;
}

bool error_handler::warning(const error_position& position,
                            const std::string& id)
  throw(error_handler_id_error,error_handler_format_error)
{
  device() << warning_message(position, id) << flush;
  return true;
}

bool error_handler::warning(const error_position& position,
                            const std::string& id,
                            const std::string& arg1)
  throw(error_handler_id_error,error_handler_format_error)
{
  device() << warning_message(position, id, arg1) << flush;
  return true;
}

bool error_handler::warning(const error_position& position,
                            const std::string& id,
                            const std::string& arg1, const std::string& arg2)
  throw(error_handler_id_error,error_handler_format_error)
{
  device() << warning_message(position, id, arg1, arg2) << flush;
  return true;
}

bool error_handler::warning(const error_position& position,
                            const std::string& id,
                            const std::string& arg1, const std::string& arg2, const std::string& arg3)
  throw(error_handler_id_error,error_handler_format_error)
{
  device() << warning_message(position, id, arg1, arg2, arg3) << flush;
  return true;
}

////////////////////////////////////////////////////////////////////////////////
// error messages

bool error_handler::error(const std::string& id,
                          const std::vector<std::string>& args)
  throw(error_handler_id_error,error_handler_format_error,error_handler_limit_error)
{
  device() << error_message(id, args) << flush;
  m_body->increment();
  if (m_body->limit_reached()) throw error_handler_limit_error(m_body->limit());
  return false;
}

bool error_handler::error(const std::string& id)
  throw(error_handler_id_error,error_handler_format_error,error_handler_limit_error)
{
  device() << error_message(id) << flush;
  m_body->increment();
  if (m_body->limit_reached()) throw error_handler_limit_error(m_body->limit());
  return false;
}

bool error_handler::error(const std::string& id,
                          const std::string& arg1)
  throw(error_handler_id_error,error_handler_format_error,error_handler_limit_error)
{
  device() << error_message(id, arg1) << flush;
  m_body->increment();
  if (m_body->limit_reached()) throw error_handler_limit_error(m_body->limit());
  return false;
}

bool error_handler::error(const std::string& id,
                          const std::string& arg1, const std::string& arg2)
  throw(error_handler_id_error,error_handler_format_error,error_handler_limit_error)
{
  device() << error_message(id, arg1, arg2) << flush;
  m_body->increment();
  if (m_body->limit_reached()) throw error_handler_limit_error(m_body->limit());
  return false;
}

bool error_handler::error(const std::string& id,
                          const std::string& arg1, const std::string& arg2, const std::string& arg3)
  throw(error_handler_id_error,error_handler_format_error,error_handler_limit_error)
{
  device() << error_message(id, arg1, arg2, arg3) << flush;
  m_body->increment();
  if (m_body->limit_reached()) throw error_handler_limit_error(m_body->limit());
  return false;
}

bool error_handler::error(const error_position& position,
                          const std::string& id,
                          const std::vector<std::string>& args)
  throw(error_handler_id_error,error_handler_format_error,error_handler_limit_error)
{
  device() << error_message(position, id, args) << flush;
  m_body->increment();
  if (m_body->limit_reached()) throw error_handler_limit_error(m_body->limit());
  return false;
}

bool error_handler::error(const error_position& position,
                          const std::string& id)
  throw(error_handler_id_error,error_handler_format_error,error_handler_limit_error)
{
  device() << error_message(position, id) << flush;
  m_body->increment();
  if (m_body->limit_reached()) throw error_handler_limit_error(m_body->limit());
  return false;
}

bool error_handler::error(const error_position& position,
                          const std::string& id,
                          const std::string& arg1)
  throw(error_handler_id_error,error_handler_format_error,error_handler_limit_error)
{
  device() << error_message(position, id, arg1) << flush;
  m_body->increment();
  if (m_body->limit_reached()) throw error_handler_limit_error(m_body->limit());
  return false;
}

bool error_handler::error(const error_position& position,
                          const std::string& id,
                          const std::string& arg1, const std::string& arg2)
  throw(error_handler_id_error,error_handler_format_error,error_handler_limit_error)
{
  device() << error_message(position, id, arg1, arg2) << flush;
  m_body->increment();
  if (m_body->limit_reached()) throw error_handler_limit_error(m_body->limit());
  return false;
}

bool error_handler::error(const error_position& position,
                          const std::string& id,
                          const std::string& arg1, const std::string& arg2, const std::string& arg3)
  throw(error_handler_id_error,error_handler_format_error,error_handler_limit_error)
{
  device() << error_message(position, id, arg1, arg2, arg3) << flush;
  m_body->increment();
  if (m_body->limit_reached()) throw error_handler_limit_error(m_body->limit());
  return false;
}

////////////////////////////////////////////////////////////////////////////////
// fatal messages

bool error_handler::fatal(const std::string& id,
                          const std::vector<std::string>& args)
  throw(error_handler_id_error,error_handler_format_error,error_handler_fatal_error)
{
  device() << fatal_message(id, args) << flush;
  throw error_handler_fatal_error(id);
}

bool error_handler::fatal(const std::string& id)
  throw(error_handler_id_error,error_handler_format_error,error_handler_fatal_error)
{
  device() << fatal_message(id) << flush;
  throw error_handler_fatal_error(id);
}

bool error_handler::fatal(const std::string& id,
                          const std::string& arg1)
  throw(error_handler_id_error,error_handler_format_error,error_handler_fatal_error)
{
  device() << fatal_message(id, arg1) << flush;
  throw error_handler_fatal_error(id);
}

bool error_handler::fatal(const std::string& id,
                          const std::string& arg1, const std::string& arg2)
  throw(error_handler_id_error,error_handler_format_error,error_handler_fatal_error)
{
  device() << fatal_message(id, arg1, arg2) << flush;
  throw error_handler_fatal_error(id);
}

bool error_handler::fatal(const std::string& id,
                          const std::string& arg1, const std::string& arg2, const std::string& arg3)
  throw(error_handler_id_error,error_handler_format_error,error_handler_fatal_error)
{
  device() << fatal_message(id, arg1, arg2, arg3) << flush;
  throw error_handler_fatal_error(id);
}

bool error_handler::fatal(const error_position& position,
                          const std::string& id,
                          const std::vector<std::string>& args)
  throw(error_handler_id_error,error_handler_format_error,error_handler_fatal_error)
{
  device() << fatal_message(position, id, args) << flush;
  throw error_handler_fatal_error(id);
}

bool error_handler::fatal(const error_position& position,
                          const std::string& id)
  throw(error_handler_id_error,error_handler_format_error,error_handler_fatal_error)
{
  device() << fatal_message(position, id) << flush;
  throw error_handler_fatal_error(id);
}

bool error_handler::fatal(const error_position& position,
                          const std::string& id,
                          const std::string& arg1)
  throw(error_handler_id_error,error_handler_format_error,error_handler_fatal_error)
{
  device() << fatal_message(position, id, arg1) << flush;
  throw error_handler_fatal_error(id);
}

bool error_handler::fatal(const error_position& position,
                          const std::string& id,
                          const std::string& arg1, const std::string& arg2)
  throw(error_handler_id_error,error_handler_format_error,error_handler_fatal_error)
{
  device() << fatal_message(position, id, arg1, arg2) << flush;
  throw error_handler_fatal_error(id);
}

bool error_handler::fatal(const error_position& position,
                          const std::string& id,
                          const std::string& arg1, const std::string& arg2, const std::string& arg3)
  throw(error_handler_id_error,error_handler_format_error,error_handler_fatal_error)
{
  device() << fatal_message(position, id, arg1, arg2, arg3) << flush;
  throw error_handler_fatal_error(id);
}

///////////////////////////////////////////////////////////////////////////////
// plain text

bool error_handler::plaintext(const std::string& text)
{
  device() << text << endl << flush;
  return true;
}

////////////////////////////////////////////////////////////////////////////////
