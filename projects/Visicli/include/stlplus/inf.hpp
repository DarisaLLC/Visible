#ifndef INF_HPP
#define INF_HPP
/*------------------------------------------------------------------------------

  Author:    Andy Rushton
  Copyright: (c) Andy Rushton, 2004
  License:   BSD License, see ../docs/license.html

  An infinite-precision integer class

  ------------------------------------------------------------------------------*/
#include "os_fixes.hpp"
#include <string>
#include <stdexcept>
#include "textio.hpp"
#include "string_utilities.hpp"
#include "persistent.hpp"

////////////////////////////////////////////////////////////////////////////////

class inf
{
public:
  // this class can throw the following exceptions:
  //   std::out_of_range
  //   std::overflow_error
  //   std::invalid_argument
  //   inf::divide_by_zero    // why doesn't std have this?
  // all of these are derivations of the baseclass:
  //   std::logic_error
  // So you can catch all of them by catching the baseclass
  class divide_by_zero : public std::logic_error {
  public:
    divide_by_zero (const std::string& what_arg): std::logic_error (what_arg) { }
  };

  //////////////////////////////////////////////////////////////////////////////
  // constructors and assignments initialise the inf
  // the void constructor initialises to zero, the others initialise to the value of the C integer type
  // see also the section on string representations

  inf(void);
  explicit inf(short);
  explicit inf(unsigned short);
  explicit inf(int);
  explicit inf(unsigned);
  explicit inf(long);
  explicit inf(unsigned long);
  explicit inf(const std::string&) throw(std::invalid_argument);
  inf(const inf&);

  ~inf(void);

  inf& operator = (short);
  inf& operator = (unsigned short);
  inf& operator = (int);
  inf& operator = (unsigned);
  inf& operator = (long);
  inf& operator = (unsigned long);
  inf& operator = (const std::string&) throw(std::invalid_argument);
  inf& operator = (const inf&);

  //////////////////////////////////////////////////////////////////////////////
  // conversions back to the C types - note that the inf must be in range for this to be legal

  short to_short(void) const throw(std::overflow_error);
  unsigned short to_ushort(void) const throw(std::overflow_error);
  int to_int(void) const throw(std::overflow_error);
  unsigned to_unsigned(void) const throw(std::overflow_error);
  unsigned to_uint(void) const throw(std::overflow_error);
  long to_long(void) const throw(std::overflow_error);
  unsigned long to_ulong(void) const throw(std::overflow_error);

  //////////////////////////////////////////////////////////////////////////////
  // bitwise manipulation

  void resize(unsigned bits);
  void reduce(void);

  // the number of significant bits in the value
  unsigned bits (void) const;
  unsigned size (void) const;

  // the number of bits that can be accessed by the bit() method (=bits() rounded up to the next byte)
  unsigned indexable_bits(void) const;

  bool bit (unsigned index) const throw(std::out_of_range);
  bool operator [] (unsigned index) const throw(std::out_of_range);

  void set (unsigned index) throw(std::out_of_range);
  void clear (unsigned index) throw(std::out_of_range);
  void preset (unsigned index, bool value) throw(std::out_of_range);

  inf slice(unsigned low, unsigned high) const throw(std::out_of_range);

  //////////////////////////////////////////////////////////////////////////////
  // tests for common values or ranges

  bool negative (void) const;
  bool natural (void) const;
  bool positive (void) const;
  bool zero (void) const;
  bool non_zero (void) const;

  // tests used in if(i) and if(!i)
//  operator bool (void) const;
  bool operator ! (void) const;

  //////////////////////////////////////////////////////////////////////////////
  // comparisons

  friend bool operator == (const inf&, const inf&);
  friend bool operator != (const inf&, const inf&);
  friend bool operator < (const inf&, const inf&);
  friend bool operator <= (const inf&, const inf&);
  friend bool operator > (const inf&, const inf&);
  friend bool operator >= (const inf&, const inf&);

  //////////////////////////////////////////////////////////////////////////////
  // bitwise logic operations

  inf& invert (void);
  friend inf operator ~ (const inf&);

  inf& operator &= (const inf&);
  friend inf operator & (const inf&, const inf&);

  inf& operator |= (const inf&);
  friend inf operator | (const inf&, const inf&);

  inf& operator ^= (const inf&);
  friend inf operator ^ (const inf&, const inf&);

  inf& operator <<= (unsigned shift);
  friend inf operator << (const inf&, unsigned shift);

  inf& operator >>= (unsigned shift);
  friend inf operator >> (const inf&,unsigned shift);

  //////////////////////////////////////////////////////////////////////////////
  // arithmetic operations

  inf& negate (void);
  friend inf operator - (const inf&);

  inf& abs(void);
  friend inf abs(const inf&);

  inf& operator += (const inf&);
  friend inf operator + (const inf&, const inf&);

  inf& operator -= (const inf&);
  friend inf operator - (const inf&, const inf&);

  inf& operator *= (const inf&);
  friend inf operator * (const inf&, const inf&);

  inf& operator /= (const inf&) throw(divide_by_zero);
  friend inf operator / (const inf&, const inf&) throw(divide_by_zero);
  inf& operator %= (const inf&) throw(divide_by_zero);
  friend inf operator % (const inf&, const inf&) throw(divide_by_zero);

  // combined division operator - returns the result pair(quotient,remainder) in one go
  friend std::pair<inf,inf> divide(const inf&, const inf&) throw(divide_by_zero);

  //////////////////////////////////////////////////////////////////////////////
  // pre- and post- increment and decrement

  inf& operator ++ (void);
  inf operator ++ (int);
  inf& operator -- (void);
  inf operator -- (int);

  //////////////////////////////////////////////////////////////////////////////
  // string representation and I/O

  std::string image_debug(void) const;

  // conversion to a string representation - see string_utilities for an explanation of the arguments
  // provided as a method and a friend

  std::string to_string(unsigned radix = 10, radix_display_t display = radix_c_style_or_hash, unsigned width = 0) const
    throw(std::invalid_argument);

  friend std::string to_string(const inf& i,
                               unsigned radix = 10, radix_display_t display = radix_c_style_or_hash, unsigned width = 0)
    throw(std::invalid_argument);

  friend inf to_inf(const std::string&, unsigned radix = 0)
    throw(std::invalid_argument);

  friend otext& operator << (otext&, const inf&);
  friend itext& operator >> (itext&, inf&);

  //////////////////////////////////////////////////////////////////////////////
  // persistence

  void dump(dump_context&) const throw(persistent_dump_failed);
  void restore(restore_context&) throw(persistent_restore_failed);

  friend void dump(dump_context& str, const inf& data) throw(persistent_dump_failed);
  friend void restore(restore_context& str, inf& data) throw(persistent_restore_failed);

  //////////////////////////////////////////////////////////////////////////////
private:
  std::string m_data;

  void set_head(void);
};

// redefine friends for gcc v4.1
bool operator == (const inf&, const inf&);
bool operator != (const inf&, const inf&);
bool operator < (const inf&, const inf&);
bool operator <= (const inf&, const inf&);
bool operator > (const inf&, const inf&);
bool operator >= (const inf&, const inf&);
inf operator ~ (const inf&);
inf operator & (const inf&, const inf&);
inf operator | (const inf&, const inf&);
inf operator ^ (const inf&, const inf&);
inf operator << (const inf&, unsigned shift);
inf operator >> (const inf&,unsigned shift);
inf operator - (const inf&);
inf abs(const inf&);
inf operator + (const inf&, const inf&);
inf operator - (const inf&, const inf&);
inf operator * (const inf&, const inf&);
inf operator / (const inf&, const inf&) throw(inf::divide_by_zero);
inf operator % (const inf&, const inf&) throw(inf::divide_by_zero);
std::pair<inf,inf> divide(const inf&, const inf&) throw(inf::divide_by_zero);
std::string to_string(const inf& i, unsigned radix, radix_display_t display, unsigned width) throw(std::invalid_argument);
inf to_inf(const std::string&, unsigned radix) throw(std::invalid_argument);
otext& operator << (otext&, const inf&);
itext& operator >> (itext&, inf&);
void dump(dump_context& str, const inf& data) throw(persistent_dump_failed);
void restore(restore_context& str, inf& data) throw(persistent_restore_failed);

////////////////////////////////////////////////////////////////////////////////
#endif
