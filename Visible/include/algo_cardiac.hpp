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

using namespace std;
using namespace stl_utils;

struct IntensityStatisticsRunner
{
    typedef std::vector<roiWindow<P8U>> channel_images_t;
    void operator()(channel_images_t& channel,timedVecOfVals_t& results)
    {
        results.clear();
        for (auto ii = 0; ii < channel.size(); ii++)
        {
            timedVal_t res;
            index_time_t ti;
            ti.first = ii;
            res.first = ti;
            res.second = static_cast<float>(histoStats::mean(channel[ii]) / 256.0);
            results.emplace_back(res);
        }
    }
};


struct SequenceAccumulator
{

    typedef std::vector<roiWindow<P8U>> channel_images_t;
    
    void operator()(channel_images_t& channel, cv::Mat& m_sum, cv::Mat& m_sqsum, int& image_count)
    {
        image_count = 0;
        for (const roiWindow<P8U>& ir : channel){
            cv::Mat im (ir.height(), ir.width(), CV_8UC(1), ir.pelPointer(0,0), size_t(ir.rowUpdate()));
            
            if( image_count == 0 ) {
                m_sum = cv::Mat::zeros( im.size(), CV_32FC(im.channels()) );
                m_sqsum = cv::Mat::zeros( im.size(), CV_32FC(im.channels()) );
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




// For base classing
class OCVImageWriterBase : public base_signaler
{
    virtual std::string getName () const { return "OCVImageWriterBase"; }
};


class OCVImageWriter : public OCVImageWriterBase
{
public:
    typedef std::function<std::string(const uint32_t& index, uint32_t field_width, char fill_char)> file_naming_fn_t;
    
    typedef void (writer_info_delegate) (int,bool);
    
    OCVImageWriter(const std::string& image_name = "image",
                   const file_naming_fn_t namer = OCVImageWriter::default_namer,
                   const std::string& file_sep = "/"):
        m_name(std::move(image_name)), m_sep(std::move(file_sep)),m_namer(namer)
    {
        // Signals we provide
        signal_write_info = createSignal<OCVImageWriter::writer_info_delegate>();
    }
 
    typedef std::vector<roiWindow<P8U>> channel_images_t;
    void operator()(const std::string& dir_fqfn, channel_images_t& channel)
    {
        m_fqfn = dir_fqfn;
        m_valid = boost::filesystem::exists( m_fqfn );
        m_valid = m_valid && boost::filesystem::is_directory( m_fqfn );
        if(!m_valid){
            if (signal_write_info && signal_write_info->num_slots() > 0)
                signal_write_info->operator()(0,true);
            return;
        }
        m_name = m_fqfn + m_sep +m_name;
        
        if (channel.empty()) return;
        int fw = 1 + std::log10(channel.size());
        std::lock_guard<std::mutex> guard (m_mutex);
        
        uint32_t count = 0;
        uint32_t total = static_cast<uint32_t>(channel.size());
        vector<roiWindow<P8U> >::const_iterator vitr = channel.begin();
        do
        {
            cv::Mat mat(vitr->height(), vitr->width(), CV_8UC(1), vitr->rowPointer(0), 512);
            std::string fn = m_name +  m_namer(count++, fw, '0') + ".png";
            cv::imwrite(fn, mat);
            // Signal to listeners
            if (signal_write_info && signal_write_info->num_slots() > 0)
                signal_write_info->operator()(count-1,count == total);
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
    
    
protected:
    boost::signals2::signal<OCVImageWriter::writer_info_delegate>* signal_write_info;
    
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


