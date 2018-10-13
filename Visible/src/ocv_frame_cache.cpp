
#include "seq_frame_container.hpp"
#include <iterator>
#include "core/stl_utils.hpp"
#include <algorithm>
/*
 *  Concepte:
 *  Movie consisting of M frames identified by time and index in the movie context
 *  qTimeFrameCache is a container of frames identified by time and index 
 *                access is done through the following maps
 *                movie time <-> movie index
 *                movie index -> container index ( or iterator )
 *
 *  If the container is large enough for all frames in the movie, after initial load, all frame fetches are nearly free
 *  To Do: If the container is smaller then number of frames in the movie, a least used stack will be used to refresh
 */

std::string qTimeFrameCache::getName () const { return "qTimeFrameCache"; }

std::shared_ptr<qTimeFrameCache> qTimeFrameCache::create (const ci::qtime::MovieSurfaceRef& mMovie)
{
    general_movie_info minfo;
    minfo.mWidth = mMovie->getWidth();
    minfo.mHeight = mMovie->getHeight();
    minfo.mFps = mMovie->getFramerate();
    minfo.count = mMovie->getNumFrames ();
    minfo.duration = mMovie->getDuration();
    minfo.mTscale = 1.0;
   return std::make_shared<qTimeFrameCache>( minfo);
    
}

std::shared_ptr<qTimeFrameCache> qTimeFrameCache::create (const ci::qtime::MovieGlRef& mMovie)
{
    general_movie_info minfo;
    minfo.mWidth = mMovie->getWidth();
    minfo.mHeight = mMovie->getHeight();
    minfo.mFps = mMovie->getFramerate();
    minfo.count = mMovie->getNumFrames ();
    minfo.duration = mMovie->getDuration();
    minfo.mTscale = 1.0;
    return std::make_shared<qTimeFrameCache>( minfo);
   
}


qTimeFrameCache::qTimeFrameCache ( const general_movie_info& info ) : general_movie_info(info)
{
    m_time_hist.resize (0);
    m_stats.first = m_stats.second = 0;
}

/*
 * Get the frame at offset from current. 
 * Updates currentIndex only if successful and consistent 
 * Returns a shared_ptr to frame. Valid until that frame is still alive.
 */

const Surface8uRef  qTimeFrameCache::getFrame (int64_t offset) const
{
    std::unique_lock<std::mutex> lock( mMutex );
    Surface8uRef s8;
    indexToContainer::const_iterator p = _checkFrame(offset);
    if (p == m_itIter.end()) return s8;

    assert(p->second > 0 && p->second <= mFrames.size());
    return mFrames[p->second-1].first;
}

const Surface8uRef qTimeFrameCache::getFrame(const time_spec_t& dtime) const
{
    std::unique_lock<std::mutex> lock( mMutex );
    Surface8uRef s8;
    indexToContainer::const_iterator fc = _checkFrame (dtime);
    if (fc == m_itIter.end())
        return s8;

    assert(fc->second > 0 && fc->second <= mFrames.size());
    return mFrames[fc->second-1].first;

}



bool qTimeFrameCache::checkFrame (const time_spec_t& dtime) const
{
    return _checkFrame(dtime) != m_itIter.end();
}


bool qTimeFrameCache::checkFrame (int64_t offset) const
{
    return _checkFrame(offset) != m_itIter.end();
}

bool qTimeFrameCache::isValid () const
{
    mValid = !mFrames.empty();
    return mValid;
}

general_movie_info qTimeFrameCache::movie_info ()
{
    return *((general_movie_info*) this);
}


const std::ostream& qTimeFrameCache::print_to_ (std::ostream& std_stream)
{
    std_stream << (general_movie_info*)this << std::endl;
    std_stream << "Hits:    " << m_stats.first << std::endl;
    std_stream << "Misses: " << m_stats.second << std::endl;
    return std_stream;
}



/*
 * Increment the time histogram if the new time stamp is increasing.
 */
bool qTimeFrameCache::make_unique_increasing_time_indices (const time_spec_t& tic, index_time_t& outi)
{
    bool check = m_time_hist.empty() || tic > m_time_hist.back();
    if (!check) return check;
    auto pre_size = m_time_hist.size();
    outi = make_pair((int64_t)m_time_hist.size(), tic);
    m_time_hist.push_back(tic);
    return (m_time_hist.size() == (pre_size+1) && outi.second == tic);
}


bool qTimeFrameCache::loadFrame (const Surface8uRef frame, const index_time_t& tid )
{
    assert (frame );
    index_time_t ltid (tid);
//    SurTiIndexRef_t pp (std::make_shared<Surface8u>(frame->clone(true)), ltid);
    mFrames.emplace_back(std::make_shared<Surface8u>(frame->clone(true)), ltid);
    m_tti[tid.second] = tid.first;
    m_itt[tid.first] = tid.second;
    m_itIter[tid.first] = mFrames.size();
    mCurrentTime = tid;
    m_stats.first += 1;
    return true;
}

/*
 * Return true of frame was loaded, false if it was already in cache
 */

bool qTimeFrameCache::loadFrame (const Surface8uRef frame, const time_spec_t& tic )
{
    assert(frame );

    std::unique_lock <std::mutex> lock( mMutex );
    if (!mFrames.empty())
    {
        // Check to see if it exists
        auto pp = _checkFrame (tic);
        if (pp != m_itIter.end()) return false;
    }
    // It is not in the cache. 
    // Get a time_index for it and load it
    index_time_t ti;
    return make_unique_increasing_time_indices (tic, ti) && loadFrame(frame, ti);
}

/*
 * Returns an iterator to the container of <TimeIndex,SurfaceRef> 
 * The end iterator is returned if entry is not found.
 */

int64_t qTimeFrameCache::currentIndex (const time_spec_t& time) const
{
    indexToContainer::const_iterator ci = _checkFrame(time);
    if(ci == m_itIter.end()) return -1;
    if(ci == m_itIter.begin()) return 0;
    return std::distance(m_itIter.begin(), ci);
}

qTimeFrameCache::indexToContainer::const_iterator qTimeFrameCache::_checkFrame (const time_spec_t& query_time) const
{
//    std::lock_guard<std::mutex> lock( mMutex );
    // First see if this time exist in time index maps
    timeToIndex::const_iterator low, prev;
    low = m_tti.lower_bound(query_time);
    if (low == m_tti.end()) {
        return m_itIter.end();
    }
    m_stats.second += 1;
    
    if (low == m_tti.begin())
        return m_itIter.begin();

    prev = low;
    --prev;
    if ((query_time - prev->first) < (low->first - query_time))
        return m_itIter.find(prev->second);
    else
        return m_itIter.find(low->second);


}

qTimeFrameCache::indexToContainer::const_iterator qTimeFrameCache::_checkFrame (const int64_t query_index) const
{
    return m_itIter.find(query_index);
}


#if 0
bool getSurfaceAndCopy (cached_frame_ref& ptr)
{
    std::lock_guard<std::mutex> lk(mx);
    if (!mMovie) return false;
    Surface8u surf = mMovie->getSurface ();
    if (!surf) return false;
    
    Surface::Iter iter = mSurface->getIter ( surf->getBounds() );
    int rows = 0;
    while (iter.line () )
    {
        uint8_t* pels = ptr->rowPointer (rows++);
        while ( iter.pixel () ) *pels++ = iter.g ();
    }
    return true;
}
#endif
