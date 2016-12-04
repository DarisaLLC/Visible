/*------------------------------------------------------------------------------

  Author:    Andy Rushton, from an original by Dan Milton
  Copyright: (c) Andy Rushton, 2004
  License:   BSD License, see ../docs/license.html

  ------------------------------------------------------------------------------*/

////////////////////////////////////////////////////////////////////////////////
// the foursome class

template<typename T1, typename T2, typename T3, typename T4>
foursome<T1,T2,T3,T4>::foursome(void) :
  first(), second(), third(), fourth()
{
}

template<typename T1, typename T2, typename T3, typename T4>
foursome<T1,T2,T3,T4>::foursome(const T1& p1, const T2& p2, const T3& p3, const T4& p4) :
  first(p1), second(p2), third(p3), fourth(p4)
{
}

template<typename T1, typename T2, typename T3, typename T4>
foursome<T1,T2,T3,T4>::foursome(const foursome<T1,T2,T3,T4>& t2) :
  first(t2.first), second(t2.second), third(t2.third), fourth(t2.fourth)
{
}

////////////////////////////////////////////////////////////////////////////////
// creation

template<typename T1, typename T2, typename T3, typename T4>
foursome<T1,T2,T3,T4> make_foursome(const T1& first, const T2& second, const T3& third, const T4& fourth)
{
  return foursome<T1,T2,T3,T4>(first,second,third,fourth);
}

////////////////////////////////////////////////////////////////////////////////
// comparison

template<typename T1, typename T2, typename T3, typename T4>
bool operator == (const foursome<T1,T2,T3,T4>& left, const foursome<T1,T2,T3,T4>& right)
{
  // foursomes are equal if all elements are equal
  return 
    left.first == right.first && 
    left.second == right.second && 
    left.third == right.third &&
    left.fourth == right.fourth;
}

template<typename T1, typename T2, typename T3, typename T4>
bool operator < (const foursome<T1,T2,T3,T4>& left, const foursome<T1,T2,T3,T4>& right)
{
  // sort according to first element, then second, then third, then fourth
  // as per pair, define sort order entirely in terms of operator< for elements
  if (left.first < right.first) return true;
  if (right.first < left.first) return false;
  if (left.second < right.second) return true;
  if (right.second < left.second) return false;
  if (left.third < right.third) return true;
  if (right.third < left.third) return false;
  if (left.fourth < right.fourth) return true;
  return false;
}

////////////////////////////////////////////////////////////////////////////////
// string utilities

template<typename T1, typename T2, typename T3, typename T4>
std::string foursome_to_string(const foursome<T1,T2,T3,T4>& values, const std::string& separator)
{
  return 
    to_string(values.first) + 
    separator + 
    to_string(values.second) + 
    separator + 
    to_string(values.third) +
    separator + 
    to_string(values.fourth);
}

template<typename T1, typename T2, typename T3, typename T4>
otext& print_foursome(otext& str, const foursome<T1,T2,T3,T4>& values, const std::string& separator)
{
  print(str, values.first);
  str << separator;
  print(str, values.second);
  str << separator;
  print(str, values.third);
  str << separator;
  print(str, values.fourth);
  return str;
}

template<typename T1, typename T2, typename T3, typename T4>
otext& print_foursome(otext& str, const foursome<T1,T2,T3,T4>& values, const std::string& separator, unsigned indent)
{
  print_indent(str, indent);
  print_foursome(str, values, separator);
  return str << endl;
}

////////////////////////////////////////////////////////////////////////////////
// data persistence

template<typename T1, typename T2, typename T3, typename T4>
void dump_foursome(dump_context& str, const foursome<T1,T2,T3,T4>& data) throw(persistent_dump_failed)
{
  dump(str,data.first);
  dump(str,data.second);
  dump(str,data.third);
  dump(str,data.fourth);
}

template<typename T1, typename T2, typename T3, typename T4>
void restore_foursome(restore_context& str, foursome<T1,T2,T3,T4>& data) throw(persistent_restore_failed)
{
  restore(str,data.first);
  restore(str,data.second);
  restore(str,data.third);
  restore(str,data.fourth);
}

////////////////////////////////////////////////////////////////////////////////
