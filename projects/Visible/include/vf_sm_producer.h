
#ifndef __VF_SIMILARITY_PRODUCER_H
#define __VF_SIMILARITY_PRODUCER_H

#include <stdio.h>
#include <deque>
#include "vf_types.h"
#include <boost/shared_ptr.hpp>
#include <boost/signals2.hpp>
#include <boost/signals2/slot.hpp>
#include <typeinfo>
#include <string>
#include "signaler.h"
#include "grabber.h"
#include "roi_window.h"

using namespace std;
using namespace boost;


typedef boost::shared_ptr<class sm_producer> smProducerRef;


class sm_producer
{
public:
    typedef std::vector<roi_window> images_vector;
    typedef std::deque<double> sMatrixProjectionType;
    typedef std::deque< std::deque<double> > sMatrixType;
    typedef void (sig_cb_content_loaded) ();
    typedef void (sig_cb_frame_loaded) (int&, double&);
    typedef void (sig_cb_frames_cached) ();
    typedef void (sig_cb_sm1d_available) ();
    typedef void (sig_cb_sm2d_available) ();
    sm_producer ();
    
    bool load_content_file (const string& fq_path);
    void load_images (const images_vector&);
    
    bool operator () (int start_frame, int frames) const;
    
    bool set_auto_run_on () const;
    bool set_auto_run_off () const;
    
    bool has_content () const;
    int bytes_per_pixel () const;
    int pixels_per_channel () const;
    int channels_per_plane () const;
    int planes_per_image () const;
    int frames_in_content () const;
    grabber_error last_error () const;
    
    
    const std::string& content_file () const;
    int process_start_frame () const;
    int process_last_frame () const;
    int process_count () const;
    
    const sMatrixType& similarityMatrix () const;
    
    const sMatrixProjectionType& meanProjection () const;
    
    const sMatrixProjectionType& shannonProjection () const;
    
    /** \brief registers a callback function/method to a signal with the corresponding signature
     * \param[in] callback: the callback function/method
     * \return Connection object, that can be used to disconnect the callback method from the signal again.
     */
    template<typename T>
    boost::signals2::connection registerCallback (const boost::function<T>& callback);
    
    /** \brief indicates whether a signal with given parameter-type exists or not
     * \return true if signal exists, false otherwise
     */
    template<typename T>
    bool providesCallback () const;
    
    
private:
    class spImpl;
    boost::shared_ptr<spImpl> _impl;
    
    
};





#endif /* __VF_SIMILARITY_H */





