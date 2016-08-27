
#ifndef __VF_SIMILARITY_PRODUCER_IMPL_H
#define __VF_SIMILARITY_PRODUCER_IMPL_H




//#include "rc_fileutils.h"
#include <iostream>
#include <algorithm>
#include <cctype>
#include <string>
#include "signaler.h"
#include <static.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/lock_guard.hpp>
#include <boost/ref.hpp>
#include "vf_utils.hpp"
#include "rpixel.hpp"
#include "qtime_cache.h"
#include "roi_window.h"
#include "cache_grabber.h"
#include "vf_sm_producer.h"
using namespace boost;




SINGLETON_FCN(gen_filename::random_name,get_name_generator);

class sm_producer::spImpl : public base_signaler
{
public:
    friend class sm_producer;
    
    typedef boost::mutex mutex_t;
    
    spImpl () : _has_content(-1), m_auto_run (false)
    {
        m_name = get_name_generator().get_anew();
        signal_content_loaded = createSignal<sm_producer::sig_cb_content_loaded>();
        signal_frame_loaded = createSignal<sm_producer::sig_cb_frame_loaded>();
        signal_sm1d_available = createSignal<sm_producer::sig_cb_sm1d_available> ();
        signal_sm2d_available = createSignal<sm_producer::sig_cb_sm2d_available> ();
        m_loaded_ref =  boost::shared_ptr<images_vector> ( new images_vector );
    }
    
    bool load_content_file (const std::string& movie_fqfn)
    {
        _movieFile = movie_fqfn;
        grabber_error fr;
        _frameCount = loadMovie (movie_fqfn, fr);
        _has_content.store (fr == grabber_error::OK);
        if (m_auto_run) generate_ssm (0,0);
        return has_content ();
    }
    
    
    bool set_auto_run_on () { bool tmp = m_auto_run; m_auto_run = true; return tmp; }
    bool set_auto_run_off () { bool tmp = m_auto_run; m_auto_run = false; return tmp; }

    int32 loadFrames( );
    void loadImages ( const images_vector& );

    bool has_content () const { return _has_content > 0 ; }
    bool generate_ssm (int start_frame, int frames);
    int first_process_index () { return _analysisFirstFrame; }
    int last_process_index () { return _analysisLastFrame; }
    int frame_count () { return _frameCount; }
    std::string  getName () const { return m_name; }
    grabber_error last_error () const { return m_last_error; }
    
    
private:
    int loadMovie( const std::string& movieFile, grabber_error& error );
    
    int saveFrames(std::string imageExportDir) const;
    
    void ipp (const roi_window& tmp, roi_window& image, time_spec_t& current);
    
protected:
    boost::signals2::signal<sm_producer::sig_cb_content_loaded>* signal_content_loaded;
    boost::signals2::signal<sm_producer::sig_cb_frame_loaded>* signal_frame_loaded;
    boost::signals2::signal<sm_producer::sig_cb_sm1d_available>* signal_sm1d_available;
    boost::signals2::signal<sm_producer::sig_cb_sm2d_available>* signal_sm2d_available;
    
private:
    boost::atomic<int> _has_content;
    mutable bool m_auto_run;
    
    mutable mutex_t   m_mutex;
    double            _physcial_memory_hint;
    int32             _analysisFirstFrame;
    int32             _analysisLastFrame;
    time_spec_t       _currentTime, _startTime;
    boost::shared_ptr<images_vector> m_loaded_ref;
    std::string            _imageFileNames;  // image file names separated
    mutable std::string            _movieFile;  //
    int                          _frameRate;
    int                          _frameCount;
    boost::shared_ptr<cache_grabber>            m_grabber_ref;
    deque<deque<double> >        m_SMatrix;   // Used in eExhaustive and
    deque<double>                m_entropies; // Final entropy signal
    deque<double>                m_means; // Final entropy signal
    pixel_ipl_t                    m_depth;
    std::string                  m_name;
    grabber_error          m_last_error;
    SharedQtimeCache             m_qtime_cache_ref;
};


#endif // __VF_SIMILARITY_PRODUCER_IMPL_H
