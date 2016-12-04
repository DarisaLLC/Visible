/*------------------------------------------------------------------------------

  Author:    Andy Rushton
  Copyright: (c) Andy Rushton, 2004
  License:   BSD License, see ../docs/license.html

  ------------------------------------------------------------------------------*/
#include "cli_parser.hpp"
#include "string_utilities.hpp"
#include "file_system.hpp"
#include "debug.hpp"

////////////////////////////////////////////////////////////////////////////////
// cli_definition internals

const std::string& cli_definition::name(void) const
{
  return m_name;
}

cli_kind_t cli_definition::kind(void) const
{ 
  return m_kind;
}

cli_mode_t cli_definition::mode(void) const
{
  return m_mode;
}

const std::string& cli_definition::message(void) const
{
  return m_message;
}

const std::string& cli_definition::default_value(void) const
{
  return m_default;
}

////////////////////////////////////////////////////////////////////////////////
// internal data structures

class cli_value
{
public:
  unsigned m_definition;
  std::string m_value;
  unsigned m_level;
  std::string m_source;

  cli_value(unsigned definition, const std::string& value, unsigned level, const std::string& source) :
    m_definition(definition), m_value(value), m_level(level), m_source(source) {}
};

typedef std::vector<cli_value> cli_value_vector;

class cli_parser_data
{
public:
  error_handler& m_errors;
  std::string m_executable;
  cli_parser::definitions m_definitions;
  cli_value_vector m_values;
  unsigned m_level;
  bool m_valid;
  std::vector<std::string> m_ini_files;

  cli_parser_data(error_handler& errors);
  unsigned add_definition(const std::string& name, cli_parser::kind_t kind, cli_parser::mode_t mode, const std::string& message);
  unsigned find_definition(const std::string& name);
  void clear_definitions(void);

  void reset_level(void);
  void increase_level(void);
  void clear_level(unsigned definition, unsigned level);

  void set_valid(void);
  void set_invalid(void);
  bool valid(void) const;

  unsigned add_value(unsigned definition, const std::string& value, const std::string& source);
  unsigned add_switch(unsigned definition, bool value, const std::string& source);
  void erase_value(unsigned definition);

  void add_ini_file(const std::string& file);
  unsigned ini_file_size(void) const;
  const std::string& ini_file(unsigned) const;

private:
  // make this class uncopyable
  cli_parser_data(const cli_parser_data&);
  cli_parser_data& operator = (const cli_parser_data&);
};

cli_parser_data::cli_parser_data(error_handler& errors) : 
  m_errors(errors), m_level(1), m_valid(true)
{
}

unsigned cli_parser_data::add_definition(const std::string& name, cli_parser::kind_t kind, cli_parser::mode_t mode, const std::string& message)
{
  m_definitions.push_back(cli_parser::definition(name, kind, mode, message));
  return m_definitions.size()-1;
}

unsigned cli_parser_data::find_definition(const std::string& name)
{
  // this does substring matching on the definitions and returns the first
  // match - however, it requires at least one character in the substring so
  // that the "" convention for command line arguments doesn't match with
  // anything. It returns size() if it fails
  for (unsigned i = 0; i < m_definitions.size(); i++)
  {
    std::string candidate = m_definitions[i].m_name;
    if ((candidate.empty() && name.empty()) ||
        (!name.empty() && candidate.size() >= name.size() && candidate.substr(0,name.size()) == name))
      return i;
  }
  return m_definitions.size();
}

void cli_parser_data::clear_definitions(void)
{
  m_definitions.clear();
  m_values.clear();
  reset_level();
  set_valid();
}

void cli_parser_data::reset_level(void)
{
  // the starting level is 1 so that later functions can call clear_level with a value of m_level-1 without causing underflow
  m_level = 1;
}

void cli_parser_data::increase_level(void)
{
  m_level++;
}

void cli_parser_data::clear_level(unsigned definition, unsigned level)
{
  // clears all values with this definition at the specified level or below
  for (cli_value_vector::iterator i = m_values.begin(); i != m_values.end(); )
  {
    if (i->m_definition == definition && i->m_level <= level)
      i = m_values.erase(i);
    else
      i++;
  }
}

void cli_parser_data::set_valid(void)
{
  m_valid = true;
}

void cli_parser_data::set_invalid(void)
{
  m_valid = false;
}

bool cli_parser_data::valid(void) const
{
  return m_valid;
}

unsigned cli_parser_data::add_value(unsigned definition, const std::string& value, const std::string& source)
{
  // behaviour depends on mode:
  //  - single: erase all previous values
  //  - multiple: erase values at a lower level than current
  //  - cumulative: erase no previous values
  switch (m_definitions[definition].m_mode)
  {
  case cli_single_mode:
    clear_level(definition, m_level);
    break;
  case cli_multiple_mode:
    clear_level(definition, m_level-1);
    break;
  case cli_cumulative_mode:
    break;
  }
  m_values.push_back(cli_value(definition,value,m_level,source));
  return m_values.size()-1;
}

unsigned cli_parser_data::add_switch(unsigned definition, bool value, const std::string& source)
{
  return add_value(definition, value ? "on" : "off", source);
}

void cli_parser_data::erase_value(unsigned definition)
{
  // this simply erases all previous values
  clear_level(definition, m_level);
}

void cli_parser_data::add_ini_file(const std::string& file)
{
  m_ini_files.push_back(file);
}

unsigned cli_parser_data::ini_file_size(void) const
{
  return m_ini_files.size();
}

const std::string& cli_parser_data::ini_file(unsigned i) const
{
  return m_ini_files[i];
}

////////////////////////////////////////////////////////////////////////////////

cli_parser::cli_parser(error_handler& errors) throw() : 
  m_data(new cli_parser_data(errors))
{
}

cli_parser::cli_parser(cli_parser::definitions_t definitions, error_handler& errors) throw(cli_mode_error) : 
  m_data(new cli_parser_data(errors))
{
  add_definitions(definitions);
}

cli_parser::cli_parser(cli_parser::definitions_t definitions, const ini_manager& defaults, const std::string& ini_section, error_handler& errors)  throw(cli_mode_error) : 
  m_data(new cli_parser_data(errors))
{
  add_definitions(definitions);
  set_defaults(defaults, ini_section);
}

cli_parser::cli_parser(char* argv[], cli_parser::definitions_t definitions, error_handler& errors)  throw(cli_mode_error,error_handler_id_error,error_handler_format_error) : 
  m_data(new cli_parser_data(errors))
{
  add_definitions(definitions);
  parse(argv);
}

cli_parser::cli_parser(char* argv[], cli_parser::definitions_t definitions, const ini_manager& defaults, const std::string& ini_section, error_handler& errors)  throw(cli_mode_error,error_handler_id_error,error_handler_format_error) : 
  m_data(new cli_parser_data(errors))
{
  add_definitions(definitions);
  set_defaults(defaults, ini_section);
  parse(argv);
}

cli_parser::cli_parser(cli_parser::definitions definitions, error_handler& errors) throw(cli_mode_error) : 
  m_data(new cli_parser_data(errors))
{
  add_definitions(definitions);
}

cli_parser::cli_parser(cli_parser::definitions definitions, const ini_manager& defaults, const std::string& ini_section, error_handler& errors)  throw(cli_mode_error) : 
  m_data(new cli_parser_data(errors))
{
  add_definitions(definitions);
  set_defaults(defaults, ini_section);
}

cli_parser::cli_parser(char* argv[], cli_parser::definitions definitions, error_handler& errors)  throw(cli_mode_error,error_handler_id_error,error_handler_format_error) : 
  m_data(new cli_parser_data(errors))
{
  add_definitions(definitions);
  parse(argv);
}

cli_parser::cli_parser(char* argv[], cli_parser::definitions definitions, const ini_manager& defaults, const std::string& ini_section, error_handler& errors)  throw(cli_mode_error,error_handler_id_error,error_handler_format_error) : 
  m_data(new cli_parser_data(errors))
{
  add_definitions(definitions);
  set_defaults(defaults, ini_section);
  parse(argv);
}

cli_parser::~cli_parser(void) throw()
{
}

void cli_parser::add_definitions(cli_parser::definitions_t definitions) throw(cli_mode_error)
{
  m_data->clear_definitions();
  // the definitions array is terminated by a definition with a null name pointer
  for (unsigned i = 0; definitions[i].m_name; i++)
    add_definition(definitions[i]);
}

unsigned cli_parser::add_definition(const cli_parser::definition_t& definition) throw(cli_mode_error,cli_argument_error)
{
  if (!definition.m_name) throw cli_argument_error("Definition must have a valid name");
  // chk for stupid combinations
  // at this stage the only really stupid one is to declare command line arguments to be switch mode
  if (definition.m_name[0] == '\0' && definition.m_kind == cli_switch_kind) 
  {
    m_data->set_invalid();
    throw cli_mode_error("CLI arguments cannot be switch kind");
  }
  // add the definition to the set of all definitions
  unsigned i = m_data->add_definition(definition.m_name, definition.m_kind, definition.m_mode, definition.m_message);
  // also add it to the list of values, but only if it has a default value
  if (definition.m_default) m_data->add_value(i, definition.m_default, "builtin default");
  return i;
}

void cli_parser::add_definitions(cli_parser::definitions definitions) throw(cli_mode_error)
{
  m_data->clear_definitions();
  for (unsigned i = 0; i < definitions.size(); i++)
    add_definition(definitions[i]);
}

unsigned cli_parser::add_definition(const cli_parser::definition& definition) throw(cli_mode_error)
{
  // chk for stupid combinations
  // at this stage the only really stupid one is to declare command line arguments to be switch mode
  if (definition.m_name.empty() && definition.m_kind == cli_switch_kind) 
  {
    m_data->set_invalid();
    throw cli_mode_error("CLI arguments cannot be switch kind");
  }
  // add the definition to the set of all definitions
  unsigned i = m_data->add_definition(definition.m_name, definition.m_kind, definition.m_mode, definition.m_message);
  // also add it to the list of values, but only if it has a default value
  if (!definition.m_default.empty()) m_data->add_value(i, definition.m_default, "builtin default");
  return i;
}

void cli_parser::set_defaults(const ini_manager& defaults, const std::string& ini_section) throw()
{
  // import default values from the Ini Manager
  m_data->increase_level();
  // get the set of all names from the Ini manager so that illegal names generate meaningful error messages
  std::vector<std::string> names = defaults.variable_names(ini_section);
  for (unsigned i = 0; i < names.size(); i++)
  {
    std::string name = names[i];
    unsigned definition = m_data->find_definition(name);
    if (definition == m_data->m_definitions.size())
    {
      // not found - give an error report
      error_position position(defaults.variable_filename(ini_section,name),
                              defaults.variable_linenumber(ini_section,name),
                              0);
      m_data->m_errors.error(position,"CLI_INI_VARIABLE", name);
    }
    else
    {
      // found - so add the value
      // multi-valued variables are entered as a comma-separated list and this is then turned into a vector
      // the vector is empty if the value was empty
      std::vector<std::string> values = defaults.variable_values(ini_section, name);
      // an empty string is used to negate the value
      if (values.empty())
        m_data->erase_value(definition);
      else
      {
        std::string source = filespec_to_relative_path(defaults.variable_filename(ini_section, name));
        for (unsigned j = 0; j < values.size(); j++)
          m_data->add_value(definition, values[j], source);
      }
    }
  }
  // add the set of ini files to the list for usage reports
  for (unsigned j = 0; j < defaults.size(); j++)
    m_data->add_ini_file(defaults.filename(j));
}

bool cli_parser::parse(char* argv[]) throw(cli_argument_error,error_handler_id_error,error_handler_format_error)
{
  bool result = true;
  if (!argv) throw cli_argument_error("Argument vector cannot be null");
  m_data->increase_level();
  if (argv[0])
    m_data->m_executable = argv[0];
  for (unsigned i = 1; argv[i]; i++)
  {
    std::string name = argv[i];
    if (!name.empty() && name[0] == '-')
    {
      // we have a command line option
      unsigned found = m_data->find_definition(name.substr(1, name.size()-1));
      if (found < m_data->m_definitions.size())
      {
        // found it in its positive form
        switch (m_data->m_definitions[found].m_kind)
        {
        case cli_switch_kind:
          m_data->add_switch(found, true, "command line");
          break;
        case cli_value_kind:
          // get the next argument in argv as the value of this option
          // first chk that there is a next value
          if (!argv[i+1])
            result &= m_data->m_errors.error("CLI_VALUE_MISSING", name);
          else
            m_data->add_value(found, argv[++i], "command line");
          break;
        }
      }
      else if (name.size() > 3 && name.substr(1,2) == "no")
      {
        found = m_data->find_definition(name.substr(3, name.size()-3));
        if (found < m_data->m_definitions.size())
        {
          // found it in its negated form
          switch (m_data->m_definitions[found].m_kind)
          {
          case cli_switch_kind:
            m_data->add_switch(found, false, "command line");
            break;
          case cli_value_kind:
            m_data->erase_value(found);
            break;
          }
        }
        else
        {
          // could not find this option in either its true or negated form
          result &= m_data->m_errors.error("CLI_UNRECOGNISED_OPTION", name);
        }
      }
      else
      {
        // could not find this option and it could not be negated
        result &= m_data->m_errors.error("CLI_UNRECOGNISED_OPTION", name);
      }
    }
    else
    {
      // we have a command-line value which is represented internally as an option with an empty string as its name
      // some very obscure commands do not have values - only options, so allow for that case too
      unsigned found = m_data->find_definition("");
      if (found < m_data->m_definitions.size())
        m_data->add_value(found, name, "command line");
      else
        result &= m_data->m_errors.error("CLI_NO_VALUES", name);
    }
  }
  if (!result) m_data->set_invalid();
  return result;
}

bool cli_parser::valid(void) throw()
{
  return m_data->valid();
}

unsigned cli_parser::size(void) const throw()
{
  return m_data->m_values.size();
}

std::string cli_parser::name(unsigned i) const throw(cli_index_error)
{
  if (i >= size()) throw cli_index_error("Index " + to_string(i) + " out of range");
  return m_data->m_definitions[m_data->m_values[i].m_definition].m_name;
}

unsigned cli_parser::id(unsigned i) const throw(cli_index_error)
{
  if (i >= size()) throw cli_index_error("Index " + to_string(i) + " out of range");
  return m_data->m_values[i].m_definition;
}

cli_parser::kind_t cli_parser::kind(unsigned i) const throw(cli_index_error)
{
  if (i >= size()) throw cli_index_error("Index " + to_string(i) + " out of range");
  return m_data->m_definitions[m_data->m_values[i].m_definition].m_kind;
}

bool cli_parser::switch_kind(unsigned i) const throw(cli_index_error)
{
  return kind(i) == cli_switch_kind;
}

bool cli_parser::value_kind(unsigned i) const throw(cli_index_error)
{
  return kind(i) == cli_value_kind;
}

cli_parser::mode_t cli_parser::mode(unsigned i) const throw(cli_index_error)
{
  if (i >= size()) throw cli_index_error("Index " + to_string(i) + " out of range");
  return m_data->m_definitions[m_data->m_values[i].m_definition].m_mode;
}

bool cli_parser::single_mode(unsigned i) const throw(cli_index_error)
{
  return mode(i) == cli_single_mode;
}

bool cli_parser::multiple_mode(unsigned i) const throw(cli_index_error)
{
  return mode(i) == cli_multiple_mode;
}

bool cli_parser::cumulative_mode(unsigned i) const throw(cli_index_error)
{
  return mode(i) == cli_cumulative_mode;
}

bool cli_parser::switch_value(unsigned i) const throw(cli_mode_error,cli_index_error)
{
  if (i >= size()) throw cli_index_error("Index " + to_string(i) + " out of range");
  if (kind(i) != cli_switch_kind) throw cli_mode_error(name(i) + " is not a switch kind");
  std::string value = lowercase(m_data->m_values[i].m_value);
  return value == "on" || value == "true" || value == "1";
}

std::string cli_parser::string_value(unsigned i) const throw(cli_mode_error,cli_index_error)
{
  if (i >= size()) throw cli_index_error("Index " + to_string(i) + " out of range");
  if (kind(i) != cli_value_kind) throw cli_mode_error(name(i) + " is not a value kind");
  return m_data->m_values[i].m_value;
}

////////////////////////////////////////////////////////////////////////////////

void cli_parser::usage(void) const throw(std::runtime_error)
{
  m_data->m_errors.information("CLI_USAGE_PROGRAM", m_data->m_executable);
  m_data->m_errors.information("CLI_USAGE_DEFINITIONS");
  for (unsigned d = 0; d < m_data->m_definitions.size(); d++)
  {
    m_data->m_errors.information(m_data->m_definitions[d].m_message);
  }
  if (size() != 0)
  {
    m_data->m_errors.information("CLI_USAGE_VALUES");
    for (unsigned v = 0; v < size(); v++)
    {
      std::string source = m_data->m_values[v].m_source;
      std::string key = name(v);
      if (key.empty())
      {
        // command-line values
        m_data->m_errors.information("CLI_USAGE_VALUE_RESULT", string_value(v), source);
      }
      else if (kind(v) == cli_switch_kind)
      {
        // a switch
        m_data->m_errors.information("CLI_USAGE_SWITCH_RESULT", (switch_value(v) ? name(v) : "no" + name(v)), source);
      }
      else
      {
        // other values
        std::vector<std::string> args;
        args.push_back(name(v));
        args.push_back(string_value(v));
        args.push_back(source);
        m_data->m_errors.information("CLI_USAGE_OPTION_RESULT", args);
      }
    }
  }
  if (m_data->ini_file_size() > 0)
  {
    m_data->m_errors.information("CLI_INI_HEADER");
    for (unsigned i = 0; i < m_data->ini_file_size(); i++)
    {
      if (file_exists(m_data->ini_file(i)))
        m_data->m_errors.information("CLI_INI_FILE_PRESENT", filespec_to_relative_path(m_data->ini_file(i)));
      else
        m_data->m_errors.information("CLI_INI_FILE_ABSENT", filespec_to_relative_path(m_data->ini_file(i)));
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
