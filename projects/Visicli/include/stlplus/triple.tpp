/*------------------------------------------------------------------------------

  Author:    Andy Rushton, from an original by Dan Milton
  Copyright: (c) Andy Rushton, 2004
  License:   BSD License, see ../docs/license.html

  ------------------------------------------------------------------------------*/

////////////////////////////////////////////////////////////////////////////////
// the triple class

template<typename T1, typename T2, typename T3>
triple<T1,T2,T3>::triple(void) :
  first(), second(), third()
{
}

template<typename T1, typename T2, typename T3>
triple<T1,T2,T3>::triple(const T1& p1, const T2& p2, const T3& p3) :
  first(p1), second(p2), third(p3)
{
}

template<typename T1, typename T2, typename T3>
triple<T1,T2,T3>::triple(const triple<T1,T2,T3>& t2) :
  first(t2.first), second(t2.second), third(t2.third)
{
}

////////////////////////////////////////////////////////////////////////////////
// creation

template<typename T1, typename T2, typename T3>
triple<T1,T2,T3> make_triple(const T1& first, const T2& second, const T3& third)
{
  return triple<T1,T2,T3>(first,second,third);
}

////////////////////////////////////////////////////////////////////////////////
// comparison

template<typename T1, typename T2, typename T3>
bool operator == (const triple<T1,T2,T3>& left, const triple<T1,T2,T3>& right)
{
  // triples are equal if all elements are equal
  return left.first == right.first && left.second == right.second && left.third == right.third;
}

template<typename T1, typename T2, typename T3>
bool operator < (const triple<T1,T2,T3>& left, const triple<T1,T2,T3>& right)
{
  // sort according to first element, then second, then third
  // as per pair, define sort order entirely in terms of operator< for elements
  if (left.first < right.first) return true;
  if (right.first < left.first) return false;
  if (left.second < right.second) return true;
  if (right.second < left.second) return false;
  if (left.third < right.third) return true;
  return false;
}

////////////////////////////////////////////////////////////////////////////////
// string utilities

template<typename T1, typename T2, typename T3>
std::string triple_to_string(const triple<T1,T2,T3>& values, const std::string& separator)
{
  return to_string(values.first) + separator + to_string(values.second) + separator + to_string(values.third);
}

template<typename T1, typename T2, typename T3>
otext& print_triple(otext& str, const triple<T1,T2,T3>& values, const std::string& separator)
{
  print(str, values.first);
  str << separator;
  print(str, values.second);
  str << separator;
  print(str, values.third);
  return str;
}

template<typename T1, typename T2, typename T3>
otext& print_triple(otext& str, const triple<T1,T2,T3>& values, const std::string& separator, unsigned indent)
{
  print_indent(str, indent);
  print_triple(str, values, separator);
  return str << endl;
}

////////////////////////////////////////////////////////////////////////////////
// data persistence

template<typename T1, typename T2, typename T3>
void dump_triple(dump_context& str, const triple<T1,T2,T3>& data) throw(persistent_dump_failed)
{
  dump(str,data.first);
  dump(str,data.second);
  dump(str,data.third);
}

template<typename T1, typename T2, typename T3>
void restore_triple(restore_context& str, triple<T1,T2,T3>& data) throw(persistent_restore_failed)
{
  restore(str,data.first);
  restore(str,data.second);
  restore(str,data.third);
}

////////////////////////////////////////////////////////////////////////////////
