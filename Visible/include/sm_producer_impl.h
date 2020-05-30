
#ifndef _SIMILARITY_PRODUCER_IMPL_H
#define _SIMILARITY_PRODUCER_IMPL_H




#include <iostream>
#include <algorithm>
#include <cctype>
#include <string>
#include "core/signaler.h"
#include "static.hpp"
#include <atomic>
#include <thread>
#include <mutex>
#include <queue>
#include <tuple>
#include <condition_variable>

#include "roiWindow.h"
#include "seq_frame_container.hpp"
#include "AVReader.hpp"

#include "sm_producer.h"

class sm_signaler : public base_signaler
{
    virtual std::string
    getName () const { return "smSignaler"; }
};




class sm_producer::spImpl : public sm_signaler
{
public:
    friend class sm_producer;
    
    typedef std::mutex mutex_t;

    enum source_type : int { movie = 0, imageFileDirectory = 1, imageInMemory = 2, Unknown = -1 };

 
    
    spImpl ()
    {
        m_name = get_random_string();
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
    const source_type type () const { return m_source_type; }
    
    bool done_grabbing () const;
    bool generate_ssm (int frames, const sm_producer::progress_fn_t& reporter = nullptr);
    int64_t frame_count () { return m_frameCount; }
    bool image_file_entropy_result_ok () const;
    
    void asset_reader_done_cb ();
    
    // They always return the input order. To get other orders use the Tuple Interface
    images_vector_t& images () const { return m_loaded_ref; }
    const std::vector<bfs::path >& frame_paths () const { return m_framePaths; }
    const ordered_outuple_t& ordered_input_output (const outputOrderOption) const;
    
    const sm_producer::sMatrixProjection_t& shortterm (const uint32_t tWinSz) const;
    
  
private:
    int32_t loadMovie( const std::string& movieFile );
    
protected:
    boost::signals2::signal<sm_producer::sig_cb_content_loaded>* signal_content_loaded;
    boost::signals2::signal<sm_producer::sig_cb_frame_loaded>* signal_frame_loaded;
    boost::signals2::signal<sm_producer::sig_cb_sm1d_available>* signal_sm1d_available;
    boost::signals2::signal<sm_producer::sig_cb_sm2d_available>* signal_sm2d_available;
    
private:
    
    bool setup_image_directory_result_repo () const;
    mutable source_type m_source_type;
    
    void get_next_frame ();

    typedef std::vector<bfs::path > paths_vector_t;
  
    std::chrono::milliseconds m_frame_time;

    mutable mutex_t   m_mutex;
    
    time_spec_t       m_currentTime, m_startTime;
    mutable images_vector_t                 m_loaded_ref;
    int64_t                         m_frameRate;
    int64_t                          m_frameCount;
    int64_t                          m_elapasedFrames;
    int64_t                          m_fcm1;
    
    mutable long m_index, m_must_stop, m_frame;
    
    std::shared_ptr<avcc::avReader>    m_assetReader;
    std::shared_ptr<seqFrameContainer>   m_qtime_cache_ref;

    std::queue<float> m_queue;
    paths_vector_t m_framePaths;
    mutable std::map<outputOrderOption, ordered_outuple_t> m_output_repo;
    
    sMatrix_t                                       m_SMatrix;   // Used in eExhaustive and
    sm_producer::sMatrixProjection_t               m_entropies; // Final entropy signal
    sm_producer::sMatrixProjection_t               m_means; // Final entropy signal
    int                                      m_depth;
    std::string                               m_name;
    
};


#endif // __VF_SIMILARITY_PRODUCER_IMPL_H
