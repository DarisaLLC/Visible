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

#include <OpenImageIO/imagebuf.h>
#include <OpenImageIO/imagecache.h>
#include <OpenImageIO/imageio.h>

#include "timed_types.h"
#include "core/signaler.h"
#include "sm_producer.h"
#include "cinder_xchg.hpp"
#define float16_t opencv_broken_float16_t
#include "vision/histo.h"
#undef float16_t
#include "vision/labelBlob.hpp"
#include "vision/opencv_utils.hpp"
#include "opencv2/video/tracking.hpp"
#include "algo_runners.hpp"
#include "segmentation_parameters.hpp"
#include "iowriter.hpp"
#include "moving_region.h"
#include "input_selector.hpp"
#include "mediaInfo.h"
#include "thread.h"
#include "contraction.hpp"
#include "median_levelset.hpp"
#include "mediaInfo.h"

using namespace cv;
using blob = svl::labelBlob::blob;
using namespace boost;
namespace bfs=boost::filesystem;
using namespace OIIO;

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

        params (const TypeDesc ct = TypeUInt8, const voxel_params_t voxel_params = voxel_params_t()):
		m_type(ct), m_vparams(voxel_params), m_channel_to_use(0), m_channel_root(-1,m_channel_to_use){}
        
        const TypeDesc& content_type () { return m_type; }
        
        const std::pair<uint32_t,uint32_t>& voxel_sample () const {return m_vparams.voxel_sample(); }
        const std::pair<uint32_t,uint32_t>& voxel_pad () const {return m_vparams.voxel_sample_half(); }
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
		void magnification (const float& mmag) const { m_magnification_x = mmag; }
		float magnification () const { return m_magnification_x; }

		void channel_to_use(int c){ m_channel_to_use = c; m_channel_root = result_index_channel_t(-1,c); }
		int channel_to_use() const { return m_channel_to_use; }
		const result_index_channel_t& channel_to_use_root () const {
			return m_channel_root;
		}
		
		
		
    private:
		mutable float m_magnification_x;
        mutable voxel_params_t m_vparams;
        mutable TypeDesc m_type;
		mutable int m_channel_to_use;
		mutable result_index_channel_t m_channel_root;
		
    };
    
  //  using contractionContainer_t = contractionLocator::contractionContainer_t;
    
    typedef void (sig_cb_content_loaded) (int64_t&); // Content is loaded and Voxel Processing is initiated
    typedef void (sig_cb_frame_loaded) (int&, double&); // a frame is loaded
    
    typedef void (sig_intensity_over_time_ready) (); // Fluorescense stats are done
    typedef void (sig_cb_contraction_ready) (contractionLocator::contractionContainer_t&,const result_index_channel_t& in); // Scene Contraction is ready

    typedef void (sig_cb_root_pci_ready) (std::vector<float> &, const result_index_channel_t&); // Scene self_similarity is done
    typedef void (sig_cb_root_pci_median_regularized_ready) (const result_index_channel_t&);// regularization via median level sets are done -1 or mobj index

    typedef void (sig_cb_volume_ready) (); // Scene Image Volume stats are done
    typedef void (sig_cb_ss_voxel_ready) (std::vector<float> &); // Voxel Self-Similarity is done
    typedef void (sig_cb_geometry_ready) (int count, const result_index_channel_t&); // Moving segments are generated
    typedef void (sig_cb_segmented_view_ready) (cv::Mat &, cv::Mat &); // Motion Entropic Space is done


    typedef std::vector<roiWindow<P8U>> channel_images_t;
    typedef std::vector<channel_images_t> channel_vec_t;
    
    /*
       ssmt_processor constructor (takes an optional path to cache to be used or constructed )
    */
    ssmt_processor ( const mediaSpec&, const bfs::path& = bfs::path(), const ssmt_processor::params& = ssmt_processor::params () );
    
   
    const int64_t& frame_count () const;
    const uint32_t channel_count () const;
    const ssmt_processor::params& parameters () const { return m_params; }
    const mediaSpec& media_spec () const { return m_loaded_spec;}
    
    const svl::stats<int64_t> stats3D () const;
    cv::Mat & segmented () const { return m_temporal_ss;  }
    const std::vector<Rectf>& channel_rois () const;
    const  std::deque<double>& medianSet () const;
 
    // Update. Called also when cutoff offset has changed
    //void update (const input_section_selector_t&);
	void update();
    const vector<vector<double>>& ssMatrix () const { return m_smat; }
	const vector<double>& entropies () const { return m_entropies; }
	const vector<float>& entropies_F () const { return m_entropies_F; }
    
  
  
    // Load frames from cache
    void load_channels_from_ImageBuf (const std::shared_ptr<ImageBuf>& frames, const ustring& contentName, const mediaSpec& );

    
    // Run Luminance info on a vector of channel indices over time
    // Signals completion using intensity_over_time_ready
    // Returns results in tracks
 //   std::shared_ptr<vecOfNamedTrack_t> run_intensity_statistics (const std::vector<int>& channels);
    
    // Run accumulator of Volume stats on a channel at index
    //Signals completion using signal_volume_ready
    svl::stats<int64_t> run_volume_stats (const int channel_index);
    
    // Run temporal segmentation and find moving bodies
    // Runs Voxel based segmentation
    //  indicates completion of voxel generation using signal_ss_voxel_ready
    //  indicates completion of motion map using signal_segmented_view_ready
    //     it used signal_segmented_view_ready triggers finlization and reporting.
    // signal_geometry_ready indicates results are ready.
    void find_moving_regions (const int channel_index);
    const std::vector<std::shared_ptr<ssmt_result>>& moving_bodies ()const { return m_results; }
    const std::vector<moving_region>& moving_regions ()const;
    const Rectf& measuredArea () const;
    
    // Run to get Ranks and Median Level Set. Both short term and long term pic
    // Short term is specifically run on the channel contraction pci was run on
    // duplictes will be removed from indices for short term pci.
    // args:
    // channel_index which channel of multi-channel input. Usually visible channel is the last one
    // input is -1 for the entire root or index of moving object area in results container
    void run_selfsimilarity_on_selected_input (const result_index_channel_t&,const progress_fn_t& reporter );
   
    
    std::vector<float> shortterm_pci (const std::vector<uint32_t>& indices);
    void shortterm_pci (const uint32_t& temporal_window);
    
    // Return 2D latice of pixels over time
    void generateVoxels_on_channel (const int channel_index);
//    void generateVoxelsOfSampled (const std::vector<roiWindow<P8U>>&);
//
    void generateVoxelsAndSelfSimilarities (const std::vector<roiWindow<P8U>>& images);
    
    void finalize_segmentation (cv::Mat& mono, cv::Mat& label);
    const channel_vec_t& content () const;
  
	medianLevelSet& medianLeveler () { return m_leveler; }
	
	
	const mediaSpec& media() const { return m_media_spec; }
	
    
    
protected:
    boost::signals2::signal<ssmt_processor::sig_cb_content_loaded>* signal_content_loaded;
    boost::signals2::signal<ssmt_processor::sig_intensity_over_time_ready>* intensity_over_time_ready;
    boost::signals2::signal<ssmt_processor::sig_cb_frame_loaded>* signal_frame_loaded;
    boost::signals2::signal<ssmt_processor::sig_cb_root_pci_ready>* signal_root_pci_ready;
    boost::signals2::signal<ssmt_processor::sig_cb_root_pci_median_regularized_ready>* signal_root_pci_med_reg_ready;
    boost::signals2::signal<ssmt_processor::sig_cb_contraction_ready>* signal_contraction_ready;
    boost::signals2::signal<ssmt_processor::sig_cb_volume_ready>* signal_volume_ready;
    boost::signals2::signal<ssmt_processor::sig_cb_geometry_ready>* signal_geometry_ready;
    boost::signals2::signal<ssmt_processor::sig_cb_segmented_view_ready>* signal_segmented_view_ready;
    boost::signals2::signal<ssmt_processor::sig_cb_ss_voxel_ready>* signal_ss_voxel_ready;
    
private:
    // 2nd interface is lower level interface
       // args:
       // images: vector of roiWindow<P8U>s. roiWindow<P8U> is a single plane image container.
   void internal_run_selfsimilarity_on_selected_input  (const std::vector<roiWindow<P8U>>& images,  const result_index_channel_t&,const progress_fn_t& reporter);

    // Assumes LIF data -- use multiple window.
    void internal_load_channels_from_lif_buffer2d (const std::shared_ptr<ImageBuf>& frames,  const ustring& contentName, const mediaSpec& sd);

   void create_named_tracks (const std::vector<std::string>& names, const std::vector<std::string>& plot_names);
       
    
    // Internal use
    // Vector of 8bit roiWindows API for IDLab custom organization
    svl::stats<int64_t> run_volume_stats (std::vector<roiWindow<P8U>>&);
    void internal_find_moving_regions (std::vector<roiWindow<P8U>>& );
    
    
    // Default params. @place_holder for increasing number of params
    mutable ssmt_processor::params m_params;
    mutable mediaSpec m_loaded_spec;
    
    void volume_variance_peak_promotion (std::vector<roiWindow<P8U>>& images);
    
    const smProducerRef similarity_producer () const;
    void create_voxel_surface (std::vector<float>&);
    // return -1 for an error or number of moving object directories created ( 0 is valid )
    
    int create_cache_paths ();
    // channel_index which channel of multi-channel input. Usually visible channel is the last one
    // input is -1 for the entire root or index of moving object area in results container
    bfs::path get_cache_location (const int channel_index,const int input);
    
    const std::vector<blob>& blobs () const { return m_blobs; }
    
    void compute_shortterm (const uint32_t halfWinSz) const;
    
	void contraction_ready (contractionLocator::contractionContainer_t&, const result_index_channel_t&);
    void volume_stats_computed ();
    void pci_done ();
    void sm_content_loaded ();
    void signal_geometry_done (int, const result_index_channel_t&);
    
//    std::shared_ptr<ioImageWriter>& get_image_writer ();
//    std::shared_ptr<ioImageWriter>& get_csv_writer ();
//    int save_channel_images (const input_section_selector_t& in,  const std::string& dir_fqfn);

    // Generate profiles and images for moving regions & save them
    void generate_affine_windows ();
    void internal_generate_affine_profiles ();
    void internal_generate_affine_translations ();
    void internal_generate_affine_windows (const std::vector<roiWindow<P8U>>&);
    void save_affine_windows ();
    void save_affine_profiles ();
    
    // path to cache folder for this serie
    bfs::path mCurrentCachePath;
    
	medianLevelSet m_leveler;
  
    // Note tracks contained timed data.
 //   void median_leveled_pci (namedTrack_t& track, const input_section_selector_t& in);
    
    std::vector<std::shared_ptr<ssmt_result>> m_results;
    std::vector<moving_region> m_regions;
    
    mutable std::mutex m_mutex;
    mutable std::mutex m_shortterms_mutex;
    mutable std::mutex m_segmentation_mutex;
    mutable std::mutex m_io_mutex;
    mutable recursive_mutex m_input_mutex;  ///< Mutex protecting ssmt
    
    uint32_t m_channel_count;
    mutable std::condition_variable m_shortterm_cv;
    
    // One similarity engine
    mutable smProducerRef m_sm_producer;
    vector<float> m_voxel_entropies;

	mutable vector<double> m_entropies;
	mutable vector<float> m_entropies_F;
    mutable vector<vector<double>> m_smat;
    
    channel_images_t m_images;
    channel_vec_t m_all_by_channel;
    
    int64_t m_frameCount;
    Rectf m_measured_area;
    Rectf m_all;
    
    std::shared_ptr<ioImageWriter> m_image_writer;
    std::shared_ptr<ioImageWriter> m_csv_writer;
    
    
    mutable svl::stats<int64_t> m_volume_stats;
    std::atomic<bool> m_variance_peak_detection_done;
    mutable cv::Mat m_temporal_ss;
    mutable cv::Mat m_var_image;
    uiPair m_voxel_sample;
    iPair m_expected_segmented_size;
    std::map<index_time_t, labelBlob::weak_ref_t> m_blob_cache;
    labelBlob::ref m_main_blob;
    std::vector<blob> m_blobs;
    std::vector<cv::Point> m_var_peaks;
    mutable fPair m_length_range;
    mutable std::vector<roiWindow<P8U>> m_voxels;
    result_index_channel_t m_instant_input;
    std::map<uint32_t, bfs::path> m_result_index_to_cache_path;
	mediaSpec m_media_spec;
    
  
    
};

// @note Specific to ID Lab Lif Files
// 3 channels: 2 flu one visible
// 1 channel: visible
// Note create_named_tracks
class ssmt_result : public moving_region,std::enable_shared_from_this<ssmt_result> {
public:
    typedef std::shared_ptr<ssmt_result> ref_t;
    typedef std::weak_ptr<ssmt_result> weak_ref_t;
    
    static const ref_t create (std::shared_ptr<ssmt_processor>&, const moving_region&,
                               const result_index_channel_t& in);

    ssmt_result::ref_t getShared () {
        return shared_from_this();
    }
    
    ssmt_result::weak_ref_t getWeakRef(){
        weak_ref_t weak = getShared();
        return weak;
    }
	
	bool generateRegionImages () const;
	
	bool run_selfsimilarity ();

	bool process ();
    bool segment_at_contraction (const std::vector<roiWindow<P8U>>& images, const std::vector<int> &peak_indices);
    
//    const trackMap_t& trackBook () const;
    const uint64_t frameCount () const;
    const uint32_t channelCount () const;
    const ssmt_processor::channel_vec_t& content () const;
    size_t Id() const;
    const result_index_channel_t& input() const;
    const std::shared_ptr<contractionLocator> & locator () const;
	const vector<float>& entropies () const;
	const vector<double>& leveled () const;
	const std::vector<std::vector<double>> ssMatrix () const;
	const medianLevelSet& leveler () const { return m_leveler; }
	const lengthFromMotion& lfm () const { return m_scale_space; }
	
	
private:
    bool run_selfsimilarity_on_region (const std::vector<roiWindow<P8U>>& images);
	bool run_scale_space (const std::vector<roiWindow<P8U>>& images);
	
    ssmt_result (const moving_region&,const result_index_channel_t& in);
    void signal_sm1d_ready (vector<float>&, const result_index_channel_t&);
    void contraction_ready (contractionLocator::contractionContainer_t& contractions, const result_index_channel_t&);
    bool get_channels (int channel) const ;
    result_index_channel_t m_input;
    
	vector<float> m_entropies, m_leveled;
    std::vector<std::vector<double>> m_smat;
    
    std::shared_ptr<contractionLocator> m_caRef;
	medianLevelSet m_leveler;
    
    mutable std::vector<cv::Mat> m_affine_windows;
    mutable std::vector<float> m_affine_translations;
    mutable std::vector<float> m_norm_affine_translations;
    std::vector<std::vector<float>> m_hz_profiles;
    std::vector<std::vector<float>> m_vt_profiles;
    
//    ssmt_processor::channel_images_t m_images;
    mutable ssmt_processor::channel_vec_t m_all_by_channel;
	
	mutable lengthFromMotion m_scale_space;
    
    uint64_t m_frameCount;
    mutable uint32_t m_channel_count;
    std::mutex m_mutex;
    std::weak_ptr<ssmt_processor> m_weak_parent;
	mutable std::atomic<bool> m_images_loaded;
	std::atomic<bool> m_pci_done;
	float m_magnification_x;

    
};



#endif
