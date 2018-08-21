
#ifndef _SIMILARITY_PRODUCER_H
#define _SIMILARITY_PRODUCER_H

#include <stdio.h>
#include <algorithm>
#include <deque>
#include <vector>
#include <memory>
#include "qtime_frame_cache.hpp"
#include <typeinfo>
#include <string>
#include <tuple>
#include "signaler.h"
#include "roiWindow.h"
#include <chrono>
#include "core/singleton.hpp"
#include "core/stats.hpp"
#include "core/stl_utils.hpp"


using namespace std;
using namespace boost;
using namespace svl;


class sm_producer
{
public:
    
    enum sizeMappingOption : int { dontCare = 0,  mostCommon = 1, reportFail = 2 };
    enum outputOrderOption : int { input = 0,  sorted = 1, binned = 2 };
    
    typedef roiWindow<P8U> image_t;
    typedef std::vector<image_t> images_vector_t;
    typedef std::deque<double> sMatrixProjection_t;
    typedef std::deque< std::deque<double> > sMatrix_t;
    typedef std::tuple<size_t, double, fs::path, image_t> outuple_t;
    typedef std::vector<outuple_t> ordered_outuple_t;
    
    typedef void (sig_cb_content_loaded) ();
    typedef void (sig_cb_frame_loaded) (int, double);
    typedef void (sig_cb_frames_cached) ();
    typedef void (sig_cb_sm1d_available) ();
    typedef void (sig_cb_sm2d_available) ();
    sm_producer (bool auto_on_off = false);
    
    bool load_content_file (const string& fq_path);
    bool load_image_directory (const string& fq_path, sizeMappingOption szmap = dontCare);
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
    
    /**
     *  Results Output & Options
     */
    const sMatrix_t& similarityMatrix () const;
    
    const sMatrixProjection_t& meanProjection (outputOrderOption ooo = input) const;
    
    const sMatrixProjection_t& shannonProjection (outputOrderOption ooo = input) const;
    
    const sMatrixProjection_t& medianLeveledProjection () const;
    
    /**
     *  Image Directory Output & Options
     *  The first two functions return the natural order ( i.e. input order )
     *
     */
    images_vector_t& images () const;
    const std::vector<fs::path >& paths () const;
    const ordered_outuple_t& ordered_input_output (const outputOrderOption) const;
    
    /**
     *  Callback Registration
     */
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
    
    
private:
    class spImpl;
    std::shared_ptr<spImpl> _impl;
    
};

typedef std::shared_ptr<sm_producer> smProducerRef;



#endif 





