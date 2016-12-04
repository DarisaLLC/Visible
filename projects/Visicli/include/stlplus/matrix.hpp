#ifndef MATRIX_HPP
#define MATRIX_HPP
/*----------------------------------------------------------------------------

  Author:    Andy Rushton
  Copyright: (c) Andy Rushton, 2004
  License:   BSD License, see ../docs/license.html

  General-purpose 2D matrix data structure 

------------------------------------------------------------------------------*/
#include "os_fixes.hpp"
#include "textio.hpp"
#include "persistent.hpp"

////////////////////////////////////////////////////////////////////////////////

template<typename T> class matrix
{
public:
  matrix(unsigned rows = 0, unsigned cols = 0, const T& fill = T());
  ~matrix(void);

  matrix(const matrix&);
  matrix& operator =(const matrix&);

  void resize(unsigned rows, unsigned cols, const T& fill = T());

  unsigned rows(void) const;
  unsigned columns(void) const;

  void erase(const T& fill = T());
  void erase(unsigned row, unsigned col, const T& fill = T());
  void insert(unsigned row, unsigned col, const T&);
  const T& item(unsigned row, unsigned col) const;
  T& item(unsigned row, unsigned col);
  const T& operator()(unsigned row, unsigned col) const;
  T& operator()(unsigned row, unsigned col);

  void fill(const T& item = T());
  void fill_column(unsigned col, const T& item = T());
  void fill_row(unsigned row, const T& item = T());
  void fill_leading_diagonal(const T& item = T());
  void fill_trailing_diagonal(const T& item = T());
  void make_identity(const T& one, const T& zero = T());

  void transpose(void);

  // persistence routines
  
  void dump(dump_context&) const throw(persistent_dump_failed);
  void restore(restore_context&) throw(persistent_restore_failed);
  
private:
  unsigned m_rows;
  unsigned m_cols;
  T** m_data;
};

////////////////////////////////////////////////////////////////////////////////

template<typename T> otext& print(otext& str, const matrix<T>& mat, unsigned indent = 0);
template<typename T> otext& operator << (otext& str, const matrix<T>& mat);

////////////////////////////////////////////////////////////////////////////////
// non-member versions of the persistence functions

template<typename T>
void dump_matrix(dump_context& str, const matrix<T>& data) throw(persistent_dump_failed);

template<typename T>
void restore_matrix(restore_context& str, matrix<T>& data) throw(persistent_restore_failed);

////////////////////////////////////////////////////////////////////////////////

#include "matrix.tpp"
#endif
