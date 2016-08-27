#ifndef __MATH_H
#define __MATH_H
#include <algorithm>
#include <vector>
#include <deque>
#include <numeric>

using namespace std;
#include <cmath>
#include <boost/numeric/conversion/bounds.hpp>

using namespace std;

//! Returns an index to the left of two interpolation values.
/*! \a begin points to an array containing a sequence of values
 sorted in ascending order. The length of the array is \a len.
 If \a val is lower than the first element, 0 is returned.
 If \a val is higher than the last element, \a len-2
 is returned. Else, the index of the largest element smaller
 than \a val is returned. */
template<typename T> inline int interpol_left
(const T *begin, int len, const T &val)
{
    const T *end = begin+len;
    const T *iter = std::lower_bound (begin, end, val);
    if (iter==begin) return 0;
    if (iter==end) return len-2;
    return (iter-begin)-1;
}

//! Returns an index to the nearest interpolation value.
/*! \a begin points to an array containing a sequence of values
 sorted in ascending order. The length of the array is \a len.
 If \a val is lower than the first element, 0 is returned.
 If \a val is higher than the last element, \a len-1 is returned.
 Else, the index of the nearest element within the sequence of
 values is returned. */
template<typename T> inline int interpol_nearest
(const T *begin, int len, const T &val)
{
    int left = interpol_left(begin, len, val);
    T delleft = val-(*(begin+left));
    T delright = (*(begin+left+1))-val;
    if (delright<0) return left+1;
    return (delright<delleft) ? (left+1) : left;
}

// Some other constants
#define rkPI (3.1415926535897932384626433832795)
#define rk2PI (2*rkPI)
#define rkRadian (rkPI / 180.0)

// The usual Min and Max
#define rmMin(a,b) ((a) < (b) ? (a) : (b))
#define rmMax(a,b) ((a) > (b) ? (a) : (b))
#define rmABS(a) ((a) < 0 ? (-(a)) : (a))
#define rmSquare(a) ((a) * (a))


#define rmMedOf3(a, b, c)                                       \
((a) > (b) ? ((a) < (c) ? (a) : ((b) < (c) ? (c) : (b))) :    \
((a) > (c) ? (a) : ((b) < (c) ? (b) : (c))))

#define rmMaxOf3(a, b, c) (rmMax(rmMax(a, b), c))

#define rmMinOf3(a, b, c) (rmMin(rmMin(a, b), c))

#define rmSGN(a) (((a) < 0) ? -1 : 1)

#define rmLimit(a,b,c) (rmMin((rmMax((a),(c))),(b)))

bool r_IsPowerOf2 (uint32_t);

template <class T>
inline T r_Sqr(T x) {return x*x;}

/** \brief Check if val1 and val2 are equals to an epsilon extent
 * \param[in] val1 first number to check
 * \param[in] val2 second number to check
 * \param[in] eps epsilon
 * \return true if val1 is equal to val2, false otherwise.
 */
template<typename T> bool
equal (T val1, T val2, T eps = std::numeric_limits<T>::epsilon() )
{
    return (fabs (val1 - val2) < eps);
}

inline bool real_equal(double x,double y,double epsilon = 1.e-15  )
{return std::abs (x - y) <= epsilon;}

inline bool real_equal(float x,float y,float epsilon = 1.e-10  )
{return std::abs (x - y) <= epsilon;}


template<class T>
T r_Sigmoid(T x)
{
    return ((T) 1.0)/((T) 1.0 + exp(-x));
}

template<class T>
T r_NormPositiveSigmoid(T x)
{
    // Transform x [0 1] to [-1 1]
    T xp = x + x + (T) (-1);
    xp = r_Sigmoid (xp);
    
    // Transform [-1 1] back to [0 1]
    xp = (xp + 1) / (T) 2;
    
    return xp;
}


template<class T>
bool r_Quadradic (T a, T b, T c, T& pos, T& neg)
{
    T det = rmSquare (b) - (4 * a * c);
    
    if (a == T (0))
    {
        pos = -c / b;
        neg = -c / b;
        return true;
    }
    
    if (det >= 0)
    {
        pos = (-b + sqrt (det)) / (2*a);
        neg = (-b - sqrt (det)) / (2*a);
        return true;
    }
    
    return false;
}


// Fix point Fast vector support using cordic
const int32 r_FxPrecision = 16;
int32 r_FxAtan2(int32 x, int32 y);
void r_FxUnitVec(int32 *cos, int32 *sin, int32 theta);
void r_FxRotate(int32 *argx, int32 *argy, int32 theta);
void r_FxPolarize(int32 *argx, int32 *argy);

// Rounding Templates and Specialization

template <class S, class D>
inline D r_RoundPlus(S val, D) { return D(val); }

template <class S, class D>
inline D r_Round(S val, D) { return D(val); }

inline int32  r_RoundPlus(double val, int32)  { return int32(val + 0.5); }
inline int32  r_RoundPlus(float val,  int32)  { return int32(val + 0.5); }

inline int32  r_RoundNeg(double val, int32)  { return int32(val -  0.5); }
inline int32  r_RoundNeg(float val,  int32)  { return int32(val - 0.5); }

inline int32 r_Round(double val, int32 Dummy)
{
    return val < 0 ? -r_RoundPlus(-val, Dummy) : r_RoundPlus(val, Dummy);
}

inline int32 r_Round(float val, int32 Dummy)
{
    return val < 0 ? -r_RoundPlus(-val, Dummy) : r_RoundPlus(val, Dummy);
}

// Integer versions of ceil and div
inline int32 r_iDiv (int32 x, int32 y)
{
    return (x >= 0 ? x : x - abs(y) + 1) / y;
}

inline int32 r_iFloor (int x, int y)
{
    return r_iDiv (x, y) * y;
}

inline int32 r_iCeil (int x, int y)
{
    return (x < 0 ? x : x + abs(y) - 1) / y * y;
}


/*
 * Angle Support
 *
 * Usage: transformation <-> binary angles, radians, and degrees
 */


class r_Angle8;
class r_Angle16;
class r_Radian;
class r_Degree;
class r_Fixed16;


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

class r_Radian
{
    rmMakeUnit (double, r_Radian);
    
public:
    inline r_Radian (r_Degree);
    inline r_Radian (r_Angle8);
    inline r_Radian (r_Angle16);
    
    r_Radian norm () const;	// result range is [0-2PI)
    r_Radian normSigned () const;	// result range is [-PI, PI)
};

bool real_equal (r_Radian x, r_Radian y, r_Radian epsilon = r_Radian(1.e-15));

class r_Degree
{
    rmMakeUnit (double, r_Degree);
    
public:
    inline r_Degree (r_Radian);
    inline r_Degree (r_Angle8);
    inline r_Degree (r_Angle16);
    
    r_Degree norm () const;	// result range is [0-360)
    r_Degree normSigned () const;	// result range is [-180, 180)
};

class r_Angle8
{
    rmMakeUnit (uint8, r_Angle8);
    
public:
    inline r_Angle8 (r_Radian);
    inline r_Angle8 (r_Degree);
    inline r_Angle8 (r_Angle16);
    inline static const r_Angle8 piOverTwo () { return r_Angle8 (boost::numeric::bounds<uint8>::highest() / 4); }
};

class r_Angle16
{
    rmMakeUnit (uint16, r_Angle16);
    
public:
    inline r_Angle16 (r_Radian);
    inline r_Angle16 (r_Degree);
    inline r_Angle16 (r_Angle8);
    inline static const r_Angle8 piOverTwo () { return r_Angle16 (boost::numeric::bounds<uint16>::highest()  / 4); }
};


// Classes for Fixed Pixel Positions

const int32 r_Fixed16Precision = 8;

class r_Fixed16
{
    rmMakeUnit (int16, r_Fixed16);
    
public:
    inline r_Fixed16 (double d) : val (int16 (d * (1<<r_Fixed16Precision))) {}
    
    inline r_Fixed16 (float d) : val (int16 (d * (1<<r_Fixed16Precision))) {}
    
    inline r_Fixed16 (int32 d) : val (int16 (d * (1<<r_Fixed16Precision))) {}
    
    float real () const;
};


// Actual class methods: created from defines

#define rmAngularUnitIntRange(T) (2. * ((unsigned)1 << (8 * sizeof(T) - 1)))

#define rmAngularUnitFloatFromAny(dst, sr_, dstRange, sr_Range)			\
inline dst::dst (sr_ x)								\
{										\
val = x.Double() * ((dstRange) / (sr_Range));					\
}

#define rmAngularUnitIntFromFloat(dst, sr_, dstT, sr_Range)				\
inline dst::dst (sr_ x)								\
{										\
val = (dstT) (int) floor(x.Double() *	(rmAngularUnitIntRange(dstT)/sr_Range) + 0.5);\
}

#define rmAngularUnitIntFromSmaller(dst, sr_, dstT, sr_T)				\
inline dst::dst (sr_ x)								\
{										\
val = (dstT) (x.basic() << (8 * (sizeof(dstT) - sizeof(sr_T))));		\
}

#define rmAngularUnitIntFromBigger(dst, sr_, dstT, sr_T)				\
inline dst::dst (sr_ x)								\
{										\
val = (dstT) ((x.basic() + (1 << (8 * (sizeof(sr_T) - sizeof(dstT)) - 1)))	\
>> (8 * (sizeof(sr_T) - sizeof(dstT))));				\
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

rmAngularUnitFloatFloat (r_Radian, r_Degree, rk2PI, 360.)

rmAngularUnitFloatInt (r_Radian, r_Angle8 , rk2PI, uint8 )
rmAngularUnitFloatInt (r_Radian, r_Angle16, rk2PI, uint16)
rmAngularUnitFloatInt (r_Degree, r_Angle8 ,  360., uint8 )
rmAngularUnitFloatInt (r_Degree, r_Angle16,  360., uint16)

rmAngularUnitIntInt (r_Angle8, r_Angle16, uint8, uint16)

// Define trig functions for the new classes

inline r_Radian ar_cos (double x) { return r_Radian (acos (x));}
inline r_Radian ar_sin (double x) { return r_Radian (asin (x));}
inline r_Radian ar_tan (double x) { return r_Radian (atan (x));}
inline r_Radian ar_tan (double y, double x) { return r_Radian(atan2(y, x));}

inline double sin (r_Radian x) { return sin (x.Double());}
inline double cos (r_Radian x) { return cos (x.Double());}
inline double tan (r_Radian x) { return tan (x.Double());}

inline r_Radian abs (r_Radian x) { return r_Radian (rmABS (x.Double()));}
inline r_Degree abs (r_Degree x) { return r_Degree (rmABS (x.Double()));}

inline float r_Fixed16::real () const { return val/((float) (1<<r_Fixed16Precision)); }

////////////////////////////////////////////////////////////////////////////////
//
//	log2max
//
//	This function computes the ceiling of log2(n).  For example:
//
//	log2max(7) = 3
//	log2max(8) = 3
//	log2max(9) = 4
//
////////////////////////////////////////////////////////////////////////////////
int32 r_Log2max(int32 n);

int32 r_GCDeuclid(int32 a, int32 b);


// reimplemented because ISO C99 function "nearbyint",

template < typename T >
int32 r_Nearbyint(const T& d)
{
    int32 z = int32 (d);
    T frac = d - z;
    
    assert (abs(frac) < T(1.0));
    
    if (frac > 0.5)
        ++z;
    else if (frac < -0.5)
        --z;
    else if (frac == 0.5 && (z&1) != 0) // NB: We also need the round-to-even rule.
        ++z;
    else if (frac == -0.5 && (z&1) != 0)
        --z;
    
    assert (abs(T(z) - d) < T(0.5) ||
              (abs(T(z) - d) == T(0.5) && ((z&1) == 0)));
    return z;
}




/*! \fn
 \brief compute median of a list. If length is odd it returns the center element in the sorted list, otherwise (i.e.
 * even length) it returns the average of the two center elements in the list
 \note the definition is identical to Mathematica
 * Partial_sort_copy copies the smallest Nelements from the range [first, last) to the range [result_first, result_first + N)
 * where Nis the smaller of last - first and result_last - result_first .
 * The elements in [result_first, result_first + N) will be in ascending order.
 */

template <class T>
double rfMedian (const vector<T>& a )
{
    const int32 length =  a.size();
    if ( length == 0 )
        return 0.0;
    
    const bool is_odd = length % 2;
    const int array_length = (length / 2) + 1;
    vector<T> array (array_length);
    
    
    
    partial_sort_copy(a.begin(), a.end(), array.begin(), array.end());
    
    if (is_odd)
        return double(array [array_length-1]);
    else
        return double( (array [array_length - 2] + array [array_length-1]) / T(2) );
}

template <class T>
double rfMedian (const deque<T>& a )
{
    const int32 length =  a.size();
    if ( length == 0 )
        return 0.0;
    
    const bool is_odd = length % 2;
    const int array_length = (length / 2) + 1;
    deque<T> array (array_length);
    
    
    
    partial_sort_copy(a.begin(), a.end(), array.begin(), array.end());
    
    if (is_odd)
        return double(array [array_length-1]);
    else
        return double( (array [array_length - 2] + array [array_length-1]) / T(2) );
}

template <class T>
double rfSum (const vector<T>& a )
{
    const int32 length =  a.size();
    if ( length == 0 )
        return 0.0;
    return (double) accumulate (a.begin(), a.end(), T(0));
}


template <class T>
double rfSum (const deque <T>& a )
{
    const int32 length =  a.size();
    if ( length == 0 )
        return 0.0;
    return (double) accumulate (a.begin(), a.end(), T(0));
}


template <class T>
double rfMean (const vector<T>& a )
{
    const int32 length =  a.size();
    if ( length == 0 )
        return 0.0;
    
    double s = rfSum (a);
    return s / (double) a.size();
}



template <class T>
T rfRMS (const vector<T>& a)
{
    rmAssert (a.size() > 1);
    
    // These typename definitions silence gcc compiler warnings
    typedef typename vector<T>::iterator iterator;
    typedef typename vector<T>::const_iterator const_iterator;
    
    const_iterator accPtr = a.begin();
    
    T sofar ( (T) 0);
    int32 i (0);
    for (; accPtr < a.end(); accPtr++, i++)
        sofar += rmSquare (*accPtr);
    return (T) (sqrt (sofar / (T) i));
}

template <class T>
void rfRMS (const vector<T>& a, vector<T>& b)
{
    if (a.size() < 1) return;
    rmAssert (a.size() == b.size());
    
    // These typename definitions silence gcc compiler warnings
    typedef typename vector<T>::iterator iterator;
    typedef typename vector<T>::const_iterator const_iterator;
    
    iterator fs = b.begin();
    const_iterator accPtr = a.begin();
    
    T sofar ( (T) 0);
    int32 i (1);
    for (; accPtr < a.end(); fs++, accPtr++, i++)
    {
        sofar += rmSquare (*accPtr);
        *fs = sqrt (sofar / (T) i);
    }
}

template <class T>
void rfcSum (const deque<T>& a, deque <T>& b)
{
    if (a.size() < 1) return;
    rmAssert (a.size() == b.size());
    
    // These typename definitions silence gcc compiler warnings
    typedef typename deque<T>::iterator iterator;
    typedef typename deque<T>::const_iterator const_iterator;
    
    iterator fs = b.begin();
    const_iterator accPtr = a.begin();
    
    T sofar ( (T) 0);
    int32 i (1);
    for (; accPtr < a.end(); fs++, accPtr++, i++)
    {
        sofar += (*accPtr);
        *fs = sofar;
    }
}


template <class T>
void rfProductOfrValues (const vector<T>& a, vector<T>& b)
{
    if (a.size() < 1) return;
    rmAssert (a.size() == b.size());
    
    // These typename definitions silence gcc compiler warnings
    typedef typename vector<T>::iterator iterator;
    typedef typename vector<T>::const_iterator const_iterator;
    
    iterator fs = b.begin();
    const_iterator accPtr = a.begin();
    
    T sofar ( (T) 1);
    int32 i (1);
    for (; accPtr < a.end(); fs++, accPtr++, i++)
    {
        sofar *= (*accPtr);
        *fs = sofar;
    }
}


#endif /* __r_MATH_H */
