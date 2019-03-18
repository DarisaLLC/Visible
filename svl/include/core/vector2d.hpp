
#ifndef _vector_2d_H_
#define _vector_2d_H_

#include "core/angle_units.h"
#include <cmath>
#include <assert.h>
#include <iostream>
#include "core/core.hpp"
#include "core/svl_exception.hpp"
using namespace std;

// Note:
// Other than doubles and floats, integer vectors are not currently 
// implemented correctly for angle calculation.

namespace svl {
    

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


// A General Templated Matrix Class (pieced together from various open sources)
// Note on notation:
//   1) In accessors, matrix elements have indices starting at
// zero.
//   2) Indices are ordered row first, column second

template <int D> class matrix;  /* D is the dimension */

typedef matrix<2> matrix2;

template <>
class matrix<2>
/* Named degrees of freedom:
 *    There are various ways of specifying the 4 degrees of freedom represented
 * by a 2x2 matrix.  Two sets of four "named" degrees of freedom are
 * implemented for constructors and accessors:
 *   1) xRot, yRot, xScale, yScale     (Rx, Ry, Sx, Sy)
 *   2) scale, aspect, shear, rotation  (s, a, K, R)
 *
 * A 2x2 matrix is defined as follows in terms of these variables:
 *   |e11 e12|   |Sx(cosRx)  -Sy(sinRy)|   |s(cosR)  as(-sinR - cosR tanK)|
 *   |e21 e22| = |Sx(sinRx)   Sy(cosRy)| = |s(sinR)  as( cosR - sinR tanK)|
 *
 * The composition order is:
 *   |Sx(cosRx)  -Sy(sinRy)|   |cosRx  -sinRy| |Sx  0 |
 *   |Sx(sinRx)   Sy(cosRy)| = |sinRx   cosRy| |0   Sy|
 *
 * Or:
 *   |s(cosR)  as(-sinR - cosR tanK)|       |cosR  -sinR| |1  -tanK| |1  0|
 *   |s(sinR)  as( cosR - sinR tanK)| = s * |sinR   cosR| |0    1  | |0  a|
 *
 * The named degrees of freedom are extracted from a matrix as follows:
 *   Rx = atan2(e21, e11)           s = sqrt(e11*e11 + e21*e21)
 *   Ry = atan2(-e12, e22)          a = det / (e11*e11 + e21*e21)
 *   Sx = sqrt(e11*e11 + e21*e21)   K = -atan((e11*e12 + e21*e22) / det)
 *   Sy = sqrt(e12*e12 + e22*e22)   R = atan2(e21, e11)
 *                                  Note: det = a * (s^2)
 * Note that none of these is meaningful if the matrix is singular, even though
 * some values might be well-defined by the above formulas.
 *
 * Also note that the extraction formulas define the canonical ranges for the
 * variables (for a non-singular matrix), as follows:
 *   Rx [-PI, PI]                     s (0, +Inf)
 *   Ry [-PI, PI]                     R [-PI, PI]
 *   Sx (0, +Inf)                     a (-Inf, +Inf), a != 0
 *   Sy (0, +Inf)                     K (-PI/2, PI/2)
 *
 * Be careful if you use both the (Rx, Ry, Sx, Sy) style of
 * decomposition and the (s, a, K, R) style.  There are some
 * non-obvious interactions between the two.  For example, when shear
 * is present, aspect != yScale/xScale.
 */
{
public:
    
    // default copy ctor, assign, dtor OK
    // Constructors. Default is the identity matrix
    matrix() { me[0][0] = 1.; me[0][1] = 0.;
        me[1][0] = 0.; me[1][1] = 1.; mDt = 1.;}
    
    matrix(double e11, double e12, double e21, double e22)
    {
        me[0][0] = e11; me[0][1] = e12;
        me[1][0] = e21; me[1][1] = e22; setdt();
    }
    
    
    matrix(const uRadian& rot, const std::pair<double,double>& scale);
    
    matrix(double scale, double aspect,
           const double& shear, const uRadian& rotation);
    
    double determinant() const { return mDt;}
    
    matrix<2> inverse() const;
    /*
     * throws bfMathError::Singular
     */
    matrix<2> transpose() const;
    bool isSingular() const;
    bool isIdentity() const;
    
    inline double element(int row,int column) const;
    void   element(int row,int column, double value);
    /*
     * effect   get/set element value
     * requires row and column 0 or 1
     * note     The setter recalculates the determinant
     */
    
    // Decomposition methods
    double xRot() const;
    double yRot() const;
    double   xScale() const;
    double   yScale() const;
    /*
     * effect xRot, yRot, xScale, yScale are one way to dissect a transform.
     * throws bfMathError::Singular
     */
    double   scale() const;
    double   aspect() const;
    double shear() const;
    uRadian rotation() const;
    
    /*
     * effect scale, aspect, shear, rotation are another way.
     * throws bfMathError::Singular if matrix is singular
     */
    // Identity matrix.
    static const matrix<2> I;
    
    inline matrix<2>  operator+(const matrix<2>&) const;
    inline matrix<2>& operator+=(const matrix<2>&);
    
    inline matrix<2>  operator-() const;
    inline matrix<2>  operator-(const matrix<2>&) const;
    inline matrix<2>& operator-=(const matrix<2>&);
    
    inline matrix<2>  operator*(double) const;
    inline matrix<2>& operator*=(double);
    
    inline matrix<2>  operator*(const matrix<2>&) const;
    inline matrix<2>& operator*=(const matrix<2>&);
    
    inline matrix<2>  operator/(double) const;
    inline matrix<2>& operator/=(double);
    
    inline matrix<2>  operator/(const matrix<2>&) const;
    inline matrix<2>& operator/=(const matrix<2>&);
    /*
     * note   m1 / m2 = m1 * m2.inverse()
     * throws bfMathError::Singular if the matrix used as a divisor
     *        is singular
     */
    inline bool operator==(const matrix<2>&) const;
    inline bool operator!=(const matrix<2>&) const;
    
    // Vector operations.
    inline fVector_2d operator*(const fVector_2d&) const;
    inline friend fVector_2d operator*(const fVector_2d&, const matrix<2>&);
    
    inline dVector_2d operator*(const dVector_2d&) const;
    inline friend dVector_2d operator*(const dVector_2d&, const matrix<2>&);
 
    
    friend ostream& operator<< (ostream& ous, const matrix<2>& dis)
    {
        ous << "[" << dis.element (0,0) << " " << dis.element(0,1) << endl << dis.element(1,0) << " " << dis.element(1,1) << "]" << endl;
        return ous;
    }
    
private:
    double me[2][2];  // Matrix elements.
    double mDt;        // Determinant
    void setdt() { mDt = me[0][0] * me[1][1] - me[0][1] * me[1][0];}
};

inline double matrix<2>::element(int row,int column) const
{ assert((row == 0 || row == 1) && (column == 0 || column == 1));
    return me[row][column];
}

inline bool matrix<2>::isSingular() const
{return (mDt == 0.); }

inline bool matrix<2>::isIdentity() const
{return (me[0][0] == 1. && me[0][1] == 0. &&
         me[1][0] == 0. && me[1][1] == 1.);
}

inline matrix<2> matrix<2>::transpose() const
{ return matrix<2>(me[0][0], me[1][0], me[0][1], me[1][1]); }

inline matrix<2>& matrix<2>::operator+=(const matrix<2>& m)
{ return *this = *this + m; }

inline matrix<2> matrix<2>::operator-() const
{ return matrix<2>(-me[0][0], -me[0][1],
                   -me[1][0], -me[1][1]);
}

inline matrix<2>& matrix<2>::operator-=(const matrix<2>& m)
{ return *this = *this - m; }

inline matrix<2>& matrix<2>::operator*=(double s)
{ return *this = *this * s; }

inline matrix<2>& matrix<2>::operator*=(const matrix<2>& m)
{ return *this = *this * m; }

inline matrix<2> matrix<2>::operator/(const matrix<2>& m) const
{ return (*this) * m.inverse(); }

inline matrix<2>& matrix<2>::operator/=(const matrix<2>& m)
{ return *this *= m.inverse(); }

inline matrix<2> matrix<2>::operator/(double s) const
{ return *this * (1. / s); }  // Do division once

inline matrix<2>& matrix<2>::operator/=(double s)
{ return *this = *this / s; }

inline matrix<2> operator*(double s, const matrix<2>& m)
{return m * s;}

inline matrix<2> operator/(double s, const matrix<2>& m)
{ return m.inverse() * s;}

inline dVector_2d& operator*=(dVector_2d& v, const matrix<2>& m)
{ return v = v * m;}

inline dVector_2d operator/(const dVector_2d& v, const matrix<2>& m)
{ return v * m.inverse();}

inline dVector_2d& operator/=(dVector_2d& v, const matrix<2>& m)
{ return v = v * m.inverse();}

/*    ************************
 *                      *
 *     Arithmetic       *
 *                      *
 ************************
 */

inline bool matrix<2>::operator == (const matrix<2>& m) const
{ return (me[0][0] == m.me[0][0] && me[0][1] == m.me[0][1] &&
          me[1][0] == m.me[1][0] && me[1][1] == m.me[1][1]);
}

inline bool matrix<2>::operator != (const matrix<2>& m) const
{ return (!(*this==m)); }

inline matrix<2> matrix<2>::operator + (const matrix<2>& m) const
{ return matrix<2>(me[0][0] + m.me[0][0], me[0][1] + m.me[0][1],
                   me[1][0] + m.me[1][0], me[1][1] + m.me[1][1]);
}

inline matrix<2> matrix<2>::operator - (const matrix<2>& m) const
{ return matrix<2>(me[0][0] - m.me[0][0], me[0][1] - m.me[0][1],
                   me[1][0] - m.me[1][0], me[1][1] - m.me[1][1]);
}

inline matrix<2> matrix<2>::operator * (double s) const
{ return matrix<2>(me[0][0] * s, me[0][1] * s, me[1][0] * s, me[1][1] * s); }

inline matrix<2> matrix<2>::operator * (const matrix<2>& m) const
{ return matrix<2>(me[0][0]*m.me[0][0] + me[0][1]*m.me[1][0],
                   me[0][0]*m.me[0][1] + me[0][1]*m.me[1][1],
                   me[1][0]*m.me[0][0] + me[1][1]*m.me[1][0],
                   me[1][0]*m.me[0][1] + me[1][1]*m.me[1][1]);
}

inline dVector_2d matrix<2>::operator * (const dVector_2d& v) const
{ return dVector_2d(me[0][0] * v.x() + me[0][1] * v.y(),
                     me[1][0] * v.x() + me[1][1] * v.y());
}

inline dVector_2d operator * (const dVector_2d& v, const matrix<2>& m)
{ return dVector_2d(m.me[0][0] * v.x() + m.me[1][0] * v.y(),
                     m.me[0][1] * v.x() + m.me[1][1] * v.y());
}

inline fVector_2d matrix<2>::operator * (const fVector_2d& v) const
{ return fVector_2d((float) (me[0][0] * v.x() + me[0][1] * v.y()),
                     (float) (me[1][0] * v.x() + me[1][1] * v.y()));
}

inline fVector_2d operator * (const fVector_2d& v, const matrix<2>& m)
{ return fVector_2d((float) (m.me[0][0] * v.x() + m.me[1][0] * v.y()),
                     (float) (m.me[0][1] * v.x() + m.me[1][1] * v.y()));
}

    
    // Minimal 2Dxform for now:
        
        class xform
    {
    public:
        xform() : mT(dVector_2d()), mC(matrix()) {}
        /*
         * effect Default ctor constructs the identity transform
         */
        
        xform(const matrix2& c, const dVector_2d& t) : mT(t), mC(c) {}
        xform(const matrix2& c, const fVector_2d& t) : mC(c) { trans (t); }
        
        // default dtor, copy, assignment OK
        
        const matrix2& matrix() const { return mC; }
        
        const dVector_2d& trans() const  { return mT; }
        const fVector_2d transf () const { return fVector_2d ((float) mT.x(), (float) mT.y()); }
        
        void matrix(const matrix2& c) { mC = c; }
        void trans(const dVector_2d& t)  { mT = t; }
        void trans(const fVector_2d& t)  { mT.x((double) t.x());mT.y((double) t.y()); }
        
        xform inverse() const
        {
            matrix2 ic = mC.inverse();
            return xform(ic, ic * -mT);
        }
        
        /*
         * effect   return the inverse transformation
         * throws   general_exception ("xform is Singular") if the transform is singular.
         */
        xform operator * (const xform& xform) const
        { return svl::xform(mC * xform.mC, mC * xform.mT + mT); }
        xform compose (const xform& xform) const {return *this * xform;}
        /*
         * effect  Compose methods:  Compose are left to right
         *         so A * B = AB
         */
        double xRot() const   {return mC.xRot();}
        double yRot() const   {return mC.yRot();}
        double   xScale() const {return mC.xScale();}
        double   yScale() const {return mC.yScale();}
        /*
         * effect   xRot, yRot, xScale, yScale are one way to dissect a transform.
         * note     Be careful if you mix this decomposition with the one below.
         *          For example, when shear is present, aspect != yScale/xScale.
         * throws   general_exception ("xform is Singular") if the transform is singular.
         */
        double   scale() const    {return mC.scale();}
        double   aspect() const   {return mC.aspect();}
        double shear() const    {return mC.shear();}
        uRadian rotation() const {return mC.rotation();}
        /*
         * effect  scale, aspect, shear, rotation are another decomposition
         * note     Be careful if you mix this decomposition with the one above.
         *          For example, when shear is present, aspect != yScale/xScale.
         * throws  general_exception ("xform is Singular") if the transform is singular.
         */
        double mapAngle (const double& ang) const
        {
            dVector_2d v( cos(ang),sin(ang) );
            v = mC * v;
            return ( atan2 (v.y(), v.x()) );
        }
        /*
         * effect   Returns the new angle after mapping by the xform.
         */
        double  invMapAngle (const double& ang) const;
        /*
         * effect   Returns the new angle after mapping by inverse of xform;
         * throws   general_exception ("xform is Singular") if the transform is singular.
         */
        dVector_2d mapPoint (const dVector_2d &pt) const
        { return *this * pt; }
       fVector_2d mapPoint (const fVector_2d &pt) const
        { return *this * pt; }
        
        dVector_2d operator * (const dVector_2d &pt) const
        { return (mC * pt) + mT; }
       fVector_2d operator * (const fVector_2d &pt) const
        {
            dVector_2d tmp (pt.x(), pt.y());
            tmp = (mC * tmp) + mT;
            return fVector_2d ((float) tmp.x(), (float) tmp.y());
        }
        
        /*
         * effect   Maps the point by the xform: result = c * pt + t.
         * note     Both mapPoint and * are mapping the vector like a full 2D-point,
         *          with location as well as length and direction
         */
        dVector_2d invMapPoint (const dVector_2d &pt) const
        { return (inverse() * pt); }
       fVector_2d invMapPoint (const fVector_2d &pt) const
        {
            dVector_2d tmp (pt.x(), pt.y());
            tmp = inverse() * tmp;
            return fVector_2d ((float) tmp.x(), (float) tmp.y());
        }
        
        
        /*
         * effect   Maps the point by the inverse xform: result = c^-1 * (pt - t)
         * throws   general_exception ("xform is Singular") if the transform is singular.
         */
        
        //The following 2 are mapping the vector like a vector, with only length
        //  and orientation
        dVector_2d mapVector (const dVector_2d &vect) const
        { return (mC * vect); }
        /*
         * effect   Rotates and scales the vector: result = c * vect
         */
        dVector_2d invMapVector (const dVector_2d &vect) const
        { return (mC.inverse() * vect); }
        
        
        // The area decomposes in to two vectors who cross product it equals.
        // Mapping vectors (1,0), and (0,1) in to  the new vector,
        // and then take the cross product.
        //
        // for matrix |a b|, U=(1,0) maps to (a,c) and V=(0,1) maps to (b,d)
        //            |c d|
        //
        // |ad - bc| ==>  |determinant|.
        //
        double mapArea (double area) const
        { return (area * std::fabs (mC.determinant())); }
        
        // determinant (inverse) == inverse (determinant)
        double invMapArea (double area) const
        {
            if (isSingular())
                throw svl::singular_error ("invMapArea");
            return (area / (std::fabs (mC.determinant())));
        }
        
        /*
         * effect   Rotates and scales the vector: result = c^-1 * vect
         * throws   general_exception ("xform is Singular") if the transform is singular.
         */
        //  double    mapArea (double area) const;
        /*
         * effect   Returns the new area after mapping by the xform.
         * notes    mapArea() computes the new area by multiplying the old area
         *          by the area of a mapped unit square.  The user may wish
         *          to pass in 1.0 as the old area to get the area
         *          conversion constant which then can be applied to map all
         *          the needed areas.
         */
        //  double    invMapArea (double area) const;
        /*
         * effect   Returns the new area after mapping by the inverse.
         * throws   general_exception ("xform is Singular") if the transform is singular.
         */
        
        bool operator==(const xform& xform) const
        { return ( (mC == xform.mC) && (mT == xform.mT) ); }
        
        bool operator != (const xform& xform) const
        { return !(*this == xform); }
        bool operator <  (const xform&) const;
        
        // Identity transform
        static const xform I;
        
        bool isIdentity() const { return mT.isNull() && mC.isIdentity(); }
        /*
         * effect   Returns true if the transform is identity;
         */
        bool isSingular() const {return mC.isSingular();}
        /*
         * effect   Returns true if the transform is singular.
         */
        
        friend ostream& operator<< (ostream& ous,const xform& xform)
        {
            ous << xform.matrix() << "v" << xform.trans();
            return ous;
        }
        
    private:
        dVector_2d mT;
        matrix2 mC;
    };

}

#endif // _vector_2d_H_
