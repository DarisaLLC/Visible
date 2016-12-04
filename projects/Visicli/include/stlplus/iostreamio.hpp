#ifndef IOSTREAMIO_HPP
#define IOSTREAMIO_HPP
/*------------------------------------------------------------------------------

  Author:    Andy Rushton
  Copyright: (c) Andy Rushton, 2004
  License:   BSD License, see ../docs/license.html

  TextIO devices layered on IOStream so that the two can be mixed in one application

  ------------------------------------------------------------------------------*/
#include "os_fixes.hpp"
#include "textio.hpp"
#include <iostream>

////////////////////////////////////////////////////////////////////////////////
// Stream Output
// oiotext = (o)utput (io)stream (text)io device

class oiotext : public otext
{
public:
  oiotext(std::ostream&);
  void open(std::ostream&);

  std::ostream& get_stream(void);
  const std::ostream& get_stream(void) const;
};

////////////////////////////////////////////////////////////////////////////////
// Stream Input
// iiotext = (i)nput (io)stream (text)io device

class iiotext : public itext
{
public:
  iiotext(std::istream&);
  void open(std::istream&);

  std::istream& get_stream(void);
  const std::istream& get_stream(void) const;
};

////////////////////////////////////////////////////////////////////////////////
// Internals

class oiobuff : public obuff
{
  friend class oiotext;
protected:
  std::ostream& m_stream;
public:
  oiobuff(std::ostream&);
  ~oiobuff(void);
protected:
  virtual unsigned put(unsigned char);
  virtual void flush(void);
private:
  // make this class uncopyable
  oiobuff(const oiobuff&);
  oiobuff& operator = (const oiobuff&);
};

class iiobuff : public ibuff
{
  friend class iiotext;
protected:
  std::istream& m_stream;
public:
  iiobuff(std::istream&);
  ~iiobuff(void);
protected:
  virtual int peek (void);
  virtual int get (void);
private:
  // make this class uncopyable
  iiobuff(const iiobuff&);
  iiobuff& operator = (const iiobuff&);
};

////////////////////////////////////////////////////////////////////////////////
#endif
