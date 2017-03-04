#ifndef __UT_UTIL__
#define __UT_UTIL__


#include <iostream>
#include <string>
#include <functional>
#include "core/core.hpp"
#include "core/make_function.hpp"

//using namespace ci;
using namespace std;
using functional::make_function;


template<typename Return, typename... Args>
int func(std::function<Return(Args...)> f)
{
    return sizeof...(Args);
}

int f2(int a, int b, int c) { return a+b+c; }

struct t3 {
    int operator() (int a, int b, int c, int d) { return a+b-c-d; }
};

struct t4 {
    int roger() { return 0; }
};



struct make_function_test
{
    static int run ()
    {
        t3 test;
        t4 test2;
        
        std::cout << func(make_function(f2)) << "\n";
        std::cout << func(make_function([](int a, int b) { return a+b; })) << "\n";
        std::cout << func(make_function(test)) << "\n";
        
        auto bind_expr = std::bind(&t4::roger, test2);
        std::function<int()> test3 = bind_expr;
        std::cout << func(make_function(test3)) << "\n";
        //std::cout << func(make_function(bind_expr)) << "\n";  // static_assert blocks this
        return 0;

    }
};




#endif

