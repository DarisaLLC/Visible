#ifndef __UT_ALGO__
#define __UT_ALGO__

#include <iostream>
#include <string>
#include "async_producer.h"
#include "core/core.hpp"
#include "vision/histo.h"
#include "algo_in_frames_out_tracks.hpp"

using namespace std;


bool algo_registry_ut (framesInTracksOut::framesInTracksOutRef algo)
{
    return true;
}


#endif

