#ifndef __GET_LUMINANCE__
#define __GET_LUMINANCE__

#include <iostream>
#include <string>
#include "async_producer.h"
#include "core/core.hpp"
#include "vision/histo.h"
#include "vision/pixel_traits.h"
#include "vision/opencv_utils.hpp"
#include "core/stl_utils.hpp"

using namespace std;
using namespace stl_utils;

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

class OCVImageWriter
{
public:
    typedef std::function<std::string(const uint32_t& index, uint32_t field_width, char fill_char)> file_naming_fn_t;
    
    OCVImageWriter(const std::string& dir_fqfn,
                   const std::string& image_name = "image",
                   const file_naming_fn_t namer = OCVImageWriter::default_namer,
                   const std::string& file_sep = "/"):
    m_fqfn(std::move(dir_fqfn)), m_name(std::move(image_name)), m_namer(namer), m_sep(std::move(file_sep))
    {
        m_valid = boost::filesystem::exists( dir_fqfn );
        m_valid = boost::filesystem::is_directory( dir_fqfn );
        m_name = m_fqfn + m_sep +m_name;
    }
    typedef std::vector<roiWindow<P8U>> channel_images_t;
    void operator()(channel_images_t& channel)
    {
        if (channel.empty()) return;
        int fw = 1 + std::log10(channel.size());
        std::lock_guard<std::mutex> guard (m_mutex);
        
        uint32_t count = 0;
        vector<roiWindow<P8U> >::const_iterator vitr = channel.begin();
        do
        {
            cv::Mat mat(vitr->height(), vitr->width(), CV_8UC(1), vitr->rowPointer(0), 512);
            std::string fn = m_name +  m_namer(count++, fw, '0') + ".png";
            cv::imwrite(fn, mat);
            std::cout << fn << std::endl;
            
        }
        while (++vitr != channel.end());
    }
    static std::string default_namer (const uint32_t& image_index, uint32_t field_width, char fill_char)
    {
        std::stringstream ss;
        std::string s;
        ss << std::setfill(fill_char) << std::setw(field_width) << image_index;
        s = ss.str();
        return s;
    }
private:
    mutable std::mutex m_mutex;
    bool m_valid;
    std::string m_fqfn;
    std::string m_name;
    std::string m_sep;
    file_naming_fn_t  m_namer;
  
};



struct fbFlowRunner
{
    typedef std::vector<roiWindow<P8U>> channel_images_t;
    void operator()(channel_images_t& channel, timed_mat_vec_t& results)
    {
        if (channel.size () < 2) return;
        std::mutex m_mutex;
        std::lock_guard<std::mutex> guard (m_mutex);
        
        cv::Mat cflow;
        results.clear();
        for (int ii = 1; ii < channel.size(); ii++)
        {
            cv::UMat uflow;
            timed_mat_t res;
            index_time_t ti;
            ti.first = ii;
            res.first = ti;
            const roiWindow<P8U> prev = channel[ii-1].clone();
            const roiWindow<P8U> curr = channel[ii].clone();

            std::string imgpath = "/Users/arman/oflow/fback_prev" + toString(ii) + ".png";
            cv::Mat prevMat (prev.height(), prev.width(), CV_8UC(1), prev.pelPointer(0,0), size_t(prev.rowUpdate()));
            cv::Mat currMat (curr.height(), curr.width(), CV_8UC(1), curr.pelPointer(0,0), size_t(curr.rowUpdate()));
            cv::imwrite(imgpath, prevMat);

            
//            calcOpticalFlowFarneback(prevMat, currMat, uflow, 0.5, 1, 15, 3, 5, 1.0, cv::OPTFLOW_FARNEBACK_GAUSSIAN);
//            uflow.copyTo(res.second);
//            cvtColor(currMat, cflow, COLOR_GRAY2BGR);
//            drawOptFlowMap(res.second, cflow, 16, 1.5, Scalar(0, 255, 0));
//
//
//            std::cout << "wrote Out " << imgpath << std::endl;
//            results.emplace_back(res);
            
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


