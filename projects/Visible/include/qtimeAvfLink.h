

#ifndef __qtimeAvfLink___h
#define __qtimeAvfLink___h

#include "cinder/app/App.h"
#include "cinder/qtime/QuicktimeGl.h"

class qtimeAvfLink
{
public:
    static float MovieBaseGetCurrentTime(cinder::qtime::MovieSurfaceRef& movie);
    static void MovieBaseSeekToTime( cinder::qtime::MovieSurfaceRef& movie, float seconds );
    static void MovieBaseSeekToFrame( cinder::qtime::MovieSurfaceRef& movie, int frame );
    static void MovieBaseSeekToStart(cinder::qtime::MovieSurfaceRef& movie);
};







#endif
