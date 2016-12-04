/*------------------------------------------------------------------------------

  Author:    Andy Rushton
  Copyright: (c) Andy Rushton, 2004
  License:   BSD License, see ../docs/license.html

  ------------------------------------------------------------------------------*/
#include "timer.hpp"
#include "dprintf.hpp"
#include "string_utilities.hpp"
#include <stdio.h>

timer::timer(void)
{
  reset();
}

timer::~timer(void)
{
}

void timer::reset(void)
{
  m_clock = clock();
  m_time = time(0);
}

float timer::cpu(void) const
{
  return ((float)(clock() - m_clock)) / ((float)CLOCKS_PER_SEC);
}

float timer::elapsed(void) const
{
  return ((float)(time(0) - m_time));
}

std::string timer::text(void) const
{
  return dformat("%4.2fs CPU, %s elapsed", cpu(), display_time(time(0)-m_time).c_str());
}

otext& operator << (otext& str, const timer& t)
{
  return str << t.text();
}
