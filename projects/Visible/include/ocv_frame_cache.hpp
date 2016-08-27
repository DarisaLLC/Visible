#ifndef __QTIME_FRAMES_
#define __QTIME_FRAMES_

#include <limits>
#include "sshist.hpp"
#include "roiWindow.h"
#include <fstream>
#include <mutex>
#include <memory>
#include <functional>
#include <map>
#include "timestamp.h"
#include "base_signaler.h"
#include "CinderOpenCV.h"
#include "opencv2/highgui.hpp"
#include "opencv_utils.hpp"

using namespace ci;
using namespace std;


struct general_movie_info
{
    static general_movie_info create (const ci::qtime::MovieSurfaceRef& movie);
    
    int32_t getWidth () const { return mWidth; }
    int32_t getHeight () const { return mHeight; }
    double getDuration () const { return duration; }
    double getFramerate () const { return mFps; }
    double getNumFrames () const { return count; }
    double getFrameDuration () const { return getDuration () / getNumFrames (); }
    
    
    uint32_t count;
    uint32_t mWidth;
    uint32_t mHeight;
    double duration;
    double mFps;
    double mTscale;
    
    
    const std::ostream& operator<< (std::ostream& std_stream)
    {
        std_stream << " -- General Movie Info -- " << std::endl;
        std_stream << "Dimensions:" << getWidth() << " x " << getHeight() << std::endl;
        std_stream << "Duration:  " << getDuration() << " seconds" << std::endl;
        std_stream << "Frames:    " << getNumFrames() << std::endl;
        std_stream << "Framerate: " << getFramerate() << std::endl;
        return std_stream;
    }
    
};



typedef std::pair<int64_t, time_spec_t> index_time_t;
typedef std::pair<SurfaceRef, index_time_t> SurTiIndexRef_t;

typedef std::function<SurfaceRef ()> getSurfaceCb_t;

class qTimeFrameCache : general_movie_info, public base_signaler
{
public:
    typedef std::map<time_spec_t, int64_t> timeToIndex;
    typedef std::map<int64_t, time_spec_t> indexToTime;
    typedef std::vector<SurTiIndexRef_t> container_t;
    typedef typename container_t::size_type container_index_t;
    
    typedef std::map<int64_t, container_index_t > indexToContainer;
    
    static std::shared_ptr<qTimeFrameCache> create (const ci::qtime::MovieSurfaceRef& movie);
    static std::shared_ptr<qTimeFrameCache> create (const ci::qtime::MovieGlRef& movie);

    // Initializes for the movie. Frame indices are generated for unique increasing time stamps.
    // time-stamped Frames are copied and cached at the first load. Further references to the frame
    // by time stamp or index is from cache.
    qTimeFrameCache ( const general_movie_info& );
    bool isValid () const;
    
    general_movie_info movie_info ();

    const std::ostream& print_to_ (std::ostream& std_stream);

    // Load a frame at the time stamp indicated.
    // If a frame at that time stamp is already cached, it will return false
    // If the frame is loaded in, it will return true.
    bool loadFrame (const Surface8uRef frame, const time_spec_t& tic );
    
    const Surface8uRef  getFrame (const int64_t) const;
    const Surface8uRef  getFrame (const time_spec_t& ) const;
    
    bool checkFrame (const int64_t) const;
    bool checkFrame (const time_spec_t& ) const;
    
    int64_t currentIndex (const time_spec_t& time) const;
    
    /** \brief returns the name of the concrete subclass.
     * \return the name of the concrete driver.
     */
    std::string getName () const;

    
private:
    bool loadFrame (const Surface8uRef, const index_time_t&);
    indexToContainer::const_iterator _checkFrame (const int64_t) const;
    indexToContainer::const_iterator _checkFrame (const time_spec_t&) const;
    bool make_unique_increasing_time_indices (const time_spec_t&, index_time_t&);
    mutable bool                mValid;
    mutable container_t      mFrames;
    mutable index_time_t     mCurrentTime;
    timeToIndex m_tti;
    indexToTime m_itt;
    indexToContainer m_itIter;
    std::vector<time_spec_t> m_time_hist;
    getSurfaceCb_t m_getSurface_cb;
    mutable std::pair<uint32_t,uint32_t> m_stats;
    mutable std::mutex			mMutex;
};


#endif // __QTIME_VC_IMPL__
