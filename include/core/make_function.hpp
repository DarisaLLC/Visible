#ifndef __MAKE_FUNCTION__
#define __MAKE_FUNCTION__

#include <functional>

namespace functional {
template <typename Function> struct function_traits;

template <typename ClassType, typename ReturnType, typename... Args>
struct function_traits<ReturnType(ClassType::*)(Args...) const> {
  using function = const std::function<ReturnType(Args...)>;
};

// Non-const version, to be used for function objects with a non-const operator()
// a rare thing
template <typename ClassType, typename ReturnType, typename... Args>
struct function_traits<ReturnType(ClassType::*)(Args...)> {
  using function = std::function<ReturnType(Args...)>;
};

template<typename T>
auto make_function(T const& f) ->
typename std::enable_if<std::is_function<T>::value && !std::is_bind_expression<T>::value, std::function<T>>::type
{ return f; }

template<typename T>
auto make_function(T const& f) ->
typename std::enable_if<!std::is_function<T>::value && !std::is_bind_expression<T>::value, typename function_traits<decltype(&T::operator())>::function>::type
{ return static_cast<typename function_traits<decltype(&T::operator())>::function>(f); }

// This overload is only used to display a clear error message in this case
// A bind expression supports overloads so its impossible to determine
// the corresponding std::function since several are viable
template<typename T>
auto make_function(T const& f) ->
typename std::enable_if<std::is_bind_expression<T>::value, void>::type
{ static_assert(std::is_bind_expression<T>::value && false, "functional::make_function cannot be used with a bind expression."); }

}  // namespace functional

#endif


#if 0

int func(int x, int y, int z) { return x + y + z;}
int overloaded(char x, int y, int z) { return x + y + z;}
int overloaded(int x, int y, int z) { return x + y + z;}
struct functionoid {
    int operator()(int x, int y, int z) { return x + y + z;}
};
struct functionoid_overload {
    int operator()(int x, int y, int z) { return x + y + z;}
    int operator()(char x, int y, int z) { return x + y + z;}
};
int first = 0;
auto lambda = [](int x, int y, int z) { return x + y + z;};
auto lambda_state = [=](int x, int y, int z) { return x + y + z + first;};
auto bound = std::bind(func,_1,_2,_3);


std::function<int(int,int,int)> f0 = make_function(func); assert(f0(1,2,3)==6);
std::function<int(char,int,int)> f1 = make_function<char,int,int>(overloaded); assert(f1(1,2,3)==6);
std::function<int(int,int,int)> f2 = make_function<int,int,int>(overloaded); assert(f2(1,2,3)==6);
std::function<int(int,int,int)> f3 = make_function(lambda); assert(f3(1,2,3)==6);
std::function<int(int,int,int)> f4 = make_function(lambda_state); assert(f4(1,2,3)==6);
std::function<int(int,int,int)> f5 = make_function<int,int,int>(bound); assert(f5(1,2,3)==6);
std::function<int(int,int,int)> f6 = make_function(functionoid{}); assert(f6(1,2,3)==6);
std::function<int(int,int,int)> f7 = make_function<int,int,int>(functionoid_overload{}); assert(f7(1,2,3)==6);
std::function<int(char,int,int)> f8 = make_function<char,int,int>(functionoid_overload{}); assert(f8(1,2,3)==6);



#endif
