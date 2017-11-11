#ifndef __ALGO_LIF__
#define __ALGO_LIF__


#include <map>
#include <iostream>
#include <string>
#include <typeinfo>
#include <vector>
#include <sstream>
#include <typeindex>
#include <map>
#include <future>
#include "core/singleton.hpp"
#include "async_producer.h"
#include "signaler.h"
#include "sm_producer.h"
#include "cinder_xchg.hpp"
#include "vision/histo.h"

class vSignaler : public base_signaler
{
    virtual std::string
    getName () const { return "vSignaler"; }
};


struct OpticalFlowFarnebackRunner
{
    typedef std::vector<roiWindow<P8U>> channel_images_t;
    void operator()(channel_images_t& channel, timed_mat_vec_t& results)
    {
        results.clear();
        for (auto ii = 0; ii < channel.size(); ii++)
        {
            timed_mat_t res;
            index_time_t ti;
            ti.first = ii;
            res.first = ti;
            res.second = histoStats::mean(channel[ii]) / 256.0;
            results.emplace_back(res);
        }
    }
};

struct IntensityStatisticsRunner
{
    typedef std::vector<roiWindow<P8U>> channel_images_t;
    void operator()(channel_images_t& channel, timed_double_vec_t& results)
    {
        results.clear();
        for (auto ii = 0; ii < channel.size(); ii++)
        {
            timed_double_t res;
            index_time_t ti;
            ti.first = ii;
            res.first = ti;
            res.second = histoStats::mean(channel[ii]) / 256.0;
            results.emplace_back(res);
        }
    }
};

class lif_processor : public vSignaler
{
public:
    
    typedef void (sig_cb_content_loaded) ();
    typedef void (sig_cb_frame_loaded) (int&, double&);
    typedef void (sig_cb_sm1d_available) (int&);
    typedef void (sig_cb_sm1dmed_available) (int&,int&);
    typedef std::vector<roiWindow<P8U>> channel_images_t;
    
    lif_processor ()
    {
        signal_content_loaded = createSignal<lif_processor::sig_cb_content_loaded>();
        signal_frame_loaded = createSignal<lif_processor::sig_cb_frame_loaded>();
        signal_sm1d_available = createSignal<lif_processor::sig_cb_sm1d_available>();
        signal_sm1dmed_available = createSignal<lif_processor::sig_cb_sm1dmed_available>();
        m_sm = std::shared_ptr<sm_producer> ( new sm_producer () );
        
    }
    
    const smProducerRef sm () const { return m_sm; }
    std::shared_ptr<sm_filter> smFilterRef () { return m_smfilterRef; }
    
protected:
    boost::signals2::signal<lif_processor::sig_cb_content_loaded>* signal_content_loaded;
    boost::signals2::signal<lif_processor::sig_cb_frame_loaded>* signal_frame_loaded;
    boost::signals2::signal<lif_processor::sig_cb_sm1d_available>* signal_sm1d_available;
    boost::signals2::signal<lif_processor::sig_cb_sm1dmed_available>* signal_sm1dmed_available;
    
private:
    
    // Assumes LIF data -- use multiple window.
    void load_channels_from_images (const std::shared_ptr<qTimeFrameCache>& frames)
    {
        int64_t fn = 0;
        m_channel_images.clear();
        m_channel_images.resize (3);
        m_rois.resize (0);
        std::vector<std::string> names = {"Red", "Green","Blue"};
        
        while (frames->checkFrame(fn))
        {
            auto su8 = frames->getFrame(fn++);
            auto m3 = svl::NewRefMultiFromSurface (su8, names, fn);
            for (auto cc = 0; cc < m3->planes(); cc++)
                m_channel_images[cc].emplace_back(m3->plane(cc));
            
            // Assumption: all have the same 3 channel concatenated structure
            // Fetch it only once
            if (m_rois.empty())
            {
                for (auto cc = 0; cc < m3->planes(); cc++)
                {
                    const iRect& ir = m3->roi(cc);
                    m_rois.emplace_back(vec2(ir.ul().first, ir.ul().second), vec2(ir.lr().first, ir.lr().second));
                }
            }
        }
    }
    
 
    
    // Note tracks contained timed data.
    void entropiesToTracks (namedTrackOfdouble_t& track)
    {
        track.second.clear();
        if (m_smfilterRef->median_levelset_similarities())
        {
            // Signal we are done with median level set
            static int dummy;
            if (signal_sm1dmed_available && signal_sm1dmed_available->num_slots() > 0)
                signal_sm1dmed_available->operator()(dummy, dummy);
        }

        auto bee = m_smfilterRef->entropies().begin();
        auto mee = m_smfilterRef->median_adjusted().begin();
        for (auto ss = 0; bee != m_smfilterRef->entropies().end() && ss < frame_count(); ss++, bee++, mee++)
        {
            index_time_t ti;
            ti.first = ss;
            timed_double_t res;
            res.first = ti;
            res.second = *mee;
            track.second.emplace_back(res);
        }
    }
    
    size_t frame_count () const
    {
        if (m_channel_images[0].size() == m_channel_images[1].size() && m_channel_images[1].size() == m_channel_images[2].size())
            return m_channel_images[0].size();
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
        std::vector<timed_double_vec_t> cts (2);
        std::vector<std::thread> threads(2);
        for (auto tt = 0; tt < 2; tt++)
        {
            threads[tt] = std::thread(IntensityStatisticsRunner(),
                                      std::ref(m_channel_images[tt]), std::ref(cts[tt]));
        }
        std::for_each(threads.begin(), threads.end(), std::mem_fn(&std::thread::join));

        for (auto tt = 0; tt < 2; tt++)
            m_tracksRef->at(tt).second = cts[tt];
    }
    
    void compute_sm_threaded ()
    {
        
    }
public:
    // Run to get Entropies and Median Level Set
    std::shared_ptr<vectorOfnamedTrackOfdouble_t>  run (const std::shared_ptr<qTimeFrameCache>& frames, const std::vector<std::string>& names,
                                                bool test_data = false)
    {
        
        load_channels_from_images(frames);
        create_named_tracks(names);

        compute_channel_statistics_threaded();
        
        channel_images_t c2 = m_channel_images[2];
        auto sp =  sm();
        sp->load_images (c2);

        // Call the content loaded cb if any
        if (signal_content_loaded && signal_content_loaded->num_slots() > 0)
            signal_content_loaded->operator()();
        
        std::packaged_task<bool()> task([sp](){ return sp->operator()(0, 0);}); // wrap the function
        std::future<bool>  future_ss = task.get_future();  // get a future
        std::thread(std::move(task)).detach(); // launch on a thread
        if (future_ss.get())
        {
            // Signal we are done with ACI
            static int dummy;
            if (signal_sm1d_available && signal_sm1d_available->num_slots() > 0)
                signal_sm1d_available->operator()(dummy);
            
            const deque<double>& entropies = sp->shannonProjection ();
            const deque<deque<double>>& smat = sp->similarityMatrix();
            m_smfilterRef = std::make_shared<sm_filter> (entropies, smat);

       
            update ();
        }
        return m_tracksRef;
    }
    
    const std::vector<Rectf>& rois () const { return m_rois; }
    const namedTrackOfdouble_t& similarity_track () const { return m_tracksRef->at(2); }
    
    // Update. Called also when cutoff offset has changed
    void update ()
    {
        if(m_tracksRef && !m_tracksRef->empty() && m_smfilterRef && m_smfilterRef->isValid())
            entropiesToTracks(m_tracksRef->at(2));
    }
    
private:

    smProducerRef m_sm;
    channel_images_t m_images;
    std::vector<channel_images_t> m_channel_images;
    std::vector<Rectf> m_rois;
    Rectf m_all;
    std::shared_ptr<sm_filter> m_smfilterRef;
    std::deque<double> m_medianLevel;
    std::shared_ptr<vectorOfnamedTrackOfdouble_t>  m_tracksRef;
};

template boost::signals2::connection lif_processor::registerCallback(const std::function<lif_processor::sig_cb_content_loaded>&);
template boost::signals2::connection lif_processor::registerCallback(const std::function<lif_processor::sig_cb_frame_loaded>&);
template boost::signals2::connection lif_processor::registerCallback(const std::function<lif_processor::sig_cb_sm1d_available>&);
template boost::signals2::connection lif_processor::registerCallback(const std::function<lif_processor::sig_cb_sm1dmed_available>&);

#endif
