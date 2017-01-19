#ifndef __GEN_MOVIE_INFO__
#define __GEN_MOVIE_INFO__

#include <CoreMedia/CMTime.h>
#include <CoreMedia/CMTimeRange.h>
#include <CoreGraphics/CGBitmapContext.h>

#ifdef __cplusplus
#include <iostream>
#include "coreGLM.h"
using namespace std;
#define BOOL bool
#endif

struct tiny_media_info
{
    uint32_t count;
    double duration;
    double mFps;
    CGSize size;
    CMTimeRange timerange;
    CGAffineTransform  embedded;
    CGAffineTransform  toSize;
    float              orientation; // 0:Right, 90: Up, 180 Left, -90 Down
    BOOL mIsImageFolder;
    BOOL mIsLifSerie;
    uint32_t mChannels;
    
      
    void printout ()
    {
       if (! isImageFolder ())
           std::cout  << " -- General Movie Info -- " << std::endl;
        else
            std::cout  << " -- Image Folder Info -- " << std::endl;
        
       std::cout  << "Dimensions:" << getWidth() << " x " << getHeight() << std::endl;
       std::cout  << "Duration:  " << getDuration() << " seconds" << std::endl;
       std::cout  << "Frames:    " << getNumFrames() << std::endl;
       std::cout  << "Framerate: " << getFramerate() << std::endl;
    }
    
#ifdef __cplusplus
    
    BOOL isImageFolder () const { return mIsImageFolder; }
    int32_t getWidth () const { return size.width; }
    int32_t getHeight () const { return size.height; }
    glm::vec2 getSize () const { return glm::vec2(size.width, size.height); }
    
    double getDuration () const { return duration; }
    double getFramerate () const { return mFps; }
    double getNumFrames () const { return count; }
    double getFrameDuration () const { return getDuration () / getNumFrames (); }
 
    
    
    const std::ostream& operator<< (std::ostream& std_stream)
    {
        if (! isImageFolder ())
            std::cout  << " -- General Movie Info -- " << std::endl;
        else
            std::cout  << " -- Image Folder Info -- " << std::endl;

        std_stream << "Dimensions:" << getWidth() << " x " << getHeight() << std::endl;
        std_stream << "Duration:  " << getDuration() << " seconds" << std::endl;
        std_stream << "Frames:    " << getNumFrames() << std::endl;
        std_stream << "Framerate: " << getFramerate() << std::endl;
        return std_stream;
    }
#endif
    
    
};



#endif
