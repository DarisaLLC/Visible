#ifndef _LINE_H_
#define _LINE_H_

#include <vector>
#include "angle_units.h"
#include "vector2d.hpp"
#include "core/core.hpp"
#include "core/svl_exception.hpp"

using namespace svl;

class fLineSegment2d
{
public:
    
    class invalid_segment : public std::exception {
    public:
        virtual const char* what() const throw () {
            return "Invalid Segment";
        }
    };
    
    fLineSegment2d();
		
    fLineSegment2d (const uRadian& angle, const float distance);

    //! construct from two bf2dVector<int>
    /*!@param p1 an endpoint on the line
      @param p2 the second endpoint of the line line*/
    fLineSegment2d(const fVector_2d & p1, const fVector_2d & p2);

    fLineSegment2d (const uRadian& angle,  const fVector_2d& pa);
    //! get a point on the line
    /*!@param n get point point + n * direction; for n = 0 (default),
      this is the point entered in the constructor or in reset*/
    const fVector_2d& point1() const;
		
    //! returns the direction vector for this straight line
    const fVector_2d& point2() const;
		
    //!returns the angle of the line
    uRadian angle() const;
		
    uRadian angleBetween(fLineSegment2d &line) const;
		
    bool intersects(fLineSegment2d &line, double &xcoord, double &ycoord) const;
		
    double distance(const fVector_2d& point) const ;
	
    fLineSegment2d average (const fLineSegment2d&) const;
		
    float length() const;
		
    //! whether this object is valid
    bool isValid() const;
	
		void id (int mid) { mId = mid; }
		int id () const { return mId; }
    bool operator==(const fLineSegment2d& ls) const;

    bool operator!=(const fLineSegment2d& ls) const;
		
private:
    fVector_2d mPoint1;
    fVector_2d mPoint2;
    uRadian mAngle;
    float mDist;
    bool mValid;
    int mId;
    bool mType; // true means angle dir ctor type hack alert
};


class lineInf
{
public:
    lineInf() : mDir(1.,0.), mPos(0.,0.) {}
    // effect     Constructs the line with direction (1,0), passing
    //            through the point (0,0).
    
    lineInf(const fVector_2d& dir, const fVector_2d& pos);
    // effect     Constructs the line in the direction "dir", passing through
    //            the point "pos".
    // requires   dir must have unit length
    // throws     ccShapesError::DegenerateShape if dir == (0,0).
    
    lineInf(const uRadian& t, const fVector_2d& pos);
    // effect     Constructs the line in the direction "t", passing through
    //            the point "pos".
    
    lineInf(const fVector_2d& v);
    // effect     Constructs the line in the direction v.angle(), passing
    //            through the point v.x(), v.y().
    // throws     ccShapesError::DegenerateShape if v == (0,0).
    
    const fVector_2d& dir() const { return mDir; }
    const fVector_2d& pos() const { return mPos; }
   fVector_2d& dir() { return mDir; }
   fVector_2d& pos() { return mPos; }
    
//    lineInf transform(const rc2Xform& c) const;
 //   void transform(const rc2Xform& c, lineInf& result) const;
    // returns    transformed line
    
    double toPoint(const fVector_2d& v) const;
    // returns    The shortest distance from this line to v.
    
    uRadian angle() const { return dir().angle(); }
    // effect     Returns the line angle. -PI < a <= PI
    
    void angle(uRadian a) { mDir.x(cos(a)); mDir.y(sin(a)); }
    // effect     Set Direction
    
    lineInf parallel(const fVector_2d&) const;
    // returns    Line through the specified point that is parallel to me
    
    bool isParallel(const lineInf& l) const;
    // returns    true if it is
    
    uRadian angle (const lineInf&) const;
    // returns    Angle from me to this line. -PI <  <= PI
    
   fVector_2d intersect(const lineInf&, bool isPar) const;
    // returns    intersection
    
    lineInf normal(const fVector_2d&) const;
    // returns    +90 degrees line through the specified point.
    
   fVector_2d project(const fVector_2d&) const;
    // returns    The projection of point onto this line.
    
    double offset(const fVector_2d&) const;
    // returns    Signed distance from me to the point
    
    bool operator==(const lineInf&) const;
    bool operator!=(const lineInf&) const;
    
    
private:
    fVector_2d mDir, mPos;
};


// Least Square Solver Support for

template<typename T>
class  lsFit
{
public:
    typedef T val_t;
     lsFit () : mSuu(0.0f), mSuv(0.0f), mSvv(0.0f), mSu (0.0f), mSv (0.0f),mN (0) {}
    void clear () const;
    
    void update (T u, T v) const;
    
    /*
     * Least Square Fit to a Line
     */
    std::pair<uRadian, double> fit () const;
    T error (const std::pair<uRadian, double>& ) const;
    
private:
   mutable T mSuu;
   mutable T mSuv;
   mutable T mSvv;
   mutable T mSu;
   mutable T mSv;
   mutable T mN;
};




#if 0
template<class T>
bool  lsFit<T>::solve3p ( fVector_2d& trans, uRadian& rot)
{
    if (mN < 3) return false;
    
    T e = mN * (mSux + mSvy) - mSu * mSx - mSv * mSy;
    T d = mN * (mSvx + mSuy) + mSu * mSy - mSv * mSx;
    
    rot = atan2 ((double) d, (double) e);
    T c = (T) cos (rot);
    T s = (T) sin (rot);
    trans.x ((mSu - c * mSx + s * mSy) / mN);
    trans.y ((mSv - s * mSx - c * mSy) / mN);
    
    
    return true;
}



template<class T>
bool  lsFit<T>::solvetp (xform& xm)
{
    if (mN < 3) return false;
    
     fVector_2d trans;  matrix2 mat;
    bool success =  lsFit<T>::solvetp (trans, mat);
    xm.trans(trans);xm.matrix (mat);
    return success;
}


template<class T>
bool  lsFit<T>::solvetp ( fVector_2d& trans,  matrix2& mat)
{
    if (mN < 3) return false;
    
    trans.x ((mSu - mSx) / mN);
    trans.y ((mSv - mSy) / mN);
    mat =  matrix2 ((mSux - (mSu * mSx) / mN) /
                       (mSxx - (mSx * mSx) / mN),
                       0.0, 0.0,
                       (mSvy - (mSv * mSy) / mN) /
                       (mSyy - (mSy * mSy) / mN));
    
    
    return true;
}

template<class T>
bool  lsFit<T>::solvetp ( fVector_2d& trans)
{
    if (mN < 2) return false;
    
    trans.x ((mSu - mSx) / mN);
    trans.y ((mSv - mSy) / mN);
    
    return true;
}

#endif

#endif /* _bfLINE_H_ */
