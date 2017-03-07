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


/*
 * Base class for one timed source in one timed source out
 *
 * A processing function is a template function of a derived
 * class of this and if image class is a templatized
 *
 */



template <class T, uint32_t C = 1>
class temporalAlgorithm
{
public:
    const static uint32_t instance_c = C;
    
    typedef temporalAlgorithm<T,C> algorithm_t;
    typedef std::shared_ptr<algorithm_t> Ref_t;
    typedef std::function<bool (float)> progress_fn_t;

    
    temporalAlgorithm () {}
    virtual ~temporalAlgorithm () {}
    virtual bool run () { assert(false); return false;}
    
    static temporalAlgorithm::Ref_t create (const std::string& name, std::shared_ptr<qTimeFrameCache>& frames, bool test_data = false)
    {
        auto thisref = std::make_shared<algorithm_t>();
        thisref->mName = name;
        thisref->mFrameSet = frames;
        thisref->mIs_test_data = test_data;
        thisref->mState = 0;
        thisref->mTimestamp = time_spec_t::get_system_time ();
        return thisref;
    }

    void clear () const { mState = 0; mOutput_tracks.clear (); }
    int state () const { return mState; }
    const track<T,C>& output () const { return mOutput_tracks; }
    bool done () { return (mDone.size() > 0 && mFrameSet && mDone.size() == mFrameSet->count()); }
    


protected:
    progress_fn_t mProgressReporter;
    
    std::string mName;
    std::shared_ptr<qTimeFrameCache> mFrameSet;
    mutable track<T,C> mOutput_tracks;
    
    bool mIs_test_data;
    mutable int mState;
    time_spec_t mTimestamp;
    bitvector mDone;
    

};
//
//template
//class temporalAlgorithm<double,1>;
//
//template
//class temporalAlgorithm<double,3>;

#endif
