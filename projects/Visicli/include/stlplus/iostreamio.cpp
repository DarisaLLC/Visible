/*------------------------------------------------------------------------------

  Author:    Andy Rushton
  Copyright: (c) Andy Rushton, 2004
  License:   BSD License, see ../docs/license.html

------------------------------------------------------------------------------*/
#include "iostreamio.hpp"
#include "debug.hpp"

////////////////////////////////////////////////////////////////////////////////
// stream output

oiotext::oiotext(std::ostream& str)
{
  open(str);
}

void oiotext::open(std::ostream& str)
{
  otext::open(new oiobuff(str));
}

std::ostream& oiotext::get_stream(void)
{
  oiobuff* buf = dynamic_cast<oiobuff*>(m_buffer);
  DEBUG_ASSERT(buf);
  return buf->m_stream;
}

const std::ostream& oiotext::get_stream(void) const
{
  const oiobuff* buf = dynamic_cast<const oiobuff*>(m_buffer);
  DEBUG_ASSERT(buf);
  return buf->m_stream;
}

////////////////////////////////////////////////////////////////////////////////
// output buffer

oiobuff::oiobuff(std::ostream& str) : m_stream(str)
{
}

oiobuff::~oiobuff(void)
{
  flush();
}

unsigned oiobuff::put(unsigned char ch)
{
  m_stream.put(ch);
  // capture any errors and clear non-fatal errors
  if (m_stream.fail())
  {
    set_error(textio_put_failed);
    m_stream.clear(m_stream.rdstate() & (~std::ios::failbit));
    return 0;
  }
  return 1;
}

void oiobuff::flush(void)
{
  m_stream.flush();
  if (m_stream.fail())
  {
    set_error(textio_put_failed);
    m_stream.clear(m_stream.rdstate() & (~std::ios::failbit));
  }
}

////////////////////////////////////////////////////////////////////////////////
// Stream Input

iiotext::iiotext(std::istream& str)
{
  open(str);
}

void iiotext::open(std::istream& str)
{
  itext::open(new iiobuff(str));
}

std::istream& iiotext::get_stream(void)
{
  iiobuff* buf = dynamic_cast<iiobuff*>(m_buffer);
  DEBUG_ASSERT(buf);
  return buf->m_stream;
}

const std::istream& iiotext::get_stream(void) const
{
  const iiobuff* buf = dynamic_cast<const iiobuff*>(m_buffer);
  DEBUG_ASSERT(buf);
  return buf->m_stream;
}

////////////////////////////////////////////////////////////////////////////////
// Input Buffer

iiobuff::iiobuff(std::istream& str) : m_stream(str)
{
}

iiobuff::~iiobuff(void)
{
}

int iiobuff::peek (void)
{
  int result = m_stream.peek();
  if (m_stream.fail())
  {
    set_error(textio_get_failed);
    m_stream.clear(m_stream.rdstate() & (~std::ios::failbit));
  }
  return result;
}

int iiobuff::get (void)
{
  int result = m_stream.get();
  if (m_stream.fail())
  {
    set_error(textio_get_failed);
    m_stream.clear(m_stream.rdstate() & (~std::ios::failbit));
  }
  return result;
};

////////////////////////////////////////////////////////////////////////////////
