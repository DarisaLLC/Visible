
#ifndef _SIMILARITY_PRODUCER_H
#define _SIMILARITY_PRODUCER_H

#include <stdio.h>
#include <deque>
#include <memory>
#include "qtime_frame_cache.hpp"
#include <typeinfo>
#include <string>
#include "base_signaler.h"
#include "roiWindow.h"
#include <chrono>

using namespace std;
using namespace boost;
using namespace svl;


typedef std::shared_ptr<class sm_producer> smProducerRef;


class sm_producer
{
public:
    typedef std::vector<roiWindow<P8U> > images_vector_t;
    typedef std::deque<double> sMatrixProjection_t;
    typedef std::deque< std::deque<double> > sMatrix_t;
    typedef void (sig_cb_content_loaded) ();
    typedef void (sig_cb_frame_loaded) (int&, double&);
    typedef void (sig_cb_frames_cached) ();
    typedef void (sig_cb_sm1d_available) ();
    typedef void (sig_cb_sm2d_available) ();
    sm_producer (bool auto_on_off = false);
    
    bool load_content_file (const string& fq_path);
    bool load_image_directory (const string& fq_path);
    void load_images (const images_vector_t&);
    
    bool operator () (int start_frame = 0, int frames = 0) const;
    
    bool set_auto_run_on () const;
    bool set_auto_run_off () const;
    
    bool has_content () const;
    int bytes_per_pixel () const;
    int pixels_per_channel () const;
    int channels_per_plane () const;
    int planes_per_image () const;
    int frames_in_content () const;
    range_error last_error () const;
    
    
    const std::string& content_file () const;
    int process_start_frame () const;
    int process_last_frame () const;
    int process_count () const;
    
    const sMatrix_t& similarityMatrix () const;
    
    const sMatrixProjection_t& meanProjection () const;
    
    const sMatrixProjection_t& shannonProjection () const;
    
    /** \brief registers a callback function/method to a signal with the corresponding signature
     * \param[in] callback: the callback function/method
     * \return Connection object, that can be used to disconnect the callback method from the signal again.
     */
    template<typename T>
    boost::signals2::connection registerCallback (const std::function<T>& callback);
    
    /** \brief indicates whether a signal with given parameter-type exists or not
     * \return true if signal exists, false otherwise
     */
    template<typename T>
    bool providesCallback () const;
    
    images_vector_t& images () const;
    
private:
    class spImpl;
    std::shared_ptr<spImpl> _impl;
    
    
};





#endif /* __VF_SIMILARITY_H */





