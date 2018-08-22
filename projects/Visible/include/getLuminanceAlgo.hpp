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

struct IntensityStatisticsPartialRunner
{
    typedef std::vector<roiWindow<P8U>> channel_images_t;
    void operator()( channel_images_t& channel, std::vector< std::tuple<int64_t, int64_t, uint32_t> >& results)
    {
        results.clear();
        for (auto ii = 0; ii < channel.size(); ii++)
        {
            roiWindow<P8U>& image = channel[ii];
            histoStats hh;
            hh.from_image(image);
            int64_t s = hh.sum();
            int64_t ss = hh.sumSquared();
            uint32_t nn = hh.n();
            results.emplace_back(s,ss,nn);
        }
    }
};


#endif


