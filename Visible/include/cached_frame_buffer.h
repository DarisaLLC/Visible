
#ifndef _Cached_Buffer_H_
#define _Cached_Buffer_H_


#include "vf_utils.hpp"
#include "frame.h"


class  cached_frame_ref
{
    
public:
    
    typedef boost::function<void(uint32_t, uint32_t)> locker_fn;
    
    // Constructors
  cached_frame_ref();
  cached_frame_ref( raw_frame* p );
  cached_frame_ref( const cached_frame_ref& p );
    
    // Destructor
    virtual ~cached_frame_ref();
      
    // Assignment operators
    virtual cached_frame_ref& operator= ( const cached_frame_ref& p );
    virtual cached_frame_ref& operator= ( raw_frame* p );
    
    /* For cached frames, the cache controller is called to load the
     * underlying frame from either cache or disk and to lock it into
     * the cache so it is safe to use it.
     *
     * Note: This is a NOOP for uncached frames.
     */
    virtual void lock() const;
    
    /* For cached frames, the cache controller is called to mark this
     * frame as available for reuse. The frame's data is retained in
     * cache, so that a subsequent call to lock() can retrieve the
     * data. Caching is controlled using a least-recently-used
     * algorithm.
     *
     * WARNING: Any raw_frame related memory references, such as the
     * pointer returned by rawData, or a reference to the raw_frame
     * object itself is only valid while the frame is locked. To use
     * these values again, it is necessary to relock the frame and
     * then reread these values.
     *
     * Note: This is a NOOP for uncached frames.
     */
    virtual void unlock() const;
    
    /* Ask cache controller to try and read in this frame now without
     * actually locking it to this object.
     */
    virtual void prefetch() const;
    
    uint32_t frameIndex() const {
        return mFrameIndex;
    }
    
    // Comparison operators
  bool operator== ( const cached_frame_ref& p ) const;
  bool operator!= ( const cached_frame_ref& p ) const;
  bool operator== ( const raw_frame* p ) const;
  bool operator!= ( const raw_frame* p ) const;
  bool operator!() const;
    
    // Accessors
  operator raw_frame*() const;
  const raw_frame& operator*() const;
  raw_frame& operator*();
  const raw_frame* operator->() const;
  raw_frame* operator->();
  int refCount() const;

  void printState() const;
    
    /* Helper fct intended for use by rcVideoCache class only. It allows
     * a shared frame buffer pointer to be initialized pointing to a
     * cached frame.
     */
    void setCachedFrame(cached_frame_ref& p, uint32_t cacheID,
                        uint32_t frameIndex);
    
    /* Helper fct intended for use by rcVideoCache class only. It allows
     * a shared frame buffer pointer to be initialized pointing to a
     * cached frame without reading that frame into memory.
     */
  void setCachedFrameIndex(uint32_t cacheID, uint32_t frameIndex);
    
    /*
     * public but only to avoid excessive friendship
     */
    uint32_t      mCacheCtrl;
    uint32_t      mFrameIndex;
    mutable raw_frame*    mFrameBuf;
    mutable std::mutex mMutex;
    
protected:
    virtual void internalLock();
    virtual void interlock_force_unlock ();
    void internalUnlock( bool , locker_fn );
    void const_free_lock();
   

};


#endif
