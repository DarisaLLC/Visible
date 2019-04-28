//
//  labelBlob.hpp
//  Visible
//
//  Created by Arman Garakani on 9/3/18.
//

#ifndef labelBlob_hpp
#define labelBlob_hpp


#include <algorithm>
#include <iostream>
#include <typeinfo>
#include <vector>
#include "opencv2/opencv.hpp"
#include <stdio.h>
#include "core/signaler.h"
#include <cstdint>
#include "core/pair.hpp"
#include <atomic>
#include "angle_units.h"
#include <memory>
#include <atomic>

using namespace cv;

namespace svl
{
    
    class momento : CvMoments
    {
    public:
        momento(): m_is_loaded (false), m_eigen_done(false), m_is_nan(true) {}
        momento(const momento& other);
        ~momento() {}
        
        momento(const cv::Mat&);
        
        void run (const cv::Mat&) const;
        const Point2f& com () const { return mc; }
        fPair getEllipseAspect () const;
        uRadian getOrientation () const;
        bool isLoaded () const { return m_is_loaded; }
        bool isValidEigen () const { return m_eigen_ok; }
        bool isNan () const { return m_is_nan; }
        bool isEigenDone () const { return m_eigen_done; }

        
    private:
        mutable cv::Point m_offset;
        mutable cv::Size m_size;
        mutable double m_a;
        mutable double m_b;
        mutable uRadian theta;
        mutable double eigen_angle;
        mutable double inv_m00;
        mutable Point2f mc;
        mutable bool m_eigen_ok;
        mutable bool m_is_loaded;
        mutable bool m_eigen_done;
        mutable bool m_is_nan;
        void getDirectionals () const;
        
    };
    
    

class lblMgr : public base_signaler
{
    virtual std::string
    getName () const { return "LabelManager"; }
};

    /* @todo: add filters
 * labelBlob generates label regions from an image ( @todo add color )
 * it handles filtering the result and support signals 
 * It can run async and report results via signals
 * as well as access to results via blocked api
 */

class labelBlob : public lblMgr
{
public:
    typedef void (results_ready_cb) (int64_t&);
    typedef void (graphics_ready_cb) ();
    typedef std::shared_ptr<labelBlob> ref;
    typedef std::weak_ptr<labelBlob> weak_ref;
    
    static ref create(const cv::Mat& gray, const cv::Mat& threshold_out,  const int64_t client_id = 0, const int minAreaPixelCount = 10);
    labelBlob ();
    labelBlob (const cv::Mat& gray, const cv::Mat& threshold_out, const int64_t client_id = 0, const int minAreaPixelCount = 10);
    
    class blob {
    public:
        blob ( const uint32_t label, const int64_t id, const cv::Rect2f& roi) :
            m_id(id), m_label(label), m_roi (roi), m_moments_ready(false)
        {
            m_extend = std::sqrt(roi.width*roi.width+roi.height*roi.height);
        }
        
        blob(const blob& other){
            m_id = other.m_id;
            m_label = other.m_label;
            m_roi = other.roi();
            m_moments = other.moments();
            m_points = other.m_points;
            m_moments_ready = other.moments_ready();
            
        }

        
        void update_moments (const cv::Mat& image) const;
        void update_points (const std::vector<cv::Point>& ) const;
        bool moments_ready () const { return m_moments_ready; }
        const cv::Rect2f& roi () const { return m_roi; }
        const svl::momento& moments () const { return m_moments; }
        float extend () const { return m_extend;}
        cv::RotatedRect rotated_roi () const; 
        
    private:
        int64_t m_id;
        uint32_t m_label;
        mutable svl::momento m_moments;
        mutable float m_extend;
        cv::Rect2f m_roi;
        mutable std::vector<cv::Point> m_points;
        mutable bool m_moments_ready;
        mutable std::mutex m_mutex;
    };
    
    void reload (const cv::Mat& gray, const cv::Mat& threshold_out,  const int64_t client_id = 0, const int minAreaPixelCount = 10) const;
    
    // Result available can be checked via subscription to results_ready signal or
    // checking hasResults.
    
    void run_async () const;
    void run () const;
    void drawOutput () const;
    bool hasResults () const;
    const int64_t& client_id () const { return m_client_id; }
    
    const int& min_area_count () const { return m_min_area; }
    const std::vector<labelBlob::blob>& results() const { return m_blobs; }
    const cv::Mat& graphicOutput () const { return m_graphics; }
    const std::vector<cv::KeyPoint>& keyPoints (bool regen = false) const;
    
    
    // Support moving
    labelBlob(labelBlob&&) = default;
    labelBlob& operator=(labelBlob&&) = default;
    
protected:
    
    boost::signals2::signal<results_ready_cb>* signal_results_ready;
    boost::signals2::signal<graphics_ready_cb>* signal_graphics_ready;
    
private:
    mutable chrono::time_point<std::chrono::high_resolution_clock> m_start;
    mutable cv::Mat m_grey;
    mutable cv::Mat m_threshold_out;
    mutable std::vector<labelBlob::blob> m_blobs;
    mutable std::atomic<bool> m_results_ready;
    mutable cv::Mat m_graphics;
    mutable cv::Mat m_labels;
    mutable cv::Mat m_stats;
    mutable cv::Mat m_centroids;
    mutable std::vector<svl::momento> m_moments;
    mutable std::vector<cv::Rect2f> m_rois;
    mutable int64_t m_client_id;
    mutable std::vector<cv::KeyPoint> m_kps;
    mutable int m_min_area;
    mutable std::mutex m_mutex;
  
};

}

#endif /* labelBlob_hpp */
