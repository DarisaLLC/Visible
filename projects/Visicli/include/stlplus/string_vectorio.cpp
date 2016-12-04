/*------------------------------------------------------------------------------

  Author:    Andy Rushton
  Copyright: (c) Andy Rushton, 2004
  License:   BSD License, see ../docs/license.html

  ------------------------------------------------------------------------------*/
#include "os_fixes.hpp"
#include "string_vectorio.hpp"
#include "debug.hpp"

////////////////////////////////////////////////////////////////////////////////

osvtext::osvtext(void) :
  otext(new osvbuff)
{
}

std::vector<std::string>& osvtext::get_vector(void)
{
  osvbuff* buf = dynamic_cast<osvbuff*>(m_buffer);
  DEBUG_ASSERT(buf);
  return buf->m_data;
}

const std::vector<std::string>& osvtext::get_vector(void) const
{
  const osvbuff* buf = dynamic_cast<const osvbuff*>(m_buffer);
  DEBUG_ASSERT(buf);
  return buf->m_data;
}

////////////////////////////////////////////////////////////////////////////////

osvbuff::osvbuff(void) : 
  // switch character conversion to Unix mode - i.e. newlines are \n characters
  obuff(false,otext::unix_mode),
  m_eoln(true)
{
}

unsigned osvbuff::put(unsigned char ch)
{
  if (m_eoln)
  {
    m_data.push_back(std::string());
    m_eoln = false;
  }
  // detect only Unix-mode end-of-line
  // TODO - allow any output mode
  m_eoln = (ch == newline);
  if (!m_eoln)
    m_data.back() += (char)ch;
  return 1;
}

////////////////////////////////////////////////////////////////////////////////

isvtext::isvtext(const std::vector<std::string>& data) :
  itext(new isvbuff(data))
{
}

std::vector<std::string>& isvtext::get_vector(void)
{
  isvbuff* buf = dynamic_cast<isvbuff*>(m_buffer);
  DEBUG_ASSERT(buf);
  return buf->m_data;
}

const std::vector<std::string>& isvtext::get_vector(void) const
{
  const isvbuff* buf = dynamic_cast<const isvbuff*>(m_buffer);
  DEBUG_ASSERT(buf);
  return buf->m_data;
}

////////////////////////////////////////////////////////////////////////////////

isvbuff::isvbuff(const std::vector<std::string>& d) :
  m_data(d),
  m_row(0),
  m_column(0)
{
}

int isvbuff::peek(void)
{
  if (m_row >= m_data.size())
    return -1;
  std::string& line = m_data[m_row];
  if (m_column >= line.size())
    return newline;
  else
    return (int)(unsigned char)line[m_column];
}

int isvbuff::get(void)
{
  if (m_row >= m_data.size())
    return -1;
  std::string& line = m_data[m_row];
  if (m_column >= line.size())
  {
    m_row++;
    m_column = 0;
    return newline;
  }
  return (int)(unsigned char)line[m_column++];
}

////////////////////////////////////////////////////////////////////////////////
