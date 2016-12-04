#ifndef STRING_ARITHMETIC_HPP
#define STRING_ARITHMETIC_HPP
/*------------------------------------------------------------------------------

  Author:    Andy Rushton
  Copyright: (c) Andy Rushton, 2004
  License:   BSD License, see ../docs/license.html

  Package of functions for performing bitwise logic and arithmetic on strings of 0, 1 and X values

  ------------------------------------------------------------------------------*/
#include "os_fixes.hpp"
#include "inf.hpp"
#include <string>

////////////////////////////////////////////////////////////////////////////////
// Logical operations
// I've had to name the bitwise_xxx and not logical_xxx to avoid name conflicts with STL
////////////////////////////////////////////////////////////////////////////////

// size changer

std::string bitwise_resize(const std::string& argument, unsigned size = 0, char pad = '0');

// comparisons

bool bitwise_equality(const std::string& left, const std::string& right);
bool bitwise_inequality(const std::string& left, const std::string& right);

// logical operations

std::string bitwise_not(const std::string& argument);
std::string bitwise_and(const std::string& left, const std::string& right);
std::string bitwise_nand(const std::string& left, const std::string& right);
std::string bitwise_or(const std::string& left, const std::string& right);
std::string bitwise_nor(const std::string& left, const std::string& right);
std::string bitwise_xor(const std::string& left, const std::string& right);
std::string bitwise_xnor(const std::string& left, const std::string& right);

// shift operations

std::string bitwise_shift_left(const std::string& argument, unsigned shift);
std::string bitwise_shift_right(const std::string& argument, unsigned shift);
std::string bitwise_rotate_left(const std::string& argument, unsigned shift);
std::string bitwise_rotate_right(const std::string& argument, unsigned shift);

////////////////////////////////////////////////////////////////////////////////
// Unsigned arithmetic
////////////////////////////////////////////////////////////////////////////////

// size changers

std::string unsigned_resize(const std::string& argument, unsigned size = 0);

// comparisons

bool unsigned_equality(const std::string& left, const std::string& right);
bool unsigned_inequality(const std::string& left, const std::string& right);
bool unsigned_less_than(const std::string& left, const std::string& right);
bool unsigned_less_than_or_equal(const std::string& left, const std::string& right);
bool unsigned_greater_than(const std::string& left, const std::string& right);
bool unsigned_greater_than_or_equal(const std::string& left, const std::string& right);

// arithmetic operations

std::string unsigned_add(const std::string& left, const std::string& right, unsigned size = 0);
std::string unsigned_subtract(const std::string& left, const std::string& right, unsigned size = 0);

std::string unsigned_multiply(const std::string& left, const std::string& right, unsigned size = 0);
std::string unsigned_exponent(const std::string& left, const std::string& right, unsigned size = 0);
std::string unsigned_divide(const std::string& left, const std::string& right, unsigned size = 0);
std::string unsigned_modulus(const std::string& left, const std::string& right, unsigned size = 0);
std::string unsigned_remainder(const std::string& left, const std::string& right, unsigned size = 0);

// logical operations

std::string unsigned_not(const std::string& argument, unsigned size = 0);
std::string unsigned_and(const std::string& left, const std::string& right, unsigned size = 0);
std::string unsigned_nand(const std::string& left, const std::string& right, unsigned size = 0);
std::string unsigned_or(const std::string& left, const std::string& right, unsigned size = 0);
std::string unsigned_nor(const std::string& left, const std::string& right, unsigned size = 0);
std::string unsigned_xor(const std::string& left, const std::string& right, unsigned size = 0);
std::string unsigned_xnor(const std::string& left, const std::string& right, unsigned size = 0);

// shift operations

std::string unsigned_shift_left(const std::string& argument, unsigned shift, unsigned size = 0);
std::string unsigned_shift_right(const std::string& argument, unsigned shift, unsigned size = 0);

// integer conversions

unsigned long unsigned_to_ulong(const std::string& argument);
inf unsigned_to_inf(const std::string& argument);
std::string ulong_to_unsigned(unsigned long argument, unsigned size = 0);
std::string inf_to_unsigned(inf argument, unsigned size = 0);

////////////////////////////////////////////////////////////////////////////////
// Signed arithmetic
////////////////////////////////////////////////////////////////////////////////

// size changers

std::string signed_resize(const std::string& argument, unsigned size = 0);

// tests

bool is_negative(const std::string& argument);
bool is_natural(const std::string& argument);
bool is_positive(const std::string& argument);
bool is_zero(const std::string& argument);

// comparisons

bool signed_equality(const std::string& left, const std::string& right);
bool signed_inequality(const std::string& left, const std::string& right);
bool signed_less_than(const std::string& left, const std::string& right);
bool signed_less_than_or_equal(const std::string& left, const std::string& right);
bool signed_greater_than(const std::string& left, const std::string& right);
bool signed_greater_than_or_equal(const std::string& left, const std::string& right);

// arithmetic operations

std::string signed_negate(const std::string& argument, unsigned size = 0);
std::string signed_abs(const std::string& argument, unsigned size = 0);
std::string signed_add(const std::string& left, const std::string& right, unsigned size = 0);
std::string signed_subtract(const std::string& left, const std::string& right, unsigned size = 0);

std::string signed_multiply(const std::string& left, const std::string& right, unsigned size = 0);
std::string signed_exponent(const std::string& left, const std::string& right, unsigned size = 0);
std::string signed_divide(const std::string& left, const std::string& right, unsigned size = 0);
std::string signed_modulus(const std::string& left, const std::string& right, unsigned size = 0);
std::string signed_remainder(const std::string& left, const std::string& right, unsigned size = 0);

// logical operations

std::string signed_not(const std::string& argument, unsigned size = 0);
std::string signed_and(const std::string& left, const std::string& right, unsigned size = 0);
std::string signed_nand(const std::string& left, const std::string& right, unsigned size = 0);
std::string signed_or(const std::string& left, const std::string& right, unsigned size = 0);
std::string signed_nor(const std::string& left, const std::string& right, unsigned size = 0);
std::string signed_xor(const std::string& left, const std::string& right, unsigned size = 0);
std::string signed_xnor(const std::string& left, const std::string& right, unsigned size = 0);

// shift operations

std::string signed_shift_left(const std::string& argument, unsigned shift, unsigned size = 0);
std::string signed_shift_right(const std::string& argument, unsigned shift, unsigned size = 0);

// integer conversions

long signed_to_long(const std::string& argument);
inf signed_to_inf(const std::string& argument);
std::string long_to_signed(long argument, unsigned size = 0);
std::string inf_to_signed(inf argument, unsigned size = 0);

////////////////////////////////////////////////////////////////////////////////

#endif
