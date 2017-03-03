#ifndef __ASYNC_PRODUCER__
#define __ASYNC_PRODUCER__


#include <map>
#include <iostream>
#include <string>
#include <typeinfo>
#include <vector>
#include <sstream>
#include <map>
#include <future>
#include "core/bitvector.h"

#include "timestamp.h"
#include "time_index.h"
#include "qtime_frame_cache.hpp"



// *****************
// *                *
// *  Algo IO & Run *
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
typedef std::future<std::vector<trackD1_t> > async_tracksD1_t;

typedef std::map<std::string, tracksD1_t> algo_output_tracks_t;


// Steps:
// 1. create a global function generating the non-future vector
// 2. launch the async call: std::async(std::launch::async, generating function, args .... )
// 3. Check if the async result is ready. If it is copy it to the non-future.
// Using
// template<typename R>
// bool is_ready(std::future<R> const& f)



struct algoIO
{
    typedef std::function<bool (algoIO& )> algo_fn_t;
    typedef std::shared_ptr<algoIO> algoIORef;
    
   
    static algoIORef create (const std::string& name, algo_fn_t algo, std::shared_ptr<qTimeFrameCache>& frames, bool test_data = false)
    {
        auto thisref = std::make_shared<algoIO>();
        thisref->mName = name;
        thisref->mFrameSet = frames;
        thisref->mAlgo_fn = algo;
        thisref->mIs_test_data = test_data;
        thisref->mState = 0;
        thisref->mTimestamp = time_spec_t::get_system_time ();
        return thisref;
    }
    // Accessors

    int state () const { return mState; }
    tracksD1_t output () const { return mOutput_tracks; }
    bool done () { return (mDone.size() > 0 && mFrameSet && mDone.size() == mFrameSet->count()); }
    


    std::string mName;
    std::shared_ptr<qTimeFrameCache> mFrameSet;
    tracksD1_t mOutput_tracks;
    bool mIs_test_data;
    int mState;
    algo_fn_t mAlgo_fn;
    time_spec_t mTimestamp;
    bitvector mDone;
    
    
};

#endif
