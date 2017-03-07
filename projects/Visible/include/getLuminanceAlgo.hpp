#ifndef __GET_LUMINANCE__
#define __GET_LUMINANCE__

#include <iostream>
#include <string>
#include "async_producer.h"
#include "core/core.hpp"
#include "vision/histo.h"
#include "algoFunctions.hpp"

using namespace std;

class meanLumMultiChannelAlgorithm : public temporalAlgorithm<double,3>
{
public:
    ~meanLumMultiChannelAlgorithm () {};
    
    meanLumMultiChannelAlgorithm ()
    {
        mProgressReporter = std::bind(&meanLumMultiChannelAlgorithm::updateProgress, this, std::placeholders::_1);
    }
    
    static meanLumMultiChannelAlgorithm::Ref_t create (const std::vector<std::string>& names, std::shared_ptr<qTimeFrameCache>& frames, bool test_data = false)
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
        
        if (names.size() != 3) return false;
        
        // We either have 3 channels or a single one ( that is content is same in all 3 channels ) --
        // Note that Alpha channel is not processed
        
        int64_t fn = 0;
        bool test_data = mIs_test_data;
        float fcnt = mFrameSet->count ();
        
        while (mFrameSet->checkFrame(fn))
        {
            if (! mDone.empty() && mDone[fn]) continue;
            
            auto su8 = mFrameSet->getFrame(fn++);
            const index_time_t ti = mFrameSet->currentIndexTime ();
            
            std::vector<roiWindow<P8U> > rois;
            rois.emplace_back(svl::NewRedFromSurface(su8));
            rois.emplace_back(svl::NewGreenFromSurface(su8));
            rois.emplace_back(svl::NewBlueFromSurface(su8));
            
            assert(rois.size() == 3);
            
            mOutput_tracks.first.push_back (ti);
            track<double,3>::result_array_t res;
            track<double,3>::result_array_t::iterator be = res.begin();
            
            // Now get average intensity for each channel
            for (roiWindow<P8U> roi : rois)
            {
                auto nmg = histoStats::mean(roi) / 256.0;
                nmg = (! test_data) ? nmg :  (((float) fn) / mFrameSet->count() );
                *be++ = nmg;
            }
            mOutput_tracks.second.push_back(res);
            mDone.push_back(true);
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


class meanLumAlgorithm : public temporalAlgorithm<double,1>
{
public:
    ~meanLumAlgorithm () {};
    
    meanLumAlgorithm ()
    {
        mProgressReporter = std::bind(&meanLumAlgorithm::updateProgress, this, std::placeholders::_1);
    }

    static meanLumAlgorithm::Ref_t create (const std::string& name, std::shared_ptr<qTimeFrameCache>& frames, bool test_data = false)
    {
        auto thisref = std::make_shared<meanLumAlgorithm>();
        thisref->mName = name;
        thisref->mFrameSet = frames;
        thisref->mIs_test_data = test_data;
        thisref->mState = 0;
        thisref->mTimestamp = time_spec_t::get_system_time ();
        return thisref;
    }
    
    virtual bool run () override
    {

        const std::vector<std::string>& names = mFrameSet->channel_names();
        
        // ToDo: select channel API 
        
        // We either have 3 channels or a single one ( that is content is same in all 3 channels ) --
        // Note that Alpha channel is not processed
        
        int64_t fn = 0;
        bool test_data = mIs_test_data;
        float fcnt = mFrameSet->count ();
        
        while (mFrameSet->checkFrame(fn))
        {
            if (! mDone.empty() && mDone[fn]) continue;
            
            auto su8 = mFrameSet->getFrame(fn);
            
            roiWindow<P8U> roi = svl::NewRedFromSurface(su8);
            const index_time_t ti = mFrameSet->currentIndexTime ();
            
            // Now get average intensity for each channel
            mOutput_tracks.first.push_back (ti);
            track<double,1>::result_array_t res;
            res[0] = fn / fcnt;
            if (!test_data)
                res[0] = histoStats::mean(roi) / 256.0;
            mOutput_tracks.second.push_back(res);
            bool tt = true;
            mDone.push_back(tt);
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


