#ifndef __ALGO_MOV__
#define __ALGO_MOV__


#include <map>
#include <iostream>
#include <string>
#include <typeinfo>
#include <vector>
#include <sstream>
#include <typeindex>
#include <map>
#include <future>
//#include "core/singleton.hpp"
#include "async_tracks.h"
#include "core/signaler.h"
#include "sm_producer.h"
#include "cinder_xchg.hpp"
#include "vision/histo.h"
#include "vision/labelBlob.hpp"
#include "vision/opencv_utils.hpp"
#include "opencv2/video/tracking.hpp"
#include "algo_cardiac.hpp"
using namespace cv;

// For base classing
class movSignaler : public base_signaler
{
    virtual std::string
    getName () const { return "movSignaler"; }
};


class sequence_processor : public movSignaler
{
public:

    typedef void (sig_cb_content_loaded) (int64_t&);
    typedef void (sig_cb_frame_loaded) (int&, double&);
    typedef void (sig_cb_sm1d_available) (int&);
    typedef void (sig_cb_sm1dmed_available) (int&,int&);
    typedef void (sig_cb_3dstats_available) ();
    typedef void (sig_cb_geometry_available) ();
    typedef void (sig_cb_ss_image_available) (cv::Mat &);
    typedef std::vector<roiWindow<P8U>> channel_images_t;
    typedef std::vector<channel_images_t> channel_vec_t;
    
    sequence_processor (const fs::path& = fs::path());
    
    const smProducerRef similarity_producer () const;
    const int64_t& frame_count () const;
    const int64_t channel_count () const;
    const svl::stats<int64_t> stats3D () const;
    cv::Mat & segmented () const { return m_temporal_ss;  }
    
    // Load frames from cache
    int64_t load (const std::shared_ptr<seqFrameContainer>& frames,const std::vector<std::string>& names, const std::vector<std::string>& plot_names);
    

    // Channel Index API for IDLab custom organization
    // Run accumulator of 3d stats on a channel at index
    svl::stats<int64_t> run_volume_sum_sumsq_count (const int channel_index);
    // Run per pixel stdev accumulator a channel at index
    void run_detect_geometry (const int channel_index);
    
    
    // Vector of 8bit roiWindows API for IDLab custom organization
    svl::stats<int64_t> run_volume_sum_sumsq_count (std::vector<roiWindow<P8U>>&);
    void run_detect_geometry (std::vector<roiWindow<P8U>>& );
    
    // Run to get Entropies and Median Level Set. Both short term and long term pic
    // @todo event though index is an argument, only room for one channel is kept.
    // Short term is specifically run on the channel contraction pci was run on
    // duplictes will be removed from indices for short term pci.
    std::shared_ptr<vecOfNamedTrack_t> run_longterm_pci_on_channel (const int channel_index);
    std::shared_ptr<vecOfNamedTrack_t> run_longterm_pci (const std::vector<roiWindow<P8U>>&);
    std::vector<float> shortterm_pci (const std::vector<uint32_t>& indices);
    void shortterm_pci (const uint32_t& temporal_window);
    const vecOfNamedTrack_t& shortterm_pci () const { return m_shortterm_pci_tracks; }
    
    // Return 2D latice of pixels over time
    void generateVoxels_on_channel (const int channel_index, std::vector<std::vector<roiWindow<P8U>>>&,
                                    uint32_t sample_x = 1, uint32_t sample_y = 1);
    void generateVoxels (const std::vector<roiWindow<P8U>>&, std::vector<std::vector<roiWindow<P8U>>>&,
                         uint32_t sample_x = 1, uint32_t sample_y = 1);
    
    // Return 2D latice of voxel self-similarity
    void generateVoxelSelfSimilarities (std::vector<std::vector<roiWindow<P8U>>>&);
    void generateVoxelSelfSimilarities_on_channel (const int channel_index, uint32_t sample_x = 1, uint32_t sample_y = 1);
    
    void finalize_segmentation (cv::Mat&);
    const std::vector<Rectf>& channel_rois () const;
    const cv::RotatedRect& motion_surface () const;
    const cv::RotatedRect& motion_surface_bottom () const;
    const  std::deque<double>& medianSet () const;
    const fPair& ellipse_ab () const;
    
    
    // Update. Called also when cutoff offset has changed
    void update ();
    
    std::shared_ptr<OCVImageWriter>& get_image_writer ();
    void save_channel_images (int channel_index, std::string& dir_fqfn);
    
    const vector<vector<double>>& ssMatrix () const { return m_smat; }
    const vector<double>& entropies () const { return m_entropies; }
    
    
protected:
    boost::signals2::signal<sequence_processor::sig_cb_content_loaded>* signal_content_loaded;
    boost::signals2::signal<sequence_processor::sig_cb_frame_loaded>* signal_frame_loaded;
    boost::signals2::signal<sequence_processor::sig_cb_sm1d_available>* signal_sm1d_available;
    boost::signals2::signal<sequence_processor::sig_cb_sm1dmed_available>* signal_sm1dmed_available;
    boost::signals2::signal<sequence_processor::sig_cb_3dstats_available>* signal_3dstats_available;
    boost::signals2::signal<sequence_processor::sig_cb_geometry_available>* signal_geometry_available;
    boost::signals2::signal<sequence_processor::sig_cb_ss_image_available>* signal_ss_image_available;
    
private:

    
    void compute_shortterm (const uint32_t halfWinSz) const;
    

    void stats_3d_computed ();
    void pci_done ();
    void sm_content_loaded ();
    
    // path to cache folder for this serie
    fs::path mCurrentCachePath;
    
    // Assumes LIF data -- use multiple window.
    void load_channels_from_images (const std::shared_ptr<seqFrameContainer>& frames);
    
    
    // Note tracks contained timed data.
    void median_leveled_pci (namedTrack_t& track);
    
    // @note Specific to ID Lab Lif Files
    void create_named_tracks (const std::vector<std::string>& names, const std::vector<std::string>& plot_names);
    
    void fill_longterm_pci(namedTrack_t& track);
    
    mutable std::mutex m_mutex;
    mutable std::mutex m_shortterms_mutex;
    mutable std::mutex m_segmentation_mutex;
    uint32_t m_channel_count;
    mutable std::condition_variable m_shortterm_cv;
    
    // One similarity engine
    mutable smProducerRef m_sm_producer;
    mutable vector<double> m_entropies;
    mutable vector<vector<double>> m_smat;
    
    // Median Levels
    std::vector<double> m_medianLevel;
    
    // Std Dev Image

    
    channel_images_t m_images;
    channel_vec_t m_all_by_channel;
    
    int64_t m_frameCount;
    std::vector<Rectf> m_channel_rois;
    Rectf m_all;
    
    // @note Specific to ID Lab Lif Files
    // 3 channels: 2 flu one visible
    // 1 channel: visible
    // Note create_named_tracks
    std::shared_ptr<vecOfNamedTrack_t>  m_longterm_pci_tracksRef;
    mutable vecOfNamedTrack_t                   m_shortterm_pci_tracks;
    mutable std::queue<float>               m_shortterms;
    
    
    mutable svl::stats<int64_t> m_3d_stats;
    std::atomic<bool> m_3d_stats_done;
    cv::RotatedRect m_motion_mass;
    cv::RotatedRect m_motion_mass_bottom;
    mutable cv::Mat m_temporal_ss;
    mutable fPair m_ab;
    
    mutable std::vector<std::vector<float>> m_temporal_ss_raw;
    uiPair m_voxel_sample;
    std::map<index_time_t, labelBlob::weak_ref> m_blob_cache;
    labelBlob::ref m_main_blob;
    std::vector<labelBlob::blob> m_blobs;
    
    std::shared_ptr<OCVImageWriter> m_writer;
    
};


#endif
