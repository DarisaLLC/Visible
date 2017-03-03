
#ifndef __AVReader_Impl__
#define __AVReader_Impl__

#include <string>
#include <memory>
#include <functional>
#include <mutex>
#include <condition_variable>
#include "time_index.h"
#include "mediaInfo.h"
#include <cinder/cinder.h>
#include <cinder/Surface.h>
#include "shared_queue.hpp"
#include "cm_time.hpp"

using namespace ci;

namespace avcc
{
    struct avReader_impl;
    
    class avReader
    {
    public:
        
        // tbd: add state info and accessors
        typedef std::function<void(CVPixelBufferRef)> image_cb;
        typedef std::function<void(CMTime)> progress_cb;
        typedef std::function<void(void)> done_cb;
        
        typedef std::pair<cm_time,cm_time> pres_duration_t;
        
        
        typedef void (progress_callback) (float);

        avReader (const std::string&, bool andRun = true);
        avReader (const std::string&, void (*get_progress)(CMTime), void (*get_image)(CVPixelBufferRef));
        bool isValid () const;
        bool isEmpty () const;
        bool isDone () const;
        
        void run ();
        
        std::pair<size_t,size_t> size () const;
        void pop (cm_time& ts, Surface8uRef& frame) const;
        
        tiny_media_info info () const;

        void setImageCallBack (image_cb icb);
        void setProgressCallBack (progress_cb icb);
        void setDoneCallBack (done_cb dcb);

        static image_cb m_image_cb;
        static progress_cb m_progress_cb;
        static done_cb m_done_cb;
        
        static shared_queue<Surface8uRef> m_surfaces;
        static shared_queue<cm_time> m_timestamps;
        
        static std::mutex m_mu;
        static std::condition_variable m_cv;
        static int m_done;
        
    private:
        void internal_setup (const std::string&);
        std::shared_ptr<avReader_impl> m_impl;
        std::pair<size_t,size_t> m_sizes;
        void m_default_progress_cb (CMTime p);
        void m_default_image_cb (CVPixelBufferRef cvp);
        void m_default_done_cb ();
        


    };
    
    typedef std::shared_ptr<avReader> avReaderRef;
}

#endif


