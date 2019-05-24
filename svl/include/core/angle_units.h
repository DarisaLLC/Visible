
#ifndef __ANGLEUNITS__
#define __ANGLEUNITS__

#include <limits>
#include <cmath>
#include <stdint.h>

using namespace std;



/*
 * Angle Support
 *
 * Usage: transformation <-> binary angles, radians, and degrees
 */


class uAngle8;
class uAngle16;
class uRadian;
class uDegree;


// Allow only explicit constructions.
// Implement 1D fixed units

#define rmMakeUnit(T, U)					\
public:								\
  U () : val (0) {}							\
  explicit U (T a) : val(a) {}					\
								\
  U operator + (U a) const { return U (val + a.val);}		\
  U operator - (U a) const { return U (val - a.val);}		\
								\
  U operator * (T a) const { return U (val * a);}			\
  U operator / (T a) const { return U (val / a);}			\
								\
  U operator - () const { return U (-val);}			\
								\
  T operator * (U a) const { return val * a.val;}			\
  T operator / (U a) const { return val / a.val;}			\
								\
  friend U operator * (T a, U b) { return U (a * b.val);}		\
								\
  double Double () const { return (double) val;}		\
  T      basic    () const { return          val;}		\
  U& operator  = (T a) { val  = a; return *this;}			\
  U& operator *= (T a) { val *= a; return *this;}			\
  U& operator /= (T a) { val /= a; return *this;}			\
								\
  U& operator *= (U a) { val *= a.val; return *this;}		\
  U& operator /= (U a) { val /= a.val; return *this;}		\
								\
  U& operator += (U a) { val += a.val; return *this;}		\
  U& operator -= (U a) { val -= a.val; return *this;}		\
								\
  bool operator == (U a) const { return val == a.val;}		\
  bool operator != (U a) const { return val != a.val;}		\
  bool operator <  (U a) const { return val <  a.val;}		\
  bool operator <= (U a) const { return val <= a.val;}		\
  bool operator >  (U a) const { return val >  a.val;}		\
  bool operator >= (U a) const { return val >= a.val;}		\
								\
								\
private:							\
  T val


/*
 * Classes reprenting Angular units
 *
*/


#define uPI 3.141592653589793238462643383279502884197169399375105820974944
#define u2PI 6.2831853071795864769252867665590057683943388015061

class uRadian
{
  rmMakeUnit (double, uRadian);
  

public:
  inline uRadian (uDegree);
  inline uRadian (uAngle8);
  inline uRadian (uAngle16);

  uRadian norm () const;	// result range is [0-2PI)
  uRadian normSigned () const;	// result range is [-PI, PI)
};

bool real_equal (uRadian x, uRadian y, uRadian epsilon = uRadian(1.e-15));

class uDegree
{
  rmMakeUnit (double, uDegree);

public:
  inline uDegree (uRadian);
  inline uDegree (uAngle8);
  inline uDegree (uAngle16);
  inline static const uDegree piOverTwo () { return uDegree (360.0 / 4); }
    
  uDegree norm () const;	// result range is [0-360)
  uDegree normSigned () const;	// result range is [-180, 180)
};

class uAngle8
{
  rmMakeUnit (uint8_t, uAngle8);

public:
  inline uAngle8 (uRadian);
  inline uAngle8 (uDegree);
  inline uAngle8 (uAngle16);
  inline static const uAngle8 piOverTwo () { return uAngle8 (std::numeric_limits<uint8_t>::max() / 4); }
};

class uAngle16
{
  rmMakeUnit (uint16_t, uAngle16);

public:
  inline uAngle16 (uRadian);
  inline uAngle16 (uDegree);
  inline uAngle16 (uAngle8);
  inline static const uAngle8 piOverTwo () { return uAngle16 (std::numeric_limits<uint16_t>::max() / 4); }
};




// Actual class methods: created from defines

#define rmAngularUnitIntRange(T) (2. * ((unsigned)1 << (8 * sizeof(T) - 1)))

#define rmAngularUnitFloatFromAny(dst, src, dstRange, srcRange)			\
inline dst::dst (src x)								\
{										\
  val = x.Double() * ((dstRange) / (srcRange));					\
}

#define rmAngularUnitIntFromFloat(dst, src, dstT, srcRange)				\
inline dst::dst (src x)								\
{										\
  val = (dstT) (int) floor(x.Double() *	(rmAngularUnitIntRange(dstT)/srcRange) + 0.5);\
}

#define rmAngularUnitIntFromSmaller(dst, src, dstT, srcT)				\
inline dst::dst (src x)								\
{										\
  val = (dstT) (x.basic() << (8 * (sizeof(dstT) - sizeof(srcT))));		\
}

#define rmAngularUnitIntFromBigger(dst, src, dstT, srcT)				\
inline dst::dst (src x)								\
{										\
  val = (dstT) ((x.basic() + (1 << (8 * (sizeof(srcT) - sizeof(dstT)) - 1)))	\
	      >> (8 * (sizeof(srcT) - sizeof(dstT))));				\
}

#define rmAngularUnitFloatFloat(aType, bType, aRange, bRange)			\
  rmAngularUnitFloatFromAny (aType, bType, aRange, bRange)				\
  rmAngularUnitFloatFromAny (bType, aType, bRange, aRange)

#define rmAngularUnitFloatInt(fUnit, bUnit, fRange, bType)				\
  rmAngularUnitFloatFromAny (fUnit, bUnit, fRange, rmAngularUnitIntRange(bType))		\
  rmAngularUnitIntFromFloat (bUnit, fUnit, bType, fRange)

#define rmAngularUnitIntInt(smallUnit, bigUnit, smallType, bigType)			\
  rmAngularUnitIntFromSmaller (bigUnit, smallUnit, bigType, smallType)		\
  rmAngularUnitIntFromBigger  (smallUnit, bigUnit, smallType, bigType)

rmAngularUnitFloatFloat (uRadian, uDegree, u2PI, 360.)

rmAngularUnitFloatInt (uRadian, uAngle8 , u2PI, uint8_t )
rmAngularUnitFloatInt (uRadian, uAngle16, u2PI, uint16_t)
rmAngularUnitFloatInt (uDegree, uAngle8 ,  360., uint8_t )
rmAngularUnitFloatInt (uDegree, uAngle16,  360., uint16_t)

rmAngularUnitIntInt (uAngle8, uAngle16, uint8_t, uint16_t)

// Define trig functions for the new classes

inline uRadian arccos (double x) { return uRadian (acos (x));}
inline uRadian arcsin (double x) { return uRadian (asin (x));}
inline uRadian arctan (double x) { return uRadian (atan (x));}
inline uRadian arctan (double y, double x) { return uRadian(atan2(y, x));}

inline double sin (uRadian x) { return sin (x.Double());}
inline double cos (uRadian x) { return cos (x.Double());}
inline double tan (uRadian x) { return tan (x.Double());}

inline uRadian abs (uRadian x) { return uRadian (std::fabs (x.Double()));}
inline uDegree abs (uDegree x) { return uDegree (std::fabs (x.Double()));}

inline bool isnan (uRadian x) { return std::isnan<double> (x.Double()); }





#endif
