
#ifndef __rcLSFIT_H
#define __rcLSFIT_H

#include <rc_vector2d.h>
#include <rc_fit.h>
#include <rc_matrix.h>
#include <rc_xforms.h>
#include <rc_line.h>


// Least Square Solver Support for

template<class T>
class rcLsFit
{
 public:

  rcLsFit () :
  mSvx (T(0)), mSux (T(0)), mSuy (T(0)), mSvy (T(0)),
    mSxx(T(0)), mSxy(T(0)), mSyy(T(0)),
    mSuu(T(0)), mSuv(T(0)), mSvv(T(0)), mSu (T(0)),
    mSv (T(0)), mSx (T(0)), mSy (T(0)), mN (0), mX2 (T(0)),
    mRMSn (0) {}

  void update (const rc2dVector<T>& xy, const rc2dVector<T>& uv);

  void update (const rc2dVector<T>& uv);

  bool solve3p (rc2dVector<T>& trans, rcRadian& rot);
  /*
   * Translation + Rotation
   */

  bool solvetp (rc2dVector<T>& trans, rcMatrix_2d& mat);
  /*
   * Translation + Scale in X and Y
   */

  bool solvetp (rc2Xform& xform);
  /*
   * Translation + Scale in X and Y
   */

  bool solvetp (rc2dVector<T>& trans);
  /*
   * Translation
   */

  // bool solve5p (rc2Xform& xform);
  /* @note UnitTestFailure
   * Translation + Scale in X and Y + Rotation
   */
  // bool solve6p (rc2Xform& xform);
  /* @note UnitTestFailure
   * Translation + Scale in X and Y + Rotation X and Y
   */

  T rms (const rc2dVector<T>& xy, const rc2dVector<T>& uv,
	 rc2dVector<T>& trans, rcMatrix_2d& mat);
  T rms (const rc2dVector<T>& xy, const rc2dVector<T>& uv, const rc2Xform&);

  /*
   * Least Square Fit to a Line
   */
  rcLineSegment<T> fit () const;
  T error (const rcLineSegment<T>& ) const;

 private:
  T mSvx;
  T mSux;
  T mSuy;
  T mSvy;
  T mSxx;
  T mSxy;
  T mSyy;
  T mSuu;
  T mSuv;
  T mSvv;
  T mSu;
  T mSv;
  T mSx;
  T mSy;
  T mN;
  T mX2;
  rcInt32 mRMSn;
};


template<class T>
bool rcLsFit<T>::solve3p (rc2dVector<T>& trans, rcRadian& rot)
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
bool rcLsFit<T>::solvetp (rc2Xform& xm)
  {
    if (mN < 3) return false;

    rc2dVector<T> trans; rcMatrix_2d mat;
    bool success = rcLsFit<T>::solvetp (trans, mat);
    xm.trans(trans);xm.matrix (mat);
    return success;
  }


template<class T>
bool rcLsFit<T>::solvetp (rc2dVector<T>& trans, rcMatrix_2d& mat)
  {
    if (mN < 3) return false;

    trans.x ((mSu - mSx) / mN);
    trans.y ((mSv - mSy) / mN);
    mat = rcMatrix_2d ((mSux - (mSu * mSx) / mN) /
		     (mSxx - (mSx * mSx) / mN),
		     0.0, 0.0,
		     (mSvy - (mSv * mSy) / mN) /
		     (mSyy - (mSy * mSy) / mN));


    return true;
  }

template<class T>
bool rcLsFit<T>::solvetp (rc2dVector<T>& trans)
  {
    if (mN < 2) return false;

    trans.x ((mSu - mSx) / mN);
    trans.y ((mSv - mSy) / mN);

    return true;
  }


template<class T>
void rcLsFit<T>::update (const rc2dVector<T>& xy, const rc2dVector<T>& uv)
  {
    const T& x(xy.x()), y(xy.y()), u(uv.x()), v(uv.y());

    mSx += x, mSy += y, mSu += u, mSv +=v;
    mSxx += x * x;
    mSyy += y * y;
    mSxy += x * y;
    mSuu += u * u;
    mSuv += u * v;
    mSvv += v * v;
    mSux += u * x;
    mSvx += v * x;
    mSuy += u * y;
    mSvy += v * y;
    mN += T (1);
  };


template<class T>
void rcLsFit<T>::update (const rc2dVector<T>& uv)
  {
    const T& u(uv.x()), v(uv.y());

    mSu += u, mSv +=v;
    mSuu += u * u;
    mSuv += u * v;
    mSvv += v * v;
    mN += T (1);
  };

#if 0
template<class T>
bool rcLsFit<T>::solve5p (rc2Xform& xform)
  {
    if (mN < 3) return false;

    T k1 = mSxx - mSx * mSx / mN;
    T k2 = mSyy - mSy * mSy / mN;

    T c1 = mSux - mSu * mSx / mN;
    T c2 = mSuy - mSu * mSy / mN;
    T c3 = mSvx - mSv * mSx / mN;
    T c4 = mSvy - mSv * mSy / mN;

    T A = c1 * c2 + c3 * c4;
    T B = k1 * (c2 * c2 + c4 * c4) + k2 * (c1 * c1 + c3 * c3);
    T C = k1 * k2 * A;

    T pos, neg;
    if (!rfQuadradic (A, B, C, pos, neg))
      return false;

    T det = k1 * k2 - pos * pos;
    if (rfRealEq (det, (T) 0.0)) return false;

    rcMatrix_2d mat;
    rc2dVector<T> trans;

    mat = rcMatrix_2d ((k2 * c1 + pos * c2) / det,
		     (k1 * c2 + pos * c1) / det,
		     (k2 * c3 + pos * c4) / det,
		     (k1 * c4 + pos * c3) / det);

    trans.x((mSu - mat.element(0,0) * mSx -
	     mat.element(0,1) * mSy) / mN);
    trans.y((mSv - mat.element(1,0) * mSx -
	     mat.element(1,1) * mSy) / mN);

    xform.matrix (mat);
    xform.trans (trans);

    return true;
  }
#endif

template<class T>
T rcLsFit<T>::rms (const rc2dVector<T>& xy, const rc2dVector<T>& uv,
	 rc2dVector<T>& trans, rcMatrix_2d& mat)
{
  rc2Xform xm (mat, trans);
  return rcLsFit<T>::rms (xy, uv, xm);
#if 0
  rc2Fvector d = xy * mat.inverse() + trans;
  mX2 += uv.distanceSquared (d);
  mRMSn++;
  return (T) (sqrt ((double) mX2 / mRMSn));
#endif
}

template<class T>
T rcLsFit<T>::rms (const rc2dVector<T>& xy, const rc2dVector<T>& uv, const rc2Xform& xm)
{
  rc2Fvector d = xm.invMapPoint (xy);
  mX2 += uv.distanceSquared (d);
  mRMSn++;
  return (T) (sqrt ((double) mX2 / mRMSn));
}


#if 0
template<class T>
bool rcLsFit<T>::solve6p (rc2Xform& xform)
{
  if (mN < 3) return false;

  T k1 = mSxx - mSx * mSx / mN;
  T k2 = mSxy - mSx * mSy / mN;
  T k4 = mSyy - mSy * mSy / mN;

  T c1 = mSux - mSu * mSx / mN;
  T c2 = mSuy - mSu * mSy / mN;
  T c3 = mSvx - mSv * mSx / mN;
  T c4 = mSvy - mSv * mSy / mN;

  T det = k1 * k4 - k2 * k2;
  if (rfRealEq (det, (T) 0.0)) return false;

  rcMatrix_2d mat;
  rc2dVector<T> trans;

  mat = rcMatrix_2d ((k4 * c1 - k2 * c2) / det,
		   (k1 * c2 - k2 * c1) / det,
		   (k4 * c3 - k2 * c4) / det,
		   (k1 * c4 - k2 * c3) / det);

  trans.x((mSu - mat.element(0,0) * mSx -
	   mat.element(0,1) * mSy) / mN);
  trans.y((mSv - mat.element(1,0) * mSx -
	   mat.element(1,1) * mSy) / mN);

  xform.matrix (mat);
  xform.trans (trans);
  return true;
}

#endif

/*
 * Horn's "Robot Vision" pages 49-53.
 */


template<class T>
rcLineSegment<T> rcLsFit<T>::fit () const
{
  T
    xnom = mSu / mN,
    ynom = mSv / mN,
    a = mSuu - mSu * xnom,
    b = 2 * (mSuv - mSu * ynom),
    c = mSvv - mSv * ynom;

  if (fabs(b) < 1e-12 && fabs(a - c) < 1e-12)
    throw general_exception ("Unstable line Fit");

  T
    hypotenuse = sqrt(rmSquare(b) + rmSquare(a - c)),
    cos2theta = (a - c) / hypotenuse,
    sn = sqrt( (1 - cos2theta) / 2),
    cn = sqrt( (1 + cos2theta) / 2);

  if (b < 0) cn = -cn;

  T distance = -(xnom * sn)+(ynom * cn);
  rcRadian angle(atan2(sn,cn));
  if (distance >= 0)
    return rcLineSegment<T>(angle,distance);
  return rcLineSegment<T>(angle+rcRadian(rkPI),-distance);

}

template<class T>
T rcLsFit<T>::error (const rcLineSegment<T> & line) const
{
  T d, s, c;

  d = line.distance();
  s = sin (line.angle());
  c = cos (line.angle());

  return (  d*d*mN
	  + 2*d*(s*mSu - c*mSv)
	  + s*s*mSuu + c*c*mSvv
	  - 2*s*c*mSuv);
}



#endif /* __rcLSFIT_H */
