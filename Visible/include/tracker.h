
#ifndef __Tracker__
#define __Tracker__

#include "cinder_opencv.h"
#include "time_index.h"
#include "cinder/TriMesh.h"
#include "cinder/Timeline.h"
#include "cinder/Triangulate.h"
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
    tracker (const index_time_t& ti, const Rectf& frame, const vec2 center, const Rectf bounds, float z = 0.0f)
    : index_time_t (ti), mPosition(center), mFrame(frame), mBox(bounds)
    {
        m_ttrace.push_back(ti);
        m_xytrace.push_back (center);
        m_ztrace.push_back(z);
        m_btrace.push_back(bounds);
    }
    
    const vec2 anchor () const { return mPosition; }
    const Rectf box () const { return mBox; }
    
    // Is Tracking Done
    bool done () const
    {
        std::lock_guard<std::mutex> lock(mMutex);
        return mSearching;
    }
    
private:
    polyTimes_t m_ttrace;
    PolyLine2f m_xytrace;
    std::vector<float> m_ztrace;
    std::vector<Rectf> m_btrace;
    bool mSearching;
    
    Rectf mFrame;
    vec2 mPosition;
    Rectf mBox;
    mutable std::mutex mMutex;
    
};



#endif

