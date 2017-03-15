#ifndef __GET_LUMINANCE__
#define __GET_LUMINANCE__

#include <iostream>
#include <string>
#include "async_producer.h"
#include "core/core.hpp"
#include "vision/histo.h"
#include "algoFunctions.hpp"
#include "vision/pixel_traits.h"

using namespace std;

class meanLumMultiChannelAlgorithm : public temporalAlgorithm<P8UC3, double,3>
{
public:
    ~meanLumMultiChannelAlgorithm () {};
    
    meanLumMultiChannelAlgorithm () : temporalAlgorithm<P8UC3, double,3> ()
    {
        mProgressReporter = std::bind(&meanLumMultiChannelAlgorithm::updateProgress, this, std::placeholders::_1);
    }
    
    static temporalAlgorithm<P8UC3, double,3>::Ref_t create (const std::vector<std::string>& names, std::shared_ptr<qTimeFrameCache>& frames, bool test_data = false)
    {
        auto thisref = std::make_shared<meanLumMultiChannelAlgorithm>();
        thisref->mNames = names;
        thisref->mFrameSet = frames;
        thisref->mIs_test_data = test_data;
        thisref->mState = 0;
        thisref->mTimestamp = time_spec_t::get_system_time ();
        return thisref;
    }
    
    bool run () override
    {
        
        const std::vector<std::string>& names = mFrameSet->channel_names();
        if (mDone.empty())
            mDone.resize (mFrameSet->count ());
        
        if (names.size() != 3) return false;
        
        // We either have 3 channels or a single one ( that is content is same in all 3 channels ) --
        // Note that Alpha channel is not processed
        
        int64_t fn = 0;
        bool test_data = mIs_test_data;
        float fcnt = mFrameSet->count ();
        
        while (mFrameSet->checkFrame(fn))
        {
            if (! mDone.empty() && mDone[fn]) continue;
            
            auto su8 = mFrameSet->getFrame(fn);
            const index_time_t ti = mFrameSet->currentIndexTime ();
            track_t te (ti);
            
            for (auto cc = 0; cc < 3; cc++)
            {
                auto roi = NewChannelFromSurfaceAtIndex (su8, cc);
                te[cc] = fn / fcnt;
                if (!test_data)
                    te[cc] = histoStats::mean(roi) / 256.0;
            }
            
            mOutput_tracks.push_back(te);
            mDone[fn] = true;
            fn++;
            if (mProgressReporter)
                mProgressReporter (fn / fcnt);
            
        }
        
        return done ();
    }
    
    bool updateProgress(float pct)
    {
        int ipct (pct * 100);
        std::cout << ipct << "% complete...\n";
        return(true);
    }
};


class meanLumAlgorithm : public temporalAlgorithm<P8U, double,1>
{
public:
    ~meanLumAlgorithm () {};
    
    meanLumAlgorithm () : temporalAlgorithm<P8U, double,1> ()
    {
        mProgressReporter = std::bind(&meanLumAlgorithm::updateProgress, this, std::placeholders::_1);
        mOutput_tracks.resize (0);
    }

    static temporalAlgorithm<P8U, double,1>::Ref_t create (const std::string& name, const std::shared_ptr<qTimeFrameCache>& frames, bool test_data = false)
    {
        auto thisref = std::make_shared<meanLumAlgorithm>();
        thisref->mNames[0] = name;
        thisref->mFrameSet = frames;
        thisref->mIs_test_data = test_data;
        thisref->mState = 0;
        thisref->mDone.resize(frames->count());
        thisref->mTimestamp = time_spec_t::get_system_time ();
        return thisref;
    }
    
    virtual bool run () override
    {

        if (mDone.empty())
            mDone.resize (mFrameSet->count ());
        
        // ToDo: select channel API 
        
        int64_t fn = 0;
        bool test_data = mIs_test_data;
        float fcnt = mFrameSet->count ();
        
        while (mFrameSet->checkFrame(fn))
        {
            if (! mDone.empty() && mDone[fn]) continue;
            
            auto su8 = mFrameSet->getFrame(fn);
            
            auto roi = NewChannelFromSurfaceAtIndex (su8, 0);
            const index_time_t ti = mFrameSet->currentIndexTime ();
            
            // Now get average intensity for each channel
            track_t te (ti);
            te[0] = fn / fcnt;
            if (!test_data)
                te[0] = histoStats::mean(roi) / 256.0;
            mOutput_tracks.push_back(te);
            mDone[fn] = true;
            fn++;
            if (mProgressReporter)
                mProgressReporter (fn / fcnt);
        }
        
        return done ();
    }
    
    bool updateProgress(float pct)
    {
        int ipct (pct * 100);
        std::cout << ipct << "% complete...\n";
        return(true);
    }
};


class meanLifMultiChannelAlgorithm : public temporalAlgorithm<P8U, double,3>
{
public:
    ~meanLifMultiChannelAlgorithm () {};
    
    meanLifMultiChannelAlgorithm () : temporalAlgorithm<P8U, double,3> ()
    {
        mProgressReporter = std::bind(&meanLifMultiChannelAlgorithm::updateProgress, this, std::placeholders::_1);
        mOutput_tracks.resize (0);
    }
    
    static temporalAlgorithm<P8U, double,3>::Ref_t create (const std::string& name, const std::shared_ptr<qTimeFrameCache>& frames, bool test_data = false)
    {
        auto thisref = std::make_shared<meanLifMultiChannelAlgorithm>();
        thisref->mNames = frames->channel_names ();
        thisref->mFrameSet = frames;
        thisref->mIs_test_data = test_data;
        thisref->mState = 0;
        thisref->mDone.resize(frames->count());
        thisref->mTimestamp = time_spec_t::get_system_time ();
        return thisref;
    }
    
    virtual bool run () override
    {
        
        if (mDone.empty())
            mDone.resize (mFrameSet->count ());
        
        // ToDo: select channel API
        
        int64_t fn = 0;
        bool test_data = mIs_test_data;
        float fcnt = mFrameSet->count ();
        
        while (mFrameSet->checkFrame(fn))
        {
            if (! mDone.empty() && mDone[fn]) continue;
            
            auto su8 = mFrameSet->getFrame(fn);
            
            auto rois = svl::NewRefMultiFromSurface (su8, mNames, fn);
            
            const index_time_t ti = mFrameSet->currentIndexTime ();
            
            // Now get average intensity for each channel
            int index = 0;
            {
                auto roi = rois->plane(index);
                track_t te (ti);
                te[0] = fn / fcnt;
                if (!test_data)
                    te[0] = histoStats::mean(roi) / 256.0;
                mOutput_tracks.push_back(te);
            }
            
            mDone[fn] = true;
            fn++;
            if (mProgressReporter)
                mProgressReporter (fn / fcnt);
        }
        
        return done ();
    }
    
    bool updateProgress(float pct)
    {
        int ipct (pct * 100);
        std::cout << ipct << "% complete...\n";
        return(true);
    }
};

#endif


