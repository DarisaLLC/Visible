#ifndef __ALGO_FRAMES_IN_
#define __ALGO_FRAMES_IN_


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
#include "async_producer.h"


struct framesInTracksOut
{
    typedef std::shared_ptr<framesInTracksOut> framesInTracksOutRef;
    typedef std::function<bool (framesInTracksOutRef& )> algo_fn_t;
    typedef std::map<std::string, tracksD1_t> algo_output_tracks_t;
    
   
    static framesInTracksOutRef create (const std::string& name, std::shared_ptr<qTimeFrameCache>& frames, bool test_data = false)
    {
        auto thisref = std::make_shared<framesInTracksOut>();
        thisref->mName = name;
        thisref->mFrameSet = frames;
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
    time_spec_t mTimestamp;
    bitvector mDone;
    
    
};

#endif
