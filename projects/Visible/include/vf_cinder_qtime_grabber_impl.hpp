#ifndef __VF_UTILS__QTIME_IMPL__
#define __VF_UTILS__QTIME_IMPL__

#include <cinder/Channel.h>
#include <cinder/Area.h>
#include <limits>
#include "cinder/ImageIo.h"
#include "cinder/Utilities.h"
#include "cinder/Surface.h"
#include "cinder/qtime/QuickTime.h"
#include "cinder/Thread.h"
//#include "rc_window.h"
#include <boost/algorithm/string.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/scoped_thread.hpp>
#include <boost/math/special_functions.hpp>
//#include "rc_fileutils.h"
//#include <stlplus_lite.hpp>
//#include "rc_filegrabber.h"
//#include "vf_image_conversions.hpp"
#include "vf_cinder_qtime_grabber.h"

#include "sshist.hpp"

#include <fstream>

using namespace ci;
using namespace std;



namespace vf_utils
{
    
    namespace qtime_support
    {
        
        
        class CinderQtimeGrabber::qtfgImpl //: public rcFrameGrabber
        {
        public:
            friend class vf_cinder_qtime_grabber;
            
            // ctor
            qtfgImpl( const std::string fileName,   // Input file
                     double frameInterval = -1.0, // Forced frame interval
                     int32  startAfterFrame = -1,
                     int32  frames = -1 ) :
            mFileName( fileName ), mFrameInterval( frameInterval ),mFrameCount(-1),
            mCurrentTimeStamp( rcTimestamp::from_seconds(frameInterval) ), mCurrentIndex( 0  )
            {
                
                
                boost::lock_guard<boost::mutex> (this->mMuLock);
                mValid = file_exists ( fileName ) && file_readable ( fileName );
                if ( mValid )
                {
                    mMovie = ci::qtime::MovieSurface( fileName );
                    m_width = mMovie.getWidth ();
                    m_height = mMovie.getHeight ();
                    mFrameCount = mMovie.getNumFrames();
                    mMovieFrameInterval = 1.0 / (mMovie.getFramerate() + std::numeric_limits<double>::epsilon() );
                    mFrameInterval = boost::math::signbit (frameInterval) == 1 ? mMovieFrameInterval : mFrameInterval;
                    mMovie.setLoop( true, false);
                    setLastError( rcFrameGrabberError::OK );
                }
                else
                {
                    setLastError( rcFrameGrabberError::FileInit );
                }
                
            }
            
            // virtual dtor
            virtual ~qtfgImpl() {}
            
            virtual bool isValid () const
            {
                return mValid && ( getLastError() == rcFrameGrabberError::OK );
            }
            
            bool contentValid () const
            {
                return mValid;
            }
            //
            // rcFrameGrabber API
            //
            
            // Start grabbing
            virtual bool start()
            {
                boost::lock_guard<boost::mutex> (this->mMuLock);
                
                mCurrentIndex = 0;
                mMovie.seekToStart ();
                mMovie.play ();
                if (mMovie.isPlaying () )
                    return true;
                else
                    setLastError( rcFrameGrabberError::UnsupportedFormat );
                return false;
            }
            
            
            // Stop grabbing
            virtual bool stop()
            {
                boost::lock_guard<boost::mutex> (this->mMuLock);
                
                bool what = mMovie.isDone ();
                if (what) return what;
                what = mMovie.isPlaying ();
                if (! what ) return true;
                // It is not done and is playing
                mMovie.stop ();
                return ! mMovie.isPlaying ();
                
            }
            
            // Returns the number of frames available
            virtual int32 frameCount() { return mFrameCount; }
            
            // Movie grabbers don't have a cache.
            virtual int32 cacheSize() { return 0; }
            
            // Get next frame, assign the frame to ptr
            virtual rcFrameGrabberStatus getNextFrame( rcFrameRef& ptr, bool isBlocking )
            {
                ci::ThreadSetup threadSetup;
                
                boost::lock_guard<boost::mutex> (this->mMuLock);
                rcFrameGrabberStatus ret =  eFrameStatusOK;
                setLastError( rcFrameGrabberError::Unknown );
                
                if (mCurrentIndex >= 0 && mCurrentIndex < mFrameCount)
                {
                    if ( mMovie.checkNewFrame () )
                    {
                        double tp = mMovie.getCurrentTime ();
                        mSurface = mMovie.getSurface ();
                        if (mSurface )
                        {
                            ptr = new rcFrame (m_width, m_height, rcPixel8);
                            ptr->setIsGray (true);
                            Surface::Iter iter = mSurface.getIter ( mSurface.getBounds() );
                            int rows = 0;
                            while (iter.line () )
                            {
                                uint8_t* pels = ptr->rowPointer (rows++);
                                while ( iter.pixel () ) *pels++ = iter.g ();
                            }
                            
                            ptr->setTimestamp (rcTimestamp::from_seconds (tp ) );
                            ret = eFrameStatusOK;
                            setLastError( rcFrameGrabberError::OK );
                            mMovie.stepForward ();
                            mCurrentIndex++;
                        }
                    }
                    else
                    {
                        setLastError( rcFrameGrabberError::FileRead );
                        ret = eFrameStatusError;
                    }
                    
                }
                else
                {
                    ret = eFrameStatusEOF;
                }
                
                return ret;
            }
            
            // Get name of input source, ie. file name, camera name etc.
            virtual const std::string getInputSourceName() {  return mFileName; }
            
            
            // Get last error value.
            virtual rcFrameGrabberError getLastError() const
            {
                return mLastError;
            }
            
            
            // Set last error value
            void setLastError( rcFrameGrabberError error )
            {
                mLastError = error;
            }
            
            double frame_duration  () const { return mFrameInterval; }
            
            const std::ostream& print_to_ (std::ostream& std_stream)
            {
                std_stream << " -- Extracted Info -- " << std::endl;
                std_stream << "Dimensions:" << m_width << " x " << m_height << std::endl;
                std_stream << "Frames:    " << mFrameCount << std::endl;
                
                std_stream << " -- Embedded Movie Info -- " << std::endl;
                std_stream << "Dimensions:" << mMovie.getWidth() << " x " << mMovie.getHeight() << std::endl;
                std_stream << "Duration:  " << mMovie.getDuration() << " seconds" << std::endl;
                std_stream << "Frames:    " << mMovie.getNumFrames() << std::endl;
                std_stream << "Framerate: " << mMovie.getFramerate() << std::endl;
                std_stream << "Alpha channel: " << mMovie.hasAlpha() << std::endl;
                std_stream << "Has audio: " << mMovie.hasAudio() << " Has visuals: " << mMovie.hasVisuals() << std::endl;
                return std_stream;
            }
            
            
            rcFrameGrabberStatus  getTOC (std::vector<rcTimestamp>& tocItoT, std::map<rcTimestamp,uint32>& tocTtoI)
            {
                if (! isValid () ) return eFrameStatusError;
                if ( ! stop () ) return eFrameStatusError;
                setLastError( rcFrameGrabberError::Unknown );
                boost::lock_guard<boost::mutex> (this->mMuLock);
                rcFrameGrabberStatus ret =  eFrameStatusOK;
                mMovie.seekToStart ();

                tocItoT.resize (0);
                tocTtoI.clear ();
                long        frameCount = 0;
                TimeValue   curMovieTime = 0;
                auto movObj = mMovie.getMovieHandle();
                
                // MediaSampleFlags is defined in ImageCompression.h:
                OSType types[] = { VisualMediaCharacteristic };
                    
                    while( curMovieTime >= 0 )
                    {
                        ::GetMovieNextInterestingTime( movObj, nextTimeStep, 1, types, curMovieTime, fixed1, &curMovieTime, NULL );
                        double timeSecs (curMovieTime / ::GetMovieTimeScale( movObj ) );
                        rcTimestamp timestamp = rcTimestamp::from_seconds (timeSecs);
                        
                        tocItoT[frameCount] = timestamp;
                        tocTtoI[timestamp] = frameCount;

                        
                        frameCount++;
                    }
                     setLastError( rcFrameGrabberError::OK );
                  return eFrameStatusEOF;
            }
            
            
        private:
            
            ci::qtime::MovieSurface	    mMovie;
            ci::Surface				mSurface;
            bool                mValid;
            const std::string          mFileName;
            double mFrameInterval;
            double mMovieFrameInterval;
            int32                 mFrameCount;       // Number of frames in a movie
            rcTimestamp             mCurrentTimeStamp; // Current frame timestamp
            int32                 mCurrentIndex;     // Current index within movie
            
            
            int32 m_width, m_height;
            rcFrameGrabberError  mLastError;
            
            void lock()  { this->mMuLock.lock (); }
            void unlock()  { this->mMuLock.unlock (); }
            boost::mutex    mMuLock;    // explicit mutex for locking QuickTime
            
        };
        
    }
}

#endif // __VF_UTILS__QTIME__IMPL__
