#include "cinder/Cinder.h"
#include <AvailabilityMacros.h>

// This path is used on iOS or Mac OS X 10.8+
#if defined( CINDER_COCOA_TOUCH ) || ( defined( CINDER_MAC ) && ( MAC_OS_X_VERSION_MIN_REQUIRED >= 1080 ) )

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






#endif // defined( CINDER_COCOA_TOUCH ) || ( defined( CINDER_MAC ) && ( MAC_OS_X_VERSION_MIN_REQUIRED >= 1080 ) )
