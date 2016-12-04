#ifndef STRING_VECTORIO_HPP
#define STRING_VECTORIO_HPP
/*------------------------------------------------------------------------------

  Author:    Andy Rushton
  Copyright: (c) Andy Rushton, 2004
  License:   BSD License, see ../docs/license.html

  Classes for redirecting I/O to/from a vector of strings

  ------------------------------------------------------------------------------*/
#include "os_fixes.hpp"
#include "textio.hpp"
#include <string>
#include <vector>

////////////////////////////////////////////////////////////////////////////////
// string-vector Output

class osvtext : public otext
{
public:
  osvtext(void);
  std::vector<std::string>& get_vector(void);
  const std::vector<std::string>& get_vector(void) const;
};

////////////////////////////////////////////////////////////////////////////////
// string-vector Input

class isvtext : public itext
{
public:
  isvtext(const std::vector<std::string>& data);
  std::vector<std::string>& get_vector(void);
  const std::vector<std::string>& get_vector(void) const;
};

////////////////////////////////////////////////////////////////////////////////
// Internal buffers

class osvbuff : public obuff
{
public:
  osvbuff(void);
  virtual unsigned put (unsigned char);

private:
  friend class osvtext;
  std::vector<std::string> m_data;
  bool m_eoln;

  // make this class uncopyable
  osvbuff(const osvbuff&);
  osvbuff& operator = (const osvbuff&);
};

class isvbuff : public ibuff
{
public:
  isvbuff(const std::vector<std::string>& data);
  virtual int peek (void);
  virtual int get (void);

private:
  friend class isvtext;
  std::vector<std::string> m_data;
  unsigned m_row, m_column;

  // make this class uncopyable
  isvbuff(const isvbuff&);
  isvbuff& operator = (const isvbuff&);
};

////////////////////////////////////////////////////////////////////////////////

#endif
