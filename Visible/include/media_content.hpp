#ifndef __MEDIA_CONTENT__
#define __MEDIA_CONTENT__


#include <iostream>
#include <string>
#include <typeinfo>
#include <vector>
#include <deque>
#include <sstream>
#include <typeindex>
#include <map>
#include "timed_value_containers.h"
#include "core/signaler.h"
#include "vision/opencv_utils.hpp"
#include "sm_producer.h"
#include "vision/labelBlob.hpp"
#include <boost/foreach.hpp>
#include "seq_frame_container.hpp"
#include <boost/filesystem.hpp>
#include <boost/assign/list_of.hpp> // for 'map_list_of()'
#include <boost/assert.hpp>
#include <map>
#include <Eigen/Dense>
#include "segmentation_parameters.hpp"
#include "iowriter.hpp"

using namespace Eigen;

using namespace boost::assign; // bring 'map_list_of()' into scope
using namespace cv;
using namespace std;
using namespace boost;
using namespace boost::filesystem;
namespace fs = boost::filesystem;




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

  typedef std::pair<vec2,vec2> sides_length_t;

/*
 * if index is -1, data is not valid 
 */

class media_data
{
public:
    media_data ();
    media_data (const MediaIO::MediaReader::ref& m_MediaRef, const unsigned index, const MediaIO::ContentType_t& ct = "");
    int index () const { return m_index; }
    const std::string name () const { return m_name; }
   
    float seconds () const { return m_length_in_seconds; }
    uint32_t timesteps () const { return m_timesteps; }
    uint32_t pixelsInOneTimestep () const { return m_pixelsInOneTimestep; }
    uint32_t channelCount () const { return m_channelCount; }
    const std::vector<size_t>& dimensions () const { return m_dimensions; }
    const std::vector<MediaIO::ChannelData>& channels () const { return m_channels; }
    const std::vector<std::string>& channel_names () const { return m_channel_names; }
    const std::vector<time_spec_t>& timeSpecs () const { return  m_timeSpecs; }
    const MediaIO::MediaReader::weak_ref& readerWeakRef () const;
    const cv::Mat& poster () const { return m_poster; }
    const std::string& custom_identifier () const { return m_contentType; }
 

private:
    mutable int32_t m_index;
    mutable std::string m_name;
    uint32_t    m_timesteps;
    uint32_t    m_pixelsInOneTimestep;
    uint32_t    m_channelCount;
    std::vector<size_t> m_dimensions;
    std::vector<MediaIO::ChannelData> m_channels;
    std::vector<std::string> m_channel_names;
    std::vector<time_spec_t> m_timeSpecs;
    cv::Mat m_poster;
    mutable MediaIO::MediaReader::weak_ref m_MediaWeakRef;
    
    mutable float  m_length_in_seconds;
    mutable std::string m_contentType; // "" denostes canonical Media
    
    
    friend std::ostream& operator<< (std::ostream& out, const media_data& se)
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
class MediaBrowser : public base_signaler
{
    virtual std::string getName () const { return "MediaBrowser"; }
};

class Media_browser : public MediaBrowser
{
public:
    
    typedef std::shared_ptr<class Media_browser> ref;
    
    // @todo move ctor to private
    Media_browser(const std::string&  fqfn_path, const MediaIO::ContentType_t& ct);
    
    static Media_browser::ref create (const std::string&  fqfn_path,
                                    const MediaIO::ContentType_t& ct = ""){
        return std::shared_ptr<Media_browser> ( new Media_browser (fqfn_path, ct));
    }
    
    const MediaIO::MediaReader::ref& reader () const { return m_MediaRef; }
    const MediaIO::ContentType_t& content_type () const { return m_content_type; }
    
    const media_data get_serie_by_index (unsigned index);
    const std::vector<media_data>& get_all_series  () const;
    const std::string& path () const { return mFqfnPath; }
    const std::vector<std::string>& names () const;
    const std::map<std::string,int>& name_to_index_map () const { return m_name_to_index; }
    const std::map<int,std::string>& index_to_name_map () const { return m_index_to_name; }
    
private:
    void  get_series_info () const;
    void  internal_get_series_info () const;
    mutable MediaIO::MediaReader::ref m_MediaRef;
    mutable std::vector<media_data> m_series_book;
    std::vector<cv::Mat> m_series_posters;
    mutable std::vector<std::string> m_series_names;
    mutable std::map<std::string,int> m_name_to_index;
    mutable std::map<int,std::string> m_index_to_name;
    mutable std::mutex m_mutex;
    mutable std::atomic<bool> m_data_ready;
    MediaIO::ContentType_t m_content_type;
    std::string mFqfnPath;
    void get_first_frame (media_data& si,  const int frameCount, cv::Mat& out);
};




    
#endif
