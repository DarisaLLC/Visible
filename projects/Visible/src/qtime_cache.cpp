// Copyright 2003 Reify, Inc.

#include "vf_qtime_cache_impl.hpp"
#include <stdio.h>
#include "vf_types.h"


using namespace std;
using namespace ci;


namespace anonymous
{
    template<typename P, int row_alignment = 16>
    int32 row_bytes (int width)
    {
        int32 rowBytes = width * sizeof(P);
        int32 pad = rowBytes % row_alignment;
        
        if (pad) pad = row_alignment - pad;
        
        // Total storage need
        return rowBytes + pad;
    }
}

//#define VID_TRACE


#ifdef VID_TRACE

enum fctName {
    fnGetFrameI = 0,
    fnGetFrameT,
    fnInternalGetFrame,
    fnCacheAlloc,
    fnCacheInsert,
    fnUnlockFrame
};

typedef struct traceInfo {
    fctName  name;
    uint32 dToken;
    bool     enter;
    uint32 frameIndex;
    rcFrame* fp;
} traceInfo;

static const uint32 traceSz = 1000;
static traceInfo traceBuf[traceSz];
static const uint32 traceIndex = 0;
static const uint32 traceCount = 0;
static const rcMutex  traceMutex;
static const uint32 tracePrint = 0;

void vcAddTrace(fctName name, bool enter, uint32 frameIndex,
                rcFrame* fp, uint32 dToken)
{
    std::lock_guard<std::mutex>  lock(traceMutex);
    
    if (traceIndex == traceSz)
        traceIndex = 0;
    
    traceBuf[traceIndex].name = name;
    traceBuf[traceIndex].dToken = dToken;
    traceBuf[traceIndex].enter =enter;
    traceBuf[traceIndex].frameIndex = frameIndex;
    traceBuf[traceIndex].fp = fp;
    
    traceIndex++; traceCount++;
}

void vcDumpTrace()
{
    std::lock_guard<std::mutex>  lock(traceMutex);
    
    if (tracePrint)
        return;
    
    tracePrint = 1;
    
    fprintf(stderr, "trace count %d\n", traceCount);
    
    uint32 printCount = traceCount < traceSz ? traceCount : traceSz;
    uint32 myIndex = traceCount == traceSz ? traceIndex : 0;
    
    if (printCount)
        fprintf(stderr,
                "  Token    Name             Enter    frameIndex   frame ptr\n");
    
    for ( ; printCount; printCount--) {
        if (myIndex == traceSz)
            myIndex = 0;
        char* name = 0;
        switch (traceBuf[myIndex].name) {
            case fnGetFrameI:
                name = "getFrameI       ";
                break;
            case fnGetFrameT:
                name = "getFrameT       ";
                break;
            case fnInternalGetFrame:
                name = "internalGetFrame";
                break;
            case fnCacheAlloc:
                name = "cacheAlloc      ";
                break;
            case fnCacheInsert:
                name = "cacheInsert     ";
                break;
            case fnUnlockFrame:
                name = "unlockFrame     ";
                break;
            default:
                name = "INVALID         ";
                break;
        }
        char* enter = "yes";
        if (traceBuf[myIndex].enter == false)
            enter = "NO ";
        
        fprintf(stderr, " %06d %s     %s        %02d         0x%X\n",
                traceBuf[myIndex].dToken, name, enter,
                traceBuf[myIndex].frameIndex, (intptr_t)traceBuf[myIndex].fp);
        myIndex++;
    }
}

uint32  QtimeCache::getNextToken()
{
    std::lock_guard<std::mutex>  lock(_cacheMutex); return _debuggingToken++;
}

#endif


QtimeCache*  QtimeCache:: QtimeCacheCtor(const std::string fileName,
                                         uint32 cacheSize,
                                         bool verbose,
                                         bool prefetch,
                                         uint32 maxMemory)

{
    return finishSetup(new  QtimeCache(fileName, cacheSize, verbose, prefetch, maxMemory));
}

QtimeCache*
QtimeCache:: QtimeCacheUTCtor(const vector<time_spec_t>& frameTimes)
{
    return finishSetup(new  QtimeCache(frameTimes));
}

QtimeCache*  QtimeCache::finishSetup( QtimeCache* cacheP)
{
    return cache_manager ().register_cache (cacheP);
}

void  QtimeCache:: QtimeCacheDtor( QtimeCache* cacheP)
{
    if (cacheP)
    {
        cache_manager ().remove (cacheP);
        delete cacheP;
    }
}

QtimeCache:: QtimeCache(std::string fileName, uint32 cacheSize,
                        bool verbose, bool prefetch,
                        uint32 maxMemory)
: _lastTouchIndex(0), _verbose(verbose), _prefetch (prefetch),
_isValid(true), _fatalError( QtimeCacheError::OK), _fileName(fileName),  _pendingCtrl(_cacheMutex),
_cacheID(0), _cacheMisses(0), _cacheHits(0), _prefetchThread(0)

{
#ifdef VID_TRACE
    _debuggingToken = 0;
#endif
    
    if (_fileName.empty()) {
        setError( QtimeCacheError::FileInit);
        return;
    }
    
    _impl = std::shared_ptr<QtimeCache::qtImpl> ( new QtimeCache::qtImpl (fileName) );
    
    // calling this will actually load the movie
    m_ginfo = _impl->movie_info();
    
    if (! _impl->isValid())
    {
        if (_verbose) perror("fopen failed");
        setError( QtimeCacheError::FileInit);
        return;
    }
    
    _frameWidth = m_ginfo.mWidth;
    _frameHeight = m_ginfo.mHeight;
    _frameDepth = rpixel8;
    _averageFrameRate = m_ginfo.mFps;
    _frameCount = m_ginfo.count;
    _baseTime = 0;
    
    /* First, calculate the cache overflow number based on both the
     * number of frames in the movie and the maximum amount of memory
     * the caller wants to use for this purpose.
     */
    _cacheSize = _frameCount;
    _bytesInFrame = anonymous::row_bytes<uint8>(_frameWidth) * _frameHeight;
    
    if (maxMemory != 0) {
        if (_cacheSize > (maxMemory / _bytesInFrame)) {
            _cacheSize = maxMemory / _bytesInFrame;
            
            if (_cacheSize == 0) {
                setError( QtimeCacheError::SystemResources);
                return;
            }
        }
    }
    
    /* If user specified a cache size use it as an upper bound on the
     * cache overflow limit.
     */
    if (cacheSize && (cacheSize < _cacheSize))
        _cacheSize = cacheSize;
    
    std::cout << _frameCount << "," << _cacheSize << std::endl;
    
    _frameCache = std::vector<frame_ref_t>(_frameCount);
    
    
    /* Make all the cache entries available for use by pushing them all
     * onto the unused list.
     */
    for (uint32 i = 0; i < _frameCache.size(); i++)
        _unusedCacheFrames.push_back(&_frameCache[i]);
    
    assert (_unusedCacheFrames.size() >= _cacheSize);
    _cacheOverflowLimit = (int) _unusedCacheFrames.size() - _cacheSize;

    std::cout << _frameCount << "," << _cacheSize << " ovfl " << _cacheOverflowLimit << std::endl;
    
    /* Just about done. If prefetch is enabled, create a prefetch thread
     * here.
     */
    if (_prefetch) {
        _prefetchThread = std::shared_ptr< QtimeCachePrefetchUnit> (new  QtimeCachePrefetchUnit(*this));
        assert (_prefetchThread);
        _prefetchThread->start();
    }
}

bool QtimeCache::prefetch_running () const
{
    return _prefetch && _prefetchThread != 0;
}

QtimeCacheError  QtimeCache::tocLoad()
{
    std::lock_guard<std::mutex> lk (_diskMutex);
    
    if (!_tocItoT.empty())
        return  QtimeCacheError::OK;
    
    _tocItoT.resize(_frameCount);
    assert (_tocTtoI.empty());
    assert (_impl);
    QtimeCache::qtImpl::toc_t_ref raws;
    double dscale = _impl->get_time_index_map(raws);
    std::vector<time_spec_t> frametimes (_frameCount);
    
    for (int fn=0; fn<_frameCount; fn++)
    {
        frametimes[fn] = time_spec_t (raws->at(fn)/dscale);
    }
    for (uint32 frameIndex = 0; frameIndex < _frameCount; frameIndex++)
    {
        if (frameIndex != 0) assert (frametimes[frameIndex].secs() > frametimes[frameIndex-1].secs());
        _tocItoT[frameIndex] = frametimes[frameIndex];
        _tocTtoI[frametimes[frameIndex]] = frameIndex;
    }
    for (int fn=0; fn<_frameCount; fn++)
    {
        if (_tocTtoI[frametimes[fn]] != fn) return QtimeCacheError::CacheInvalid;
    }
    
//    dump_toc ();
    
    return  QtimeCacheError::OK;
}

QtimeCache:: QtimeCache(const vector<time_spec_t>& frameTimes)
: _lastTouchIndex(0), _verbose(false),
_isValid(true), _fatalError( QtimeCacheError::OK), _fileName(""),  _pendingCtrl(_cacheMutex),
_cacheID(0), _cacheMisses(0), _cacheHits(0), _prefetchThread(0)
{
#ifdef VID_TRACE
    _debuggingToken = 0;
#endif
    assert (frameTimes.size());
    
    _frameWidth = _frameHeight = 0;
    _frameDepth = rpixel8;
    _averageFrameRate = 0.0;
    _frameCount = (uint32_t)frameTimes.size();
    _baseTime = 0;
    
    /* This stuff is normally done in tocLoad().
     */
    _tocItoT.resize(_frameCount);
    for (uint32 frameIndex = 0; frameIndex < _frameCount; frameIndex++) {
        if (frameIndex != 0)
            assert (frameTimes[frameIndex] > frameTimes[frameIndex-1]);
        
        _tocItoT[frameIndex] = frameTimes[frameIndex];
        _tocTtoI[frameTimes[frameIndex]] = frameIndex;
    }
    
    _cacheSize = _frameCount;
    _frameCache.resize(_frameCount);
    
    /* Make all the cache entries available for use by pushing them all
     * onto the unused list.
     */
    for (uint32 i = 0; i < _frameCache.size(); i++)
        _unusedCacheFrames.push_back(&_frameCache[i]);
    
    assert (_unusedCacheFrames.size() >= _cacheSize);
    _cacheOverflowLimit = (uint32_t)_unusedCacheFrames.size() - _cacheSize;
}

QtimeCache::~ QtimeCache()
{
    std::lock_guard<std::mutex> lk (_diskMutex);
    
    if (_prefetchThread)
    {
        
        /* Want prefetch thread to end itself, but it may be asleep
         * waiting on input. deal with this using 3 step algorithm:
         * 1) Ask thread to kill itself.
         * 2) Send a phony prefetch request to jostle it awake.
         * 3) Join the thread.
         */
        _prefetchThread->requestSeppuku();
        _prefetchThread->prefetch(0);
        _prefetchThread->join(true);
    }
}

QtimeCacheStatus  QtimeCache::getFrame(uint32 frameIndex,
                                        frame_ref_t& frameBuf,
                                        QtimeCacheError* error,
                                        bool locked)
{
    /* Before stuffing a new frame reference into here, invalidate any
     * existing reference.
     */
    frameBuf = 0;
    
    const uint32 dToken = GET_TOKEN();
    ADD_VID_TRACE(fnGetFrameI, true, frameIndex, frameBuf.mFrameBuf, dToken);
    if (!isValid()) {
        if (error) *error =  QtimeCacheError::CacheInvalid;
        ADD_VID_TRACE(fnGetFrameI, false, frameIndex, frameBuf.mFrameBuf, dToken);
        return  QtimeCacheStatus::Error;
    }
    
    if (frameIndex >= _frameCount) {
        setError( QtimeCacheError::NoSuchFrame);
        if (error) *error =  QtimeCacheError::NoSuchFrame;
        ADD_VID_TRACE(fnGetFrameI, false, frameIndex, frameBuf.mFrameBuf, dToken);
        return  QtimeCacheStatus::Error;
    }
    
    QtimeCacheError toc_error = tocLoad();
    if (toc_error !=  QtimeCacheError::OK) {
        if (error) *error = toc_error;
        ADD_VID_TRACE(fnGetFrameI, false, 0xFFFFFFFF, frameBuf.mFrameBuf, dToken);
        return  QtimeCacheStatus::Error;
    }
    
    QtimeCacheStatus status =  QtimeCacheStatus::OK;
    if (locked)
        status = internalGetFrame(frameIndex, frameBuf, error, dToken);
    else
        frameBuf.setCachedFrameIndex(_cacheID, frameIndex);
    
    ADD_VID_TRACE(fnGetFrameI, false, frameIndex, frameBuf.mFrameBuf, dToken);
    
    return status;
}

QtimeCacheStatus  QtimeCache::getFrame(const time_spec_t& time,
                                        frame_ref_t& frameBuf,
                                        QtimeCacheError* error,
                                        bool locked)
{
    /* Before stuffing a new frame reference into here, invalidate any
     * existing reference.
     */
    frameBuf = 0;
    
    const uint32 dToken = GET_TOKEN();
    ADD_VID_TRACE(fnGetFrameT, true, 0xFFFFFFFF, frameBuf.mFrameBuf, dToken);
    
    if (!isValid()) {
        if (error) *error =  QtimeCacheError::CacheInvalid;
        ADD_VID_TRACE(fnGetFrameT, false, 0xFFFFFFFF, frameBuf.mFrameBuf, dToken);
        return  QtimeCacheStatus::Error;
    }
    
    QtimeCacheError status = tocLoad();
    if (status !=  QtimeCacheError::OK) {
        if (error) *error = status;
        ADD_VID_TRACE(fnGetFrameT, false, 0xFFFFFFFF, frameBuf.mFrameBuf, dToken);
        return  QtimeCacheStatus::Error;
    }
    
    assert (!_tocTtoI.empty());
    
    map<time_spec_t, uint32>::iterator frameIndexPtr;
    frameIndexPtr = _tocTtoI.find(time);
    
    if (frameIndexPtr == _tocTtoI.end()) {
        setError( QtimeCacheError::NoSuchFrame);
        if (error) *error =  QtimeCacheError::NoSuchFrame;
        ADD_VID_TRACE(fnGetFrameT, false, 0xFFFFFFFF, frameBuf.mFrameBuf, dToken);
        return  QtimeCacheStatus::Error;
    }
    
    QtimeCacheStatus status2 =  QtimeCacheStatus::OK;
    if (locked)
        status2 = internalGetFrame(frameIndexPtr->second, frameBuf, error, dToken);
    else
        frameBuf.setCachedFrameIndex(_cacheID, frameIndexPtr->second);
    
    ADD_VID_TRACE(fnGetFrameT, false, frameIndexPtr->second,
                  frameBuf.mFrameBuf, dToken);
    
    return status2;
}

// Static method for mapping an error value to a string
std::string  QtimeCache::getErrorString( QtimeCacheError error)
{
    switch (error) {
        case  QtimeCacheError::OK:
            return std::string("Video cache: OK");
        case  QtimeCacheError::FileInit:
            return std::string("Video cache: video file initialization error");
        case  QtimeCacheError::FileSeek:
            return std::string("Video cache: video file seek error");
        case  QtimeCacheError::FileRead:
            return std::string("Video cache: video file read error");
        case  QtimeCacheError::FileClose:
            return std::string("Video cache: video file close error");
        case  QtimeCacheError::FileFormat:
            return std::string("Video cache: video file invalid/corrupted error");
        case  QtimeCacheError::FileUnsupported:
            return std::string("Video cache: video file format unsupported error");
        case  QtimeCacheError::FileRevUnsupported:
            return std::string("Video cache: video file revision unsupported error");
        case  QtimeCacheError::SystemResources:
            return std::string("Video cache: Inadequate system resources");
        case  QtimeCacheError::NoSuchFrame:
            return std::string("Video cache: no video frame at given timestamp/frame index");
        case  QtimeCacheError::CacheInvalid:
            return std::string("Video cache: previous error put cache in invalid state");
        case  QtimeCacheError::BomUnsupported:
            return std::string("Video cache: unsupported byte order error");
        case  QtimeCacheError::DepthUnsupported:
            return std::string("Video cache: unsupported image depth error");
        case  QtimeCacheError::UnInitialized:
        default:
            return std::string("Video cache: unintialized or unknown error");
            // Note: no default case to force a compiler warning if a new enum
            // value is defined without adding a corresponding string here.
    }
    
    return std::string(" Video cache: undefined error" );
}

QtimeCacheError  QtimeCache::getFatalError() const
{
    std::lock_guard<std::mutex>  lock(const_cast< QtimeCache*>(this)->_cacheMutex);
    
    return _fatalError;
}

// Returns the timestamp in the movie closest to the goal time.
QtimeCacheStatus
QtimeCache::closestTimestamp(const time_spec_t& goalTime,
                             time_spec_t& match,
                             QtimeCacheError* error)
{
    if (!isValid()) {
        if (error) *error =  QtimeCacheError::CacheInvalid;
        return  QtimeCacheStatus::Error;
    }
    
    QtimeCacheError status = tocLoad();
    if (error) *error = status;
    if (status !=  QtimeCacheError::OK)
        return  QtimeCacheStatus::Error;
    
    assert (!_tocTtoI.empty());
    
    /* Find timestamp > goal timestamp.
     */
    map<time_spec_t, uint32>::iterator it;
    it = _tocTtoI.upper_bound(goalTime);
    
    if (it == _tocTtoI.end()) {
        /* Goal is >= last timestamp so just return it.
         */
        it--;
        match = it->first;
    } else if (it == _tocTtoI.begin()) {
        /* Goal is < first timestamp so just return it.
         */
        match = it->first;
    }
    else {
        const time_spec_t afterGoal = it->first;
        it--;
        const time_spec_t beforeOrEqualGoal = it->first;
        
        //  @note: goalTime.bi_compare (beforeOrEqualGoal, afterGoal, match);
        if ((afterGoal - goalTime) < (goalTime - beforeOrEqualGoal))
            match = afterGoal;
        else
            match = beforeOrEqualGoal;
    }
    
    return  QtimeCacheStatus::OK;
}

// Returns first timestamp > goalTime.
QtimeCacheStatus
QtimeCache::nextTimestamp(const time_spec_t& goalTime,
                          time_spec_t& match,
                          QtimeCacheError* error)
{
    if (!isValid()) {
        if (error) *error =  QtimeCacheError::CacheInvalid;
        return  QtimeCacheStatus::Error;
    }
    
    QtimeCacheError status = tocLoad();
    if (status !=  QtimeCacheError::OK) {
        if (error) *error = status;
        return  QtimeCacheStatus::Error;
    }
    
    assert (!_tocTtoI.empty());
    
    /* Find timestamp > goal timestamp.
     */
    map<time_spec_t, uint32>::iterator it;
    it = _tocTtoI.upper_bound(goalTime);
    
    if (it == _tocTtoI.end()) {
        /* Goal is >= last timestamp. Return error.
         */
        if (error) *error =  QtimeCacheError::NoSuchFrame;
        return  QtimeCacheStatus::Error;
    }
    
    match = it->first;
    
    return  QtimeCacheStatus::OK;
}

// Returns timestamp closest to goalTime that is < goalTime
QtimeCacheStatus
QtimeCache::prevTimestamp(const time_spec_t& goalTime,
                          time_spec_t& match,
                          QtimeCacheError* error)
{
    if (!isValid()) {
        if (error) *error =  QtimeCacheError::CacheInvalid;
        return  QtimeCacheStatus::Error;
    }
    
    QtimeCacheError status = tocLoad();
    if (status !=  QtimeCacheError::OK) {
        if (error) *error = status;
        return  QtimeCacheStatus::Error;
    }
    
    assert (!_tocTtoI.empty());
    
    /* Find timestamp >= goal timestamp.
     */
    map<time_spec_t, uint32>::iterator it;
    it = _tocTtoI.lower_bound(goalTime);
    
    if (it == _tocTtoI.begin()) {
        /* Goal is <= first timestamp. Return error.
         */
        if (error) *error =  QtimeCacheError::NoSuchFrame;
        return  QtimeCacheStatus::Error;
    }
    
    it--;
    match = it->first;
    
    return  QtimeCacheStatus::OK;
}

// Returns the timestamp of the first frame in the movie.
QtimeCacheStatus
QtimeCache::firstTimestamp(time_spec_t& match,
                           QtimeCacheError* error)
{
    if (!isValid()) {
        if (error) *error =  QtimeCacheError::CacheInvalid;
        return  QtimeCacheStatus::Error;
    }
    
    QtimeCacheError status = tocLoad();
    if (status !=  QtimeCacheError::OK) {
        if (error) *error = status;
        return  QtimeCacheStatus::Error;
    }
    
    assert (!_tocTtoI.empty());
    
    match = _tocTtoI.begin()->first;
    
    return  QtimeCacheStatus::OK;
}

// Returns the timestamp of the last frame in the movie.
QtimeCacheStatus
QtimeCache::lastTimestamp(time_spec_t& match,
                          QtimeCacheError* error)
{
    if (!isValid()) {
        if (error) *error =  QtimeCacheError::CacheInvalid;
        return  QtimeCacheStatus::Error;
    }
    
    QtimeCacheError status = tocLoad();
    if (status !=  QtimeCacheError::OK) {
        if (error) *error = status;
        return  QtimeCacheStatus::Error;
    }
    
    assert (!_tocTtoI.empty());
    
    map<time_spec_t, uint32>::iterator it;
    it = _tocTtoI.end();
    it--;
    
    match = it->first;
    
    return  QtimeCacheStatus::OK;
}

// Returns the timestamp for the frame at frameIndex.
QtimeCacheStatus
QtimeCache::frameIndexToTimestamp(uint32 frameIndex,
                                  time_spec_t& match,
                                  QtimeCacheError* error)
{
    if (!isValid()) {
        if (error) *error =  QtimeCacheError::CacheInvalid;
        return  QtimeCacheStatus::Error;
    }
    
    QtimeCacheError status = tocLoad();
    if (status !=  QtimeCacheError::OK) {
        if (error) *error = status;
        return  QtimeCacheStatus::Error;
    }
    
    if (frameIndex >= _tocItoT.size()) {
        /* No frame for given frame index. Return error.
         */
        if (error) *error =  QtimeCacheError::NoSuchFrame;
        return  QtimeCacheStatus::Error;
    }
    
    match = _tocItoT.at(frameIndex);
    
    return  QtimeCacheStatus::OK;
}

// Returns the frame index for the frame at time timestamp (must be
// exact match).
QtimeCacheStatus
QtimeCache::timestampToFrameIndex(const time_spec_t& timestamp,
                                  uint32& match,
                                  QtimeCacheError* error)
{
    if (!isValid()) {
        if (error) *error =  QtimeCacheError::CacheInvalid;
        return  QtimeCacheStatus::Error;
    }
    
    QtimeCacheError status = tocLoad();
    if (status !=  QtimeCacheError::OK) {
        if (error) *error = status;
        return  QtimeCacheStatus::Error;
    }
    
    assert (!_tocTtoI.empty());
    
    map<time_spec_t, uint32>::iterator it;
    it = _tocTtoI.find(timestamp);
    
    if (it == _tocTtoI.end()) {
        /* No frame for given timestamp. Return error.
         */
        if (error) *error =  QtimeCacheError::NoSuchFrame;
        return  QtimeCacheStatus::Error;
    }
    
    match = it->second;
    
    return  QtimeCacheStatus::OK;
}

bool  QtimeCache::isValid() const
{
    std::lock_guard<std::mutex>  lock(const_cast< QtimeCache*>(this)->_cacheMutex);
    
    return _isValid;
}



uint32  QtimeCache::cacheMisses() const
{
    std::lock_guard<std::mutex>  lock(const_cast< QtimeCache*>(this)->_cacheMutex);
    
    return _cacheMisses;
}

uint32  QtimeCache::cacheHits() const
{
    std::lock_guard<std::mutex>  lock(const_cast< QtimeCache*>(this)->_cacheMutex);
    
    return _cacheHits;
}

void  QtimeCache::setError ( QtimeCacheError error)
{
    std::lock_guard<std::mutex>  lock(_cacheMutex);
    
    /* Don't bother setting this if a fatal error has already occurred.
     */
    if (_isValid == false)
        return;
    
    switch (error) {
        case  QtimeCacheError::FileInit:
        case  QtimeCacheError::FileSeek:
        case  QtimeCacheError::FileRead:
        case  QtimeCacheError::FileClose:
        case  QtimeCacheError::FileFormat:
        case  QtimeCacheError::FileUnsupported:
        case  QtimeCacheError::FileRevUnsupported:
        case  QtimeCacheError::SystemResources:
        case  QtimeCacheError::BomUnsupported:
        case  QtimeCacheError::DepthUnsupported:
        case  QtimeCacheError::UnInitialized:
            _fatalError = error;
            _isValid = false;
            break;
            
        case  QtimeCacheError::NoSuchFrame:
        case  QtimeCacheError::OK:
            break;
            
        case  QtimeCacheError::CacheInvalid:
            assert (0);
            break;
    }
}

QtimeCacheStatus
QtimeCache::internalGetFrame(uint32 frameIndex,
                             frame_ref_t& frameBufPtr,
                             QtimeCacheError* error,
                             const uint32 dToken)
{
    ADD_VID_TRACE(fnInternalGetFrame, true, frameIndex,
                  frameBufPtr.mFrameBuf, dToken);
    
    /* Read in the frame using the following algorithm:
     *
     * Step 1: Call cache allocation fct to check and see if the frame
     * is already in cache. If so, frameBufPtr will be pointed towards
     * this frame and processing will be complete.
     *
     * Step 2: If not, the cache allocation fct will return a pointer to
     * an available frame buffer from within the cache. Read the frame
     * data in from disk and store it in this frame buffer. (Note: If
     * there aren't any cache slots available, an error is returned.)
     *
     * Step 3: Call cache insert fct to add this to the set of available
     * cached frames. This will also set frameBufPtr to point towards
     * the newly cached frame.
     *
     * Note that this gets broken into three steps to allow the cache
     * mutex to be freed during the possibly costly disk read operation,
     * with the idea of allowing other threads concurrent access to the
     * cache.
     */
    frame_ref_t* cacheFrameBufPtr;
    
    if (cacheAlloc(frameIndex, frameBufPtr, cacheFrameBufPtr, dToken) ==
        QtimeCacheStatus::OK) {
        ADD_VID_TRACE(fnInternalGetFrame, false, frameIndex,
                      frameBufPtr.mFrameBuf, dToken);
        return  QtimeCacheStatus::OK;
    }
    
    /* If status is error but no frame was provided some other thread
     * is already loading the frame into memory. Go to sleep until
     * that thread completes the work.
     */
    if (!cacheFrameBufPtr) {
        _pendingCtrl.wait(frameBufPtr.mFrameBuf);
        assert (frameBufPtr.mFrameBuf != 0);
        ADD_VID_TRACE(fnInternalGetFrame, false, frameIndex,
                      frameBufPtr.mFrameBuf, dToken);
        return  QtimeCacheStatus::OK;
    }
    
    assert (cacheFrameBufPtr->refCount() == 1);
    
    time_spec_t timestamp;
    {
      //  std::lock_guard<std::mutex>  lock(_diskMutex);
        assert (_impl);
        
        /* Read in the frame from disk.
         */
        if (! _impl->checkPlayable())
        {
            setError( QtimeCacheError::FileSeek);
            if (error) *error =  QtimeCacheError::FileSeek;
            /* Restore cache ID and frame index which were cleared by
             * calling function.
             */
            frameBufPtr.mCacheCtrl = _cacheID;
            frameBufPtr.mFrameIndex = frameIndex;
            return  QtimeCacheStatus::Error;
        }

        if (! _impl->getSurfaceAndCopy (*cacheFrameBufPtr, frameIndex))
        {
            if (_verbose) perror("fread of frame data during cache fill failed");
            setError( QtimeCacheError::FileRead);
            if (error) *error =  QtimeCacheError::FileRead;
            /* Restore cache ID and frame index which were cleared by
             * calling function.
             */
            frameBufPtr.mCacheCtrl = _cacheID;
            frameBufPtr.mFrameIndex = frameIndex;
            return  QtimeCacheStatus::Error;
        }
    } // End of: { rcLock lock(_diskMutex);
    
    (*cacheFrameBufPtr)->setTimestamp(timestamp);
    
    assert (cacheFrameBufPtr->refCount() == 1);
    
    QtimeCacheStatus status = cacheInsert(frameIndex, *cacheFrameBufPtr,
                                           dToken);
    
    assert (frameBufPtr->frameIndex() == frameIndex);
    
    ADD_VID_TRACE(fnInternalGetFrame, false, frameIndex, frameBufPtr.mFrameBuf,
                  dToken);
    
    return status;
}

QtimeCacheStatus
QtimeCache::cacheAlloc(uint32 frameIndex,
                       frame_ref_t& userFrameBuf,
                       frame_ref_t*& cacheFrameBufPtr,
                       const uint32 dToken)
{
#ifndef VID_TRACE
    UnusedParameter (dToken);
#endif
    
    std::unique_lock <std::mutex>  lock(_cacheMutex);
    ADD_VID_TRACE(fnCacheAlloc, true, frameIndex, userFrameBuf.mFrameBuf, dToken);
    
    /* First, look to see if the frame has been cached. If so, return
     * it to the caller.
     */
    map<uint32, frame_ref_t*>::iterator cachedEntryPtr;
    cachedEntryPtr = _cachedFramesItoB.find(frameIndex);
    
    if (cachedEntryPtr != _cachedFramesItoB.end()) {
        frame_ref_t* bufPtr = cachedEntryPtr->second;
        assert ((*bufPtr)->frameIndex() == frameIndex);
        
        /* If frame is in the unlocked map, remove it so no one else
         * tries to grab it.
         */
        map<uint32, int64>::iterator unlockedPtrItoL;
        unlockedPtrItoL = _unlockedFramesItoL.find(frameIndex);
        
        if (unlockedPtrItoL != _unlockedFramesItoL.end()) {
#ifdef VID_TRACE
            if (bufPtr.refCount() != 1) {
                fprintf(stderr, "C token %d fi %d frame* 0x%X\n", dToken, frameIndex,
                        (intptr_t)bufPtr->mFrameBuf);
                DUMP_VID_TRACE();
            }
#endif
            assert (bufPtr->refCount() == 1);
            
            int64 lastUseIndex = unlockedPtrItoL->second;
            
            map<int64, uint32>::iterator unlockedPtrLtoI;
            unlockedPtrLtoI = _unlockedFramesLtoI.find(lastUseIndex);
            
            assert (unlockedPtrLtoI != _unlockedFramesLtoI.end());
            assert (unlockedPtrLtoI->second == frameIndex);
            
            _unlockedFramesItoL.erase(unlockedPtrItoL);
            _unlockedFramesLtoI.erase(unlockedPtrLtoI);
        }
        
        assert (bufPtr->mCacheCtrl == 0);
        userFrameBuf.setCachedFrame(*bufPtr, _cacheID, frameIndex);
        
        _cacheHits++;
        ADD_VID_TRACE(fnCacheAlloc, false, frameIndex, userFrameBuf.mFrameBuf,
                      dToken);
        return  QtimeCacheStatus::OK;
    } // End of: if (cachedEntryPtr != _cachedFramesItoB.end())
    
    cacheFrameBufPtr = 0;
    
    /* Not currently in cache. If load is pending add the user frame
     * buffer to the pending list and return to caller to allow
     * him to wait for load to complete.
     */
    map<uint32, vector<frame_ref_t*> >::iterator pendingEntry;
    pendingEntry = _pending.find(frameIndex);
    if (pendingEntry != _pending.end()) {
        vector<frame_ref_t*>& pendingBuffers = pendingEntry->second;
        pendingBuffers.push_back(&userFrameBuf);
        return  QtimeCacheStatus::Error;
    }
    
    /* Frame is neither in cache nor pending. Allocate a frame buffer
     * and return it to the caller. The caller is responsible for
     * loading the frame from disk and seeing that it gets linked
     * into the cache.
     */
    if ((_unusedCacheFrames.size() > _cacheOverflowLimit) ||
        (_unlockedFramesLtoI.empty())) {
        /* Grab an unused cache entry if either:
         * a) We haven't reached the cache's overflow limit, or
         * b) The unlocked portion of the cache is empty.
         */
        assert (!_unusedCacheFrames.empty());
        cacheFrameBufPtr = _unusedCacheFrames.front();
        assert (cacheFrameBufPtr->refCount() < 2);
        assert (cacheFrameBufPtr->mCacheCtrl == 0);
        _unusedCacheFrames.pop_front();
        if (*cacheFrameBufPtr == 0) {
            *cacheFrameBufPtr =
            frame_ref_t(new raw_frame (frameWidth(),
                                    frameHeight(),
                                    frameDepth()));
            assert (*cacheFrameBufPtr != 0);
            (*cacheFrameBufPtr)->cacheCtrl(_cacheID);
        }
#ifdef VID_TRACE
        if (cacheFrameBufPtr.refCount() != 1) {
            fprintf(stderr, "A token %d fi %d ref_count %d frame* 0x%X\n", dToken,
                    frameIndex, cacheFrameBufPtr.refCount(),
                    (intptr_t)cacheFrameBufPtr->mFrameBuf);
            DUMP_VID_TRACE();
        }
#endif
    }
    else if (!_unlockedFramesLtoI.empty()) {
        /* Reuse the least-recently-used unlocked frame. Step 1 is to
         * figure out the LRU frame's frame index and then clear the
         * related info from the unlocked frame maps.
         */
        map<int64, uint32>::iterator unlockedPtrLtoI;
        unlockedPtrLtoI = _unlockedFramesLtoI.begin();
        assert (unlockedPtrLtoI != _unlockedFramesLtoI.end());
        
        uint32 oldFrameIndex = unlockedPtrLtoI->second;
        
        map<uint32, int64>::iterator unlockedPtrItoL;
        unlockedPtrItoL = _unlockedFramesItoL.find(oldFrameIndex);
        
        assert (unlockedPtrItoL != _unlockedFramesItoL.end());
        assert (unlockedPtrItoL->first == unlockedPtrLtoI->second);
        assert (unlockedPtrItoL->second == unlockedPtrLtoI->first);
        
        _unlockedFramesItoL.erase(unlockedPtrItoL);
        _unlockedFramesLtoI.erase(unlockedPtrLtoI);
        
        /* Step 2 - Remove association of frame buffer with old frame
         * index.
         */
        cachedEntryPtr = _cachedFramesItoB.find(oldFrameIndex);
        assert (cachedEntryPtr != _cachedFramesItoB.end());
        cacheFrameBufPtr = cachedEntryPtr->second;
        if (!((*cacheFrameBufPtr)->frameIndex() == oldFrameIndex))
            printf("error %d %d\n", (*cacheFrameBufPtr)->frameIndex(),
                   oldFrameIndex);
        assert ((*cacheFrameBufPtr)->frameIndex() == oldFrameIndex);
        (*cacheFrameBufPtr)->frameIndex(0);
        
        _cachedFramesItoB.erase(cachedEntryPtr);
        
#ifdef VID_TRACE
        if (cacheFrameBufPtr.refCount() != 1) {
            fprintf(stderr, "B token %d fi %d ref_count %d  frame* 0x%X\n", dToken,
                    frameIndex, cacheFrameBufPtr.refCount(),
                    (intptr_t)cacheFrameBufPtr->mFrameBuf);
            DUMP_VID_TRACE();
        }
#endif
        
        assert (cacheFrameBufPtr->refCount() == 1);
    }
    else {
        assert (0); // Should always be able to find a free frame buffer ptr.
        return  QtimeCacheStatus::Error;
    }
    
    /* Setting cacheFrameBufPtr to non-null and returning the error
     * status acts as a signal that the frame should be read from disk.
     */
    assert (cacheFrameBufPtr);
    assert (cacheFrameBufPtr->mCacheCtrl == 0);
    assert ((*cacheFrameBufPtr)->frameIndex() == 0);
    _cacheMisses++;
    
    /* As final step, create an entry in the pending map to allow all
     * the consumers of this frame to get serviced once the frame is
     * available.
     */
    pair<map<uint32, vcPendingFills>::iterator, bool> ret;
    ret = _pending.insert(pair<uint32, vcPendingFills>(frameIndex,
                                                       vcPendingFills()));
    assert (ret.second); // Check that insert succeeded
    
    vcPendingFills& pendingBuffers = ret.first->second;
    pendingBuffers.push_back(&userFrameBuf);
    
    ADD_VID_TRACE(fnCacheAlloc, false, frameIndex+20,
                  cacheFrameBufPtr->mFrameBuf, dToken);
    return  QtimeCacheStatus::Error;
}

QtimeCacheStatus
QtimeCache::cacheInsert(uint32 frameIndex,
                        frame_ref_t& cacheFrameBuf,
                        const uint32 dToken)
{
#ifndef VID_TRACE
    UnusedParameter (dToken);
#endif
    
    std::lock_guard<std::mutex>  lock(_cacheMutex);
    ADD_VID_TRACE(fnCacheInsert, true, frameIndex, userFrameBuf.mFrameBuf, dToken);
    
    assert (cacheFrameBuf.mCacheCtrl == 0);
    assert (cacheFrameBuf.refCount() == 1);
    
    /* Frame shouldn't be in cache
     */
    assert (_cachedFramesItoB.find(frameIndex) == _cachedFramesItoB.end());
    
    assert (cacheFrameBuf->frameIndex() == 0);
    cacheFrameBuf->frameIndex(frameIndex);
    _cachedFramesItoB[frameIndex] = &cacheFrameBuf;
    
    /* Frame is loaded into cache and ready to use. Set up reference
     * to it in all the pending frame buffer ptrs.
     */
    map<uint32, vcPendingFills>::iterator pendingEntry;
    pendingEntry = _pending.find(frameIndex);
    assert (pendingEntry != _pending.end());
    
    vcPendingFills& pendingBuffers = pendingEntry->second;
    uint32 pendingCount = pendingBuffers.size();
    assert (pendingCount);
    
    for (uint32 i = 0; i < pendingCount; i++)
        (pendingBuffers[i])->setCachedFrame(cacheFrameBuf, _cacheID,
                                            frameIndex);
    
    /* Finished with list of pending frame buffers. Delete entry
     * from pending map.
     */
    _pending.erase(pendingEntry);
    
    assert (cacheFrameBuf.refCount() == int32(pendingCount + 1));
    
    /* Broadcast signal to waiting threads so they can check and see if
     * their frame buffer is ready.
     */
    if (pendingCount > 1)
        _pendingCtrl.broadcast();
    
    ADD_VID_TRACE(fnCacheInsert, false, frameIndex, userFrameBuf.mFrameBuf, dToken);
    return  QtimeCacheStatus::OK;
}



void  QtimeCache::unlockFrame(uint32 frameIndex)
{
#ifdef VID_TRACE
    const uint32 dToken = GET_TOKEN();
#endif
    
    std::lock_guard<std::mutex>  lock(_cacheMutex);
    ADD_VID_TRACE(fnUnlockFrame, true, frameIndex, 0, dToken);
    
    map<uint32, frame_ref_t*>::iterator cachedEntryPtr;
    cachedEntryPtr = _cachedFramesItoB.find(frameIndex);
    
    /* If someone else has already removed this frame from the
     * cache there is nothing left to do.
     */
    if (cachedEntryPtr == _cachedFramesItoB.end()) {
        ADD_VID_TRACE(fnUnlockFrame, false, frameIndex, 0, dToken);
        return;
    }
    
    /* Check to see if some other thread has locked this frame
     * between the time the calling frame buffer decremented the
     * count and this function call locked the cache mutex.
     */
    frame_ref_t* bufPtr = cachedEntryPtr->second;
    const uint32 ref_count = (*bufPtr).refCount();
    if (ref_count > 1) {
        ADD_VID_TRACE(fnUnlockFrame, false, frameIndex + 10,
                      (*bufPtr).mFrameBuf, dToken);
        return;
    }
    
    assert ((*bufPtr).refCount() == 1);
    assert ((*bufPtr)->frameIndex() == frameIndex);
    assert ((*bufPtr)->cacheCtrl() == _cacheID);
    
    /* Check to see if some other thread has locked and unlocked this
     * frame between the time the calling frame buffer decremented the
     * count and this function call locked the cache mutex.
     */
    map<uint32, int64>::iterator unlockedPtrItoL;
    unlockedPtrItoL = _unlockedFramesItoL.find(frameIndex);
    
    if (unlockedPtrItoL != _unlockedFramesItoL.end()) {
        ADD_VID_TRACE(fnUnlockFrame, false, frameIndex + 20,
                      (*bufPtr).mFrameBuf, dToken);
        return;
    }
    
    /* Have confirmed this frame buffer should be added to unlocked
     * maps.
     *
     * Note: Wrap around is bad. Could implement an algorithm to
     * compress last touch values down, but for now I think it is
     * good enough to just use a 64 bit counter.
     */
    assert (_lastTouchIndex >= 0);
    assert (_unlockedFramesLtoI.find(_lastTouchIndex) == _unlockedFramesLtoI.end());
    
    _unlockedFramesLtoI[_lastTouchIndex] = frameIndex;
    _unlockedFramesItoL[frameIndex] = _lastTouchIndex;
    _lastTouchIndex++;
    
    /* If the cache is currently overflowing its bounds, remove the LRU
     * element.
     */
    if (_unusedCacheFrames.size() < _cacheOverflowLimit) {
        /* Remove the least-recently-used unlocked frame. Step 1 is to
         * figure out the LRU frame's frame index and then clear the
         * related info from the unlocked frame maps.
         */
        map<int64, uint32>::iterator unlockedPtrLtoI;
        unlockedPtrLtoI = _unlockedFramesLtoI.begin();
        assert (unlockedPtrLtoI != _unlockedFramesLtoI.end());
        
        uint32 oldFrameIndex = unlockedPtrLtoI->second;
        
        map<uint32, int64>::iterator unlockedPtrItoL;
        unlockedPtrItoL = _unlockedFramesItoL.find(oldFrameIndex);
        
        assert (unlockedPtrItoL != _unlockedFramesItoL.end());
        assert (unlockedPtrItoL->first == unlockedPtrLtoI->second);
        assert (unlockedPtrItoL->second == unlockedPtrLtoI->first);
        
        _unlockedFramesItoL.erase(unlockedPtrItoL);
        _unlockedFramesLtoI.erase(unlockedPtrLtoI);
        
        /* Step 2 is to remove it from the cache entirely.
         */
        cachedEntryPtr = _cachedFramesItoB.find(oldFrameIndex);
        assert (cachedEntryPtr != _cachedFramesItoB.end());
        bufPtr = cachedEntryPtr->second;
        _cachedFramesItoB.erase(cachedEntryPtr);
        
        /* Finally, free the associated buffer and put it back on the
         * unused list.
         */
        *bufPtr = 0;
        _unusedCacheFrames.push_back(bufPtr);
        return;
    }
    
    ADD_VID_TRACE(fnUnlockFrame, false, frameIndex, (*bufPtr).mFrameBuf, dToken);
}

void  QtimeCache::prefetchFrame(uint32 frameIndex)
{
    if (_prefetchThread)
        _prefetchThread->prefetch(frameIndex);
}

void  QtimeCache::cachePrefetch(uint32 cacheID, uint32 frameIndex)
{
    QtimeCache* cacheP =   cache_manager ().cacheById(cacheID);
    if (cacheP) cacheP->prefetchFrame(frameIndex);
}

void  QtimeCache::cacheUnlock(uint32 cacheID, uint32 frameIndex)
{
    QtimeCache* cacheP =   cache_manager ().cacheById(cacheID);
    if (cacheP)cacheP->unlockFrame(frameIndex);
}

QtimeCacheStatus  QtimeCache::cacheLock(uint32 cacheID, uint32 frameIndex,
                                         frame_ref_t& frameBuf,
                                         QtimeCacheError* error)
{
    QtimeCache* cacheP =   cache_manager ().cacheById(cacheID);
    if (cacheP) return cacheP->getFrame(frameIndex, frameBuf, error);
    if (error) *error =  QtimeCacheError::CacheInvalid;
    
    return  QtimeCacheStatus::Error;
}

/************************************************************************/
/************************************************************************/
/*                                                                      */
/*                 QtimeCachePrefetchUnit Implementation               */
/*                                                                      */
/************************************************************************/
/************************************************************************/


QtimeCache:: QtimeCachePrefetchUnit:: QtimeCachePrefetchUnit( QtimeCache& cacheCtrl)
: _cacheCtrl(cacheCtrl)
{
    assert (cacheCtrl.isValid());
}

QtimeCache:: QtimeCachePrefetchUnit::~ QtimeCachePrefetchUnit()
{
}


void QtimeCache:: QtimeCachePrefetchUnit::start ()
{
    mThread = std::unique_ptr<std::thread>(new std::thread(std::bind(&QtimeCache:: QtimeCachePrefetchUnit::run, this)));
}

void QtimeCache:: QtimeCachePrefetchUnit::join(bool seppuku)
{
    if (seppuku)
        requestSeppuku();
    mThread->join ();
    if (seppuku)
        clearSeppuku();
}

void QtimeCache:: QtimeCachePrefetchUnit::run ()
{
  //  std::cout << " prefetch thread started " << std::endl;
    
    uint32 frameIndex = 0;
    bool active = true;
    bool killMe = false;
    while (!killMe)
    {
        bool wait = false;
        {
            std::unique_lock <std::mutex> lk (_prefetchMutex);
            cond_.wait(lk,[this]() {return ! _prefetchRequests.empty(); });
            wait = _prefetchRequests.empty();
            if (! wait )
            {
                frameIndex = _prefetchRequests.front();
                _prefetchRequests.pop_front();
            }
        }
        
        killMe = seppukuRequested();
        if (!killMe && ! wait && active)
        {
            /* Have a frame to prefetch. Do this by using getFrame() to load
             * the frame into cache. Since we don't want to lock this frame
             * into memory, merely load it into cache, the frame is set up
             * in a temporary frame buffer.
             */
            QtimeCacheError error;
            frame_ref_t frameBuf;
            QtimeCacheStatus status = _cacheCtrl.getFrame(frameIndex, frameBuf,
                                                           &error);
     //       std::cerr << "got frame\n" << frameIndex << std::endl;
            /* Stop processing if something bad happens */
            if ((status ==  QtimeCacheStatus::Error) && (error !=  QtimeCacheError::NoSuchFrame))
            {
                cerr << "Prefetch error: " << _cacheCtrl.getErrorString(error) << endl;
                active = false;
            }
        }
        
    }
}



void  QtimeCache:: QtimeCachePrefetchUnit::prefetch(uint32 frameIndex)
{
    std::lock_guard <std::mutex> lk (_prefetchMutex);
    _prefetchRequests.push_back(frameIndex);
    cond_.notify_all();
    
}



#ifdef NATIVE_IMPL

QtimeCache:: QtimeCachePrefetchUnit:: QtimeCachePrefetchUnit( QtimeCache& cacheCtrl)
: _cacheCtrl(cacheCtrl), _wait(0)
{
    assert (cacheCtrl.isValid());
}

QtimeCache:: QtimeCachePrefetchUnit::~ QtimeCachePrefetchUnit()
{
}

void  QtimeCache:: QtimeCachePrefetchUnit::run()
{
    uint32 frameIndex = 0;
    bool active = true;
    
    bool killMe = false;
    while (!killMe) {
        bool wait = false;
        {
            rcLock lock(_prefetchMutex);
            
            if (_prefetchRequests.empty())
                wait = true;
            else {
                frameIndex = _prefetchRequests.front();
                _prefetchRequests.pop_front();
            }
        }
        
        /* Guard against chance that above check of prefetch queue
         * didn't occur AFTER dtor gets called. (Because dtor
         * makes dummy prefetch request with disk mutex locked - this
         * will cause hang when this fuction tries to load frame from
         * disk)
         */
        killMe = seppukuRequested();
        if (wait) {
            int32 curVal = _wait.waitUntilGreaterThan(0);
            _wait.decrementVariable(curVal, 0);
        }
        else if (active && !killMe) {
            /* Have a frame to prefetch. Do this by using getFrame() to load
             * the frame into cache. Since we don't want to lock this frame
             * into memory, merely load it into cache, the frame is set up
             * in a temporary frame buffer.
             */
            QtimeCacheError error;
            frame_ref_t frameBuf;
            QtimeCacheStatus status = _cacheCtrl.getFrame(frameIndex, frameBuf,
                                                           &error);
            std::cerr << "got frame\n" << frameIndex << std::endl;
            /* Stop processing if something bad happens */
            if ((status ==  QtimeCacheStatus::Error) &&
                (error !=  QtimeCacheError::NoSuchFrame)) {
                cerr << "Prefetch error: " << _cacheCtrl.getErrorString(error) << endl;
                active = false;
            }
        }
    }
}

void  QtimeCache:: QtimeCachePrefetchUnit::prefetch(uint32 frameIndex)
{
    bool empty;
    {
        rcLock lock(_prefetchMutex);
        empty = _prefetchRequests.empty();
        
        _prefetchRequests.push_back(frameIndex);
    }
    
    if (empty)
        _wait.incrementVariable(1, 0);
}


#endif


