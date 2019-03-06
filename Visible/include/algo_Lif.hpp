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
#include "async_tracks.h"
#include "core/signaler.h"
#include "vision/opencv_utils.hpp"
#include "algo_cardiac.hpp"
#include "sm_producer.h"
#include "contraction.hpp"
#include "vision/labelBlob.hpp"
#include <boost/foreach.hpp>
#include "seq_frame_container.hpp"

using namespace cv;
using namespace std;

#if 0
// Default logger factory-  creates synchronous loggers
struct synchronous_factory
{
    template<typename Sink, typename... SinkArgs>
    
    static std::shared_ptr<spdlog::logger> create(std::string logger_name, SinkArgs &&... args)
    {
        auto sink = std::make_shared<Sink>(std::forward<SinkArgs>(args)...);
        auto new_logger = std::make_shared<logger>(std::move(logger_name), std::move(sink));
        details::registry::instance().register_and_init(new_logger);
        return new_logger;
    }
};

using default_factory = synchronous_factory;

#endif

/*
 * if index is -1, data is not valid 
 */

class lif_serie_data
{
public:
    lif_serie_data ();
    lif_serie_data (const lifIO::LifReader::ref& m_lifRef, const unsigned index, const lifIO::ContentType_t& ct = "");
    int index () const { return m_index; }
    const std::string name () const { return m_name; }
   
    float seconds () const { return m_length_in_seconds; }
    uint32_t timesteps () const { return m_timesteps; }
    uint32_t pixelsInOneTimestep () const { return m_pixelsInOneTimestep; }
    uint32_t channelCount () const { return m_channelCount; }
    const std::vector<size_t>& dimensions () const { return m_dimensions; }
    const std::vector<lifIO::ChannelData>& channels () const { return m_channels; }
    const std::vector<std::string>& channel_names () const { return m_channel_names; }
    const std::vector<time_spec_t>& timeSpecs () const { return  m_timeSpecs; }
    const lifIO::LifReader::weak_ref& readerWeakRef () const;
    const cv::Mat& poster () const { return m_poster; }
    const std::string& custom_identifier () const { return m_contentType; }
 

private:
    mutable int32_t m_index;
    mutable std::string m_name;
    uint32_t    m_timesteps;
    uint32_t    m_pixelsInOneTimestep;
    uint32_t    m_channelCount;
    std::vector<size_t> m_dimensions;
    std::vector<lifIO::ChannelData> m_channels;
    std::vector<std::string> m_channel_names;
    std::vector<time_spec_t> m_timeSpecs;
    cv::Mat m_poster;
    mutable lifIO::LifReader::weak_ref m_lifWeakRef;
    
    mutable float  m_length_in_seconds;
    mutable std::string m_contentType; // "" denostes canonical LIF
    
    
    friend std::ostream& operator<< (std::ostream& out, const lif_serie_data& se)
    {
        out << "Serie:    " << se.name() << std::endl;
        out << "Channels: " << se.channelCount() << std::endl;
        out << "TimeSteps  " << se.timesteps() << std::endl;
        out << "Dimensions:" << se.dimensions()[0]  << " x " << se.dimensions()[1] << std::endl;
        out << "Time Length:" << se.seconds()    << std::endl;
        return out;
    }
    
    std::string info ()
    {
        std::ostringstream osr;
        osr << *this << std::endl;
        return osr.str();
    }
    
};



// For base classing
class LifBrowser : public base_signaler
{
    virtual std::string getName () const { return "LifBrowser"; }
};

class lif_browser : public LifBrowser
{
public:
    
    typedef std::shared_ptr<class lif_browser> ref;
    
    // @todo move ctor to private
    lif_browser(const std::string&  fqfn_path, const lifIO::ContentType_t& ct);
    
    static lif_browser::ref create (const std::string&  fqfn_path,
                                    const lifIO::ContentType_t& ct = ""){
        return std::shared_ptr<lif_browser> ( new lif_browser (fqfn_path, ct));
    }
    
    const lifIO::LifReader::ref& reader () const { return m_lifRef; }
    const lifIO::ContentType_t& content_type () const { return m_content_type; }
    
    const lif_serie_data get_serie_by_index (unsigned index);
    const std::vector<lif_serie_data>& get_all_series  () const;
    const std::string& path () const { return mFqfnPath; }
    const std::vector<std::string>& names () const;
    const std::map<std::string,int>& name_to_index_map () const { return m_name_to_index; }
    const std::map<int,std::string>& index_to_name_map () const { return m_index_to_name; }
    
private:
    void  get_series_info () const;
    void  internal_get_series_info () const;
    mutable lifIO::LifReader::ref m_lifRef;
    mutable std::vector<lif_serie_data> m_series_book;
    std::vector<cv::Mat> m_series_posters;
    mutable std::vector<std::string> m_series_names;
    mutable std::map<std::string,int> m_name_to_index;
    mutable std::map<int,std::string> m_index_to_name;
    mutable std::mutex m_mutex;
    mutable std::atomic<bool> m_data_ready;
    lifIO::ContentType_t m_content_type;
    std::string mFqfnPath;
    void get_first_frame (lif_serie_data& si,  const int frameCount, cv::Mat& out);
};


// For base classing
class LifSignaler : public base_signaler
{
    virtual std::string getName () const { return "LifSignaler"; }
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
     lif_serie_processor lp;
     lp.load(mFrameSet, names);
     std::string dir = "/Users/arman/tmp/c2";
     lp.save_channel_images(2,dir);
 
 }
 
 
 */

class lif_serie_processor : public LifSignaler
{
public:
    using contractionContainer_t = contraction_analyzer::contractionContainer_t;
    
    typedef std::unordered_map<uint8_t,std::vector<Point2f>> mapU8PeaksToPointfs_t;
    typedef void (sig_cb_content_loaded) (int64_t&);
    typedef void (sig_cb_flu_stats_available) ();
    typedef void (sig_cb_contraction_available) (contractionContainer_t &);
    typedef void (sig_cb_frame_loaded) (int&, double&);
    typedef void (sig_cb_sm1d_available) (int&);
    typedef void (sig_cb_sm1dmed_available) (int&,int&);
    typedef void (sig_cb_3dstats_available) ();
    typedef void (sig_cb_volume_var_available) ();
    typedef std::vector<roiWindow<P8U>> channel_images_t;
    typedef std::vector<channel_images_t> channel_vec_t;
    
    lif_serie_processor ();
    
    const smProducerRef sm () const;
    const int64_t& frame_count () const;
    const int64_t channel_count () const;
    const svl::stats<int64_t> stats3D () const;
    const cv::Mat & volume_variances () const { return m_var_image;  }
    cv::Mat & display_volume_variances () const { return m_var_display_image;  }
    
    // Check if the returned has expired
    std::weak_ptr<contraction_analyzer> contractionWeakRef ();
    
    // Load frames from cache
    int64_t load (const std::shared_ptr<seqFrameContainer>& frames,const std::vector<std::string>& names);
    
    // Run Luminance info on a vector of channel indices
    async_vecOfNamedTrack_t get_luminance_info (const std::vector<int>& channels);
    std::shared_ptr<vecOfNamedTrack_t> run_flu_statistics (const std::vector<int>& channels);
    
    // Channel Index API for IDLab custom organization
    // Run accumulator of 3d stats on a channel at index
    svl::stats<int64_t> run_volume_sum_sumsq_count (const int channel_index);
    // Run per pixel stdev accumulator a channel at index
    void run_volume_variances (const int channel_index);
    // Run to get Entropies and Median Level Set
    std::shared_ptr<vecOfNamedTrack_t> run_pci_on_channel (const int channel_index);
    
    // Vector of 8bit roiWindows API for IDLab custom organization
    svl::stats<int64_t> run_volume_sum_sumsq_count (std::vector<roiWindow<P8U>>&);
    void run_volume_variances (std::vector<roiWindow<P8U>>& );
    std::shared_ptr<vecOfNamedTrack_t> run_pci (const std::vector<roiWindow<P8U>>&);

    // Return 2D latice of pixels over time
    void generateVoxels_on_channel (const int channel_index, std::vector<std::vector<roiWindow<P8U>>>&);
    void generateVoxels (const std::vector<roiWindow<P8U>>&, std::vector<std::vector<roiWindow<P8U>>>&);
    
    // Return 2D latice of voxel self-similarity
    void generateVoxelSelfSimilarities (std::vector<std::vector<roiWindow<P8U>>>&,
                                         std::vector<std::vector<float>>&);
    void generateVoxelSelfSimilarities_on_channel (const int channel_index, std::vector<std::vector<float>>&);
    cv::Mat& temporal_selfSimilarity () const { return m_temporal_ss; }
    
    const std::vector<Rectf>& rois () const;
    const cv::RotatedRect& motion_surface () const;
    const  std::deque<double>& medianSet () const;
    
    // Update. Called also when cutoff offset has changed
    void update ();
    
    std::shared_ptr<OCVImageWriter>& get_image_writer ();
    void save_channel_images (int channel_index, std::string& dir_fqfn);
    
protected:
    boost::signals2::signal<lif_serie_processor::sig_cb_content_loaded>* signal_content_loaded;
    boost::signals2::signal<lif_serie_processor::sig_cb_flu_stats_available>* signal_flu_available;
    boost::signals2::signal<lif_serie_processor::sig_cb_frame_loaded>* signal_frame_loaded;
    boost::signals2::signal<lif_serie_processor::sig_cb_sm1d_available>* signal_sm1d_available;
    boost::signals2::signal<lif_serie_processor::sig_cb_sm1dmed_available>* signal_sm1dmed_available;
    boost::signals2::signal<lif_serie_processor::sig_cb_contraction_available>* signal_contraction_available;
    boost::signals2::signal<lif_serie_processor::sig_cb_3dstats_available>* signal_3dstats_available;
    boost::signals2::signal<lif_serie_processor::sig_cb_volume_var_available>* signal_volume_var_available;
    
private:
    
    void contraction_analyzed (contractionContainer_t&);
    void stats_3d_computed ();
    void pci_done ();
    void sm_content_loaded ();
    
    // Assumes LIF data -- use multiple window.
    void load_channels_from_images (const std::shared_ptr<seqFrameContainer>& frames);
    
    
    // Note tracks contained timed data.
    void entropiesToTracks (namedTrack_t& track);
    
    // @note Specific to ID Lab Lif Files
    void create_named_tracks (const std::vector<std::string>& names);
    
    labelBlob::ref get_blobCachedObject (const index_time_t& ti);
    
    mutable std::mutex m_mutex;
    uint32_t m_channel_count;
    
    // One similarity engine
    mutable smProducerRef m_sm;
    vector<double> m_entropies;
    vector<vector<double>> m_smat;
    
    // Median Levels
    std::vector<double> m_medianLevel;
    
    // Std Dev Image
    cv::Mat m_var_image;
    mutable cv::Mat m_var_display_image;
    mutable mapU8PeaksToPointfs_t m_map_peaks_to_points;
    
    channel_images_t m_images;
    channel_vec_t m_all_by_channel;
    
    int64_t m_frameCount;
    std::vector<Rectf> m_rois;
    Rectf m_all;
    
    // @note Specific to ID Lab Lif Files
    // 3 channels: 2 flu one visible
    // 1 channel: visible
    // Note create_named_tracks
    std::shared_ptr<vecOfNamedTrack_t>  m_tracksRef;
    std::shared_ptr<vecOfNamedTrack_t>  m_pci_tracksRef;
    std::shared_ptr<contraction_analyzer> m_caRef;
    
    mutable svl::stats<int64_t> m_3d_stats;
    std::atomic<bool> m_3d_stats_done;
    cv::RotatedRect m_motion_mass;
    mutable cv::Mat m_temporal_ss;
    mutable std::vector<std::vector<float>> m_temporal_ss_raw;
    
    std::map<index_time_t, labelBlob::weak_ref> m_blob_cache;
    
    std::shared_ptr<OCVImageWriter> m_writer;
    
};




    
#endif
