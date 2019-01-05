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
#include "vision/opencv_utils.hpp"
#include "opencv2/video/tracking.hpp"
#include "getLuminanceAlgo.hpp"
using namespace cv;

// For base classing
class movSignaler : public base_signaler
{
    virtual std::string
    getName () const { return "movSignaler"; }
};

class mov_processor : public movSignaler
{
public:
    
    typedef void (mov_cb_content_loaded) ();
    typedef void (mov_cb_frame_loaded) (int&, double&);
    typedef void (mov_cb_sm1d_available) (int&);
    typedef void (mov_cb_sm1dmed_available) (int&,int&);
    typedef std::vector<roiWindow<P8U>> channel_images_t;
    
    mov_processor ()
    {
        signal_content_loaded = createSignal<mov_processor::mov_cb_content_loaded>();
        signal_frame_loaded = createSignal<mov_processor::mov_cb_frame_loaded>();
        signal_sm1d_available = createSignal<mov_processor::mov_cb_sm1d_available>();
        signal_sm1dmed_available = createSignal<mov_processor::mov_cb_sm1dmed_available>();
        m_sm = std::shared_ptr<sm_producer> ( new sm_producer () );
    }
    
  //  const smProducerRef sm () const { return m_sm; }
//    std::shared_ptr<sm_filter> smFilterRef () { return m_smfilterRef; }
    
    
    // Assumes RGB
    static void load_channels_from_images (const std::shared_ptr<seqFrameContainer>& frames,  std::vector<channel_images_t>& channels)
    {
        int64_t fn = 0;
        channels.clear();
        channels.resize (3);
        while (frames->checkFrame(fn))
        {
            auto su8 = frames->getFrame(fn++);
            auto m1 = svl::NewRedFromSurface(su8);
            channels[0].emplace_back(m1);
            auto m2 = svl::NewGreenFromSurface(su8);
            channels[1].emplace_back(m2);
            auto m3 = svl::NewBlueFromSurface(su8);
            channels[2].emplace_back(m3);
        }
    }
    
    
protected:
    boost::signals2::signal<mov_processor::mov_cb_content_loaded>* signal_content_loaded;
    boost::signals2::signal<mov_processor::mov_cb_frame_loaded>* signal_frame_loaded;
    boost::signals2::signal<mov_processor::mov_cb_sm1d_available>* signal_sm1d_available;
    boost::signals2::signal<mov_processor::mov_cb_sm1dmed_available>* signal_sm1dmed_available;
    
public:
    // Run to get Entropies and Median Level Set
    std::shared_ptr<vectorOfnamedTrackOfdouble_t>  run (const std::shared_ptr<seqFrameContainer>& frames, const std::vector<std::string>& names,
                                                bool test_data = false)
    {
        mov_processor::load_channels_from_images(frames, m_all_by_channel);
        create_named_tracks(names);
        compute_channel_statistics_threaded();
        return m_tracksRef;
    }
    
    // Update. Called also when cutoff offset has changed
    void update ()
    {

    }
    
private:
    
    
    
    
    // Note tracks contained timed data.
    void entropiesToTracks (namedTrackOfdouble_t& track)
    {
        track.second.clear();
        
    }
    
    size_t frame_count () const
    {
        if (m_all_by_channel[0].size() == m_all_by_channel[1].size() && m_all_by_channel[1].size() == m_all_by_channel[2].size())
            return m_all_by_channel[0].size();
        else return 0;
    }
    
    void create_named_tracks (const std::vector<std::string>& names)
    {
        m_tracksRef = std::make_shared<vectorOfnamedTrackOfdouble_t> ();
        
        m_tracksRef->resize (names.size ());
        for (auto tt = 0; tt < names.size(); tt++)
            m_tracksRef->at(tt).first = names[tt];
    }
    
    void compute_channel_statistics_threaded ()
    {
        std::vector<timed_double_vec_t> cts (3);
        std::vector<std::thread> threads(3);
        for (auto tt = 0; tt < 3; tt++)
        {
            threads[tt] = std::thread(IntensityStatisticsRunner(),
                                      std::ref(m_all_by_channel[tt]), std::ref(cts[tt]));
        }
        std::for_each(threads.begin(), threads.end(), std::mem_fn(&std::thread::join));
        
        for (auto tt = 0; tt < 3; tt++)
            m_tracksRef->at(tt).second = cts[tt];
    }
    
    
    
    void loadImagesToMats (const sm_producer::images_vector_t& images, std::vector<cv::Mat>& mts)
    {
        mts.resize(0);
        vector<roiWindow<P8U> >::const_iterator vitr = images.begin();
        do
        {
            mts.emplace_back(vitr->height(), vitr->width(), CV_8UC(1), vitr->pelPointer(0,0), size_t(vitr->rowUpdate()));
        }
        while (++vitr != images.end());
    }
    
    smProducerRef m_sm;
    channel_images_t m_images;
    std::vector<channel_images_t> m_all_by_channel;
    std::vector<cv::Mat> m_channel2_mats;
    
    Rectf m_all;
    std::deque<double> m_medianLevel;
    std::shared_ptr<vectorOfnamedTrackOfdouble_t>  m_tracksRef;
};


#endif
