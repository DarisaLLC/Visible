
#ifndef _SIMILARITY_PRODUCER_IMPL_H
#define _SIMILARITY_PRODUCER_IMPL_H




#include <iostream>
#include <algorithm>
#include <cctype>
#include <string>
#include "base_signaler.h"
#include "static.hpp"
#include <atomic>
#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>

#include "app_utils.hpp"
#include "roiWindow.h"
#include "qtime_frame_cache.hpp"
#include "avReader.hpp"

#include "sm_producer.h"


SINGLETON_FCN(gen_filename::random_name,get_name_generator);

class sm_producer::spImpl : public base_signaler
{
public:
    friend class sm_producer;
    
    typedef std::mutex mutex_t;
    
    spImpl () :  m_auto_run (false)
    {
        m_name = get_name_generator().get_anew();
        signal_content_loaded = createSignal<sm_producer::sig_cb_content_loaded>();
        signal_frame_loaded = createSignal<sm_producer::sig_cb_frame_loaded>();
        signal_sm1d_available = createSignal<sm_producer::sig_cb_sm1d_available> ();
        signal_sm2d_available = createSignal<sm_producer::sig_cb_sm2d_available> ();
        m_loaded_ref.resize(0);
    }
    
    bool load_content_file (const std::string& movie_fqfn);
    void loadImages ( const images_vector_t& );
    
    bool set_auto_run_on () { bool tmp = m_auto_run; m_auto_run = true; return tmp; }
    bool set_auto_run_off () { bool tmp = m_auto_run; m_auto_run = false; return tmp; }


    bool done_grabbing () const;
    bool generate_ssm (int start_frame, int frames);
    int64_t frame_count () { return _frameCount; }
    std::string  getName () const { return m_name; }
    
    
private:
    int32_t loadMovie( const std::string& movieFile );
    
protected:
    boost::signals2::signal<sm_producer::sig_cb_content_loaded>* signal_content_loaded;
    boost::signals2::signal<sm_producer::sig_cb_frame_loaded>* signal_frame_loaded;
    boost::signals2::signal<sm_producer::sig_cb_sm1d_available>* signal_sm1d_available;
    boost::signals2::signal<sm_producer::sig_cb_sm2d_available>* signal_sm2d_available;
    
private:
    mutable bool m_auto_run;
    void get_next_frame ();
    void start ();
    void loadder ();
    void pusher ();
    void stop ();

    std::chrono::milliseconds m_frame_time;

    mutable mutex_t   m_mutex;
    mutable std::condition_variable m_frame_ready;
    
    time_spec_t       _currentTime, _startTime;
    images_vector_t                 m_loaded_ref;
    int64_t                         _frameRate;
    int64_t                          _frameCount;
    int64_t                          _elapasedFrames;
    int64_t                          _fcm1;
    
    mutable long m_index, m_must_stop, m_frame;
    
    std::shared_ptr<avcc::avReader>    m_assetReader;
    std::shared_ptr<qTimeFrameCache>   m_qtime_cache_ref;

    std::queue<float> m_queue;
    
    deque<deque<double> >        m_SMatrix;   // Used in eExhaustive and
    deque<double>                m_entropies; // Final entropy signal
    deque<double>                m_means; // Final entropy signal
    int                   m_depth;
    std::string                  m_name;
    std::shared_ptr<std::thread> m_grabber_thread;
    std::shared_ptr<std::thread> m_pusher_thread;
    
};


#endif // __VF_SIMILARITY_PRODUCER_IMPL_H
