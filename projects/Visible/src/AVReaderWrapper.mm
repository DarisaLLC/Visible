
#import "AVReader.h"
#import "AVReader.hpp"

#import "cinder/cocoa/CinderCocoa.h"
#include <objc/runtime.h>
#include "core/cm_time.hpp"


using namespace std;
using namespace cinder;
using namespace cinder::cocoa;

using namespace avcc;


namespace anonymous
{
    
    Surface8uRef convertCVPixelBufferToSurface( CVPixelBufferRef pixelBufferRef )
    {
        ::CVPixelBufferLockBaseAddress( pixelBufferRef, 0 );
        uint8_t *ptr = reinterpret_cast<uint8_t*>( CVPixelBufferGetBaseAddress( pixelBufferRef ) );
        ptrdiff_t rowBytes = (int32_t)::CVPixelBufferGetBytesPerRow( pixelBufferRef );
        OSType type = CVPixelBufferGetPixelFormatType( pixelBufferRef );
        size_t width = CVPixelBufferGetWidth( pixelBufferRef );
        size_t height = CVPixelBufferGetHeight( pixelBufferRef );
        SurfaceChannelOrder sco;
        if( type == kCVPixelFormatType_24RGB )
            sco = SurfaceChannelOrder::RGB;
        else if( type == kCVPixelFormatType_32ARGB )
            sco = SurfaceChannelOrder::ARGB;
        else if( type == kCVPixelFormatType_24BGR )
            sco = SurfaceChannelOrder::BGR;
        else if( type == kCVPixelFormatType_32BGRA )
            sco = SurfaceChannelOrder::BGRA;
        Surface8u *newSurface = new Surface8u( ptr, (int32_t)width, (int32_t)height, rowBytes, sco );
        return Surface8uRef( newSurface, [=] ( Surface8u *s )
                            {	::CVPixelBufferUnlockBaseAddress( pixelBufferRef, 0 );
                                ::CVBufferRelease( pixelBufferRef );
                                delete s;
                            }
                            );
    }
    
    void default_done_cb ()
    {
        avReader::m_done = 1;
        avReader::m_cv.notify_all();
        std::cout << "Done"  << std::endl;
    }
    
    
    void default_progress_cb (CMTime progress)
    {
        avReader::m_timestamps.push(progress);
//        std::cout << cm_time(progress)  << std::endl;
    }
    
    void default_image_cb (CVPixelBufferRef cvp)
    {
        avReader::m_surfaces.push(anonymous::convertCVPixelBufferToSurface (cvp));
//        std::cout << avReader::m_surfaces.size() << std::endl;
    }
}

namespace avcc
{
    
    avReader::progress_cb avReader::m_progress_cb;// = std::bind(anonymous::default_progress_cb, std::placeholders::_1);
    avReader::image_cb avReader::m_image_cb;
    avReader::done_cb avReader::m_done_cb;
    std::mutex avReader::m_mu;
    std::condition_variable avReader::m_cv;
    int avReader::m_done;

    shared_queue<Surface8uRef> avReader::m_surfaces;
    shared_queue<cm_time> avReader::m_timestamps;
    
    struct avReader_impl
    {
        AVReader* wrapped;
        image_callback icb;
        progress_callback pcb;
        bool m_valid;
    };
    
    
    avReader::avReader(const std::string& file_path, bool andRun) : m_impl(new avReader_impl())
    {
        setProgressCallBack(anonymous::default_progress_cb);
        setImageCallBack(anonymous::default_image_cb);
        setDoneCallBack(anonymous::default_done_cb);
        
        internal_setup(file_path);
        if (andRun)
            run ();
    }

    
    avReader::avReader (const std::string& file_path, void (*get_progress)(CMTime), void (*get_image)(CVPixelBufferRef))
    : m_impl(new avReader_impl())
    {
        setProgressCallBack(get_progress);
        setImageCallBack(get_image);
        internal_setup(file_path);
    }
    
    tiny_media_info avReader::info () const
    {
        return [m_impl->wrapped movieInfo];
    }
    
    std::pair<size_t,size_t> avReader::size () const
    {
        return std::make_pair (avReader::m_timestamps.size(), avReader::m_surfaces.size());
    }
    
    bool avReader::isEmpty() const
    {
        return avReader::m_timestamps.empty() || avReader::m_surfaces.empty();
    }
    
    
    
    void avReader::pop (cm_time& ts, Surface8uRef& frame) const
    {
        avReader::m_timestamps.wait_and_pop(ts);
        avReader::m_surfaces.wait_and_pop(frame);
    }
    
    void avReader::internal_setup(const std::string& file_path)
    {
        m_impl->wrapped = [[AVReader alloc] init];
        m_impl->m_valid = false;
        std::string fileurl ("file://");
        fileurl = fileurl + file_path;
        SafeNsString* safestr = new SafeNsString(fileurl);
        m_impl->m_valid = [m_impl->wrapped setup: *safestr ];
    }
    
    void avReader::run ()
    {
        if (m_impl->m_valid)
        {
            std::unique_lock<std::mutex> lk (avReader::m_mu);
            avReader::m_done = 0;
            //void (^block) (float) = ^ (float pct){ NSLog(@"%d", (int)(pct * 100)); };
            void (^block) (CMTime) = ^ (CMTime stmp){ avReader::m_progress_cb(stmp); };
            void (^iblock) (CVPixelBufferRef) = ^ (CVPixelBufferRef stmp){ avReader::m_image_cb(stmp); };
            void (^dblock) () = ^ (){ avReader::m_done_cb(); };
            
            [m_impl->wrapped setGetProgress: [block copy]];
            [m_impl->wrapped setGetNewFrame: [iblock copy]];
            [m_impl->wrapped setGetDone: [dblock copy]];
            
            [m_impl->wrapped run];
            avReader::m_cv.wait (lk, [] { return avReader::m_done > 0; });
            
        }
    }
    
    bool avReader::isValid() const
    {
        return (! m_impl || ! m_impl->wrapped ) ? false : m_impl->m_valid;
    }
    
    void avReader::setProgressCallBack(avReader::progress_cb pcb)
    {
        avReader::m_progress_cb = std::bind(pcb, std::placeholders::_1);
    }
   
    void avReader::setImageCallBack(avReader::image_cb icb)
    {
        avReader::m_image_cb = std::bind(icb, std::placeholders::_1);
    }
    
    void avReader::setDoneCallBack(avReader::done_cb dcb)
    {
        avReader::m_done_cb = std::bind(dcb);
    }
    
 
}


