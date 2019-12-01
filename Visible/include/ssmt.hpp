#ifndef __SSMT__
#define __SSMT__


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
#include "input_selector.hpp"
using namespace cv;
using blob = svl::labelBlob::blob;

class ssmt_result;



// For base classing
class ssmtSignaler : public base_signaler
{
    virtual std::string getName () const { return "ssmtSignaler"; }
};

class ssmt_processor : public ssmtSignaler, public std::enable_shared_from_this<ssmt_processor>
{
public:
    using voxel_params_t=fynSegmenter::params;
    
    friend class ssmt_result;
    
    class params{
    public:
        enum ContentType {
            lif = 0,
            bgra = lif+1,
        };
        params (const ContentType ct = ContentType::lif, const voxel_params_t voxel_params = voxel_params_t()): m_content(ct), m_vparams(voxel_params) {}
        
        const ContentType& content_type () { return m_content; }
        
        const std::pair<uint32_t,uint32_t>& voxel_sample () {return m_vparams.voxel_sample(); }
        const std::pair<uint32_t,uint32_t>& voxel_pad () {return m_vparams.voxel_sample_half(); }
        const iPair& expected_segmented_size () {return m_vparams.expected_segmented_size(); }
        const uint32_t& min_seqmentation_area () {return m_vparams.min_segmentation_area(); }
        
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
        ContentType m_content;
    };
    
  //  using contractionContainer_t = contractionLocator::contractionContainer_t;
    
    typedef void (sig_cb_content_loaded) (int64_t&); // Content is loaded and Voxel Processing is initiated
    typedef void (sig_cb_frame_loaded) (int&, double&); // a frame is loaded
    
    typedef void (sig_cb_flu_stats_ready) (); // Fluorescense stats are done
    typedef void (sig_cb_contraction_ready) (contractionLocator::contractionContainer_t&,const input_channel_selector_t& in); // Scene Contraction is ready

    typedef void (sig_cb_sm1d_ready) (std::vector<float> &, const input_channel_selector_t&); // Scene self_similarity is done
    typedef void (sig_cb_sm1dmed_ready) (const input_channel_selector_t&);// regularization via median level sets are done -1 or mobj index

    typedef void (sig_cb_3dstats_ready) (); // Scene Image Volume stats are done
    typedef void (sig_cb_ss_voxel_ready) (std::vector<float> &); // Voxel Self-Similarity is done
    typedef void (sig_cb_geometry_ready) (int count, const input_channel_selector_t&); // Moving segments are generated
    typedef void (sig_cb_segmented_view_ready) (cv::Mat &, cv::Mat &); // Motion Entropic Space is done


    typedef std::vector<roiWindow<P8U>> channel_images_t;
    typedef std::vector<channel_images_t> channel_vec_t;
    
    /*
       ssmt_processor constructor (takes an optional path to cache to be used or constructed )
    */
    ssmt_processor (const fs::path& = fs::path(), const ssmt_processor::params& = ssmt_processor::params () );
    
    // Assumes LIF data -- use multiple window.
    void load_channels_from_images (const std::shared_ptr<seqFrameContainer>& frames);
    // @note Specific to ID Lab Lif Files
    // ADD Create Tracks for all cells
    void create_named_tracks (const std::vector<std::string>& names, const std::vector<std::string>& plot_names);
    

    const int64_t& frame_count () const;
    const uint32_t channel_count () const;
    const std::vector<std::shared_ptr<ssmt_result>>& moving_bodies ()const { return m_results; }
    
    const svl::stats<int64_t> stats3D () const;
    cv::Mat & segmented () const { return m_temporal_ss;  }
    const std::vector<Rectf>& channel_rois () const;
    const  std::deque<double>& medianSet () const;
    const std::vector<moving_region>& moving_regions ()const;
    const Rectf& measuredArea () const;
    // Update. Called also when cutoff offset has changed
    void update (const input_channel_selector_t&);
    const vector<vector<double>>& ssMatrix () const { return m_smat[-1]; }
    const vector<double>& entropies () const { return m_entropies[-1]; }
    
    // Add generate pci and flu for a cell.
    
  
    // Load frames from cache
    int64_t load (const std::shared_ptr<seqFrameContainer>& frames,const std::vector<std::string>& names, const std::vector<std::string>& plot_names);
    
    // Run Luminance info on a vector of channel indices
    std::shared_ptr<vecOfNamedTrack_t> run_flu_statistics (const std::vector<int>& channels);
    
    // Channel Index API for IDLab custom organization
    // Run accumulator of 3d stats on a channel at index
    svl::stats<int64_t> run_volume_sum_sumsq_count (const int channel_index);
    // Run per pixel stdev accumulator a channel at index
    void find_moving_regions (const int channel_index);
    
 

    // Vector of 8bit roiWindows API for IDLab custom organization
    svl::stats<int64_t> run_volume_sum_sumsq_count (std::vector<roiWindow<P8U>>&);
    void find_moving_regions (std::vector<roiWindow<P8U>>& );
    
  
    // Run to get Ranks and Median Level Set. Both short term and long term pic
    // Short term is specifically run on the channel contraction pci was run on
    // duplictes will be removed from indices for short term pci.
    // args:
    // channel_index which channel of multi-channel input. Usually visible channel is the last one
    // input is -1 for the entire root or index of moving object area in results container
    std::shared_ptr<vecOfNamedTrack_t> run_contraction_pci_on_selected_input (const input_channel_selector_t& );
    // 2nd interface is lower level interface
    // args:
    // images: vector of roiWindow<P8U>s. roiWindow<P8U> is a single plane image container.
    std::shared_ptr<vecOfNamedTrack_t> run_contraction_pci (const std::vector<roiWindow<P8U>>& images,  const input_channel_selector_t& );
    
    std::vector<float> shortterm_pci (const std::vector<uint32_t>& indices);
    void shortterm_pci (const uint32_t& temporal_window);
    
    // Return 2D latice of pixels over time
    void generateVoxels_on_channel (const int channel_index);
    void generateVoxelsOfSampled (const std::vector<roiWindow<P8U>>&);
    
    // Return 2D latice of voxel self-similarity
    void generateVoxelSelfSimilarities ();
    void generateVoxelsAndSelfSimilarities (const std::vector<roiWindow<P8U>>& images);
    
    void finalize_segmentation (cv::Mat& mono, cv::Mat& label);
    const channel_vec_t& content () const;
  
    std::weak_ptr<contractionLocator> entireContractionWeakRef ();
    
    
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

    
    void run_volume_variances (std::vector<roiWindow<P8U>>& images);
    
    const smProducerRef similarity_producer () const;
    void create_voxel_surface (std::vector<float>&);
    // return -1 for an error or number of moving object directories created ( 0 is valid )
    
    int create_cache_paths ();
    // channel_index which channel of multi-channel input. Usually visible channel is the last one
    // input is -1 for the entire root or index of moving object area in results container
    fs::path get_cache_location (const int channel_index,const int input);
    
    const std::vector<blob>& blobs () const { return m_blobs; }
    
    void compute_shortterm (const uint32_t halfWinSz) const;
    
    void contraction_ready (contractionLocator::contractionContainer_t&);
    void stats_3d_computed ();
    void pci_done ();
    void sm_content_loaded ();
    void signal_geometry_done (int, const input_channel_selector_t&);
    
    std::shared_ptr<ioImageWriter>& get_image_writer ();
    std::shared_ptr<ioImageWriter>& get_csv_writer ();
    int save_channel_images (const input_channel_selector_t& in,  const std::string& dir_fqfn);

    // Generate profiles and images for moving regions & save them
    void generate_affine_windows ();
    void internal_generate_affine_profiles ();
    void internal_generate_affine_translations ();
    void internal_generate_affine_windows (const std::vector<roiWindow<P8U>>&);
    void save_affine_windows ();
    void save_affine_profiles ();
    
    // path to cache folder for this serie
    fs::path mCurrentCachePath;
    
    std::shared_ptr<contractionLocator> m_entireCaRef;
  
    // Note tracks contained timed data.
    void median_leveled_pci (namedTrack_t& track, const input_channel_selector_t& in);
    
    std::vector<std::shared_ptr<ssmt_result>> m_results;
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
    vector<float> m_voxel_entropies;

    mutable std::unordered_map<int,vector<double>> m_entropies;
    mutable std::unordered_map<int,vector<vector<double>>> m_smat;
    
    
    // Median Levels
    std::vector<double> m_medianLevel;
    
    channel_images_t m_images;
    channel_vec_t m_all_by_channel;
    
    int64_t m_frameCount;
    Rectf m_measured_area;
    Rectf m_all;
    
    mutable std::shared_ptr<vecOfNamedTrack_t> m_moment_tracksRef;
    mutable std::shared_ptr<vecOfNamedTrack_t> m_longterm_pci_tracksRef;

    mutable  vecOfNamedTrack_t m_shortterm_pci_tracks;
    mutable std::queue<float> m_shortterms;
    
    
    std::shared_ptr<ioImageWriter> m_image_writer;
    std::shared_ptr<ioImageWriter> m_csv_writer;
    
    
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
    mutable fPair m_length_range;
    mutable std::vector<roiWindow<P8U>> m_voxels;
    input_channel_selector_t m_instant_input;
    std::map<uint32_t, fs::path> m_result_index_to_cache_path;
    
  
    
};

// @note Specific to ID Lab Lif Files
// 3 channels: 2 flu one visible
// 1 channel: visible
// Note create_named_tracks
class ssmt_result : public moving_region {
public:
    typedef std::shared_ptr<ssmt_result> ref_t;
    typedef std::shared_ptr<vecOfNamedTrack_t> trackRef_t;
    typedef std::unordered_map<std::string, trackRef_t> trackMap_t;
    
    static const ref_t create (std::shared_ptr<ssmt_processor>&, const moving_region&,
                               const input_channel_selector_t& in);
    
    void process ();
    const trackMap_t& trackBook () const;
    const uint64_t frameCount () const;
    const uint32_t channelCount () const;
    const ssmt_processor::channel_vec_t& content () const;
    size_t Id() const;
    const input_channel_selector_t& input() const;
    const std::shared_ptr<contractionLocator> & locator () const;
private:
    bool run_contraction_pci (const std::vector<roiWindow<P8U>>& images);
    ssmt_result (const moving_region&,const input_channel_selector_t& in);
    void signal_sm1d_ready (vector<float>&, const input_channel_selector_t&);
    void contraction_ready (contractionLocator::contractionContainer_t& contractions, const input_channel_selector_t&);
    bool get_channels (int channel);
    input_channel_selector_t m_input;
    mutable trackMap_t m_trackBook;
    
    mutable trackRef_t m_flurescence_tracksRef;
    mutable trackRef_t m_contraction_pci_tracksRef;
    async_vecOfNamedTrack_t m_contraction_pci_tracks_asyn;
    
    vector<double> m_entropies;
    std::vector<std::vector<double>> m_smat;
    
    std::shared_ptr<contractionLocator> m_caRef;
    
    mutable std::vector<cv::Mat> m_affine_windows;
    mutable std::vector<float> m_affine_translations;
    mutable std::vector<float> m_norm_affine_translations;
    std::vector<std::vector<float>> m_hz_profiles;
    std::vector<std::vector<float>> m_vt_profiles;
    
    ssmt_processor::channel_images_t m_images;
    ssmt_processor::channel_vec_t m_all_by_channel;
    
    uint64_t m_frameCount;
    uint32_t m_channel_count;
    std::mutex m_mutex;
    std::weak_ptr<ssmt_processor> m_weak_parent;
    
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




#endif
