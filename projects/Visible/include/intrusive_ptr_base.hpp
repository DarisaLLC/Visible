
#ifndef __INTRUSIVE_PTR__
#define __INTRUSIVE_PTR__

#include <boost/detail/atomic_count.hpp>
#include <boost/checked_delete.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/assert.hpp>


template <typename T>
inline void intrusive_ptr_release (T* p)
{
    boost::checked_delete(static_cast<T const*>(p));
}



template<class T>
struct intrusive_ptr_base
{
    intrusive_ptr_base(intrusive_ptr_base<T> const&)
    : m_refs(0) {}
    
    intrusive_ptr_base& operator=(intrusive_ptr_base const& rhs)
    { return *this; }
    
    friend uint32_t intrusive_ptr_add_ref(intrusive_ptr_base<T> const* s)
    {
        return s ? ++s->m_refs : 0;
    }
    
    friend uint32_t intrusive_ptr_ref_count(intrusive_ptr_base<T> const* s)
    {
        return s ? s->m_refs : 0;
    }

    
    friend uint32_t intrusive_ptr_rem_ref (intrusive_ptr_base<T> const* s)
    {
         return s ? --s->m_refs : 0;
    }
    
    boost::intrusive_ptr<T> self()
    { return boost::intrusive_ptr<T>((T*)this); }
    
    boost::intrusive_ptr<const T> self() const
    { return boost::intrusive_ptr<const T>((T const*)this); }
    
    int refcount() const { return m_refs; }
    
    intrusive_ptr_base(): m_refs(0) {}
private:
    // reference counter for intrusive_ptr
    mutable boost::detail::atomic_count m_refs;
};




#endif
