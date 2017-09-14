
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
#include <tuple>
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

    enum source_type : int { movie = 0, imageFileDirectory = 1, imageInMemory = 2, Unknown = -1 };
    
    spImpl () :  m_auto_run (false)
    {
        m_name = get_name_generator().get_anew();
        signal_content_loaded = createSignal<sm_producer::sig_cb_content_loaded>();
        signal_frame_loaded = createSignal<sm_producer::sig_cb_frame_loaded>();
        signal_sm1d_available = createSignal<sm_producer::sig_cb_sm1d_available> ();
        signal_sm2d_available = createSignal<sm_producer::sig_cb_sm2d_available> ();
        m_loaded_ref.resize(0);
        m_source_type = Unknown;
    }
    
    bool load_content_file (const std::string& movie_fqfn);
    int loadImageDirectory (const std::string& image_dir, sm_producer::sizeMappingOption szmap,
                            const std::vector<std::string>& supported_extensions = { ".jpg", ".png", ".JPG", ".jpeg"});
    
    void loadImages ( const images_vector_t& );
    
    bool set_auto_run_on () { bool tmp = m_auto_run; m_auto_run = true; return tmp; }
    bool set_auto_run_off () { bool tmp = m_auto_run; m_auto_run = false; return tmp; }

    
    const source_type type () const { return m_source_type; }
    
    bool done_grabbing () const;
    bool generate_ssm (int start_frame, int frames);
    int64_t frame_count () { return _frameCount; }
    std::string  getName () const { return m_name; }
    bool image_file_entropy_result_ok () const;
    
    void asset_reader_done_cb ();
    
    // They always return the input order. To get other orders use the Tuple Interface
    images_vector_t& images () const { return m_loaded_ref; }
    const std::vector<fs::path >& frame_paths () const { return m_framePaths; }
    const ordered_outuple_t& ordered_input_output (const outputOrderOption) const;
    
  
private:
    int32_t loadMovie( const std::string& movieFile );
    
protected:
    boost::signals2::signal<sm_producer::sig_cb_content_loaded>* signal_content_loaded;
    boost::signals2::signal<sm_producer::sig_cb_frame_loaded>* signal_frame_loaded;
    boost::signals2::signal<sm_producer::sig_cb_sm1d_available>* signal_sm1d_available;
    boost::signals2::signal<sm_producer::sig_cb_sm2d_available>* signal_sm2d_available;
    
private:
    
    bool setup_image_directory_result_repo () const;
    mutable bool m_auto_run;
    mutable source_type m_source_type;
    
    void get_next_frame ();

    typedef std::vector<fs::path > paths_vector_t;
  
    std::chrono::milliseconds m_frame_time;

    mutable mutex_t   m_mutex;
    mutable std::condition_variable m_frame_ready;
    
    time_spec_t       _currentTime, _startTime;
    mutable images_vector_t                 m_loaded_ref;
    int64_t                         _frameRate;
    int64_t                          _frameCount;
    int64_t                          _elapasedFrames;
    int64_t                          _fcm1;
    
    mutable long m_index, m_must_stop, m_frame;
    
    std::shared_ptr<avcc::avReader>    m_assetReader;
    std::shared_ptr<qTimeFrameCache>   m_qtime_cache_ref;

    std::queue<float> m_queue;
    paths_vector_t m_framePaths;
    mutable std::map<outputOrderOption, ordered_outuple_t> m_output_repo;
    
    deque<deque<double> >        m_SMatrix;   // Used in eExhaustive and
    deque<double>                m_entropies; // Final entropy signal
    deque<double>                m_means; // Final entropy signal
    deque<double>                m_median_leveled; // Final entropy signal
    int                          m_depth;
    std::string                  m_name;
    
};


#endif // __VF_SIMILARITY_PRODUCER_IMPL_H
