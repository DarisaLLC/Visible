#ifndef __UT_ALGO__
#define __UT_ALGO__

#include <iostream>
#include <string>
#include "async_producer.h"
#include "core/core.hpp"
#include "vision/histo.h"

using namespace std;



bool get_mean_luminance (algoIO::algoIORef& algo)
{
    tracksD1_t& tracks = algo->mOutput_tracks;

    const std::vector<std::string>& names = algo->mFrameSet->channel_names();
    
    if (names.empty() || (names.size() != 1 && names.size() != 3))
        return false;
    
    // We either have 3 channels or a single one ( that is content is same in all 3 channels ) --
    // Note that Alpha channel is not processed
    
    tracks.resize (names.size ());
    for (auto tt = 0; tt < names.size(); tt++)
        tracks[tt].first = names[tt];
    
    int64_t fn = 0;
    bool test_data = algo->mIs_test_data;
    
    while (algo->mFrameSet->checkFrame(fn))
    {
        if (! algo->mDone.empty() && algo->mDone[fn]) continue;
        
        auto su8 = algo->mFrameSet->getFrame(fn++);
        
        auto channels = names.size();
        
        std::vector<roiWindow<P8U> > rois;
        switch (channels)
        {
            case 1:
            {
                rois.emplace_back(svl::NewRedFromSurface(su8));
                break;
            }
            case 3:
            {
                rois.emplace_back(svl::NewRedFromSurface(su8));
                rois.emplace_back(svl::NewGreenFromSurface(su8));
                rois.emplace_back(svl::NewBlueFromSurface(su8));
                break;
            }
        }
        
        assert (rois.size () == tracks.size());
        
        // Now get average intensity for each channel
        int index = 0;
        for (roiWindow<P8U> roi : rois)
        {
            index_time_t ti;
            ti.first = fn;
            timed_double_t res;
            res.first = ti;
            auto nmg = histoStats::mean(roi) / 256.0;
            res.second = (! test_data) ? nmg :  (((float) fn) / algo->mFrameSet->count() );
            tracks[index++].second.emplace_back(res);
        }
        
        algo->mDone.push_back(true);
        
        // TBD: call progress reporter
    }

    // Only when we have all, put the output out
    if (algo->done())
        algo->mOutput_tracks = tracks;
    
    return algo->done ();
}


#endif

