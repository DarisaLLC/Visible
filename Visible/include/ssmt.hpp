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
#include "timed_value_containers.h"
#include "core/signaler.h"
#include "sm_producer.h"
#include "cinder_xchg.hpp"
#include "vision/histo.h"
#include "vision/labelBlob.hpp"
#include "vision/opencv_utils.hpp"
#include "opencv2/video/tracking.hpp"
#include "algo_runners.hpp"
#include "segmentation_parameters.hpp"
#include "iowriter.hpp"
#include "moving_region.h"

using namespace cv;
using blob = svl::labelBlob::blob;


// For base classing
class ssmtSignaler : public base_signaler
{
    virtual std::string getName () const { return "ssmtSignaler"; }
};

/*
 
 Example of how to write out channel images
 
 TEST (ut_lifFile, save_channel_image)
 {
 std::string filename ("3channels.lif");
 std::pair<test_utils::genv::path_t, bool> res = dgenv_ptr->asset_path(filename);
 EXPECT_TRUE(res.second);
 lifIO::LifReader lif(res.first.string(), "");
 cout << "LIF version "<<lif.getVersion() << endl;
 EXPECT_EQ(13, lif.getNbSeries() );
 size_t serie = 4;
 lifIO::LifSerie& se4 = lif.getSerie(serie);
 se4.set_content_type("IDLab_0");
 
 std::vector<std::string> names { "green", "red", "gray" };
 
 // Create the frameset and assign the channel names
 // Fetch the media info
 auto mFrameSet = seqFrameContainer::create (se4);
 ssmt_processor lp;
 lp.load(mFrameSet, names);
 std::string dir = "/Users/arman/tmp/c2";
 lp.save_channel_images(2,dir);
 
 }
 
 
 */



class ssmt_processor : public ssmtSignaler
{
public:
    using voxel_params_t=fynSegmenter::params;
    
    class params{
    public:
        params (const voxel_params_t voxel_params = voxel_params_t()): m_vparams(voxel_params) {}
        
        const std::pair<uint32_t,uint32_t>& voxel_sample () {return m_vparams.voxel_sample(); }
        const std::pair<uint32_t,uint32_t>& voxel_pad () {return m_vparams.voxel_pad(); }
        const iPair& expected_segmented_size () {return m_vparams.expected_segmented_size(); }
        const uint32_t& min_seqmentation_area () {return m_vparams.min_segmentation_area(); }
        const fPair& normalized_symmetric_padding () { return m_vparams.normalized_symmetric_padding (); }
        
        const std::string& image_cache_name () {
            static std::string s_image_cache_name = "voxel_ss_.png";
            return s_image_cache_name;
        }
        const std::string& result_container_cache_name (){
            static std::string s_result_container_cache_name = "container_ss_";
            return s_result_container_cache_name;
        }
        const std::string& internal_container_cache_name (){
            static std::string s_internal_container_cache_name = "internal_ss_";
            return s_internal_container_cache_name;
        }
        
    private:
        voxel_params_t m_vparams;
    };
    
    using contractionContainer_t = contractionLocator::contractionContainer_t;
    
    typedef std::unordered_map<uint8_t,std::vector<Point2f>> mapU8PeaksToPointfs_t;
    typedef void (sig_cb_content_loaded) (int64_t&);
    typedef void (sig_cb_flu_stats_ready) ();
    typedef void (sig_cb_contraction_ready) (contractionContainer_t &);
    typedef void (sig_cb_frame_loaded) (int&, double&);
    typedef void (sig_cb_sm1d_ready) (int&);
    typedef void (sig_cb_sm1dmed_ready) (int&,int&);
    typedef void (sig_cb_3dstats_ready) ();
    typedef void (sig_cb_geometry_ready) (int&);
    typedef void (sig_cb_ss_voxel_ready) (std::vector<float> &);
    typedef void (sig_cb_segmented_view_ready) (cv::Mat &, cv::Mat &, iPair &);
    typedef std::vector<roiWindow<P8U>> channel_images_t;
    typedef std::vector<channel_images_t> channel_vec_t;
    
    
    ssmt_processor (const fs::path& = fs::path());
    
    // Assumes LIF data -- use multiple window.
    void load_channels_from_images (const std::shared_ptr<seqFrameContainer>& frames);
    // @note Specific to ID Lab Lif Files
    void create_named_tracks (const std::vector<std::string>& names, const std::vector<std::string>& plot_names);
    
    
    const smProducerRef similarity_producer () const;
    const int64_t& frame_count () const;
    const int64_t channel_count () const;
    const svl::stats<int64_t> stats3D () const;
    cv::Mat & segmented () const { return m_temporal_ss;  }
    
    // Check if the returned has expired
    std::weak_ptr<contractionLocator> contractionWeakRef ();
    
    // Load frames from cache
    int64_t load (const std::shared_ptr<seqFrameContainer>& frames,const std::vector<std::string>& names, const std::vector<std::string>& plot_names);
    
    // Run Luminance info on a vector of channel indices
    std::shared_ptr<vecOfNamedTrack_t> run_flu_statistics (const std::vector<int>& channels);
    
    // Channel Index API for IDLab custom organization
    // Run accumulator of 3d stats on a channel at index
    svl::stats<int64_t> run_volume_sum_sumsq_count (const int channel_index);
    // Run per pixel stdev accumulator a channel at index
    void run_detect_geometry (const int channel_index);
    
    
    // Vector of 8bit roiWindows API for IDLab custom organization
    svl::stats<int64_t> run_volume_sum_sumsq_count (std::vector<roiWindow<P8U>>&);
    void run_detect_geometry (std::vector<roiWindow<P8U>>& );
    
    void run_volume_variances (std::vector<roiWindow<P8U>>& images);
    
    // Run to get Ranks and Median Level Set. Both short term and long term pic
    // @todo event though index is an argument, only room for one channel is kept.
    // Short term is specifically run on the channel contraction pci was run on
    // duplictes will be removed from indices for short term pci.
    std::shared_ptr<vecOfNamedTrack_t> run_contraction_pci_on_channel (const int channel_index);
    std::shared_ptr<vecOfNamedTrack_t> run_contraction_pci (const std::vector<roiWindow<P8U>>&);
    std::vector<float> shortterm_pci (const std::vector<uint32_t>& indices);
    void shortterm_pci (const uint32_t& temporal_window);
    const vecOfNamedTrack_t& shortterm_pci () const { return m_shortterm_pci_tracks; }
    
    // Return 2D latice of pixels over time
    void generateVoxels_on_channel (const int channel_index);
    void generateVoxelsOfSampled (const std::vector<roiWindow<P8U>>&);
    
    // Return 2D latice of voxel self-similarity
    void generateVoxelSelfSimilarities ();
    void generateVoxelsAndSelfSimilarities (const std::vector<roiWindow<P8U>>& images);
    
    void finalize_segmentation (cv::Mat& mono, cv::Mat& label, iPair& padded_translation);
    const std::vector<Rectf>& channel_rois () const;
    const  std::deque<double>& medianSet () const;

    const std::vector<moving_region>& moving_regions ()const;
    
    const Rectf& measuredArea () const;
    
    // Update. Called also when cutoff offset has changed
    void update ();
    
    std::shared_ptr<ioImageWriter>& get_image_writer ();
    std::shared_ptr<ioImageWriter>& get_csv_writer ();
    void save_channel_images (int channel_index, std::string& dir_fqfn);
    
    const vector<vector<double>>& ssMatrix () const { return m_smat; }
    const vector<double>& entropies () const { return m_entropies; }
    
    void generate_affine_windows ();
    
    
    
protected:
    boost::signals2::signal<ssmt_processor::sig_cb_content_loaded>* signal_content_loaded;
    boost::signals2::signal<ssmt_processor::sig_cb_flu_stats_ready>* signal_flu_ready;
    boost::signals2::signal<ssmt_processor::sig_cb_frame_loaded>* signal_frame_loaded;
    boost::signals2::signal<ssmt_processor::sig_cb_sm1d_ready>* signal_sm1d_ready;
    boost::signals2::signal<ssmt_processor::sig_cb_sm1dmed_ready>* signal_sm1dmed_ready;
    boost::signals2::signal<ssmt_processor::sig_cb_contraction_ready>* signal_contraction_ready;
    boost::signals2::signal<ssmt_processor::sig_cb_3dstats_ready>* signal_3dstats_ready;
    boost::signals2::signal<ssmt_processor::sig_cb_geometry_ready>* signal_geometry_ready;
    boost::signals2::signal<ssmt_processor::sig_cb_segmented_view_ready>* signal_segmented_view_ready;
    boost::signals2::signal<ssmt_processor::sig_cb_ss_voxel_ready>* signal_ss_voxel_ready;
    
private:
    // Default params. @place_holder for increasing number of params
    ssmt_processor::params m_params;
    
    void internal_generate_affine_profiles ();
    void internal_generate_affine_translations ();
    void internal_generate_affine_windows (const std::vector<roiWindow<P8U>>&);
    
    void save_affine_windows ();
    void save_affine_profiles ();
    
    void create_voxel_surface (std::vector<float>&);
    
    const std::vector<blob>& blobs () const { return m_blobs; }
    
    void compute_shortterm (const uint32_t halfWinSz) const;
    
    void contraction_ready (contractionContainer_t&);
    void stats_3d_computed ();
    void pci_done ();
    void sm_content_loaded ();
    
    // path to cache folder for this serie
    fs::path mCurrentSerieCachePath;
    
  
    // Note tracks contained timed data.
    void median_leveled_pci (namedTrack_t& track);
    
  
    std::vector<moving_region> m_regions;
    
    mutable std::mutex m_mutex;
    mutable std::mutex m_shortterms_mutex;
    mutable std::mutex m_segmentation_mutex;
    mutable std::mutex m_processing_mutex;
    mutable std::mutex m_io_mutex;
    
    uint32_t m_channel_count;
    mutable std::condition_variable m_shortterm_cv;
    
    // One similarity engine
    mutable smProducerRef m_sm_producer;
    vector<double> m_entropies;
    vector<float> m_voxel_entropies;
    mutable vector<vector<double>> m_smat;
    
    
    // Median Levels
    std::vector<double> m_medianLevel;
    
    // Std Dev Image
    mutable mapU8PeaksToPointfs_t m_map_peaks_to_points;
    
    channel_images_t m_images;
    channel_vec_t m_all_by_channel;
    
    int64_t m_frameCount;
    std::vector<Rectf> m_channel_rois;
    Rectf m_measured_area;
    Rectf m_all;
    
    // @note Specific to ID Lab Lif Files
    // 3 channels: 2 flu one visible
    // 1 channel: visible
    // Note create_named_tracks
    std::shared_ptr<vecOfNamedTrack_t>  m_flurescence_tracksRef;
    std::shared_ptr<vecOfNamedTrack_t>  m_contraction_pci_tracksRef;
    mutable vecOfNamedTrack_t                   m_shortterm_pci_tracks;
    mutable std::queue<float>               m_shortterms;
    
    std::shared_ptr<contractionLocator> m_caRef;
    
    mutable svl::stats<int64_t> m_3d_stats;
    std::atomic<bool> m_3d_stats_done;
    mutable cv::Mat m_temporal_ss;
    mutable cv::Mat m_var_image;
    uiPair m_voxel_sample;
    iPair m_expected_segmented_size;
    std::map<index_time_t, labelBlob::weak_ref> m_blob_cache;
    labelBlob::ref m_main_blob;
    std::vector<blob> m_blobs;
    std::vector<cv::Point> m_var_peaks;
    std::vector<std::vector<float>> m_hz_profiles;
    std::vector<std::vector<float>> m_vt_profiles;
 
    mutable fPair m_length_range;
    mutable std::vector<roiWindow<P8U>> m_voxels;
    mutable std::vector<cv::Mat> m_affine_windows;
    mutable std::vector<float> m_affine_translations;
    mutable std::vector<float> m_norm_affine_translations;
    std::shared_ptr<ioImageWriter> m_image_writer;
    std::shared_ptr<ioImageWriter> m_csv_writer;
    
};



#endif
