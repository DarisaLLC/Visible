#ifndef __ASYNC_PRODUCER__
#define __ASYNC_PRODUCER__


#include <map>
#include <iostream>
#include <string>
#include <typeinfo>
#include <vector>
#include <sstream>
#include <typeindex>
#include <map>
#include <future>
#include "core/bitvector.h"
#include "timestamp.h"
#include "time_index.h"


// *****************
// *                *
// *  Tracks Async  *
// *                *
// *****************


typedef std::pair<index_time_t, double> timed_double_t;

template<typename R>

bool is_ready(std::future<R> const& f)

{ return f.valid() && f.wait_for(std::chrono::seconds(0)) == std::future_status::ready; }


typedef std::vector<timed_double_t>  timed_double_vec_t;
typedef std::future<timed_double_vec_t > async_timed_double_vec_t;

typedef std::pair<std::string, timed_double_vec_t> trackD1_t;
typedef std::vector<trackD1_t>  tracksD1_t;




// Steps:
// 1. create a global function generating the non-future vector
// 2. launch the async call: std::async(std::launch::async, generating function, args .... )
// 3. Check if the async result is ready. If it is copy it to the non-future.
// Using
// template<typename R>
// bool is_ready(std::future<R> const& f)

typedef std::future<std::vector<trackD1_t> > async_tracksD1_t;



#endif
