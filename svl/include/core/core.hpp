/** @dir core
 * @brief Namespace @ref svl
 */
/** @namespace svl
 * @brief The Simple Vision Library
 */


#ifndef _CORE_HPP
#define _CORE_HPP

#include <memory>
#include <atomic>
#include <cmath>
#include <deque>
#include <iostream>
#include <sstream>
# include <assert.h> // .h to support old libraries w/o <cassert> - effect is the same
#include "coreGLM.h"
#include <random>
#include <chrono>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcomma"

namespace svl // protection from unintended ADL
{

template<typename T>
std::string toString( const T &t ) { std::ostringstream ss; ss << t; return ss.str(); }

template<typename T>
T fromString( const std::string &s ) { std::stringstream ss; ss << s; T temp; ss >> temp; return temp; }

template<>
inline std::string fromString( const std::string &s ) { return s; }


// Sigmoid Clamping
template<class T>
T sigmoid_clip(T x, T min, T max) {
    return ((max-min)/2)*((exp(x) - exp(-x))/(exp(x) + exp(-x))) + max - (max-min)/2;
}

inline bool Zero(const double& a_value, double epsilon = 10e-10)
{
    return ((a_value < epsilon) && (a_value > -epsilon));
}



template<class T> inline T Abs(const T a_value)
{
    return (a_value >= 0 ? a_value : -a_value);
}


#define MedOf3(a, b, c)                                       \
((a) > (b) ? ((a) < (c) ? (a) : ((b) < (c) ? (c) : (b))) :    \
((a) > (c) ? (a) : ((b) < (c) ? (b) : (c))))

#define MaxOf3(a, b, c) (std::max(std::max(a, b), c))

#define MinOf3(a, b, c) (std::min(std::min(a, b), c))

template<class T> inline void Swap(T &a_value1, T &a_value2)
{
    T value = a_value1;
    a_value1 = a_value2;
    a_value2 = value;
}

template<class T> inline T LinearTerp(const double& a_level, const T& a_value1, const T& a_value2)
{
    return (a_value2 * a_level + a_value1 * (1 - a_level));
}


template<class T> inline T Clamp(const T a_value, const T a_low, const T a_high)
{
    return (a_value < a_low ? a_low : a_value > a_high ? a_high : a_value);
}

template<class T> inline T ClampZero(T& a_value)
{
    return std::max<T>(0, a_value);
}


inline double ClampZeroOne(double& a_value)
{
    return (Clamp(a_value, 0.0, 1.0));
}


template<class T, class V> inline bool Contains(const T& a_value, const V& a_low, const V& a_high)
{
    return ((a_value >= a_low) && (a_value <= a_high));
}



inline double Sqr(const double& a_value)
{
    return(a_value*a_value);
}



// http://stackoverflow.com/a/6245777
template<std::size_t...> struct seq{};

template<std::size_t N, std::size_t... Is>
struct gen_seq : gen_seq<N-1, N-1, Is...>{};

template<std::size_t... Is>
struct gen_seq<0, Is...> : seq<Is...>{};

template<class Ch, class Tr, class Tuple, std::size_t... Is>
void print_tuple(std::basic_ostream<Ch,Tr>& os, Tuple const& t, seq<Is...>){
    using swallow = int[];
    (void)swallow{0, (static_cast<void>(void(os << (Is == 0? "" : ", ") << std::get<Is>(t))), 0)...};
}


template<class Ch, class Tr, class... Args>
auto operator<<(std::basic_ostream<Ch, Tr>& os, std::tuple<Args...> const& t)-> std::basic_ostream<Ch, Tr>&{
    os << "(";
    svl::print_tuple(os, t, svl::gen_seq<sizeof...(Args)>());
    return os << ")";
    }
    
    
    
    // Need this useless class for explicitly instatiating template constructor
    template <typename T>
    struct tid {
        typedef T type;
    };
    
#if defined(IS_NUMBER_USED)
    // c++11
    static bool is_number(const std::string& s)
    {
        return !s.empty() && std::find_if(s.begin(),
                                          s.end(), [](char c) { return !std::isdigit(c); }) == s.end();
    }
#endif
    
#if have_boost_random
    static std::string default_random_string_char_set ("abcdefghijklmnopqrstuvwxyz"
                                                       "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                                       "1234567890"
                                                       "!@#$%^&*()"
                                                       "`~-_=+[{]}\\|;:'\",<.>/? ");
    
    std::string random_string (unsigned size = 8, const std::string chars = default_random_string_char_set)
    {
        boost::random::random_device rng;
        boost::random::uniform_int_distribution<> index_dist(0, chars.size() - 1);
        std::stringstream sstream;
        for(int i = 0; i < size; ++i) {
            sstream << chars[index_dist(rng)];
        }
        return sstream.str();
    }
#endif
    
    template<typename T> T
    randomT ()
    {
        // obtain a seed from the system clock:
        auto seed = std::chrono::system_clock::now().time_since_epoch().count();
        
        std::default_random_engine generator (static_cast<unsigned int>(seed));
        
        return std::generate_canonical<T,std::numeric_limits<T>::digits>(generator);
        
    }
    
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
    
    template <typename Iter, typename T = typename std::iterator_traits<Iter>::value_type>
    static std::pair<T,T> norm_min_max (Iter begin, Iter endd, bool in_place = true)
    {
        std::pair<Iter,Iter> min_max_iter = std::minmax_element (begin, endd);
        T max_val = *min_max_iter.second;
        T min_val = *min_max_iter.first;
        std::pair<T,T> extr (min_val, max_val);
        if (max_val != T(0))
        {
            T scale = max_val - min_val;
            for (auto iit = begin; iit != endd; iit++)
            {
                T val = *iit;
                if (in_place)
                    *iit = (val - min_val) / scale;
            }
        }
        return std::pair<T,T>(min_val, max_val);
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
                checked_array_delete  (static_cast<const T*>(s));
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
        xp = Sigmoid (xp);
        
        // Transform [-1 1] back to [0 1]
        xp = (xp + 1) / (T) 2;
        
        return xp;
    }
    
    
    template<class T>
    bool Quadradic (T a, T b, T c, T& pos, T& neg)
    {
        T det = (b*b) - (4 * a * c);
        
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
        
        
#pragma GCC diagnostic pop
        
#endif
