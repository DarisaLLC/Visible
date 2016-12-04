#ifndef FOURSOME_HPP
#define FOURSOME_HPP
/*------------------------------------------------------------------------------

  Author:    Andy Rushton, from an original by Dan Milton
  Copyright: (c) Andy Rushton, 2004
  License:   BSD License, see ../docs/license.html

  The next in the series pair->triple->foursome

  Originally called quadruple but that clashed (as did quad) with system
  libraries on some operating systems

  ------------------------------------------------------------------------------*/

#include "os_fixes.hpp"
#include "persistent.hpp"

////////////////////////////////////////////////////////////////////////////////
// the foursome class

template<typename T1, typename T2, typename T3, typename T4>
struct foursome
{
  typedef T1 first_type;
  typedef T2 second_type;
  typedef T3 third_type;
  typedef T4 fourth_type;

  T1 first;
  T2 second;
  T3 third;
  T4 fourth;

  foursome(void);
  foursome(const T1& p1, const T2& p2, const T3& p3, const T4& p4);
  foursome(const foursome<T1,T2,T3,T4>& t2);
};

////////////////////////////////////////////////////////////////////////////////
// creation

template<typename T1, typename T2, typename T3, typename T4>
foursome<T1,T2,T3,T4> make_foursome(const T1& first, const T2& second, const T3& third, const T4& fourth);

////////////////////////////////////////////////////////////////////////////////
// comparison

template<typename T1, typename T2, typename T3, typename T4>
bool operator == (const foursome<T1,T2,T3,T4>& left, const foursome<T1,T2,T3,T4>& right);

template<typename T1, typename T2, typename T3, typename T4>
bool operator < (const foursome<T1,T2,T3,T4>& left, const foursome<T1,T2,T3,T4>& right);

////////////////////////////////////////////////////////////////////////////////
// string utilities

template<typename T1, typename T2, typename T3, typename T4>
std::string foursome_to_string(const foursome<T1,T2,T3,T4>& values, const std::string& separator);

template<typename T1, typename T2, typename T3, typename T4>
otext& print_foursome(otext& str, const foursome<T1,T2,T3,T4>& values, const std::string& separator);

template<typename T1, typename T2, typename T3, typename T4>
otext& print_foursome(otext& str, const foursome<T1,T2,T3,T4>& values, const std::string& separator, unsigned indent);

////////////////////////////////////////////////////////////////////////////////
// data persistence

template<typename T1, typename T2, typename T3, typename T4>
void dump_foursome(dump_context& str, const foursome<T1,T2,T3,T4>& data) throw(persistent_dump_failed);

template<typename T1, typename T2, typename T3, typename T4>
void restore_foursome(restore_context& str, foursome<T1,T2,T3,T4>& data) throw(persistent_restore_failed);

////////////////////////////////////////////////////////////////////////////////
#include "foursome.tpp"
#endif
