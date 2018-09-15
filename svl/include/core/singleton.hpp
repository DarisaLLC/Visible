#ifndef SINGLETON_HPP
#define SINGLETON_HPP

#include <memory>
#include <atomic>
#include "core.hpp"


namespace svl
{
#define SINGLETONPTR(Typename)                     \
  static Typename* instance() {                 \
    static Typename e;                          \
    return &e;                                  \
  }


template <typename Derived>
class SingletonLight
{
public:
    static Derived & instance()
    {
        static Derived instance;
        return instance;
    }

protected:
    SingletonLight() {}
    ~SingletonLight() {}

private:
    SingletonLight(const SingletonLight &) = delete;
    SingletonLight(SingletonLight &&) = delete;
    SingletonLight & operator=(const SingletonLight &) = delete;
    SingletonLight & operator=(SingletonLight &&) = delete;
};




class singleton_module : 
public svl::noncopyable
   {
   private:
      static bool & get_lock(){
         static bool lock = false;
         return lock;
         }
   public:
      //    static const void * get_module_handle(){
      //        return static_cast<const void *>(get_module_handle);
      //    }
      static void lock(){
         get_lock() = true;
         }
      static void unlock(){
         get_lock() = false;
         }
      static bool is_locked() {
         return get_lock();
         }
   };

namespace core_detail {

   template<class T>
   class singleton_wrapper : public T
      {
      public:
         static bool m_is_destroyed;
         ~singleton_wrapper(){
            m_is_destroyed = true;
            }
      };

   template<class T>
   bool core_detail::singleton_wrapper< T >::m_is_destroyed = false;

   } // detail

template <class T>
class internal_singleton : public singleton_module
   {
   private:
      static T & m_instance;
      // include this to provoke instantiation at pre-execution time
      static void use(T const &) {}
      static T & get_instance() {
         static core_detail::singleton_wrapper< T > t;
         // refer to instance, causing it to be instantiated (and
         // initialized at startup on working compilers)
     //    assert (! detail::singleton_wrapper< T >::m_is_destroyed);
         use(m_instance);
         return static_cast<T &>(t);
         }
   public:
      static T & get_mutable_instance(){
         assert (! is_locked());
         return get_instance();
         }
      static const T & instance(){
         return get_instance();
         }
      static bool is_destroyed(){
         return core_detail::singleton_wrapper< T >::m_is_destroyed;
         }
   };

template<class T>
 T & internal_singleton< T >::m_instance = internal_singleton< T >::get_instance();

}

#endif
