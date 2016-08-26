
#ifndef _vector_2d_H_
#define _vector_2d_H_

#include "angle_units.h"
#include <cmath>
#include <assert.h>
#include <iostream>
#include "core.hpp"

using namespace std;

// Note:
// Other than doubles and floats, integer vectors are not currently 
// implemented correctly for angle calculation.


template <class T>
class  vector_2d
{
public:
	
	class null_length : public std::exception {
	public:
		virtual const char* what() const throw () { 
		return "null length";
		}
	};

	class divide_zero: public std::exception {
	public:
		virtual const char* what() const throw () { return "divide by zero";}
	};


    
   // default copy/assign/dtors OK.

   vector_2d ()                     {mX[0] = 0.;         mX[1] = 0.; }
   vector_2d (T x, T y)   {mX[0] = x ;         mX[1] = y; }
   vector_2d (T r, uRadian t)   {mX[0] = r * cos(t); mX[1] = r * sin(t);}

     
   // Future constructors for radians, etc

   T x () const      { return mX[0]; }
   T y () const      { return mX[1]; }
   void   x (T newX) { mX[0] = newX; }
   void   y (T newY) { mX[1] = newY; }
  T len () const { return sqrt( mX[0] * mX[0] + mX[1] * mX[1]) ; }
  T l2 () const    { return ( mX[0] * mX[0] + mX[1] * mX[1]) ; }
   /*
    * effect   Returns the vector length.
    */
  bool isNull() const { return mX[0] == 0 && mX[1] == 0; }
   bool isLenNull() const { return len() == 0; }


   T& operator [] (int d)       {assert(d >= 0 && d < 2); return mX[d];}
   T  operator [] (int d) const {assert(d >= 0 && d < 2); return mX[d];}
   T *Ref () const;

   vector_2d<T> perpendicular() const
   {
     return vector_2d<T>(-mX[1], mX[0]);
   }

   /*
    * effect   Returns the unit vector parallel to me.
    */

   vector_2d<T> unit() const;
   /*
    * effect   Returns the unit vector parallel to me.
    */

   vector_2d<T> project(const vector_2d<T>&) const;
     
  /*
   * effect   Returns the vector which is the result of projecting the argument
   * vector onto me.
   */

   T distance(const vector_2d<T>&) const;
   T distanceSquared(const vector_2d<T>&) const;

   /*
    * returns  the distance to the other vector (length of the difference)
    */

   uRadian angle () const;
   /*
    * returns  angle in the range of -PI and PI
    */

   vector_2d<T> operator+(const vector_2d<T>&) const;
   vector_2d<T>& operator+=(const vector_2d<T>&);

   vector_2d<T> operator-() const;
   vector_2d<T> operator-(const vector_2d<T>&) const;
   vector_2d<T>& operator-=(const vector_2d<T>&);
   vector_2d<T>  operator*(T d) const;
   vector_2d<T>& operator*=(T);

   T operator*(const vector_2d<T>&) const;  // dot (inner) product
   T dot(const vector_2d<T>&) const;
   T cross(const vector_2d<T>&) const; // Note: 2D cross product is a scalar

   vector_2d<T>  operator/(T d) const;
   vector_2d<T>& operator/=(T);

   bool operator==(const vector_2d<T>&) const;
   bool operator!=(const vector_2d<T>&) const;

   bool operator<  (const vector_2d<T>& rhs) const;

	friend ostream& operator<< (ostream& ous, const vector_2d<T>& dis)
  {
      ous << "|-" << dis.x() << " " << dis.y() << "-|";
    return ous;
  }
	

private:
      T mX[2];
};

typedef vector_2d<double> dVector_2d;
typedef vector_2d<float> fVector_2d;

template<class T>
inline T vector_2d<T>::distance (const vector_2d<T>& v) const
{ return (*this - v).len(); }

template<class T>
inline vector_2d<T> vector_2d<T>::operator+(const vector_2d<T>& v) const
{ return vector_2d<T> ( mX[0] + v.mX[0], mX[1] + v.mX[1]); }

template<class T>
inline vector_2d<T>& vector_2d<T>::operator+=(const vector_2d<T>& v)
{ return *this = *this + v; }

template<class T>
inline vector_2d<T> vector_2d<T>::operator- () const
{ return vector_2d<T>(-mX[0], -mX[1]); }

template<class T>
inline vector_2d<T> vector_2d<T>::operator-(const vector_2d<T>& v) const
{ return vector_2d<T>( mX[0] - v.mX[0], mX[1] - v.mX[1]); }

template<class T>
inline vector_2d<T>& vector_2d<T>::operator-=(const vector_2d<T>& v)
{ return *this = *this - v; }

template<class T>
inline vector_2d<T> vector_2d<T>::operator* (T d) const
{ return vector_2d<T>(mX[0] * d, mX[1] * d); }


template<class T>
inline vector_2d<T>& vector_2d<T>::operator*=(T d)
{ return *this = *this * d; }

template<class T>
inline T vector_2d<T>::operator*(const vector_2d<T>& v) const
{ return (mX[0] * v.mX[0] + mX[1] * v.mX[1]); }

template<class T>
inline T vector_2d<T>::dot(const vector_2d<T>& v) const
{ return *this * v; }

template<class T>
inline T vector_2d<T>::distanceSquared(const vector_2d<T>& v) const
{ return (squareOf (mX[0] - v.mX[0]) + squareOf (mX[1] - v.mX[1]));}

template<class T>
inline T vector_2d<T>::cross(const vector_2d<T>& v) const
{ return mX[0] * v.mX[1] - mX[1] * v.mX[0]; }

template<class T>
inline vector_2d<T> vector_2d<T>::operator/ (T d) const
{ 
  if (d == 0.0)
    throw null_length ();
  return *this * (1. / d);
 } // divide once

template<class T>
inline vector_2d<T>& vector_2d<T>::operator/=(T d)
{ return *this = *this / d; }

template<class T>
inline bool vector_2d<T>::operator==(const vector_2d<T>& v) const
{
    if (! std::is_floating_point<T> () )
        return ((mX[0] == v.mX[0]) && (mX[1] == v.mX[1]));
    else
        return svl::equal(mX[0] , v.mX[0]) && svl::equal(mX[0] , v.mX[0]);
}

template<class T>
inline bool vector_2d<T>::operator !=(const vector_2d<T>& v) const
{ return !(*this == v); }


template<class T>
inline uRadian vector_2d<T>::angle () const
{
  assert (! isNull() );
  return uRadian ((atan2 (double (y()), double (x()))));
}

template<class T>
inline vector_2d<T> vector_2d<T>::unit() const
{
  assert (!isLenNull());
  return *this / len();
}

template<class T>
inline T* vector_2d<T>::Ref() const
{
  return ((T*) mX);
}

template<class T>
inline vector_2d<T> vector_2d<T>::project(const vector_2d<T>& other) const
{
  return ((other * *this) / (*this * *this)) * *this;
}



#endif // _vector_2d_H_
