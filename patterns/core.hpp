#ifndef _CORE_HPP
#define _CORE_HPP

#include <memory>
#include <atomic>
# include <assert.h> // .h to support old libraries w/o <cassert> - effect is the same

namespace core  // protection from unintended ADL
{
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
            core::checked_delete(x);
        }
    };

    template<class T> struct checked_array_deleter
    {
        typedef void result_type;
        typedef T * argument_type;

        void operator()(T * x) const
        {
            core::checked_array_delete(x);
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
                core::checked_delete(static_cast<const T*>(s));
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
#ifdef _WIN32
    // EG: Commented out for Linux build. Someone please fix 
    inline bool RealEq(double x, double y, double epsilon = 1.e-15)
    {
        return std::abs(x - y) <= epsilon;
    }

    inline bool RealEq(float x, float y, float epsilon = 1.e-10)
    {
        return std::abs(x - y) <= epsilon;
    }
#endif
#define MedOf3(a, b, c)                                       \
((a) > (b) ? ((a) < (c) ? (a) : ((b) < (c) ? (c) : (b))) :    \
 ((a) > (c) ? (a) : ((b) < (c) ? (b) : (c))))

#define MaxOf3(a, b, c) (std::max(std::max(a, b), c))

#define MinOf3(a, b, c) (std::min(std::min(a, b), c))


}



#endif
