#ifndef OS_FIXES_HPP
#define OS_FIXES_HPP
/*------------------------------------------------------------------------------

  Author:    Andy Rushton
  Copyright: (c) Andy Rushton, 2004
  License:   BSD License, see ../docs/license.html

  Contains work arounds for OS or Compiler specific problems to try to make
  them look more alike

  It is strongly recommended that this header be included as the first
  #include in every source file

  ------------------------------------------------------------------------------*/

////////////////////////////////////////////////////////////////////////////////
// Problem with MicroSoft defining two different macros to identify Windows
////////////////////////////////////////////////////////////////////////////////

#if defined(_WIN32) || defined(_WIN32_WCE)
#define MSWINDOWS
#endif

////////////////////////////////////////////////////////////////////////////////
// Problems with unnecessary or unfixable compiler warnings
////////////////////////////////////////////////////////////////////////////////

#if defined(_MSC_VER)
// Microsoft Visual Studio
// shut up the following irritating warnings
//   4275 - VC6, exported class was derived from a class that was not exported
//   4786 - VC6, identifier string exceeded maximum allowable length and was truncated (only affects debugger)
//   4305 - VC6, identifier type was converted to a smaller type
//   4503 - VC6, decorated name was longer than the maximum the compiler allows (only affects debugger)
//   4309 - VC6, type conversion operation caused a constant to exceeded the space allocated for it
//   4290 - VC6, C++ exception specification ignored
//   4800 - VC6, forcing value to bool 'true' or 'false' (performance warning)
//   4675 - VC7.1, "change" in function overload resolution _might_ have altered program
//   4996 - VC8, 'xxxx' was declared deprecated
#pragma warning(disable: 4275 4786 4305 4503 4309 4290 4800 4675 4996)
#endif

#if defined(__BORLANDC__)
// Borland
// Shut up the following irritating warnings
//   8008 - Condition is always true.
//          Whenever the compiler encounters a constant comparison that (due to
//          the nature of the value being compared) is always true or false, it
//          issues this warning and evaluates the condition at compile time.
//   8060 - Possibly incorrect assignment.
//          This warning is generated when the compiler encounters an assignment
//          operator as the main operator of a conditional expression (part of
//          an if, while, or do-while statement). This is usually a
//          typographical error for the equality operator.
//   8066 - Unreachable code.
//          A break, continue, goto, or return statement was not followed by a
//          label or the end of a loop or function. The compiler chks while,
//          do, and for loops with a constant test condition, and attempts to
//          recognize loops that can't fall through.
#pragma warn -8008
#pragma warn -8060
#pragma warn -8066
#endif

////////////////////////////////////////////////////////////////////////////////
// Problems with redefinition of min/max in various different versions of library headers
////////////////////////////////////////////////////////////////////////////////

// The Windows headers define macros called max/min which conflict with the templates std::max and std::min.
// So, to avoid conflicts, MS removed the std::max/min rather than fixing the problem!
// From Visual Studio .NET (SV7, compiler version 13.00) the STL templates have been added correctly.
// This fix switches off the macros and reinstates the STL templates for earlier versions (SV6).
// Note that this could break MFC applications that rely on the macros (try it and see).

// For MFC compatibility, only undef min and max in non-MFC programs - some bits of MFC
// use macro min/max in headers. For VC7 both the macros and template functions exist
// so there is no real need for the undefs but to it anyway for consistency. So, if
// using VC6 and MFC then template functions will not exist

// I've created extra template function definitions minimum/maximum that avoid all the problems above

#if defined(_MSC_VER) && !defined(_MFC_VER)
#define NOMINMAX
#undef max
#undef min
// replace missing template definitions in VC6
#if defined(_MSC_VER) && (_MSC_VER < 1300)
namespace std 
{
  template<typename T> const T& max(const T& l, const T& r) {return l > r ? l : r;}
  template<typename T> const T& min(const T& l, const T& r) {return l < r ? l : r;}
}
#endif
#endif

template<typename T> const T& maximum(const T& l, const T& r) {return l > r ? l : r;}
template<typename T> const T& minimum(const T& l, const T& r) {return l < r ? l : r;}

////////////////////////////////////////////////////////////////////////////////
// Problem with missing __FUNCTION__ macro
////////////////////////////////////////////////////////////////////////////////
// this macro is used in debugging but was missing in Visual Studio prior to version 7
// it also has a different name in Borland

#if defined(_MSC_VER) && (_MSC_VER < 1300)
#define __FUNCTION__ 0
#endif

#if defined(__BORLANDC__)
#define __FUNCTION__ __FUNC__
#endif

////////////////////////////////////////////////////////////////////////////////
// Problems with differences between namespaces
////////////////////////////////////////////////////////////////////////////////

// problem in gcc pre-v3 where the sub-namespaces in std aren't present
// this mean that the statement "using namespace std::rel_ops" created an error because the namespace didn't exist

// I've done a fix here that creates an empty namespace for this case, but I
// do *not* try to move the contents of std::rel_ops into namespace std
// This fix only works if you use "using namespace std::rel_ops" to bring in the template relational operators (e.g. != defined i.t.o. ==)

#if defined(__GNUC__)
namespace std
{
  namespace rel_ops
  {
  }
}
#endif

////////////////////////////////////////////////////////////////////////////////
// Problems with the typename keyword
////////////////////////////////////////////////////////////////////////////////

// There are problems with using the 'typename' keyword. Technically, if you
// use a type member of a template class (i.e. a type declared within the
// template class by a local typedef), you need to tell the compiler that it
// is a type name. This is because the compiler cannot work out whether a
// member is a type, a method or a data field at compile time. However,
// support for the typename keyword has traditionally been incomplete in both
// gcc and Visual Studio. I have used macros to try to resolve this issue. The
// macros add the keyword for compiler versions that require it and omit it
// for compiler versions that do not support it

// There are five places where typename keywords cause problems:
//
//   1) in a typedef where a template class's member type is being mapped onto
//      a type definition within another template class or function 
//      e.g. template<typename T> fn () {
//             typedef typename someclass<T>::member_type local_type;
//                     ^^^^^^^^
//      Note that the typename keyword is only required when the type is of this form - a member of a template class
//      This situation is handled by the macro TYPEDEF_TYPENAME
//
//   2) in a function parameter declaration, with similar rules to the above
//      e.g. template<typename T> fn (typename someclass<T>::member_type)
//                                    ^^^^^^^^
//      Note that the typename keyword is only required when the type is of this form - a member of a template class
//      This situation is handled by the macro PARAMETER_TYPENAME
//
//   3) in instantiating a template, the parameter to the template, with similar rules to the above
//      e.g. template_class<typename someclass<T>::member_type>
//                          ^^^^^^^^
//      Note that the typename keyword is only required when the type is of this form - a member of a template class
//      This situation is handled by the macro TEMPLATE_TYPENAME
//   4) Return expressions
//      e.g. return typename ntree<T>::const_iterator(this,m_root);
//                  ^^^^^^^^
//      Note that this typename is only required when the return type is a member of a template class
//   5) Creating temporary objects when passing arguments to a function or constructor
//      e.g. return typename ntree<T>::const_prefix_iterator(typename ntree<T>::const_iterator(this,m_root));
//                                                           ^^^^^^^^
//      Note that the typename keyword is only required when the temporary's type is a member of a template class

// default values, overridden for individual problem cases below
#define TYPEDEF_TYPENAME typename
#define PARAMETER_TYPENAME typename
#define TEMPLATE_TYPENAME typename
#define RETURN_TYPENAME typename
#define TEMPORARY_TYPENAME typename

#if defined(__GNUC__)
// GCC 
//   - pre-version 3 didn't handle typename in any of these cases
//   - version 3 onwards, typename is required for all three cases as per default
#if __GNUC__ < 3
// gcc prior to v3
#undef TYPEDEF_TYPENAME
#define TYPEDEF_TYPENAME
#undef PARAMETER_TYPENAME
#define PARAMETER_TYPENAME
#undef TEMPLATE_TYPENAME
#define TEMPLATE_TYPENAME
#undef RETURN_TYPENAME
#define RETURN_TYPENAME
#undef TEMPORARY_TYPENAME
#define TEMPORARY_TYPENAME
#endif
#endif

#if defined(_MSC_VER)
// Visual Studio
//   - version 6 (compiler v.12) cannot handle typename in any of these cases
//   - version 7 (.NET) (compiler v.13) requires a typename in a parameter specification but supports all
//   - version 8 (2005) (compiler v.14) requires parameters and templates, supports all
#if _MSC_VER < 1300
// compiler version 12 and earlier
#undef TYPEDEF_TYPENAME
#define TYPEDEF_TYPENAME
#undef PARAMETER_TYPENAME
#define PARAMETER_TYPENAME
#undef TEMPLATE_TYPENAME
#define TEMPLATE_TYPENAME
#undef RETURN_TYPENAME
#define RETURN_TYPENAME
#undef TEMPORARY_TYPENAME
#define TEMPORARY_TYPENAME
#endif
#endif

////////////////////////////////////////////////////////////////////////////////
// problems with missing functions
////////////////////////////////////////////////////////////////////////////////

#if defined(_MSC_VER) || defined(__BORLANDC__)
unsigned sleep(unsigned seconds);
#else
#include <unistd.h>
#endif

////////////////////////////////////////////////////////////////////////////////
// Function for establishing endian-ness
////////////////////////////////////////////////////////////////////////////////
// Different machine architectures store data using different byte orders.
// This is referred to as Big- and Little-Endian Byte Ordering. 
//
// The issue is: where does a pointer to an integer type actually point?
//
// In both conventions, the address points to the left of the word but:
// Big-Endian - The most significant byte is on the left end of a word
// Little-Endian - The least significant byte is on the left end of a word
//
// Bytes are addressed left to right, so in big-endian order byte 0 is the
// msB, whereas in little-endian order byte 0 is the lsB. For example,
// Intel-based machines store data in little-endian byte order so byte 0 is
// the lsB.
//
// This function establishes byte order at run-time

bool little_endian(void);

////////////////////////////////////////////////////////////////////////////////
#endif
