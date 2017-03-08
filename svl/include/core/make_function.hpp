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
