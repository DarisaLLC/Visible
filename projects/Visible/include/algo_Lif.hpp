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
#include "core/signaler.h"
#include "vision/opencv_utils.hpp"
#include "getLuminanceAlgo.hpp"
#include "sm_producer.h"
#include "contraction.hpp"
#include "vision/labelBlob.hpp"


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
    typedef void (sig_cb_3dstats_available) ();
    typedef std::vector<roiWindow<P8U>> channel_images_t;
    typedef std::vector<channel_images_t> channel_vec_t;
    
    lif_processor ();
 
    const smProducerRef sm () const;
    const int64_t& frame_count () const;
    const int64_t channel_count () const;
    const svl::stats<int64_t> stats3D () const;
    const std::shared_ptr<vectorOfnamedTrackOfdouble_t> fluTracks () const;
    const std::shared_ptr<vectorOfnamedTrackOfdouble_t> PCITrack () const;
    
    // Check if the returned has expired
    std::weak_ptr<contraction_analyzer> contractionWeakRef ();
    
    // Load frames from cache
    int64_t load (const std::shared_ptr<qTimeFrameCache>& frames,const std::vector<std::string>& names);
    
    // Run Luminance info on a vector of channel indices
    async_vectorOfnamedTrackOfdouble_t get_luminance_info (const std::vector<int>& channels);
    std::shared_ptr<vectorOfnamedTrackOfdouble_t> run_flu_statistics (const std::vector<int>& channels);
    
    // Run accumulator of 3d stats on a channel at index
    svl::stats<int64_t> run_volume_sum_sumsq_count (const int channel_index);
    
    // Run to get Entropies and Median Level Set
    std::shared_ptr<vectorOfnamedTrackOfdouble_t> run_pci (const int channel_index);
    
    const std::vector<Rectf>& rois () const;
    
    const  std::deque<double>& medianSet () const;
    
    // Loads from all channels. -1 implies create a multichannel cv::Mat. 0,1,2 imply specific channel.
    // Returns false if channel(s) requested do not exist. Or in case of any error
    bool loadImagesToMats (const int channel_index);
    
    // Update. Called also when cutoff offset has changed
    void update ();
    
protected:
    boost::signals2::signal<lif_processor::sig_cb_content_loaded>* signal_content_loaded;
    boost::signals2::signal<lif_processor::sig_cb_flu_stats_available>* signal_flu_available;
    boost::signals2::signal<lif_processor::sig_cb_frame_loaded>* signal_frame_loaded;
    boost::signals2::signal<lif_processor::sig_cb_sm1d_available>* signal_sm1d_available;
    boost::signals2::signal<lif_processor::sig_cb_sm1dmed_available>* signal_sm1dmed_available;
    boost::signals2::signal<lif_processor::sig_cb_contraction_available>* signal_contraction_available;
    boost::signals2::signal<lif_processor::sig_cb_3dstats_available>* signal_3dstats_available;
    
private:
 
    void contraction_analyzed (contractionContainer_t&);
    void stats_3d_computed ();
    
    void pci_done ();
    void sm_content_loaded ();
 
    // Assumes LIF data -- use multiple window.
    void load_channels_from_images (const std::shared_ptr<qTimeFrameCache>& frames);
 
    
    // Note tracks contained timed data.
    void entropiesToTracks (namedTrackOfdouble_t& track);
    
    // @note Specific to ID Lab Lif Files
    void create_named_tracks (const std::vector<std::string>& names);

    labelBlob::ref get_blobCachedObject (const index_time_t& ti);
    
    mutable std::mutex m_mutex;
    uint32_t m_channel_count;

    // One similarity engine
    mutable smProducerRef m_sm;
    deque<double> m_entropies;
    deque<deque<double>> m_smat;

    // Results map by channel
    std::vector<cv::Mat> m_channel_mats;
    std::deque<double> m_medianLevel;
    
    channel_images_t m_images;
    channel_vec_t m_all_by_channel;

    int64_t m_frameCount;
    std::vector<Rectf> m_rois;
    Rectf m_all;

    // @note Specific to ID Lab Lif Files
    // 3 channels: 2 flu one visible
    // 1 channel: visible
    // Note create_named_tracks
    std::shared_ptr<vectorOfnamedTrackOfdouble_t>  m_tracksRef;
    std::shared_ptr<vectorOfnamedTrackOfdouble_t>  m_pci_tracksRef;
    std::shared_ptr<contraction_analyzer> m_caRef;

    mutable svl::stats<int64_t> m_3d_stats;
    std::atomic<bool> m_3d_stats_done;
    
    std::map<index_time_t, labelBlob::weak_ref> m_blob_cache;
};



#endif
