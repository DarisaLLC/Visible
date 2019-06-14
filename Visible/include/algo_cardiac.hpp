#ifndef __GET_LUMINANCE__
#define __GET_LUMINANCE__

#include <iostream>
#include <string>
#include "async_tracks.h"
#include "core/core.hpp"
#include "vision/histo.h"
#include "vision/pixel_traits.h"
#include "vision/opencv_utils.hpp"
#include "core/stl_utils.hpp"
#include "core/stats.hpp"
#include "vision/localvariance.h"
#include "core/lineseg.hpp"
#include <boost/range/irange.hpp>
#include "core/moreMath.h"

using namespace std;
using namespace stl_utils;
using namespace svl;

struct idlab_cardiac_defaults{
    float median_level_set_cutoff_fraction;
    
};


void pointsToRotatedRect (std::vector<cv::Point2f>& imagePoints, cv::RotatedRect& rotated_rect );


struct IntensityStatisticsRunner
{
    typedef std::vector<roiWindow<P8U>> channel_images_t;
    void operator()(channel_images_t& channel_images,timedVecOfVals_t& results)
    {
        std::vector<float> src, dst;
        for (auto ii = 0; ii < channel_images.size(); ii++)
        {
            auto res = static_cast<float>(histoStats::mean(channel_images[ii]) / 256.0);
            src.emplace_back(res);
        }
        
        rolling_median_3(src.begin(), src.end(), dst);
        
        results.clear();
        for (auto ii = 0; ii < channel_images.size(); ii++)
        {
            timedVal_t res;
            index_time_t ti;
            ti.first = ii;
            res.first = ti;
            res.second = dst[ii];
            results.emplace_back(res);
        }
    }
};


struct trackMaker
{
    trackMaker (const vector<float>& norm_lengths, timedVecOfVals_t& results)
    {
    
        auto fill = [](const std::vector<float>& lens){
            timedVecOfVals_t base(lens.size());
            
            for (auto i=0; i < lens.size(); i++) {
                timedVal_t res;
                index_time_t ti;
                ti.first = i;
                base[i].first = ti;
                base[i].second =  lens[i];
            }
            return base;
        };
        results = fill (norm_lengths);
    }
};

// Create sin signals

template<float (*TF)(float), int S,  int E>
struct UnitSyntheticProducer
{
    UnitSyntheticProducer (timedVecOfVals_t& results)
    {
        // Create sin signals
        auto trigvecf = [](float step, uint32_t size){
            timedVecOfVals_t base(size);
            
            for (auto i : boost::irange(0u, size)) {
                timedVal_t res;
                index_time_t ti;
                ti.first = i;
                base[i].first = ti;
                base[i].second =  TF (i * 3.14159 *  step);
            }
            return base;
        };
        results = trigvecf(1.0f/float(E),S);
    }
};



struct SequenceAccumulator
{

    typedef std::vector<roiWindow<P8U>> channel_images_t;
    
    void operator()(channel_images_t& channel_images, cv::Mat& m_sum, cv::Mat& m_sqsum, int& image_count,
                    uint8_t spatial_x = 0, uint8_t spatial_y = 0 )
    {
        image_count = 0;
        for (const roiWindow<P8U>& ir : channel_images){
            cv::Mat im (ir.height(), ir.width(), CV_8UC(1), ir.pelPointer(0,0), size_t(ir.rowUpdate()));
            if( image_count == 0 ) {
                m_sum = cv::Mat::zeros( im.size(), CV_32FC(im.channels()) );
                m_sqsum = cv::Mat::zeros( im.size(), CV_32FC(im.channels()) );
            }
            if (spatial_x > 0 && spatial_y > 0){
                cv::Mat result;
                localVAR lv(cv::Size(spatial_x, spatial_y));
                lv.process (im, result);
                cv::normalize(result, im, 0, 255, NORM_MINMAX, CV_8UC1);
            }
            cv::accumulate( im, m_sum );
            cv::accumulateSquare( im, m_sqsum );
            image_count++;
        }
    }
    

    static void computeVariance(cv::Mat& m_sum, cv::Mat& m_sqsum, int image_count, cv::Mat& variance) {
        double one_by_N = 1.0 / image_count;
        variance = (one_by_N * m_sqsum) - ((one_by_N * one_by_N) * m_sum.mul(m_sum));
    }
    
    //Same as above function, but compute standard deviation
    static void computeStdev(cv::Mat& m_sum, cv::Mat& m_sqsum, int image_count, cv::Mat& std__) {
        double one_by_N = 1.0 / image_count;
        cv::sqrt(((one_by_N * m_sqsum) -((one_by_N * one_by_N) * m_sum.mul(m_sum))), std__);
    }
    
    //And avg images
    static void computeAvg(cv::Mat& m_sum, int image_count, cv::Mat& av) {
        double one_by_N = 1.0 / image_count;
        av = one_by_N * m_sum;
    }

};

struct IntensityStatisticsPartialRunner
{
    typedef std::vector<roiWindow<P8U>> channel_images_t;
    void operator()( channel_images_t& channel, std::vector< std::tuple<int64_t, int64_t, uint32_t> >& results,
                    std::vector< std::tuple<uint8_t, uint8_t> >& ranges )
    {
        results.clear();
        for (auto ii = 0; ii < channel.size(); ii++)
        {
            roiWindow<P8U>& image = channel[ii];
            histoStats hh;
            hh.from_image(image);
            int64_t s = hh.sum();
            int64_t ss = hh.sumSquared();
            uint8_t mi = hh.min();
            uint8_t ma = hh.max();
            uint32_t nn = hh.n();
            results.emplace_back(s,ss,nn);
            ranges.emplace_back(mi,ma);
        }
    }
};


#ifdef NOT_YET
struct fbFlowRunner
{
    typedef std::vector<roiWindow<P8U>> channel_images_t;
    void operator()(channel_images_t& channel, svl::stats<int64_t>& volstats, timed_mat_vec_t& results)
    {
        if (channel.size () < 2) return;
        std::mutex m_mutex;
        std::lock_guard<std::mutex> guard (m_mutex);
        
        results.clear();
        std::vector<cv::Mat> frames;
        svl::localVAR tv (cv::Size(7,7));
        
        for (int ii = 0; ii < channel.size(); ii++)
        {
            const roiWindow<P8U>& image = channel[ii];
            std::string imgpath = "/Users/arman/oflow/out" + svl::toString(ii) + ".png";
            cv::Mat mat (image.height(), image.width(), CV_8UC(1), image.pelPointer(0,0), size_t(image.rowUpdate()));
            cv::Mat lv;
            tv.process (mat,lv);
//            cv::Mat diff (image.height(), image.width(), CV_32F);
//            diff = mat - volstats.mean();
//            cv::Mat slices[3];
//            cv::Mat mzp;
        //   cv::multiply(diff,diff,diff);
        //   diff /= std::sqrt(volstats.variance());
            cv::normalize(lv,mat,0,UCHAR_MAX,cv::NORM_MINMAX);
            cv::imwrite(imgpath, lv);
            std::cout << "wrote Out " << imgpath << std::endl;
        }
    }

    
    static void drawOptFlowMap(const cv::Mat& flow, cv::Mat& cflowmap, int step,
                               double, const Scalar& color)
    {
        for(int y = 0; y < cflowmap.rows; y += step)
            for(int x = 0; x < cflowmap.cols; x += step)
            {
                const Point2f& fxy = flow.at<Point2f>(y, x);
                line(cflowmap, cv::Point(x,y), cv::Point(cvRound(x+fxy.x), cvRound(y+fxy.y)),
                     color);
                circle(cflowmap, cv::Point(x,y), 2, color, -1);
            }
    }
    
    

};
#endif



#endif


