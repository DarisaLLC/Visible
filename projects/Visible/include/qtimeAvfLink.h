

#ifndef __qtimeAvfLink___h
#define __qtimeAvfLink___h

#include "cinder/app/App.h"
#include "cinder/qtime/QuicktimeGl.h"
#include "core/cm_time.hpp"

class qtimeAvfLink
{
public:
    static cm_time MovieBaseGetCurrentCTime(cinder::qtime::MovieSurfaceRef& movie);
    static float MovieBaseGetCurrentTime(cinder::qtime::MovieSurfaceRef& movie);
    static void MovieBaseSeekToTime( cinder::qtime::MovieSurfaceRef& movie, float seconds );
    static void MovieBaseSeekToFrame( cinder::qtime::MovieSurfaceRef& movie, int frame );
    static void MovieBaseSeekToStart(cinder::qtime::MovieSurfaceRef& movie);
    static void GetTocOfMovie(const cinder::qtime::MovieSurfaceRef& movie, std::vector<cm_time>& toc);
    static bool MovieBaseFrameReadyConnect(cinder::qtime::MovieSurfaceRef& movie, void (*frame_ready)());
};







#endif
