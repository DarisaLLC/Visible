/*------------------------------------------------------------------------------

  Author:    Andy Rushton
  Copyright: (c) Andy Rushton, 2004
  License:   BSD License, see ../docs/license.html

  An infinite-precision integer class. This allows calculations on large
  integers to be performed without overflow.

  This is a complete rewrite from scratch using a std::string as the internal
  representation. The integer is represented as a sequence of bytes. They are
  stored such that element 0 is the lsB, which makes sense when seen as an
  integer offset but is counter-intuitive when you think that a string is
  usually read from left to right, 0 to size-1, in which case the lsB is on
  the *left*.

  The rewrite was prompted by two things:
    - the old solution wasn't easily portable to 64-bit machines
    - the old solution was thrown together quickly and had a lot of inefficiencies

  This is an attempt to do a more efficient and portable job.

  Nevertheless, inf was never intended to be fast, it is just for programs
  which need a bit of infinite-precision integer arithmetic. For
  high-performance processing, use the Gnu Multi-Precision (GMP) library. The
  inf type is just easier to integrate and is already ported to all platforms
  and compilers that STLplus is ported to.

  ------------------------------------------------------------------------------*/
#include "inf.hpp"
#include <ctype.h>
#include "string_utilities.hpp"
#include "debug.hpp"

////////////////////////////////////////////////////////////////////////////////
// problem: I'm using std::string, which is an array of char. 
// However, char is not well-defined - it could be signed or unsigned.

// In fact, there's no requirement for a char to even be one byte - it can be
// any size of one byte or more. However, it's just impossible to make any
// progress with that naffness and the practice is that char on every
// platform/compiler I've ever come across is that char = byte.

// The algorithms here use unsigned char to represent bit-patterns so I have
// to be careful to type-cast from char to unsigned char a lot. This typedef
// makes life easier.

typedef unsigned char byte;

////////////////////////////////////////////////////////////////////////////////

void inf::set_head(void)
{
}

////////////////////////////////////////////////////////////////////////////////
// constructors and assignments

inf::inf(void)
{
  // void constructor initialises to zero - represented as a single-byte value containing zero
  m_data.append(1,std::string::value_type(0));
}

inf::inf(short r)
{
  operator=(r);
}

inf::inf(unsigned short r)
{
  operator=(r);
}

inf::inf(int r)
{
  operator=(r);
}

inf::inf(unsigned r)
{
  operator=(r);
}

inf::inf(long r)
{
  operator=(r);
}

inf::inf(unsigned long r)
{
  operator=(r);
}

inf::inf (const std::string& r) throw(std::invalid_argument)
{
  operator=(r);
}

inf::inf(const inf& r)
{
  m_data = r.m_data;
}

////////////////////////////////////////////////////////////////////////////////

inf::~inf(void)
{
}

////////////////////////////////////////////////////////////////////////////////

inf& inf::operator = (short r)
{
  m_data.erase();
  bool lsb_first = little_endian();
  byte* address = (byte*)&r;
  size_t bytes = sizeof(short);
  for (size_t i = 0; i < bytes; i++)
    m_data.append(1,address[(lsb_first ? i : (bytes - i - 1))]);
  reduce();
  return *this;
}

inf& inf::operator = (unsigned short r)
{
  m_data.erase();
  bool lsb_first = little_endian();
  byte* address = (byte*)&r;
  size_t bytes = sizeof(unsigned short);
  for (size_t i = 0; i < bytes; i++)
    m_data.append(1,address[(lsb_first ? i : (bytes - i - 1))]);
  // inf is signed - so there is a possible extra sign bit to add
  m_data.append(1,std::string::value_type(0));
  reduce();
  return *this;
}

inf& inf::operator = (int r)
{
  m_data.erase();
  bool lsb_first = little_endian();
  byte* address = (byte*)&r;
  size_t bytes = sizeof(int);
  for (size_t i = 0; i < bytes; i++)
    m_data.append(1,address[(lsb_first ? i : (bytes - i - 1))]);
  reduce();
  return *this;
}

inf& inf::operator = (unsigned r)
{
  m_data.erase();
  bool lsb_first = little_endian();
  byte* address = (byte*)&r;
  size_t bytes = sizeof(unsigned);
  for (size_t i = 0; i < bytes; i++)
    m_data.append(1,address[(lsb_first ? i : (bytes - i - 1))]);
  // inf is signed - so there is a possible extra sign bit to add
  m_data.append(1,std::string::value_type(0));
  reduce();
  return *this;
}

inf& inf::operator = (long r)
{
  m_data.erase();
  bool lsb_first = little_endian();
  byte* address = (byte*)&r;
  size_t bytes = sizeof(long);
  for (size_t i = 0; i < bytes; i++)
    m_data.append(1,address[(lsb_first ? i : (bytes - i - 1))]);
  reduce();
  return *this;
}

inf& inf::operator = (unsigned long r)
{
  m_data.erase();
  bool lsb_first = little_endian();
  byte* address = (byte*)&r;
  size_t bytes = sizeof(unsigned long);
  for (size_t i = 0; i < bytes; i++)
    m_data.append(1,address[(lsb_first ? i : (bytes - i - 1))]);
  // inf is signed - so there is a possible extra sign bit to add
  m_data.append(1,std::string::value_type(0));
  reduce();
  return *this;
}

inf& inf::operator = (const std::string& r) throw(std::invalid_argument)
{
  operator = (::to_inf(r, 10));
  return *this;
}

inf& inf::operator = (const inf& r)
{
  m_data = r.m_data;
  return *this;
}

////////////////////////////////////////////////////////////////////////////////

short inf::to_short(void) const throw(std::overflow_error)
{
  short result = 0;
  bool lsb_first = little_endian();
  byte* address = (byte*)&result;
  size_t bytes = sizeof(short);
  for (size_t i = 0; i < bytes; i++)
    address[(lsb_first ? i : (bytes - i - 1))] = 
      (i < m_data.size()) ? byte(m_data[i]) : (m_data.empty() || (byte(m_data[m_data.size()-1]) < (byte(128)))) ? byte(0) : byte(255);
  return result;
}

unsigned short inf::to_ushort(void) const throw(std::overflow_error)
{
  unsigned short result = 0;
  bool lsb_first = little_endian();
  byte* address = (byte*)&result;
  size_t bytes = sizeof(unsigned short);
  for (size_t i = 0; i < bytes; i++)
    address[(lsb_first ? i : (bytes - i - 1))] = (i < m_data.size()) ? byte(m_data[i]) : byte(0);
  return result;
}

int inf::to_int(void) const throw(std::overflow_error)
{
  int result = 0;
  bool lsb_first = little_endian();
  byte* address = (byte*)&result;
  size_t bytes = sizeof(int);
  for (size_t i = 0; i < bytes; i++)
    address[(lsb_first ? i : (bytes - i - 1))] = 
      (i < m_data.size()) ? byte(m_data[i]) : (m_data.empty() || (byte(m_data[m_data.size()-1]) < (byte(128)))) ? byte(0) : byte(255);
  return result;
}

unsigned inf::to_unsigned(void) const throw(std::overflow_error)
{
  unsigned result = 0;
  bool lsb_first = little_endian();
  byte* address = (byte*)&result;
  size_t bytes = sizeof(unsigned);
  for (size_t i = 0; i < bytes; i++)
    address[(lsb_first ? i : (bytes - i - 1))] = (i < m_data.size()) ? byte(m_data[i]) : byte(0);
  return result;
}

unsigned inf::to_uint(void) const throw(std::overflow_error)
{
  return to_unsigned();
}

long inf::to_long(void) const throw(std::overflow_error)
{
  long result = 0;
  bool lsb_first = little_endian();
  byte* address = (byte*)&result;
  size_t bytes = sizeof(long);
  for (size_t i = 0; i < bytes; i++)
    address[(lsb_first ? i : (bytes - i - 1))] = 
      (i < m_data.size()) ? byte(m_data[i]) : (m_data.empty() || (byte(m_data[m_data.size()-1]) < (byte(128)))) ? byte(0) : byte(255);
  return result;
}

unsigned long inf::to_ulong(void) const throw(std::overflow_error)
{
  unsigned long result = 0;
  bool lsb_first = little_endian();
  byte* address = (byte*)&result;
  size_t bytes = sizeof(unsigned long);
  for (size_t i = 0; i < bytes; i++)
    address[(lsb_first ? i : (bytes - i - 1))] = (i < m_data.size()) ? byte(m_data[i]) : byte(0);
  return result;
}

////////////////////////////////////////////////////////////////////////////////
// the number of significant bits in the number

unsigned inf::bits (void) const
{
  // The number of significant bits in the integer value - this is the number
  // of indexable bits less any redundant sign bits at the msb
  // This does not assume that the inf has been reduced to its minimum form
  unsigned result = indexable_bits();
  bool sign = bit(result-1);
  while (result > 1 && (sign == bit(result-2)))
    result--;
  return result;
}

unsigned inf::size(void) const
{
  return bits();
}

unsigned inf::indexable_bits (void) const
{
  return 8 * unsigned(m_data.size());
}

////////////////////////////////////////////////////////////////////////////////
// bitwise operations

bool inf::bit (unsigned index) const throw(std::out_of_range)
{
  if (index >= indexable_bits())
    throw std::out_of_range(std::string("inf::bit: index ") + to_string(index) + 
                            std::string(" out of range of inf size ") + to_string(indexable_bits()));
  // first split the offset into byte offset and bit offset
  unsigned byte_offset = index/8;
  unsigned bit_offset = index%8;
  return (byte(m_data[byte_offset]) & (byte(1) << bit_offset)) != 0;
}

bool inf::operator [] (unsigned index) const throw(std::out_of_range)
{
  return bit(index);
}

void inf::set (unsigned index)  throw(std::out_of_range)
{
  if (index >= indexable_bits())
    throw std::out_of_range(std::string("inf::set: index ") + to_string(index) + 
                            std::string(" out of range of inf size ") + to_string(indexable_bits()));
  // first split the offset into byte offset and bit offset
  unsigned byte_offset = index/8;
  unsigned bit_offset = index%8;
  m_data[byte_offset] |= (byte(1) << bit_offset);
}

void inf::clear (unsigned index)  throw(std::out_of_range)
{
  if (index >= indexable_bits())
    throw std::out_of_range(std::string("inf::clear: index ") + to_string(index) + 
                            std::string(" out of range of inf size ") + to_string(indexable_bits()));
  // first split the offset into byte offset and bit offset
  unsigned byte_offset = index/8;
  unsigned bit_offset = index%8;
  m_data[byte_offset] &= (~(byte(1) << bit_offset));
}

void inf::preset (unsigned index, bool value)  throw(std::out_of_range)
{
  if (value)
    set(index);
  else
    clear(index);
}

inf inf::slice(unsigned low, unsigned high) const throw(std::out_of_range)
{
  if (low >= indexable_bits())
    throw std::out_of_range(std::string("inf::slice: low index ") + to_string(low) + 
                            std::string(" out of range of inf size ") + to_string(indexable_bits()));
  if (high >= indexable_bits())
    throw std::out_of_range(std::string("inf::slice: high index ") + to_string(high) + 
                            std::string(" out of range of inf size ") + to_string(indexable_bits()));
  inf result;
  if (high >= low)
  {
    // create a result the right size and filled with sign bits
    std::string::size_type result_size = (high-low+1+7)/8;
    result.m_data.erase();
    byte extend = bit(high) ? byte(255) : byte (0);
    while (result.m_data.size() < result_size)
      result.m_data.append(1,extend);
    // now set the relevant bits
    for (unsigned i = low; i <= high; i++)
      result.preset(i-low, bit(i));
  }
  return result;
}

////////////////////////////////////////////////////////////////////////////////
// testing operations

bool inf::negative (void) const
{
  return bit(indexable_bits()-1);
}

bool inf::natural (void) const
{
  return !negative();
}

bool inf::positive (void) const
{
  return natural() && !zero();
}

bool inf::zero (void) const
{
  for (std::string::size_type i = 0; i < m_data.size(); i++)
  {
    if (m_data[i] != 0)
      return false;
  }
  return true;
}

bool inf::non_zero (void) const
{
  return !zero();
}

bool inf::operator ! (void) const
{
  return zero();
}

////////////////////////////////////////////////////////////////////////////////
// comparison operators

bool operator == (const inf& l, const inf& r)
{
  // Two infs are equal if they are numerically equal, even if they are different sizes (i.e. they could be non-reduced values).
  // This makes life a little more complicated than if I could assume that values were reduced.
  // In the first version I defined equality in terms of subtraction but I'd like to use a more direct approach here.
  byte l_extend = l.negative() ? byte(255) : byte (0);
  byte r_extend = r.negative() ? byte(255) : byte (0);
  std::string::size_type bytes = maximum(l.m_data.size(),r.m_data.size());
  for (std::string::size_type i = bytes; i--; )
  {
    byte l_byte = (i < l.m_data.size() ? byte(l.m_data[i]) : l_extend);
    byte r_byte = (i < r.m_data.size() ? byte(r.m_data[i]) : r_extend);
    if (l_byte != r_byte)
      return false;
  }
  return true;
}

bool operator != (const inf& l, const inf& r)
{
  return !(l == r);
}

bool operator < (const inf& l, const inf& r)
{
  // This was originally implemented in terms of subtraction. However, it can
  // be simplified since there is no need to calculate the accurate
  // difference, just the direction of the difference. I compare from msB down
  // and as soon as a byte difference is found, that defines the ordering. The
  // problem is that in 2's-complement, all negative values are greater than
  // all natural values if you just do a straight unsigned comparison. I
  // handle this by doing a preliminary test for different signs.

  // For example, a 3-bit signed type has the coding:
  // 000 = 0
  // ...
  // 011 = 3
  // 100 = -4
  // ...
  // 111 = -1
  // So, for natural values, the ordering of the integer values is the ordering of the bit patterns
  // Similarly, for negative values, the ordering of the integer values is the ordering of the bit patterns
  // However, the bit patterns for the negative values are *greater than* the natural values
  // This is a side-effect of the naffness of 2's-complement representation

  // first handle the case of comparing two values with different signs
  bool l_sign = l.negative();
  bool r_sign = r.negative();
  if (l_sign != r_sign)
  {
    // one argument must be negative and the other natural
    // the left is less if it is the negative one
    return l_sign;
  }
  // the arguments are the same sign
  // so the ordering is a simple unsigned byte-by-byte comparison
  // However, this is complicated by the possibility that the values could be different lengths
  byte l_extend = l_sign ? byte(255) : byte (0);
  byte r_extend = r_sign ? byte(255) : byte (0);
  std::string::size_type bytes = maximum(l.m_data.size(),r.m_data.size());
  for (std::string::size_type i = bytes; i--; )
  {
    byte l_byte = (i < l.m_data.size() ? byte(l.m_data[i]) : l_extend);
    byte r_byte = (i < r.m_data.size() ? byte(r.m_data[i]) : r_extend);
    if (l_byte != r_byte)
    {
      return l_byte < r_byte;
    }
  }
  // if I get here, the two are equal, so that is not less-than
  return false;
}

bool operator <= (const inf& l, const inf& r)
{
  return !(r < l);
}

bool operator > (const inf& l, const inf& r)
{
  return r < l;
}

bool operator >= (const inf& l, const inf& r)
{
  return !(l < r);
}

////////////////////////////////////////////////////////////////////////////////
// logical operators

inf& inf::invert (void)
{
  for (std::string::size_type i = 0; i < m_data.size(); i++)
    m_data[i] = ~m_data[i];
  return *this;
}

inf operator ~ (const inf& r)
{
  inf result = r;
  result.invert();
  return result;
}

inf& inf::operator &= (const inf& r)
{
  // bitwise AND is extended to the length of the largest argument
  byte l_extend = negative() ? byte(255) : byte (0);
  byte r_extend = r.negative() ? byte(255) : byte (0);
  std::string::size_type bytes = maximum(m_data.size(),r.m_data.size());
  for (std::string::size_type i = 0; i < bytes; i++)
  {
    byte l_byte = (i < m_data.size() ? byte(m_data[i]) : l_extend);
    byte r_byte = (i < r.m_data.size() ? byte(r.m_data[i]) : r_extend);
    byte result = l_byte & r_byte;
    if (i < m_data.size())
      m_data[i] = result;
    else
      m_data.append(1,result);
  }
  return *this;
}

inf operator & (const inf& l, const inf& r)
{
  inf result = l;
  result &= r;
  return result;
}

inf& inf::operator |= (const inf& r)
{
  // bitwise OR is extended to the length of the largest argument
  byte l_extend = negative() ? byte(255) : byte (0);
  byte r_extend = r.negative() ? byte(255) : byte (0);
  std::string::size_type bytes = maximum(m_data.size(),r.m_data.size());
  for (std::string::size_type i = 0; i < bytes; i++)
  {
    byte l_byte = (i < m_data.size() ? byte(m_data[i]) : l_extend);
    byte r_byte = (i < r.m_data.size() ? byte(r.m_data[i]) : r_extend);
    byte result = l_byte | r_byte;
    if (i < m_data.size())
      m_data[i] = result;
    else
      m_data.append(1,result);
  }
  return *this;
}

inf operator | (const inf& l, const inf& r)
{
  inf result = l;
  result |= r;
  return result;
}

inf& inf::operator ^= (const inf& r)
{
  // bitwise XOR is extended to the length of the largest argument
  byte l_extend = negative() ? byte(255) : byte (0);
  byte r_extend = r.negative() ? byte(255) : byte (0);
  std::string::size_type bytes = maximum(m_data.size(),r.m_data.size());
  for (std::string::size_type i = 0; i < bytes; i++)
  {
    byte l_byte = (i < m_data.size() ? byte(m_data[i]) : l_extend);
    byte r_byte = (i < r.m_data.size() ? byte(r.m_data[i]) : r_extend);
    byte result = l_byte ^ r_byte;
    if (i < m_data.size())
      m_data[i] = result;
    else
      m_data.append(1,result);
  }
  return *this;
}

inf operator ^ (const inf& l, const inf& r)
{
  inf result = l;
  result ^= r;
  return result;
}

////////////////////////////////////////////////////////////////////////////////
// shift operators all preserve the value by increasing the word size

inf& inf::operator <<= (unsigned shift)
{
  // left shift is a shift towards the msb, with 0s being shifted in at the lsb
  // split this into a byte shift followed by a bit shift

  // first expand the value to be big enough for the result
  // only count significant bits though
  std::string::size_type new_size = (bits() + shift + 7) / 8;
  byte extend = negative() ? byte(255) : byte (0);
  while (m_data.size() < new_size)
    m_data.append(1,extend);
  // now do the byte shift
  unsigned byte_shift = shift/8;
  if (byte_shift > 0)
  {
    for (std::string::size_type b = new_size; b--; )
      m_data[b] = (b >= byte_shift) ? m_data[b-byte_shift] : byte(0);
  }
  // and finally the bit shift
  unsigned bit_shift = shift%8;
  if (bit_shift > 0)
  {
    for (std::string::size_type b = new_size; b--; )
    {
      byte current = byte(m_data[b]);
      byte previous = b > 0 ? m_data[b-1] : byte(0);
      m_data[b] = (current << bit_shift) | (previous >> (8 - bit_shift));
    }
  }
  return *this;
}

inf operator << (const inf& l, unsigned shift)
{
  inf result = l;
  result <<= shift;
  return result;
}

inf& inf::operator >>= (unsigned shift)
{
  // right shift is a shift towards the lsb, with sign bits being shifted in at the msb
  // split this into a byte shift followed by a bit shift

  // a byte of sign bits
  byte extend = negative() ? byte(255) : byte (0);
  // do the byte shift
  unsigned byte_shift = shift/8;
  if (byte_shift > 0)
  {
    for (std::string::size_type b = 0; b < m_data.size(); b++)
      m_data[b] = (b + byte_shift < m_data.size()) ? m_data[b+byte_shift] : extend;
  }
  // and finally the bit shift
  unsigned bit_shift = shift%8;
  if (bit_shift > 0)
  {
    for (std::string::size_type b = 0; b < m_data.size(); b++)
    {
      byte current = byte(m_data[b]);
      byte next = ((b+1) < m_data.size()) ? m_data[b+1] : extend;
      byte shifted = (current >> bit_shift) | (next << (8 - bit_shift));
      m_data[b] = shifted;
    }
  }
  return *this;
}

inf operator >> (const inf& l, unsigned shift)
{
  inf result = l;
  result >>= shift;
  return result;
}

////////////////////////////////////////////////////////////////////////////////
// negation operators

inf& inf::negate (void)
{
  // do 2's-complement negation
  // equivalent to negation plus one
  invert();
  operator += (inf(1));
  return *this;
}

inf operator - (const inf& r)
{
  inf result = r;
  result.negate();
  return result;
}

inf& inf::abs(void)
{
  if (negative()) negate();
  return *this;
}

inf abs(const inf& i)
{
  inf result = i;
  result.abs();
  return result;
}

////////////////////////////////////////////////////////////////////////////////
// addition operators

inf& inf::operator += (const inf& r)
{
  // do 2's-complement addition
  // Note that the addition can give a result that is larger than either argument
  byte carry = 0;
  std::string::size_type max_size = maximum(m_data.size(),r.m_data.size());
  byte l_extend = negative() ? byte(255) : byte (0);
  byte r_extend = r.negative() ? byte(255) : byte (0);
  for (std::string::size_type i = 0; i < max_size; i++)
  {
    byte l_byte = (i < m_data.size() ? byte(m_data[i]) : l_extend);
    byte r_byte = (i < r.m_data.size() ? byte(r.m_data[i]) : r_extend);
    // calculate the addition in a type that is bigger than a byte in order to catch the carry-out
    unsigned short result = ((unsigned short)(l_byte)) + ((unsigned short)(r_byte)) + carry;
    // now truncate the result to get the lsB
    if (i < m_data.size())
      m_data[i] = byte(result);
    else
      m_data.append(1,byte(result));
    // and capture the carry out by grabbing the second byte of the result
    carry = byte(result >> 8);
  }
  // if the result overflowed or underflowed, add an extra byte to catch it
  unsigned short result = ((unsigned short)(l_extend)) + ((unsigned short)(r_extend)) + carry;
  if (byte(result) != (negative() ? byte(255) : byte(0)))
    m_data.append(1,byte(result));
  return *this;
}

inf operator + (const inf& l, const inf& r)
{
  inf result = l;
  result += r;
  return result;
}

////////////////////////////////////////////////////////////////////////////////
// subtraction operators

inf& inf::operator -= (const inf& r)
{
  // subtraction is defined in terms of negation and addition
  operator += (-r);
  return *this;
}

inf operator - (const inf& l, const inf& r)
{
  inf result = l;
  result -= r;
  return result;
}

////////////////////////////////////////////////////////////////////////////////
// multiplication operators

inf& inf::operator *= (const inf& r)
{
  // 2's complement multiplication
  // one day I'll do a more efficient version than this based on the underlying representation
  inf left = *this;
  inf right = r;
  // make the right value natural but preserve its sign for later
  bool right_negative = right.negative();
  right.abs();
  // implemented as a series of conditional additions
  operator = (0);
//  left.resize(right.bits() + left.bits() - 1);
  left <<= right.bits()-1;
  for (unsigned i = right.bits(); i--; )
  {
    if (right[i]) 
      operator += (left);
    left >>= 1;
  }
  if (right_negative)
    negate();
  return *this;
}

inf operator * (const inf& l, const inf& r)
{
  inf result = l;
  result *= r;
  return result;
}

////////////////////////////////////////////////////////////////////////////////
// division and remainder operators

inf& inf::operator /= (const inf& r) throw(inf::divide_by_zero)
{
  std::pair<inf,inf> result = divide(*this,r);
  operator=(result.first);
  return *this;
}

inf operator / (const inf& l, const inf& r) throw(inf::divide_by_zero)
{
  inf result = l;
  result /= r;
  return result;
}

inf& inf::operator %= (const inf& r) throw(inf::divide_by_zero)
{
  std::pair<inf,inf> result = divide(*this,r);
  operator=(result.second);
  return *this;
}

inf operator % (const inf& l, const inf& r) throw(inf::divide_by_zero)
{
  inf result = l;
  result %= r;
  return result;
}

std::pair<inf,inf> divide(const inf& left, const inf& right) throw(inf::divide_by_zero)
{
  if(right.zero()) throw inf::divide_by_zero("divide by zero in divide: " + ::to_string(left) + "/" + ::to_string(right));
  inf numerator = left;
  inf denominator = right;
  // make the numerator natural but preserve the sign for later
  bool numerator_negative = numerator.negative();
  numerator.abs();
  // same with the denominator
  bool denominator_negative = denominator.negative();
  denominator.abs();
  // the quotient and remainder will form the result
  // start with the quotiont zero and the remainder equal to the whole of the numerator, then do trial subtraction from this
  inf quotient;
  inf remainder = numerator;
  // there's nothing more to do if the numerator is smaller than the denominator
  // but otherwise do the division
  if (numerator.bits() >= denominator.bits())
  {
    // make the quotient big enough to take the result
    quotient.resize(numerator.bits());
    // start with the numerator shifted to the far left
    unsigned shift = numerator.bits() - denominator.bits();
    denominator <<= shift;
    // do the division by repeated subtraction, 
    for (unsigned i = shift+1; i--; )
    {
      if (remainder >= denominator)
      {
        remainder -= denominator;
        quotient.set(i);
      }
      denominator >>= 1;
    }
  }
  // now adjust the signs 
  // x/(-y) == (-x)/y == -(x/y)
  if (numerator_negative != denominator_negative)
  {
    quotient.negate();
  }
  // x%(-y) == x%y and (-x)%y == -(x%y)
  if (numerator_negative)
  {
    remainder.negate();
  }
  return std::pair<inf,inf>(quotient,remainder);
}

////////////////////////////////////////////////////////////////////////////////
// prefix (void) and postfix (int) operators

inf& inf::operator ++ (void)
{
  operator += (inf(1));
  return *this;
}

inf inf::operator ++ (int)
{
  inf old = *this;
  operator += (inf(1));
  return old;
}

inf& inf::operator -- (void)
{
  operator -= (inf(1));
  return *this;
}

inf inf::operator -- (int)
{
  inf old = *this;
  operator -= (inf(1));
  return old;
}

////////////////////////////////////////////////////////////////////////////////
// string representation and I/O routines

// Conversions to string
// Note: this is a copy and edit from string_utilities - if that changes then this must too
// TODO: edit to make it common code?

static char to_char [] = "0123456789abcdefghijklmnopqrstuvwxyz";
static int from_char [] = 
{
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, -1, -1, -1, -1, -1, -1,
  -1, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24,
  25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, -1, -1, -1, -1, -1,
  -1, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24,
  25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
};

std::string inf::to_string(unsigned radix, radix_display_t display, unsigned width) const
  throw(std::invalid_argument)
{
  return ::to_string(*this, radix, display, width);
}

std::string to_string(const inf& i,
                      unsigned radix, radix_display_t display, unsigned width)
  throw(std::invalid_argument)
{
  if (radix < 2 || radix > 36)
    throw std::invalid_argument("invalid radix value " + to_string(radix));
  inf local_i = i;
  // untangle all the options
  bool hashed = false;
  bool binary = false;
  bool octal = false;
  bool hex = false;
  switch(display)
  {
  case radix_none:
    break;
  case radix_hash_style:
    hashed = radix != 10;
    break;
  case radix_hash_style_all:
    hashed = true;
    break;
  case radix_c_style:
    if (radix == 16)
      hex = true;
    else if (radix == 8)
      octal = true;
    else if (radix == 2)
      binary = true;
    break;
  case radix_c_style_or_hash:
    if (radix == 16)
      hex = true;
    else if (radix == 8)
      octal = true;
    else if (radix == 2)
      binary = true;
    else if (radix != 10)
      hashed = true;
    break;
  default:
    throw std::invalid_argument("invalid radix display value");
  }
  // create constants of the same type as the template parameter to avoid type mismatches
  const inf t_zero(0);
  const inf t_radix(radix);
  // the C representations for binary, octal and hex use 2's-complement representation
  // all other represenations use sign-magnitude
  std::string result;
  if (hex || octal || binary)
  {
    // bit-pattern representation
    // this is the binary representation optionally shown in octal or hex
    // first generate the binary by masking the bits
    for (unsigned j = i.bits(); j--; )
      result += (i.bit(j) ? '1' : '0');
    // the result is now the full width of the type - e.g. int will give a 32-bit result
    // now interpret this as either binary, octal or hex and add the prefix
    if (binary)
    {
      // the result is already binary - but the width may be wrong
      // if this is still smaller than the width field, sign extend
      // otherwise trim down to either the width or the smallest string that preserves the value
      while (result.size() < width)
        result.insert((std::string::size_type)0, 1, result[0]);
      while (result.size() > width)
      {
        // do not trim to less than 1 bit (sign only)
        if (result.size() <= 1) break;
        // only trim if it doesn't change the sign and therefore the value
        if (result[0] != result[1]) break;
        result.erase(0,1);
      }
      // add the prefix
      result.insert((std::string::size_type)0, "0b");
    }
    else if (octal)
    {
      // the result is currently binary - but before converting get the width right
      // the width is expressed in octal digits so make the binary 3 times this
      // if this is still smaller than the width field, sign extend
      // otherwise trim down to either the width or the smallest string that preserves the value
      // also ensure that the binary is a multiple of 3 bits to make the conversion to octal easier
      while (result.size() < 3*width)
        result.insert((std::string::size_type)0, 1, result[0]);
      while (result.size() > 3*width)
      {
        // do not trim to less than 2 bits (sign plus 1-bit magnitude)
        if (result.size() <= 2) break;
        // only trim if it doesn't change the sign and therefore the value
        if (result[0] != result[1]) break;
        result.erase(0,1);
      }
      while (result.size() % 3 != 0)
        result.insert((std::string::size_type)0, 1, result[0]);
      // now convert to octal
      std::string octal_result;
      for (unsigned i = 0; i < result.size()/3; i++)
      {
        std::string slice = result.substr(i*3, 3);
        unsigned value = to_uint(slice, 2);
        octal_result += to_char[value];
      }
      result = octal_result;
      // add the prefix
      result.insert((std::string::size_type)0, "0");
    }
    else
    {
      // similar to octal
      while (result.size() < 4*width)
        result.insert((std::string::size_type)0, 1, result[0]);
      while (result.size() > 4*width)
      {
        // do not trim to less than 2 bits (sign plus 1-bit magnitude)
        if (result.size() <= 2) break;
        // only trim if it doesn't change the sign and therefore the value
        if (result[0] != result[1]) break;
        result.erase(0,1);
      }
      while (result.size() % 4 != 0)
        result.insert((std::string::size_type)0, 1, result[0]);
      // now convert to hex
      std::string hex_result;
      for (unsigned i = 0; i < result.size()/4; i++)
      {
        std::string slice = result.substr(i*4, 4);
        unsigned value = to_uint(slice, 2);
        hex_result += to_char[value];
      }
      result = hex_result;
      // add the prefix
      result.insert((std::string::size_type)0, "0x");
    }
  }
  else
  {
    // convert to sign-magnitude
    // the representation is:
    // [radix#][sign]magnitude
    bool negative = local_i.negative();
    local_i.abs();
    // create a representation of the magnitude by successive division
    do
    {
      std::pair<inf,inf> divided = divide(local_i, t_radix);
      unsigned remainder = divided.second.to_unsigned();
      DEBUG_ASSERT(remainder < radix);
      char digit = to_char[remainder];
      result.insert((std::string::size_type)0, 1, digit);
      local_i = divided.first;
    }
    while(!local_i.zero() || result.size() < width);
    // add the prefixes
    // add a sign only for negative values
    if (negative)
      result.insert((std::string::size_type)0, 1, '-');
    // then prefix everything with the radix if the hashed representation was requested
    if (hashed)
      result.insert((std::string::size_type)0, to_string(radix) + "#");
  }
  return result;
}

////////////////////////////////////////////////////////////////////////////////
// Conversions FROM string

inf to_inf(const std::string& str, unsigned radix)
  throw(std::invalid_argument)
{
  if (radix != 0 && (radix < 2 || radix > 36))
    throw std::invalid_argument("invalid radix value " + to_string(radix));
  unsigned i = 0;
  // the radix passed as a parameter is just the default - it can be overridden by either the C prefix or the hash prefix
  // Note: a leading zero is the C-style prefix for octal - I only make this override the default when the default prefix is not specified
  // first chk for a C-style prefix
  bool c_style = false;
  if (i < str.size() && str[i] == '0')
  {
    // binary or hex
    if (i+1 < str.size() && tolower(str[i+1]) == 'x')
    {
      radix = 16;
      i += 2;
      c_style = true;
    }
    else if (i+1 < str.size() && tolower(str[i+1]) == 'b')
    {
      radix = 2;
      i += 2;
      c_style = true;
    }
    else if (radix == 0)
    {
      radix = 8;
      i += 1;
      c_style = true;
    }
  }
  // now chk for a hash-style prefix if a C-style prefix was not found
  if (i == 0)
  {
    // scan for the sequence {digits}#
    bool hash_found = false;
    unsigned j = i;
    for (; j < str.size(); j++)
    {
      if (!isdigit(str[j]))
      {
        if (str[j] == '#')
          hash_found = true;
        break;
      }
    }
    if (hash_found)
    {
      // use the hash prefix to define the radix
      // i points to the start of the radix and j points to the # character
      std::string slice = str.substr(i, j-i);
      radix = to_uint(slice);
      i = j+1;
    }
  }
  if (radix == 0)
    radix = 10;
  if (radix < 2 || radix > 36)
    throw std::invalid_argument("invalid radix value " + to_string(radix));
  inf val;
  if (c_style)
  {
    // the C style formats are bit patterns not integer values - these need to be sign-extended to get the right value
    std::string binary;
    DEBUG_ASSERT(radix == 2 || radix == 8 || radix == 16);
    if (radix == 2)
    {
      for (unsigned j = i; j < str.size(); j++)
      {
        switch(str[j])
        {
        case '0':
          binary += '0';
          break;
        case '1':
          binary += '1';
          break;
        default:
          throw std::invalid_argument("invalid character in string " + str + " for radix " + to_string(radix));
          break;
        }
      }
    }
    else if (radix == 8)
    {
      for (unsigned j = i; j < str.size(); j++)
      {
        switch(str[j])
        {
        case '0':
          binary += "000";
          break;
        case '1':
          binary += "001";
          break;
        case '2':
          binary += "010";
          break;
        case '3':
          binary += "011";
          break;
        case '4':
          binary += "100";
          break;
        case '5':
          binary += "101";
          break;
        case '6':
          binary += "110";
          break;
        case '7':
          binary += "111";
          break;
        default:
          throw std::invalid_argument("invalid character in string " + str + " for radix " + to_string(radix));
          break;
        }
      }
    }
    else
    {
      for (unsigned j = i; j < str.size(); j++)
      {
        switch(tolower(str[j]))
        {
        case '0':
          binary += "0000";
          break;
        case '1':
          binary += "0001";
          break;
        case '2':
          binary += "0010";
          break;
        case '3':
          binary += "0011";
          break;
        case '4':
          binary += "0100";
          break;
        case '5':
          binary += "0101";
          break;
        case '6':
          binary += "0110";
          break;
        case '7':
          binary += "0111";
          break;
        case '8':
          binary += "1000";
          break;
        case '9':
          binary += "1001";
          break;
        case 'a':
          binary += "1010";
          break;
        case 'b':
          binary += "1011";
          break;
        case 'c':
          binary += "1100";
          break;
        case 'd':
          binary += "1101";
          break;
        case 'e':
          binary += "1110";
          break;
        case 'f':
          binary += "1111";
          break;
        default:
          throw std::invalid_argument("invalid character in string " + str + " for radix " + to_string(radix));
          break;
        }
      }
    }
    // now convert the value
    val.resize(binary.size());
    for (unsigned j = 0; j < binary.size(); j++)
    {
      DEBUG_ASSERT(from_char[(unsigned char)binary[j]] != -1);
      val.preset(binary.size() - j - 1, binary[j] == '1');
    }
  }
  else
  {
    // now scan for a sign and find whether this is a negative number
    bool negative = false;
    if (i < str.size())
    {
      switch (str[i])
      {
      case '-':
        negative = true;
        i++;
        break;
      case '+':
        i++;
        break;
      }
    }
    for (; i < str.size(); i++)
    {
      val *= inf(radix);
      int ch = from_char[(unsigned char)str[i]] ;
      if (ch == -1)
      {
        throw std::invalid_argument("invalid character in string " + str + " for radix " + to_string(radix));
      }
      val += inf(ch);
    }
    if (negative)
      val.negate();
  }
  return val;
}

otext& operator << (otext& str, const inf& i)
{
  try
  {
    str << to_string(i, str.integer_radix(), str.integer_display(), str.integer_width());
  }
  catch(const std::invalid_argument)
  {
    str.set_error(textio_format_error);
  }
  return str;
}

itext& operator >> (itext& str, inf& i)
{
  try
  {
    std::string image;
    str >> image;
    i = to_inf(image);
  }
  catch(const std::invalid_argument)
  {
    str.set_error(textio_format_error);
  }
  return str;
}

////////////////////////////////////////////////////////////////////////////////
// diagnostic dump
// just convert to hex

std::string inf::image_debug(void) const
{
  // create this dump in the human-readable form, i.e. msB to the left
  std::string result = "0x";
  for (std::string::size_type i = m_data.size(); i--; )
  {
    byte current = m_data[i];
    byte msB = (current & byte(0xf0)) >> 8;
    DEBUG_ASSERT(msB < 16);
    result += to_char[msB];
    byte lsB = (current & byte(0x0f));
    DEBUG_ASSERT(lsB < 16);
    result += to_char[lsB];
  }
  return result;
}

////////////////////////////////////////////////////////////////////////////////

// resize the inf regardless of the data

void inf::resize(unsigned bits)
{
  if (bits == 0) bits = 1;
  unsigned bytes = (bits+7)/8;
  byte extend = negative() ? byte(255) : byte (0);
  while(bytes > m_data.size())
    m_data.append(1,extend);
}

// reduce the bit count to the minimum needed to preserve the value

void inf::reduce(void)
{
  // remove leading bytes providing that it doesn't change the value
  // however, don't reduce to zero bytes
  while(m_data.size() > 1 && 
        ((byte(m_data[m_data.size()-1]) == byte(0) && byte(m_data[m_data.size()-2]) < byte(128)) ||
         (byte(m_data[m_data.size()-1]) == byte(255) && byte(m_data[m_data.size()-2]) >= byte(128))))
  {
    m_data.erase(m_data.end()-1);
  }
}

////////////////////////////////////////////////////////////////////////////////
// persistence

void inf::dump(dump_context& str) const throw(persistent_dump_failed)
{
  // don't support dumping of old versions
  if (str.version() < 2)
    throw persistent_dump_failed(std::string("inf::dump: wrong version: ") + to_string(str.version()));
  // just dump the string
  ::dump(str,m_data);
}

void inf::restore(restore_context& str) throw(persistent_restore_failed)
{
  if (str.version() < 1)
    throw persistent_restore_failed(std::string("inf::restore: wrong version: ") + to_string(str.version()));

  if (str.version() == 1)
  {
    // old-style restore relies on the word size being the same - 32-bits - on all platforms
    // this can be restored on such machines but is not portable to 64-bit machines
    unsigned bits = 0;
    ::restore(str,bits);
    unsigned words = (bits+7)/32;
    // inf was dumped msB first
    for (unsigned i = words; i--; )
    {
      // restore a word
      unsigned data = 0;
      ::restore(str,data);
      // now extract the bytes
      unsigned char* word = (unsigned char*)(&data);
      for (unsigned b = 4; b--; )
        m_data.insert(m_data.begin(),byte(word[str.little_endian() ? b : 3 - b]));
    }
  }
  else
  {
    // new-style dump just uses the string persistence
    ::restore(str,m_data);
  }
}


void dump(dump_context& str, const inf& data) throw(persistent_dump_failed)
{
  data.dump(str);
}


void restore(restore_context& str, inf& data) throw(persistent_restore_failed)
{
  data.restore(str);
}

////////////////////////////////////////////////////////////////////////////////
