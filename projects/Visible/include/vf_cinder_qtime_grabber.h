#ifndef __VF_UTILS__QTIME_
#define __VF_UTILS__QTIME_

#include "roi_window.h"

#include <fstream>

using namespace std;



namespace vf_older_utils
{
    
    namespace qtime_support
    {
        
        
        class CinderQtimeGrabber //: public rcFrameGrabber
        {
        public:
            // ctor
            CinderQtimeGrabber( const std::string fileName,   // Input file
                               double frameInterval = -1.0, // Forced frame interval
                               int32  startAfterFrame = -1,
                               int32  frames = -1 );

            // virtual dtor
            virtual ~CinderQtimeGrabber();
            
            virtual bool isValid () const;
            bool contentValid () const;
            //
            // rcFrameGrabber API
            //
            // Start grabbing
            virtual bool start();

            // Stop grabbing
            virtual bool stop();
            
            // Returns the number of frames available
            virtual int32 frameCount();
            
            // Movie grabbers don't have a cache.
            virtual int32 cacheSize();
            
            // Get next frame, assign the frame to ptr
            virtual rcFrameGrabberStatus getNextFrame( rcFrameRef& ptr, bool isBlocking );
            
            // Get name of input source, ie. file name, camera name etc.
            virtual const std::string getInputSourceName();
            
            // Get last error value.
            virtual grabber_error getLastError() const;
            
            // Set last error value
            void setLastError( grabber_error error );
            
            double frame_duration  () const;
            
            const std::ostream& print_to_ (std::ostream& std_stream) const;
            
            
        private:
            class qtfgImpl;
            boost::shared_ptr<qtfgImpl> _impl;
        };
        
    }
}

#endif // __VF_UTILS__QTIME_
