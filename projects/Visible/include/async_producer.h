#ifndef __ASYNC_PRODUCER__
#define __ASYNC_PRODUCER__


#include <map>
#include <iostream>
#include <string>
#include <typeinfo>
#include <vector>
#include <array>
#include <sstream>
#include <typeindex>
#include <map>
#include <future>
#include "core/bitvector.h"
#include "timestamp.h"
#include "time_index.h"


// *****************
// *                *
// *  Tracks        *
// *                *
// *****************

// Begining of track types. For now, just a sequence of index/time and doubles

typedef std::vector<index_time_t>  index_time_vec_t;

typedef std::pair<index_time_t, double> timed_double_t;

template<typename T, uint32_t N = 1>
class track : public std::pair<index_time_vec_t, std::vector<std::array<T,N>>>
{
public:
    typedef std::array<T,N> result_array_t;
    inline static uint32_t step () { return N; }
    track () { clear (); }
    void clear () { this->first.resize(0); this->second.resize(0); }
    
};



template<typename R>
bool is_ready(std::future<R> const& f)

{ return f.valid() && f.wait_for(std::chrono::seconds(0)) == std::future_status::ready; }


typedef std::vector<timed_double_t>  timed_double_vec_t;

typedef std::future<timed_double_vec_t > async_timed_double_vec_t;
typedef std::pair<std::string, timed_double_vec_t> trackD1_t;
typedef std::vector<trackD1_t>  tracksD1_t;



// *****************
// *                *
// *  Async         *
// *                *
// *****************


// Steps:
// 1. create a global function generating the non-future vector
// 2. launch the async call: std::async(std::launch::async, generating function, args .... )
// 3. Check if the async result is ready. If it is copy it to the non-future.
// Using
// template<typename R>
// bool is_ready(std::future<R> const& f)

typedef std::future<std::vector<trackD1_t> > async_tracksD1_t;



#endif
