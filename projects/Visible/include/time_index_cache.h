#ifndef __TimeIndex_CACHE_
#define __TimeIndex_CACHE_

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
#include "time_index.h"

using namespace ci;
using namespace std;

/*
 * T has to be copy constructable 
 * TODO: supply a clone function
 * T has to be null able.
 */


template<typename T>
class timeIndexCache : public tiny_media_info, public base_signaler
{
public:
    typedef T elem;
    typedef std::pair<elem, index_time_t> ElemTiIndexRef_t;
    typedef std::map<time_spec_t, int64_t> timeToIndex;
    typedef std::map<int64_t, time_spec_t> indexToTime;
    typedef std::vector<ElemTiIndexRef_t> container_t;
    typedef typename container_t::size_type container_index_t;
    typedef std::function< ()> getElemCb_t;
    
    typedef std::map<int64_t, container_index_t > indexToContainer;
    
    static std::shared_ptr<timeIndexCache> create (const timeIndexCache& );

    // Initializes for the movie. Frame indices are generated for unique increasing time stamps.
    // time-stamped Frames are copied and cached at the first load. Further references to the frame
    // by time stamp or index is from cache.
    timeIndexCache ( const tiny_media_info& info ) : tiny_media_info(info)
    {
        m_time_hist.resize (0);
        m_stats.first = m_stats.second = 0;
    }
    bool isValid () const'
    
    tiny_media_info media_info ();

    const std::ostream& print_to_ (std::ostream& std_stream);

    
    
    // Load a frame at the time stamp indicated.
    // If a frame at that time stamp is already cached, it will return false
    // If the frame is loaded in, it will return true.
    bool loadElem (const T& elem, const time_spec_t& tic )
    {
        assert(elem );
        
        std::unique_lock <std::mutex> lock( mMutex );
        if (!mElems.empty())
        {
            // Check to see if it exists
            auto pp = _checkFrame (tic);
            if (pp != m_itIter.end()) return false;
        }
        // It is not in the cache. 
        // Get a time_index for it and load it
        index_time_t ti;
        return make_unique_increasing_time_indices (tic, ti) && loadElem(frame, ti);
    }

    
    /*
     * Get the frame at offset from current.
     * Updates currentIndex only if successful and consistent
     * Returns a shared_ptr to frame. Valid until that frame is still alive.
     */
    
    const T  getFrame (int64_t offset) const
    {
        std::unique_lock<std::mutex> lock( mMutex );
        Surface8uRef s8;
        indexToContainer::const_iterator p = _checkFrame(offset);
        if (p == m_itIter.end()) return s8;
        
        assert(p->second > 0 && p->second <= mFrames.size());
        return mFrames[p->second-1].first;
    }
    
    const getFrame(const time_spec_t& dtime) const
    {
        std::unique_lock<std::mutex> lock( mMutex );
        T s8;
        indexToContainer::const_iterator fc = _checkFrame (dtime);
        if (fc == m_itIter.end())
            return s8;
        
        assert(fc->second > 0 && fc->second <= mElems.size());
        return mElems[fc->second-1].first;
        
    }
    
    bool checkFrame (const time_spec_t& dtime) const
    {
        return _checkFrame(dtime) != m_itIter.end();
    }
    
    
    bool checkFrame (int64_t offset) const
    {
        return _checkFrame(offset) != m_itIter.end();
    }
    
    bool isValid () const
    {
        mValid = !mElems.empty();
        return mValid;
    }

    
    /** \brief returns the name of the concrete subclass.
     * \return the name of the concrete driver.
     */
    std::string getName () const;

    
private:
    mutable bool                mValid;
    mutable container_t      mElements;
    mutable index_time_t     mCurrentTime;
    timeToIndex m_tti;
    indexToTime m_itt;
    indexToContainer m_itIter;
    std::vector<time_spec_t> m_time_hist;
    getElemCb_t m_getSurface_cb;
    mutable std::pair<uint32_t,uint32_t> m_stats;
    mutable std::mutex			mMutex;

    /*
     * Increment the time histogram if the new time stamp is increasing.
     */
    bool make_unique_increasing_time_indices (const time_spec_t& tic, index_time_t& outi)
    {
        bool check = m_time_hist.empty() || tic > m_time_hist.back();
        if (!check) return check;
        auto pre_size = m_time_hist.size();
        outi = make_pair((int64_t)m_time_hist.size(), tic);
        m_time_hist.push_back(tic);
        return (m_time_hist.size() == (pre_size+1) && outi.second == tic);
    }
    
    
    bool loadElem (const T& elem, const index_time_t& tid )
    {
        assert (elem );
        index_time_t ltid (tid);
        mElems.emplace_back(std::make_shared<T>(new T(elem), ltid);
                            m_tti[tid.second] = tid.first;
                            m_itt[tid.first] = tid.second;
                            m_itIter[tid.first] = mElems.size();
                            mCurrentTime = tid;
                            m_stats.first += 1;
                            return true;
                            }
    
    /*
     * Returns an iterator to the container of <TimeIndex,SurfaceRef>
     * The end iterator is returned if entry is not found.
     */
    
    timeIndexCache::indexToContainer::const_iterator _checkFrame (const time_spec_t& query_time) const
    {
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
    
    timeIndexCache::indexToContainer::const_iterator _checkFrame (const int64_t query_index) const
    {
        return m_itIter.find(query_index);
    }
        
};


#endif // __QTIME_VC_IMPL__
