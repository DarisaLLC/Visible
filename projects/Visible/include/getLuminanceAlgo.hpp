#ifndef __GET_LUMINANCE__
#define __GET_LUMINANCE__

#include <iostream>
#include <string>
#include "async_producer.h"
#include "core/core.hpp"
#include "vision/histo.h"
#include "vision/pixel_traits.h"

using namespace std;

struct IntensityStatisticsRunner
{
    typedef std::vector<roiWindow<P8U>> channel_images_t;
    void operator()(channel_images_t& channel, timed_double_vec_t& results)
    {
        results.clear();
        for (auto ii = 0; ii < channel.size(); ii++)
        {
            timed_double_t res;
            index_time_t ti;
            ti.first = ii;
            res.first = ti;
            res.second = histoStats::mean(channel[ii]) / 256.0;
            results.emplace_back(res);
        }
    }
};


#endif


