#ifndef __ALGO_REGISTRY__
#define __ALGO_REGISTRY__


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
#include "async_producer.h"




//////////////   Algorithm Library //////////////////////////////////
//
//
//        //Usage Pattern

// Given these functions
//        void foo1 () {}
//        void foo2 () {}

//        using func1 = algo_cb_t<>;
//        std::unique_ptr<func1> f1(new func1(&foo1));
//        using func2 = algo_cb_t<int>;
//        std::unique_ptr<func2> f2(new func2(&foo2));
//
//        // Add the to the map.
//        std::type_index index1(typeid(f1));
//        std::type_index index2(typeid(f2));
//        m_algo_callbacks.insert(callbacks_t::value_type(index1, std::move(f1)));
//        m_algo_callbacks.insert(callbacks_t::value_type(index2, std::move(f2)));
//
//        call(index1);
//        call(index2, 5);
//
/////////////////////////////////////////////////////////////////////


// The base type that is stored in the collection.
struct algo_func_t {
    virtual ~algo_func_t() = default;
};

// Derived representing a callback with its associated argument(s)
template<typename ...A>
struct algo_cb_t : public algo_func_t {
    using cb = std::function<bool(A...)>;
    cb callback;
    algo_cb_t(cb p_callback) : callback(p_callback) {}
};

class algo_library : public SingletonLight <algo_library>
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
