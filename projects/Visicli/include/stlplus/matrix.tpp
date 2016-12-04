/*------------------------------------------------------------------------------

  Author:    Andy Rushton
  Copyright: (c) Andy Rushton, 2004
  License:   BSD License, see ../docs/license.html

  ------------------------------------------------------------------------------*/
#include "string_utilities.hpp"

template<typename T>
matrix<T>::matrix(unsigned rows, unsigned cols, const T& fill)
{
  m_rows = 0;
  m_cols = 0;
  m_data = 0;
  resize(rows,cols,fill);
}

template<typename T>
matrix<T>::~matrix(void)
{
  resize(0,0);
}

template<typename T>
matrix<T>::matrix(const matrix<T>& r)
{
  m_rows = 0;
  m_cols = 0;
  m_data = 0;
  *this = r;
}

template<typename T>
matrix<T>& matrix<T>::operator =(const matrix<T>& right)
{
  // clear the old values
  resize(0,0);
  // now reconstruct with the new
  resize(right.m_rows, right.m_cols);
  for (unsigned row = 0; row < m_rows; row++)
    for (unsigned col = 0; col < m_cols; col++)
      m_data[row][col] = right.m_data[row][col];
  return *this;
}

template<typename T>
void matrix<T>::resize(unsigned rows, unsigned cols, const T& fill)
{
  // a grid is an array of rows, where each row is an array of T
  // a zero-row or zero-column matrix has a null grid
  T** new_grid = 0;
  if (rows && cols)
  {
    new_grid = new T*[rows];
    for (unsigned row = 0; row < rows; row++)
    {
      new_grid[row] = new T[cols];
      // copy old items to the new grid but only within the bounds of the intersection of the old and new grids
      // fill the rest of the grid with the initial value
      for (unsigned col = 0; col < cols; col++)
        if (row < m_rows && col < m_cols)
          new_grid[row][col] = m_data[row][col];
        else
          new_grid[row][col] = fill;
    }
  }
  // destroy the old grid
  for (unsigned row = 0; row < m_rows; row++)
    delete[] m_data[row];
  delete[] m_data;
  // move the new data into the matrix
  m_data = new_grid;
  m_rows = rows;
  m_cols = cols;
}

template<typename T>
unsigned matrix<T>::rows(void) const
{
  return m_rows;
}

template<typename T>
unsigned matrix<T>::columns(void) const
{
  return m_cols;
}

template<typename T>
void matrix<T>::erase(const T& fill)
{
  for (unsigned row = 0; row < m_rows; row++)
    for (unsigned col = 0; col < m_cols; col++)
      insert(row,col,fill);
}

template<typename T>
void matrix<T>::erase(unsigned row, unsigned col, const T& fill)
{
  insert(row,col,fill);
}

template<typename T>
void matrix<T>::insert(unsigned row, unsigned col, const T& element)
{
  DEBUG_ASSERT(row < m_rows); DEBUG_ASSERT(col < m_cols);
  m_data[row][col] = element;
}

template<typename T>
const T& matrix<T>::item(unsigned row, unsigned col) const
{
  DEBUG_ASSERT(row < m_rows); DEBUG_ASSERT(col < m_cols);
  return m_data[row][col];
}

template<typename T>
T& matrix<T>::item(unsigned row, unsigned col)
{
  DEBUG_ASSERT(row < m_rows); DEBUG_ASSERT(col < m_cols);
  return m_data[row][col];
}

template<typename T>
const T& matrix<T>::operator()(unsigned row, unsigned col) const
{
  DEBUG_ASSERT(row < m_rows); DEBUG_ASSERT(col < m_cols);
  return m_data[row][col];
}

template<typename T>
T& matrix<T>::operator()(unsigned row, unsigned col)
{
  DEBUG_ASSERT(row < m_rows); DEBUG_ASSERT(col < m_cols);
  return m_data[row][col];
}

template<typename T>
 void matrix<T>::fill(const T& item)
{
  erase(item);
}

template<typename T>
 void matrix<T>::fill_column(unsigned col, const T& item)
{
  for (unsigned row = 0; row < m_rows; row++)
    insert(row, col, item);
}

template<typename T>
 void matrix<T>::fill_row(unsigned row, const T& item)
{
  for (unsigned col = 0; col < m_cols; col++)
    insert(row, col, item);
}

template<typename T>
void matrix<T>::fill_leading_diagonal(const T& item)
{
  for (unsigned i = 0; i < m_cols && i < m_rows; i++)
    insert(i, i, item);
}

template<typename T>
void matrix<T>::fill_trailing_diagonal(const T& item)
{
  for (unsigned i = 0; i < m_cols && i < m_rows; i++)
    insert(i, m_cols-i-1, item);
}

template<typename T>
void matrix<T>::make_identity(const T& one, const T& zero)
{
  fill(zero);
  fill_leading_diagonal(one);
}

template<typename T>
void matrix<T>::transpose(void)
{
  // no gain in manipulating this, since building a new matrix is no less efficient
  matrix transposed(columns(), rows());
  for (unsigned row = 0; row < rows(); row++)
    for (unsigned col = 0; col < columns(); col++)
      transposed.insert(col,row,item(row,col));
  *this = transposed;
}

template<typename T>
otext& print(otext& str, const matrix<T>& mat, unsigned indent)
{
  // use a dump printout because we don't know the contents of each element
  for (unsigned row = 0; row < mat.rows(); row++)
  {
    for (unsigned col = 0; col < mat.columns(); col++)
    {
      print_indent(str, indent);
      str << "[" << row << "][" << col << "] = ";
      print(str,mat.item(row,col));
      str << endl;
    }
  }
  return str;
}

template<typename T>
otext& operator << (otext& str, const matrix<T>& mat)
{
  return print(str,mat);
}

////////////////////////////////////////////////////////////////////////////////
// persistence

template<typename T>
void matrix<T>::dump(dump_context& str) const throw(persistent_dump_failed)
{
  ::dump(str, m_rows);
  ::dump(str, m_cols);
  for (unsigned r = 0; r < m_rows; r++)
    for (unsigned c = 0; c < m_cols; c++)
      ::dump(str,m_data[r][c]);
}

template<typename T>
void matrix<T>::restore(restore_context& str) throw(persistent_restore_failed)
{
  unsigned rows = 0, cols = 0;
  ::restore(str, rows);
  ::restore(str, cols);
  resize(rows,cols);
  for (unsigned r = 0; r < rows; r++)
    for (unsigned c = 0; c < cols; c++)
      ::restore(str, m_data[r][c]);
}

template<typename T>
void dump_matrix(dump_context& str, const matrix<T>& data) throw(persistent_dump_failed)
{
  data.dump(str);
}

template<typename T>
void restore_matrix(restore_context& str, matrix<T>& data) throw(persistent_restore_failed)
{
  data.restore(str);
}
