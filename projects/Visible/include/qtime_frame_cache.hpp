#ifndef __QTIME_FRAMES_
#define __QTIME_FRAMES_

//#include "cinder/app/App.h"
#include <cinder/Channel.h>
#include <cinder/Area.h>
#include <limits>
#include "cinder/qtime/QuickTime.h"
#include "cinder/qtime/QuickTimeUtils.h"
#include "cinder/qtime/QuicktimeGl.h"
#include "cinder/Thread.h"
#include "sshist.hpp"
#include "roiWindow.h"
#include <fstream>
#include <mutex>
#include <memory>
#include <map>
#include "timestamp.h"
#include "mediaInfo.h"
#include "time_index.h"
#include "avReader.hpp"
#include "lifFile.hpp"


using namespace ci;
using namespace std;



typedef std::pair<SurfaceRef, index_time_t> SurTiIndexRef_t;

typedef std::function<SurfaceRef ()> getSurfaceCb_t;

class qTimeFrameCache : tiny_media_info 
{
public:
    typedef std::map<time_spec_t, int64_t> timeToIndex;
    typedef std::map<int64_t, time_spec_t> indexToTime;
    typedef std::vector<SurTiIndexRef_t> container_t;
    typedef typename container_t::size_type container_index_t;
    
    typedef std::map<int64_t, container_index_t > indexToContainer;
    
    // Cinder Movie requires rendering though the App
    static std::shared_ptr<qTimeFrameCache> create (const ci::qtime::MovieSurfaceRef& movie);
    static std::shared_ptr<qTimeFrameCache> create (const ci::qtime::MovieGlRef& movie);
    
    // Offline movie loader
    static std::shared_ptr<qTimeFrameCache> create (const std::shared_ptr<avcc::avReader>& assetReader);
    
    // From directory of images
    static std::shared_ptr<qTimeFrameCache> create (const std::vector<ci::Surface8uRef>& folderImages);
    
    // From LIF files
    static std::shared_ptr<qTimeFrameCache> create (lifIO::LifSerie&);
    

    // todo: also return index to time mapping 
    size_t convertTo (std::vector<roiWindow<P8U> >& dst);
    
    // Initializes for the movie. Frame indices are generated for unique increasing time stamps.
    // time-stamped Frames are copied and cached at the first load. Further references to the frame
    // by time stamp or index is from cache.
    qTimeFrameCache ( const tiny_media_info& );
    bool isValid () const;
    
    tiny_media_info& media_info ();

    const std::ostream& print_to_ (std::ostream& std_stream);

    const std::vector<std::string>& channel_names () const;
    void channel_names (const std::vector<std::string>& cnames) const; 
    
    // Load a frame at the time stamp indicated.
    // If a frame at that time stamp is already cached, it will return false
    // If the frame is loaded in, it will return true.
    bool loadFrame (const Surface8uRef frame, const time_spec_t& tic );
    
    const Surface8uRef  getFrame (const int64_t) const;
    const Surface8uRef  getFrame (const time_spec_t& ) const;
    
    
    
    bool checkFrame (const int64_t) const;
    bool checkFrame (const time_spec_t& ) const;
    
    int64_t indexFromTime (const time_spec_t& time) const;

    const index_time_t& currentIndexTime () const;
    size_t count () const;
    
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
    mutable index_time_t     mCurrentIndexTime;
    timeToIndex m_tti;
    indexToTime m_itt;
    indexToContainer m_itIter;
    std::vector<time_spec_t> m_time_hist;
    getSurfaceCb_t m_getSurface_cb;
    mutable std::pair<uint32_t,uint32_t> m_stats;
    mutable std::vector<std::string> m_names;
    mutable std::mutex			mMutex;
};




#endif // __QTIME_VC_IMPL__
