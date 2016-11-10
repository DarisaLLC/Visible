#ifndef _STL_UTILS_HPP
#define _STL_UTILS_HPP

#include <ostream>
#include <vector>
#include <assert.h>
#include <map>
#include <string>
#include <sstream>
#include <iostream>
#include <memory>
#include <random>
#include <iomanip>
#include <fstream>
#include <stdio.h>


using namespace std;
struct null_deleter
{
  void operator() (void const *) const {}
};

template<typename C>
struct internal_null_deleter_t
{
  void operator() (const C *) const {}
};


template <typename PT> 
inline PT align(PT val, std::size_t alignment)
   { 
   return val+(alignment - val%alignment)%alignment; 
   }

template<typename PT>
struct iBuffer
   {
       typedef std::shared_ptr<PT> PTRef;
       
       PTRef _base;
       std::ptrdiff_t _diff;
       

   iBuffer (int capture_size, int alignment)
      {  
      _base  = PTRef (new PT [capture_size] );
      unsigned char* tmp=(alignment>0) ? (unsigned char*)align((std::size_t) _base.get(),alignment) : _base.get();
      _diff = (std::size_t)(tmp) -  (std::size_t) _base.get();
      assert (_diff >= 0);
      }

 
   };


template<class T>
std::string t_to_string(T i)
   {
   std::stringstream ss;
   std::string s;
   ss << i;
   s = ss.str();

   return s;
   }


// int r = signextend<signed int,5>(x);  // sign extend 5 bit number x to r
template <typename T, unsigned B>
inline T signextend(const T x)
   {
   struct {T x:B;} s;
   return s.x = x;
   }

template <class T>
bool from_string(T& t, const std::string& s, std::ios_base& (*f)(std::ios_base&))
{
  std::istringstream iss(s);
  return !(iss >> f >> t).fail ();
}

/**
 * RAIII (Resource Allocataion is Initialization) Exception safe handling of openning and closing of files. 
 * a functor object to delete an ifstream
 * utility functions to create
 */
struct ofStreamDeleter
{
  void operator()(std::ofstream* p) const
  { 
    if ( p != nullptr )
    {
      p->close();
      delete p;
    }
  }
};

struct ifStreamDeleter
{
    void operator()(std::ifstream* p) const
    {
        if ( p != nullptr )
        {
            p->close();
            delete p;
        }
    }
};


class FileCloser
{
public:
    void operator()(FILE* file)
    {
        if (file != 0)
            fclose (file);
    }
};


// Typically T = char 
template<typename T>
std::string stringifyFile(const std::string& file)
{
    std::ifstream t(file);
    std::string str;
    t.seekg(0, std::ios::end);
    str.reserve(t.tellg());
    t.seekg(0, std::ios::beg);
    str.assign((std::istreambuf_iterator<T>(t)), std::istreambuf_iterator<T>());
    return str;
}


#if 0
static std::string get_random_string (int length) 
   {
   static std::string chars(
      "abcdefghijklmnopqrstuvwxyz"
      "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
      "1234567890");

   std::random::random_device rng;
   boost::random::uniform_int_distribution<> index_dist(0, chars.size() - 1);
   std::vector<char> str_c;
   str_c.resize (length);
   for(int i = 0; i < length; ++i) str_c[i] = chars[index_dist(rng)];
   return std::string (&str_c[0]);

   }
#endif


template< class T >
std::ostream & operator << ( std::ostream & os, const std::vector< T > & v ) {
    for ( const auto & i : v ) {
        os << i << std::endl;
    }
    return os;
}


template<class A,class B>
inline ostream& operator<<(ostream& out, const map<A,B>& m)
{
    out << "{";
    for (typename map<A,B>::const_iterator it = m.begin();
         it != m.end();
         ++it) {
        if (it != m.begin()) out << ",";
        out << it->first << "=" << &it->second;
    }
    out << "}";
    return out;
}

template<class A,class B>
inline ostream& operator<<(ostream& out, const multimap<A,B>& m)
{
    out << "{{";
    for (typename multimap<A,B>::const_iterator it = m.begin();
         it != m.end();
         ++it) {
        if (it != m.begin()) out << ",";
        out << it->first << "=" << &it->second;
    }
    out << "}}";
    return out;
}

#endif
