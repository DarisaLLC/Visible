
#ifndef __Tracker__
#define __Tracker__

#include "cinder_opencv.h"
#include "time_index.h"
#include "cinder/TriMesh.h"
#include "cinder/Timeline.h"
#include "cinder/Triangulate.h"
#include "vision/dense_motion.hpp"

#include <mutex>

using namespace std;
using namespace ci;
using namespace cv;

// Representing a image patch create at frame index time
class tracker : private index_time_t
{
public:
    
    typedef std::vector<index_time_t>  polyTimes_t;
    tracker ();
    tracker (const index_time_t& ti, const Rectf& frame, const vec2 center, iPair& fixed_half_size,const iPair& moving_half_size, float z = 0.0f)
    : index_time_t (ti), mPosition(center), mFrame(frame), m_fixed_hs(fixed_half_size), m_moving_hs(moving_half_size)
    {
        m_fixed_box = Rectf(center.x - fixed_half_size.first, center.y - fixed_half_size.second, center.x + fixed_half_size.first, center.y + fixed_half_size.second);
        m_moving_box = Rectf(center.x - moving_half_size.first, center.y - moving_half_size.second, center.x + moving_half_size.first, center.y + moving_half_size.second);
        m_ttrace.push_back(ti);
        m_xytrace.push_back (center);
        m_ztrace.push_back(z);
        m_btrace.push_back(m_fixed_box);
        m_fsize.first = frame.getWidth();
        m_fsize.second = frame.getHeight();
        m_dmPtrRef = std::make_shared<denseMotion>(m_fsize, fixed_half_size, moving_have_size);
        m_count = 0;
    }
    
    const vec2 anchor () const { return mPosition; }
    const Rectf moving_box () const { return m_moving_box; }
    const Rectf fixed_box () const { return m_fixed_box; }
    
    // Is Tracking Done
    bool done () const
    {
        std::lock_guard<std::mutex> lock(mMutex);
        return mSearching;
    }
    
    
    // if out is 0, fixed and moving are in this first image, just fill the pipe and return
    // cycle the denseMotion buffers
    bool update(cv::Mat& image, index_time_t& ti){
        if (! m_dmPtrRef ) return false;
        m_dmPtrRef->update(image);
        if( m_count == 0) { m_count++; return true; }
        
        
    }
    
private:
    polyTimes_t m_ttrace;
    PolyLine2f m_xytrace;
    std::vector<float> m_ztrace;
    std::vector<Rectf> m_btrace;
    bool mSearching;
    iPair m_moving_hs, m_fixed_hs;
    iPair m_fsize;
    Rectf m_moving_box, m_fixed_box;
    Rectf mFrame;
    vec2 mPosition;
    Rectf mBox;
    std::shared_ptr<denseMotion> m_dmPtrRef;
    mutable std::mutex mMutex;
    std::atomic<int> m_count;
    
    
};



#endif

