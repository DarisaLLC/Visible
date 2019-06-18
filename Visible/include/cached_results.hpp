#ifndef __CACHED_REGISTRY__
#define __CACHED_REGISTRY__


#include <map>
#include <iostream>
#include <string>
#include <typeinfo>
#include <vector>
#include <sstream>
#include <typeindex>
#include <map>
#include <future>
#include "core/singleton.hpp"
#include "timed_value_containers.h"





// The base type that is stored in the collection.
struct algo_func_t {
    virtual ~algo_func_t() = default;
};

// Derived representing a callback with its associated argument(s)
template<typename ...A>
struct algo_cb_t : public algo_func_t {
    using cb = std::function<timedVal_t(A...)>;
    cb callback;
    algo_cb_t(cb p_callback) : callback(p_callback) {}
};

class cached_results : public SingletonLight <cached_results>
{
public:
    typedef std::map<std::type_index, std::unique_ptr<algo_func_t>> callbacks_t;
    
        algo_library ()
    {
        
    }
    
    template<typename T>
    std::type_index add (std::unique_ptr<T>& algoFn)
    {
        std::type_index index1(typeid(algoFn));
        m_algo_callbacks.insert(callbacks_t::value_type(index1, std::move(algoFn)));
        return index1;
    }
    
    template<typename ...A>
    bool call(std::type_index index, A&& ... args)
    {
        using func_t = algo_cb_t<A...>;
        using cb_t = std::function<bool(A...)>;
        const algo_func_t& base = *m_algo_callbacks[index];
        const cb_t& fun = static_cast<const func_t&>(base).callback;
        return fun(std::forward<A>(args)...);
    }
    
    
    void clear ()
    {
        m_algo_callbacks.clear ();
    }
    
private:
    
    callbacks_t m_algo_callbacks;

};



#endif
