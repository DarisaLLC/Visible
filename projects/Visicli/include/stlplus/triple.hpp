#ifndef TRIPLE_HPP
#define TRIPLE_HPP
/*------------------------------------------------------------------------------

  Author:    Andy Rushton, from an original by Dan Milton
  Copyright: (c) Andy Rushton, 2004
  License:   BSD License, see ../docs/license.html

  Similar to the STL pair but, guess what? It has three elements!

  ------------------------------------------------------------------------------*/

#include "os_fixes.hpp"
#include "persistent.hpp"

////////////////////////////////////////////////////////////////////////////////
// the triple class

template<typename T1, typename T2, typename T3>
struct triple
{
  typedef T1 first_type;
  typedef T2 second_type;
  typedef T3 third_type;

  T1 first;
  T2 second;
  T3 third;

  triple(void);
  triple(const T1& p1, const T2& p2, const T3& p3);
  triple(const triple<T1,T2,T3>& t2);
};

////////////////////////////////////////////////////////////////////////////////
// creation

template<typename T1, typename T2, typename T3>
triple<T1,T2,T3> make_triple(const T1& first, const T2& second, const T3& third);

////////////////////////////////////////////////////////////////////////////////
// comparison

template<typename T1, typename T2, typename T3>
bool operator == (const triple<T1,T2,T3>& left, const triple<T1,T2,T3>& right);

template<typename T1, typename T2, typename T3>
bool operator < (const triple<T1,T2,T3>& left, const triple<T1,T2,T3>& right);

////////////////////////////////////////////////////////////////////////////////
// string utilities

template<typename T1, typename T2, typename T3>
std::string triple_to_string(const triple<T1,T2,T3>& values, const std::string& separator);

template<typename T1, typename T2, typename T3>
otext& print_triple(otext& str, const triple<T1,T2,T3>& values, const std::string& separator);

template<typename T1, typename T2, typename T3>
otext& print_triple(otext& str, const triple<T1,T2,T3>& values, const std::string& separator, unsigned indent);

////////////////////////////////////////////////////////////////////////////////
// data persistence

template<typename T1, typename T2, typename T3>
void dump_triple(dump_context& str, const triple<T1,T2,T3>& data) throw(persistent_dump_failed);

template<typename T1, typename T2, typename T3>
void restore_triple(restore_context& str, triple<T1,T2,T3>& data) throw(persistent_restore_failed);

////////////////////////////////////////////////////////////////////////////////
#include "triple.tpp"
#endif
