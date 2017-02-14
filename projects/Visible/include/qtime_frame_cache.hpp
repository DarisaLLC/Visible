#ifndef __QTIME_FRAMES_
#define __QTIME_FRAMES_

#include "cinder/app/App.h"
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
#include <functional>
#include <map>
#include "timestamp.h"
#include "base_signaler.h"
#include "mediaInfo.h"
#include "time_index.h"
#include "avReader.hpp"
#include "lifFile.hpp"


using namespace ci;
using namespace std;




typedef std::pair<SurfaceRef, index_time_t> SurTiIndexRef_t;

typedef std::function<SurfaceRef ()> getSurfaceCb_t;

class qTimeFrameCache : tiny_media_info, public base_signaler
{
public:
    typedef std::map<time_spec_t, int64_t> timeToIndex;
    typedef std::map<int64_t, time_spec_t> indexToTime;
    typedef std::vector<SurTiIndexRef_t> container_t;
    typedef typename container_t::size_type container_index_t;
    
    typedef std::map<int64_t, container_index_t > indexToContainer;
    
    static std::shared_ptr<qTimeFrameCache> create (const ci::qtime::MovieSurfaceRef& movie);
    static std::shared_ptr<qTimeFrameCache> create (const ci::qtime::MovieGlRef& movie);
    static std::shared_ptr<qTimeFrameCache> create (const std::shared_ptr<avcc::avReader>& assetReader);
    static std::shared_ptr<qTimeFrameCache> create (const std::vector<ci::Surface8uRef>& folderImages);
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

    // Load a frame at the time stamp indicated.
    // If a frame at that time stamp is already cached, it will return false
    // If the frame is loaded in, it will return true.
    bool loadFrame (const Surface8uRef frame, const time_spec_t& tic );
    
    const Surface8uRef  getFrame (const int64_t) const;
    const Surface8uRef  getFrame (const time_spec_t& ) const;
    
    bool checkFrame (const int64_t) const;
    bool checkFrame (const time_spec_t& ) const;
    
    int64_t currentIndex (const time_spec_t& time) const;
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
    mutable index_time_t     mCurrentTime;
    timeToIndex m_tti;
    indexToTime m_itt;
    indexToContainer m_itIter;
    std::vector<time_spec_t> m_time_hist;
    getSurfaceCb_t m_getSurface_cb;
    mutable std::pair<uint32_t,uint32_t> m_stats;
    mutable std::mutex			mMutex;
};

#if 0
class directoryPlayer : tiny_media_info
{
public:
    directoryPlayer (): mLoaded (false) {}
    
    void reset (const std::shared_ptr<qTimeFrameCache>& qf)
    {
        mCacheRef = qf;
        mLoaded = mCacheRef->isValid();
        count = qf->media_info().getNumFrames();
        duration = qf->media_info().getDuration();
        mFps = qf->media_info().getFramerate ();
        seekToStart();
        mLoaded = true;
    }
    
    bool		isLoaded() const { return mLoaded; }
    //! Returns whether the movie is playable, implying the movie is fully formed and can be played but media data is still downloading
    float		getDuration() const { return getDuration(); }
    //! Returns the movie's framerate measured as frames per second
    float		getFramerate() const { return getFramerate(); }
    //! Returns the total number of frames (video samples) in the movie
    int32_t		getNumFrames() const { return getNumFrames (); }
    
    bool		hasAudio() const { return false; }
    //! Returns whether the first video track in the movie contains an alpha channel. Returns false in the absence of visual media.
    virtual bool hasAlpha() const { return false; }
    
    void		seekToFrame( int64_t frame ) { m_current_frame = frame; }
    void		seekToStart() { m_current_frame = 0; }
    void		seekToEnd() { m_current_frame = getNumFrames() - 1; }
    
    SurfaceRef  getSurface() const
    {
        
        if (! isLoaded ()) return  Surface8uRef ();
        return mCacheRef->getFrame(m_current_frame);
    }
    
private:
    bool mLoaded;
    std::shared_ptr<qTimeFrameCache> mCacheRef;
    int64_t m_current_frame;
    
};

#endif




#endif // __QTIME_VC_IMPL__
