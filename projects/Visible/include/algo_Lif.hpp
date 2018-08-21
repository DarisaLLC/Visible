#ifndef __ALGO_LIF__
#define __ALGO_LIF__


#include <iostream>
#include <string>
#include <typeinfo>
#include <vector>
#include <deque>
#include <sstream>
#include <typeindex>
#include <map>
#include "async_producer.h"
#include "signaler.h"
#include "vision/opencv_utils.hpp"
#include "getLuminanceAlgo.hpp"
#include "sm_producer.h"
#include "contraction.hpp"


using namespace cv;
using namespace std;



// For base classing
class LifSignaler : public base_signaler
{
    virtual std::string getName () const { return "LifSignaler"; }
};


class lif_processor : public LifSignaler
{
public:
    using contractionContainer_t = contraction_analyzer::contractionContainer_t;
    
    
    typedef void (sig_cb_content_loaded) (int64_t&);
    typedef void (sig_cb_flu_stats_available) ();
    typedef void (sig_cb_contraction_available) (contractionContainer_t &);
    typedef void (sig_cb_frame_loaded) (int&, double&);
    typedef void (sig_cb_sm1d_available) (int&);
    typedef void (sig_cb_sm1dmed_available) (int&,int&);
    typedef std::vector<roiWindow<P8U>> channel_images_t;
    
    lif_processor ();
 
    const smProducerRef sm () const;
    
    // Check if the returned has expired
    std::weak_ptr<contraction_analyzer> contractionWeakRef ();
    
    int64_t load (const std::shared_ptr<qTimeFrameCache>& frames,const std::vector<std::string>& names);
    
    std::shared_ptr<vectorOfnamedTrackOfdouble_t> run_flu_statistics ();
    
    // Run to get Entropies and Median Level Set
    std::shared_ptr<vectorOfnamedTrackOfdouble_t>  run_pci ();
    
    const std::vector<Rectf>& rois () const;
    const namedTrackOfdouble_t& similarity_track () const;
    const std::shared_ptr<vectorOfnamedTrackOfdouble_t>& all_track () const;
    
    // Update. Called also when cutoff offset has changed
    void update ();
    
protected:
    boost::signals2::signal<lif_processor::sig_cb_content_loaded>* signal_content_loaded;
    boost::signals2::signal<lif_processor::sig_cb_flu_stats_available>* signal_flu_available;
    boost::signals2::signal<lif_processor::sig_cb_frame_loaded>* signal_frame_loaded;
    boost::signals2::signal<lif_processor::sig_cb_sm1d_available>* signal_sm1d_available;
    boost::signals2::signal<lif_processor::sig_cb_sm1dmed_available>* signal_sm1dmed_available;
    boost::signals2::signal<lif_processor::sig_cb_contraction_available>* signal_contraction_available;
    
private:
 
    void contraction_analyzed (contractionContainer_t&);
  
    void sm_content_loaded ();
 
    // Assumes LIF data -- use multiple window.
    void load_channels_from_images (const std::shared_ptr<qTimeFrameCache>& frames);
 
    
    // Note tracks contained timed data.
    void entropiesToTracks (namedTrackOfdouble_t& track);
    
    const int64_t frame_count () const;

    // @note Specific to ID Lab Lif Files
    void create_named_tracks (const std::vector<std::string>& names);
    
    std::shared_ptr<timed_mat_vec_t> compute_oflow_threaded ();
    
    void loadImagesToMats (const sm_producer::images_vector_t& images, std::vector<cv::Mat>& mts);

    
    
private:
    deque<double> m_entropies;
    deque<deque<double>> m_smat;
    smProducerRef m_sm;
    channel_images_t m_images;
    std::vector<channel_images_t> m_channel_images;
    std::vector<cv::Mat> m_channel2_mats;
    int64_t m_frameCount;
    std::vector<Rectf> m_rois;
    Rectf m_all;
    std::shared_ptr<contraction_analyzer> m_caRef;
    std::deque<double> m_medianLevel;
    std::shared_ptr<vectorOfnamedTrackOfdouble_t>  m_pci_tracksRef;
    std::shared_ptr<vectorOfnamedTrackOfdouble_t>  m_tracksRef;
};

#if 0
struct fbOpticalFlowRunner
{
    typedef std::vector<roiWindow<P8U>> channel_images_t;
    std::shared_ptr<timed_mat_vec_t> operator()(std::vector<cv::Mat>& frames);
    static void drawOptFlowMap(const cv::Mat& flow, cv::Mat& cflowmap, int step,
                               double, const Scalar& color);
};

#endif


#endif
