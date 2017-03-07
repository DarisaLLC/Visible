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

using namespace svl;

/*
 * Base class for one timed source in one timed source out
 *
 * A processing function is a template function of a derived
 * class of this and if image class is a templatized
 *
 */

template<typename P, typename T, uint32_t N>
class temporalAlgorithm
{
public:
    typedef track<T,N> track_t;
    typedef PixelType<P> pixel_type_t;
    typedef PixelLayout<P> pixel_layout_t;

    typedef temporalAlgorithm<P,T,N> algorithm_t;
    typedef std::shared_ptr<algorithm_t> Ref_t;
    typedef std::function<bool (float)> progress_fn_t;

    
    temporalAlgorithm ()
    {
        mNames.resize (pixel_layout_t::channels ());
    }
    
    virtual ~temporalAlgorithm () {}
    virtual bool run () = 0; //{return false; } // { assert(false); return false;}
    

    void clear () const { mState = 0; mOutput_tracks.clear (); mDone.clear(); }
    int state () const { return mState; }
    const std::vector<track_t>& output () const { return mOutput_tracks; }
    bool done () { return (mDone.size() > 0 && mFrameSet && mDone.size() == mFrameSet->count()); }
    


protected:
    progress_fn_t mProgressReporter;
    std::vector<std::string> mNames;
    std::shared_ptr<qTimeFrameCache> mFrameSet;
    mutable std::vector<track_t> mOutput_tracks;
    
    bool mIs_test_data;
    mutable int mState;
    time_spec_t mTimestamp;
    mutable std::vector<uint8_t> mDone;
    

};



#endif
