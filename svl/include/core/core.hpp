#ifndef _CORE_HPP
#define _CORE_HPP

#include <memory>
#include <atomic>
#include <cmath>
# include <assert.h> // .h to support old libraries w/o <cassert> - effect is the same
#include "coreGLM.h"


namespace svl  // protection from unintended ADL
{
    
    template<typename T>
    struct math
    {
        static T	acos  (T x)		{return ::acos (double(x));}
        static T	asin  (T x)		{return ::asin (double(x));}
        static T	atan  (T x)		{return ::atan (double(x));}
        static T	atan2 (T y, T x)	{return ::atan2 (double(y), double(x));}
        static T	cos   (T x)		{return ::cos (double(x));}
        static T	sin   (T x)		{return ::sin (double(x));}
        static T	tan   (T x)		{return ::tan (double(x));}
        static T	cosh  (T x)		{return ::cosh (double(x));}
        static T	sinh  (T x)		{return ::sinh (double(x));}
        static T	tanh  (T x)		{return ::tanh (double(x));}
        static T	exp   (T x)		{return ::exp (double(x));}
        static T	log   (T x)		{return ::log (double(x));}
        static T	log10 (T x)		{return ::log10 (double(x));}
        static T	modf  (T x, T *iptr)
        {
            double ival;
            T rval( ::modf (double(x),&ival));
            *iptr = ival;
            return rval;
        }
        static T	pow   (T x, T y)	{return ::pow (double(x), double(y));}
        static T	sqrt  (T x)		{return ::sqrt (double(x));}
#if defined( _MSC_VER )
        static T	cbrt( T x )		{ return ( x > 0 ) ? (::pow( x, 1.0 / 3.0 )) : (- ::pow( -x, 1.0 / 3.0 ) ); }
#else
        static T	cbrt( T x )		{ return ::cbrt( x ); }
#endif
        static T	ceil  (T x)		{return ::ceil (double(x));}
        static T	abs  (T x)		{return ::fabs (double(x));}
        static T	floor (T x)		{return ::floor (double(x));}
        static T	fmod  (T x, T y)	{return ::fmod (double(x), double(y));}
        static T	hypot (T x, T y)	{return ::hypot (double(x), double(y));}
        static T	signum (T x)		{return ( x >0.0 ) ? 1.0 : ( ( x < 0.0 ) ? -1.0 : 0.0 ); }
        static T	min(T x, T y)				{return ( x < y ) ? x : y; }
        static T	max(T x, T y)				{return ( x > y ) ? x : y; }
        static T	clamp(T x, T min=0, T max=1)	{return ( x < min ) ? min : ( ( x > max ) ? max : x );}
    };
    
    
    template<>
    struct math<float>
    {
        static float	acos  (float x)			{return ::acosf (x);}
        static float	asin  (float x)			{return ::asinf (x);}
        static float	atan  (float x)			{return ::atanf (x);}
        static float	atan2 (float y, float x)	{return ::atan2f (y, x);}
        static float	cos   (float x)			{return ::cosf (x);}
        static float	sin   (float x)			{return ::sinf (x);}
        static float	tan   (float x)			{return ::tanf (x);}
        static float	cosh  (float x)			{return ::coshf (x);}
        static float	sinh  (float x)			{return ::sinhf (x);}
        static float	tanh  (float x)			{return ::tanhf (x);}
        static float	exp   (float x)			{return ::expf (x);}
        static float	log   (float x)			{return ::logf (x);}
        static float	log10 (float x)			{return ::log10f (x);}
        static float	modf  (float x, float *y)	{return ::modff (x, y);}
        static float	pow   (float x, float y)	{return ::powf (x, y);}
        static float	sqrt  (float x)			{return ::sqrtf (x);}
#if defined( _MSC_VER )
        static float	cbrt( float x )		{ return ( x > 0 ) ? (::powf( x, 1.0f / 3.0f )) : (- ::powf( -x, 1.0f / 3.0f ) ); }
#else
        static float	cbrt  (float x)			{ return ::cbrtf( x ); }
#endif
        static float	ceil  (float x)			{return ::ceilf (x);}
        static float	abs   (float x)			{return ::fabsf (x);}
        static float	floor (float x)			{return ::floorf (x);}
        static float	fmod  (float x, float y)	{return ::fmodf (x, y);}
#if !defined(_MSC_VER)
        static float	hypot (float x, float y)	{return ::hypotf (x, y);}
#else
        static float hypot (float x, float y)	{return ::sqrtf(x*x + y*y);}
#endif
        static float signum (float x)		{return ( x > 0.0f ) ? 1.0f : ( ( x < 0.0f ) ? -1.0f : 0.0f ); }
        static float min(float x, float y)					{return ( x < y ) ? x : y; }
        static float max(float x, float y)					{return ( x > y ) ? x : y; }
        static float clamp(float x, float min=0, float max=1)	{return ( x < min ) ? min : ( ( x > max ) ? max : x );}
    };
    
    
    
    template<typename Iter>
    inline
    bool contains_nan (Iter start, Iter end)
    {
        while (start != end)
        {
            if (std::isnan(*start)) return true;
            ++start;
        }
        return false;
    }
    
    template<typename T> bool
    equal (T val1, T val2, T eps = std::numeric_limits<T>::epsilon() )
    {
        return (fabs (val1 - val2) < eps);
    }
    
    
    
    // Rounding Templates and Specialization
    
    template <class S, class D>
    inline D uRoundPlus(S val, D) { return D(val); }
    
    template <class S, class D>
    inline D uRound(S val, D) { return D(val); }
    
    inline int32_t  uRoundPlus(double val, int32_t)  { return int32_t(val + 0.5); }
    inline int32_t  uRoundPlus(float val,  int32_t)  { return int32_t(val + 0.5); }
    
    inline int32_t  uRoundNeg(double val, int32_t)  { return int32_t(val -  0.5); }
    inline int32_t  uRoundNeg(float val,  int32_t)  { return int32_t(val - 0.5); }
    
    inline int32_t uRound(double val, int32_t Dummy)
    {
        return val < 0 ? -uRoundPlus(-val, Dummy) : uRoundPlus(val, Dummy);
    }
    
    inline int32_t uRound(float val, int32_t Dummy)
    {
        return val < 0 ? -uRoundPlus(-val, Dummy) : uRoundPlus(val, Dummy);
    }
    
    
    class noncopyable
    {
    protected:
        noncopyable() = default;
        ~noncopyable() = default;
    private:
        noncopyable(const noncopyable&);
        noncopyable& operator=(const noncopyable&);
    };
    
    
    // verify that types are complete for increased safety
    
    template<class T> inline void checked_delete(T * x)
    {
        // intentionally complex - simplification causes regressions
        typedef char type_must_be_complete[sizeof(T) ? 1 : -1];
        (void) sizeof(type_must_be_complete);
        delete x;
    }
    
    template<class T> inline void checked_array_delete(T * x)
    {
        typedef char type_must_be_complete[sizeof(T) ? 1 : -1];
        (void) sizeof(type_must_be_complete);
        delete[] x;
    }
    
    template<class T> struct checked_deleter
    {
        typedef void result_type;
        typedef T * argument_type;
        
        void operator()(T * x) const
        {
            // ds4boost:: disables ADL
            checked_delete(x);
        }
    };
    
    template<class T> struct checked_array_deleter
    {
        typedef void result_type;
        typedef T * argument_type;
        
        void operator()(T * x) const
        {
            checked_array_delete(x);
        }
    };
    
    
    
    template<typename T>
    struct refcounted_ptr
    {
        typedef refcounted_ptr<T> this_type;
        
        refcounted_ptr() : m_refs(0) {}
        refcounted_ptr(const refcounted_ptr&) : m_refs(0) {}
        refcounted_ptr& operator=(const refcounted_ptr&) { return *this; }
        
        friend void intrusive_ptr_add_ref(this_type const* s)
        {
            assert(s != 0);
            assert(s->m_refs >= 0);
            ++s->m_refs;
        }
        
        friend void intrusive_ptr_release(this_type const* s)
        {
            assert(s != 0);
            assert(s->m_refs > 0);
            
            if (--s->m_refs == 0)
                checked_delete(static_cast<const T*>(s));
        }
        
        int64_t refcount() const { return m_refs; }
        
        // reference counter for intrusive_ptr
        mutable std::atomic_uintmax_t m_refs;
    };
    
    
    //! Returns the number closest to \a val and between \a bound1 and \a bound2.
    /*! The bounds may be specified in either order (i.e., either (lo, hi) or (hi, lo). */
    
    template <class T, class T2>
    inline T clampValue(const T& val, const T2& bound1, const T2& bound2)
    {
        if (bound1 < bound2)
        {
            if (val < bound1) return bound1;
            if (val > bound2) return bound2;
            return val;
        }
        else
        {
            if (val < bound2) return bound2;
            if (val > bound1) return bound1;
        }
        return val;
    }
    
    inline bool RealEq(double x, double y, double epsilon = 1.e-15)
    {
        return std::fabs(x - y) <= epsilon;
    }
    
    inline bool RealEq(float x, float y, float epsilon = 1.e-10)
    {
        return std::fabs(x - y) <= epsilon;
    }
    
#define MedOf3(a, b, c)                                       \
((a) > (b) ? ((a) < (c) ? (a) : ((b) < (c) ? (c) : (b))) :    \
((a) > (c) ? (a) : ((b) < (c) ? (b) : (c))))
    
#define MaxOf3(a, b, c) (std::max(std::max(a, b), c))
    
#define MinOf3(a, b, c) (std::min(std::min(a, b), c))
    
    template<class T>
    T Sigmoid(T x)
    {
        return ((T) 1.0)/((T) 1.0 + exp(-x));
    }
    
    template<class T>
    T NormPositiveSigmoid(T x)
    {
        // Transform x [0 1] to [-1 1]
        T xp = x + x + (T) (-1);
        xp = rfSigmoid (xp);
        
        // Transform [-1 1] back to [0 1]
        xp = (xp + 1) / (T) 2;
        
        return xp;
    }
    
    
    template<class T>
    bool Quadradic (T a, T b, T c, T& pos, T& neg)
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
    
}



#endif
