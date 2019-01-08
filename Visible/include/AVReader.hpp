
#ifndef __AVReader_Impl__
#define __AVReader_Impl__

#include <string>
#include <memory>
#include <functional>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include "time_index.h"
#include "mediaInfo.h"
#include <cinder/Cinder.h>
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

        avReader (const std::string&, bool andRun = true);
        avReader (const std::string&, void (*user_done_cb)(void) = nullptr, void (*get_progress)(CMTime) = nullptr,
                                      void (*get_image)(CVPixelBufferRef) = nullptr);
        bool isValid () const;
        bool isEmpty () const;
        bool isDone () const;
        
        void run ();
        void pop (cm_time& ts, Surface8uRef& frame) const;
        std::pair<size_t,size_t> size () const;
        tiny_media_info info () const;
        void setUserImageCallBack (image_cb icb);
        void setUserProgressCallBack (progress_cb icb);
        void setUserDoneCallBack (done_cb dcb);

   
        
    private:
        void default_done_cb ();
        void default_progress_cb (CMTime progress);
        void default_image_cb (CVPixelBufferRef cvp);
        void internal_setup (const std::string&);
        std::shared_ptr<avReader_impl> m_impl;
        std::pair<size_t,size_t> m_sizes;
        void m_default_progress_cb (CMTime p);
        void m_default_image_cb (CVPixelBufferRef cvp);

        image_cb m_image_cb;
        image_cb m_user_image_cb;
        progress_cb m_progress_cb;
        progress_cb m_user_progress_cb;
        done_cb m_user_done_cb;
        done_cb m_done_cb;
        
        mutable shared_queue<Surface8uRef> m_surfaces;
        mutable shared_queue<cm_time> m_timestamps;
        
        mutable std::mutex m_mu;
        mutable std::condition_variable m_cv;
        std::atomic<int> m_done;
        


    };
    
    typedef std::shared_ptr<avReader> avReaderRef;
}

#endif


