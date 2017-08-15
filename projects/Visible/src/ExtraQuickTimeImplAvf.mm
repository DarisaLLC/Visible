#include "cinder/Cinder.h"
#include <AvailabilityMacros.h>

#include "cinder/gl/platform.h"
#include "cinder/app/AppBase.h"
#include "cinder/app/RendererGl.h"
#include "cinder/Url.h"

#if defined( CINDER_COCOA )
#import <AVFoundation/AVFoundation.h>
#if defined( CINDER_COCOA_TOUCH )
#import <CoreVideo/CoreVideo.h>
#else
#import <CoreVideo/CVDisplayLink.h>
#endif
#endif

#include "cinder/qtime/QuickTimeImplAvf.h"
#include "cinder/qtime/AvfUtils.h"
#include "qtimeAvfLink.h"

using namespace cinder;
using namespace cinder::qtime;


bool qtimeAvfLink::MovieBaseFrameReadyConnect(MovieSurfaceRef& movie, void (*frame_ready)())
{
    AVPlayer* player = movie->getPlayerHandle();
    if ( ! player ) return false;
    
    movie->getNewFrameSignal().connect(frame_ready);
    return true;
}


cm_time qtimeAvfLink::MovieBaseGetCurrentCTime(MovieSurfaceRef& movie)
{
    AVPlayer* player = movie->getPlayerHandle();
    if ( ! player ) return - 1.0;
    
    return cm_time ([player currentTime]);
}


float qtimeAvfLink::MovieBaseGetCurrentTime(MovieSurfaceRef& movie)
{
    AVPlayer* player = movie->getPlayerHandle();
    if ( ! player ) return - 1.0;
    
    return CMTimeGetSeconds([player currentTime]);
}

void qtimeAvfLink::MovieBaseSeekToTime( MovieSurfaceRef& movie, float seconds )
{
    AVPlayer* player = movie->getPlayerHandle();
    if ( ! player ) return;
    
    CMTime seek_time = CMTimeMakeWithSeconds(seconds, [player currentTime].timescale);
    [player seekToTime:seek_time toleranceBefore:kCMTimeZero toleranceAfter:kCMTimeZero];
}

void qtimeAvfLink::MovieBaseSeekToFrame( MovieSurfaceRef& movie, int frame )
{
    AVPlayer* player = movie->getPlayerHandle();
    if ( ! player ) return;
    
    CMTime currentTime = [player currentTime];
    currentTime.timescale = 600;
    CMTime oneFrame = CMTimeMakeWithSeconds(1.0 / movie->getFramerate(), currentTime.timescale);
    CMTime startTime = kCMTimeZero;
    CMTime addedFrame = CMTimeMultiply(oneFrame, frame);
    CMTime added = CMTimeAdd(startTime, addedFrame);
    
    [player seekToTime:added toleranceBefore:oneFrame toleranceAfter:oneFrame];
}


void qtimeAvfLink::MovieBaseSeekToStart(MovieSurfaceRef& movie)
{
    AVPlayer* player = movie->getPlayerHandle();
    if ( ! player ) return;
    
    [player seekToTime:kCMTimeZero];
}


void qtimeAvfLink::GetTocOfMovie(const MovieSurfaceRef& movie, std::vector<cm_time>& toc)
{
    toc.resize(0);
    
    AVPlayer* player = movie->getPlayerHandle();
    if ( ! player ) return;

    uint32_t frame_count = movie->getNumFrames();
    if (frame_count < 2) return;
    
    CMTime currentTime = [player currentTime];
    currentTime.timescale = 1000000000;
    CMTime oneFrame = CMTimeMakeWithSeconds(1.0 / movie->getFramerate(), currentTime.timescale);
    CMTime startTime = kCMTimeZero;

    toc.push_back (cm_time(startTime));
    cm_time frameOne (oneFrame);
    
    for (uint32_t ff = 1; ff < frame_count; ff++)
    {
        toc.push_back(toc[toc.size()-1] + frameOne);
    }
}


