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
#include "logger.hpp"
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

class internal_serie_info
{
public:
    uint32_t index;
    std::string name;
    uint32_t    timesteps;
    uint32_t    pixelsInOneTimestep;
    uint32_t    channelCount;
    std::vector<size_t> dimensions;
    std::vector<lifIO::ChannelData> channels;
    std::vector<std::string> channel_names;
    std::vector<time_spec_t> timeSpecs;
    cv::Mat poster;

    
    float  length_in_seconds;
    
    
    friend std::ostream& operator<< (std::ostream& out, const internal_serie_info& se)
    {
        out << "Serie:    " << se.name << std::endl;
        out << "Channels: " << se.channelCount << std::endl;
        out << "TimeSteps  " << se.timesteps << std::endl;
        out << "Dimensions:" << se.dimensions[0]  << " x " << se.dimensions[1] << std::endl;
        out << "Time Length:" << se.length_in_seconds    << std::endl;
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
    lif_browser(const boost::filesystem::path&  fqfn_path);
    
    static lif_browser::ref create (const boost::filesystem::path&  fqfn_path){
        return std::shared_ptr<lif_browser> ( new lif_browser (fqfn_path));
    }
    
    const lifIO::LifReader::ref& reader () const { return m_lifRef; }
    const std::vector<internal_serie_info>& internal_serie_infos () const { return m_series_book; }
    const boost::filesystem::path& path () const { return mPath; }
    const std::vector<std::string>& names () const { return m_series_names; }
    
private:
    std::shared_ptr<lifIO::LifReader> m_lifRef;
    std::vector<internal_serie_info> m_series_book;
    std::vector<cv::Mat> m_series_posters;
    std::vector<std::string> m_series_names;
    boost::filesystem::path mPath;
    void get_first_frame (internal_serie_info& si,  const int frameCount, cv::Mat& out);
    void  get_series_info (const std::shared_ptr<lifIO::LifReader>& lifer);
    
};


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
    typedef void (sig_cb_channelmats_available) (int& );
    typedef std::vector<roiWindow<P8U>> channel_images_t;
    typedef std::vector<channel_images_t> channel_vec_t;
    typedef std::array<std::vector<cv::Mat>,4> channelMats_t;
    
    lif_processor ();
    
    const smProducerRef sm () const;
    const int64_t& frame_count () const;
    const int64_t channel_count () const;
    const svl::stats<int64_t> stats3D () const;
    const channelMats_t & channelMats () const { return m_channel_mats; }
    // Check if the returned has expired
    std::weak_ptr<contraction_analyzer> contractionWeakRef ();
    
    // Load frames from cache
    int64_t load (const std::shared_ptr<seqFrameContainer>& frames,const std::vector<std::string>& names);
    
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
    void loadImagesToMats (const int channel_index);
    
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
    boost::signals2::signal<lif_processor::sig_cb_channelmats_available>* signal_channelmats_available;
    
private:
    
    void contraction_analyzed (contractionContainer_t&);
    void stats_3d_computed ();
    void channelmats_available (int&);
    void pci_done ();
    void sm_content_loaded ();
    
    // Assumes LIF data -- use multiple window.
    void load_channels_from_images (const std::shared_ptr<seqFrameContainer>& frames);
    
    
    // Note tracks contained timed data.
    void entropiesToTracks (namedTrackOfdouble_t& track);
    
    // @note Specific to ID Lab Lif Files
    void create_named_tracks (const std::vector<std::string>& names);
    
    labelBlob::ref get_blobCachedObject (const index_time_t& ti);
    
    mutable std::mutex m_mutex;
    uint32_t m_channel_count;
    
    // One similarity engine
    mutable smProducerRef m_sm;
    vector<double> m_entropies;
    vector<vector<double>> m_smat;
    
    // Channels by channel as cv::Mats
    mutable std::array<std::vector<cv::Mat>,4> m_channel_mats;
    std::vector<double> m_medianLevel;
    
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
