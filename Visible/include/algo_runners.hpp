#ifndef __GET_LUMINANCE__
#define __GET_LUMINANCE__

#include <iostream>
#include <string>
#include "timed_types.h"
#include "core/core.hpp"
#include "vision/histo.h"
#include "vision/pixel_traits.h"
#include "vision/opencv_utils.hpp"
#include "core/stl_utils.hpp"
#include "core/stats.hpp"
#include "vision/localvariance.h"
#include <boost/range/irange.hpp>
#include "core/moreMath.h"
#include "etw_utils.hpp"
#include "vision/ellipse.hpp"

using namespace std;
using namespace stl_utils;
using namespace svl;

struct idlab_cardiac_defaults{
    float median_level_set_cutoff_fraction;
    
};



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


/*
struct trackMaker
 Single value trackMaker
*/
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


/*
struct UnitSyntheticProducer
 Synthetic signal generator ( trig )
*/
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



/*
struct SequenceAccumulator
 Callable to produce sum of per pixel variances over the pixel or a local neigbourhood around
 atomic bool is set to true to indicate completion. 
*/
struct SequenceAccumulator
{

    typedef std::vector<roiWindow<P8U>> channel_images_t;
    
    void operator()(channel_images_t& channel_images, cv::Mat& m_sum, cv::Mat& m_sqsum, int& image_count, std::atomic<bool>& done,
                    uint8_t spatial_x = 0, uint8_t spatial_y = 0 )
    {
        done = false;
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
        done = true;
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

/*
struct IntensityStatisticsPartialRunner
 Callable to produce (count, sum sumsq) and (min, max) over the volume
*/
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


class voxel_processor {
public:
    voxel_processor();

    bool generate_voxel_space (const std::vector<roiWindow<P8U>>& images, const std::vector<int>& indicies = std::vector<int> ());
    bool generate_voxel_surface (const std::vector<float>&);
    
    const uiPair &sample() { return m_voxel_sample; }
    const uiPair &half_offet() { return m_half_offset; }
    void sample(uint32_t x, uint32_t y = 0) {
        m_voxel_sample.first = x;
        m_voxel_sample.second = y == 0 ? x : y;
        m_half_offset = (m_voxel_sample - 1) / 2;
    }
    void image_size(int width, int height){
        m_image_size.first = width - m_half_offset.first;
        m_image_size.second = height - m_half_offset.second;
        m_expected_segmented_size.first = m_image_size.first / m_voxel_sample.first;
        m_expected_segmented_size.second = m_image_size.second / m_voxel_sample.second;
    }
    
    const Rectf& measured_area () { return m_measured_area; }
    const cv::Mat& temporal_ss () { return m_temporal_ss; }
    const std::vector<float>& entropies () { return m_voxel_entropies; }
    const std::vector<Eigen::Vector3d>& cloud () { return m_cloud; }
    
private:
    const smProducerRef similarity_producer () const;
    
    bool m_internal_generate();
    
  
    bool m_load(const std::vector<roiWindow<P8U>> &images, uint32_t sample_x,
                uint32_t sample_y = 0, const std::vector<int>& indicies = std::vector<int> ()); 
    
    std::vector<Eigen::Vector3d> m_cloud;
    uiPair m_voxel_sample;
    uiPair m_half_offset;
    iPair m_expected_segmented_size;
    mutable std::vector<roiWindow<P8U>> m_voxels;
    size_t m_voxel_length;
    uiPair m_image_size;
    vector<float> m_voxel_entropies;
    mutable smProducerRef m_sm_producer;
    Rectf m_measured_area;
    cv::Mat m_temporal_ss;
    std::vector<uint32_t> m_hist;
};

 
 /*   Scale Space Processing  for cardiomyocyte detection and processing
      
 
 */


class scaleSpace{
public:
    scaleSpace(): m_loaded(false), m_space_done(false), m_field_done(false), m_trim(iPair(24,24)) {}
    
    bool generate(const std::vector<roiWindow<P8U>> &images, float start_sigma, float end_sigma, float step);
    bool generate(const std::vector<cv::Mat> &images,float start_sigma, float end_sigma, float step);
	bool process_motion_peaks (int model_frame_index = 0, const iPair& = iPair(3,3), const iPair& = iPair(10,10));
    
    const std::vector<float> estimated_lengths (int model_frame_index = 0, const iPair& trim = iPair(24,24)) const;
    
	const std::vector<float>& lengths () const { return m_lengths; }
	
    const std::pair<std::vector<fVector_2d>,std::vector<fVector_2d>> estimated_positions () const;
    const std::vector<cv::Mat>& space() const { return m_scale_space; }
    const std::vector<cv::Mat>& dog() const;
	const std::vector<cv::Mat>& models() const;
	const cv::Mat& motion_field() const;
    const cv::Rect& motion_peaks() const;
    bool length_extremes (fPair& ) const;
	const std::vector<cv::Point2f>& segmented_ends() const;
	const std::vector<cv::Rect>& modeled_ends() const;
	const cv::RotatedRect& body() const { return m_body; }
	const ellipseShape& ellipse_shape() const { return m_ellipse; }
	const cv::Mat& voxel_range () const;
	
    
    
    int start_sigma() const { return m_start_sigma; }
    int end_sigma() const { return m_end_sigma; }
    int steps() const { return m_step; }
    iPair& trim () const { return m_trim; }
    void trim(const iPair& tr) const { m_trim = tr; }
    
    bool isLoaded () const { return m_loaded; }
    bool spaceDone () const { return m_space_done; }
    bool fieldDone () const { return m_field_done; }

    
private:
	bool detect_moving_profile(const cv::Mat&, const iPair& = iPair(3,3), const iPair& = iPair(10,10));
	
    bool m_loaded;
    bool m_space_done;
    mutable bool m_field_done;
    std::vector<float> m_sigmas;
	mutable cv::Mat m_voxel_range;
	mutable cv::Mat m_motion_field;
	mutable cv::Mat m_motion_field_minimas;
	mutable std::vector<cv::Point2f> m_segmented_ends;
	mutable std::vector<cv::Point2f> m_focals;
	mutable std::vector<cv::Point2f> m_directrix;
	std::vector<cv::Mat> m_filtered;
	std::vector<cv::Mat> m_inputs;
    mutable std::vector<cv::Mat> m_dogs;
    mutable std::vector<cv::Mat> m_models;
    mutable std::vector<cv::Mat> m_scale_space;
    mutable std::vector<cv::Rect> m_motion_peaks;
    mutable std::pair<std::vector<fVector_2d>,std::vector<fVector_2d>> m_ends;
    mutable std::vector<float> m_lengths;
    mutable std::vector<cv::Rect> m_all_rects;
    mutable std::vector<cv::Rect> m_rects;
    mutable iPair m_trim;
    mutable fPair m_length_extremes;
    float m_start_sigma, m_end_sigma, m_step;
	double m_profile_threshold;
	mutable cv::RotatedRect m_body;
	mutable ellipseShape m_ellipse;
    
};

#endif


