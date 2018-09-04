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

using namespace cv;

namespace svl
{
    
    class momento : CvMoments
    {
    public:
        momento(): m_is_loaded (false) {}
        momento(const momento& other);
        ~momento() {}
        
        momento(const cv::Mat&);
        
        void run (const cv::Mat&) const;
        const Point2f& com () const { return mc; }
        fPair getEllipseAspect () const;
        uRadian getOrientation () const;
        bool isLoaded () const { return m_is_loaded; }
        
    private:
        mutable cv::Point m_offset;
        mutable cv::Size m_size;
        mutable double a;
        mutable double b;
        mutable uRadian theta;
        mutable double eigen_angle;
        mutable double inv_m00;
        mutable Point2f mc;
        mutable bool eigen_ok;
        mutable bool m_is_loaded;
        mutable bool eigen_done;
        void getDirectionals () const;
        
    };
    
    

class lblMgr : public base_signaler
{
    virtual std::string
    getName () const { return "LabelManager"; }
};

/*
 * labelBlob generates label regions from an image ( @todo add color )
 * it handles filtering the result and support signals 
 * It can run async and report results via signals
 * as well as access to results via blocked api
 */

class labelBlob : public lblMgr
{
public:
    typedef void (results_ready_cb) ();
    typedef void (graphics_ready_cb) ();
    typedef std::shared_ptr<labelBlob> ref;
    
    static ref create(const cv::Mat& gray, const cv::Mat& threshold_out);
    
    class blob {
    public:
        blob ( const uint32_t label, const int64_t id, const cv::Rect2f& roi) :
            m_id(id), m_label(label), m_roi (roi), m_moments_ready(false)
        {}
        blob(const blob& other){
            m_id = other.m_id;
            m_label = other.m_label;
            m_roi = other.roi();
            m_moments = other.moments();
            m_moments_ready = other.moments_ready();
            
        }

        
        void update_moments (const cv::Mat& image) const;
        bool moments_ready () const { return m_moments_ready; }
        const cv::Rect2f& roi () const { return m_roi; }
        const svl::momento& moments () const { return m_moments; }
        
    private:
        int64_t m_id;
        uint32_t m_label;
        mutable svl::momento m_moments;
        cv::Rect2f m_roi;
        mutable std::atomic<bool> m_moments_ready;
    };
    
    void reload (const cv::Mat& gray, const cv::Mat& threshold_out) const;
    
    // Result available can be checked via subscription to results_ready signal or
    // checking hasResults.
    
    void run_async () const;
    void run () const;
    void drawOutput () const;
    bool hasResults () const;
    
    const std::vector<labelBlob::blob>& results() const { return m_blobs; }
    const cv::Mat& graphicOutput () const { return m_graphics; }
    
    
    
    // Support moving
    labelBlob(labelBlob&&) = default;
    labelBlob& operator=(labelBlob&&) = default;
    
protected:
    
    boost::signals2::signal<results_ready_cb>* signal_results_ready;
    boost::signals2::signal<graphics_ready_cb>* signal_graphics_ready;
    
private:
    labelBlob ();
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
    
  
};

}

#endif /* labelBlob_hpp */
