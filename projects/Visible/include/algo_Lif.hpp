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

class vSignaler : public base_signaler
{
    virtual std::string
    getName () const { return "vSignaler"; }
};

class algo_processor : public vSignaler, public internal_singleton <algo_processor>
{
public:
    
    typedef void (sig_cb_content_loaded) ();
    typedef void (sig_cb_frame_loaded) (int, double);
    typedef void (sig_cb_sm1d_available) ();
    typedef void (sig_cb_sm1dmed_available) ();
    
    algo_processor ()
    {
        m_sm = std::shared_ptr<sm_producer> ( new sm_producer () );
        
    }
    
    const smProducerRef sm () const { return m_sm; }
    std::shared_ptr<sm_filter> smFilterRef () { return m_smfilterRef; }
    
protected:
    boost::signals2::signal<algo_processor::sig_cb_content_loaded>* signal_content_loaded;
    boost::signals2::signal<algo_processor::sig_cb_frame_loaded>* signal_frame_loaded;
    boost::signals2::signal<algo_processor::sig_cb_sm1d_available>* signal_sm1d_available;
    boost::signals2::signal<algo_processor::sig_cb_sm1dmed_available>* signal_sm1dmed_available;
    
private:
    
    
    bool load_channels_from_images (const std::shared_ptr<qTimeFrameCache>& frames, bool LIF_data = true,
                                    uint8_t channel = 2)
    {
        // If it LIF data We will use multiple window.
        int64_t fn = 0;
        m_channel_images.clear();
        m_channel_images.resize (3);
        m_rois.resize (0);
        
        std::vector<std::string> names = {"Red", "Green","Blue"};
        
        while (frames->checkFrame(fn))
        {
            auto su8 = frames->getFrame(fn++);
            if (LIF_data)
            {
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
            else
            {
                // assuming 3 channels
                for (auto cc = 0; cc < 3; cc++)
                {
                    auto m1 = svl::NewChannelFromSurfaceAtIndex(su8,cc);
                    m_channel_images[cc].emplace_back(m1);
                }
            }
        }
        
 
        
    }
    
    timed_double_t computeIntensityStatisticsResults (const roiWindow<P8U>& roi)
    {
        index_time_t ti;
        timed_double_t res;
        res.first = ti;
        res.second = histoStats::mean(roi) / 256.0;
        return res;
    }
    
    // Note tracks contained timed data.
    void entropiesToTracks (trackD1_t& track)
    {
        track.second.clear();
        m_smfilterRef->median_levelset_similarities();
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
    
public:
    std::shared_ptr<vector_of_trackD1s_t>  run (const std::shared_ptr<qTimeFrameCache>& frames, const std::vector<std::string>& names,
                                                bool test_data = false)
    {
        
        load_channels_from_images(frames);
        
        m_tracksRef = std::make_shared<vector_of_trackD1s_t> ();
        
        m_tracksRef->resize (names.size ());
        for (auto tt = 0; tt < names.size(); tt++)
            m_tracksRef->at(tt).first = names[tt];
        
        // Run Histogram on channels 0 and 1
        // Filling up tracks 0 and 1
        // Now Do Aci on the 3rd channel
        channel_images_t c0 = m_channel_images[0];
        channel_images_t c1 = m_channel_images[1];
        channel_images_t c2 = m_channel_images[2];
        
        for (auto ii = 0; ii < m_channel_images[0].size(); ii++)
        {
            m_tracksRef->at(0).second.emplace_back(computeIntensityStatisticsResults(c0[ii]));
            m_tracksRef->at(1).second.emplace_back(computeIntensityStatisticsResults(c1[ii]));
        }
        
        auto sp =  instance().sm();
        sp->load_images (c2);
        std::packaged_task<bool()> task([sp](){ return sp->operator()(0, 0);}); // wrap the function
        std::future<bool>  future_ss = task.get_future();  // get a future
        std::thread(std::move(task)).detach(); // launch on a thread
        if (future_ss.get())
        {
            const deque<double>& entropies = sp->shannonProjection ();
            const deque<deque<double>>& smat = sp->similarityMatrix();
            m_smfilterRef = std::make_shared<sm_filter> (entropies, smat);
            update ();
        }
        return m_tracksRef;
    }
    
    const std::vector<Rectf>& rois () const { return m_rois; }
    const trackD1_t& similarity_track () const { return m_tracksRef->at(2); }
    
    void update ()
    {
        if(m_tracksRef && !m_tracksRef->empty() && m_smfilterRef && m_smfilterRef->isValid())
            entropiesToTracks(m_tracksRef->at(2));
        // Call the content loaded cb if any
        if (signal_content_loaded && signal_content_loaded->num_slots() > 0)
            signal_content_loaded->operator()();
    }
    
private:
    typedef std::vector<roiWindow<P8U>> channel_images_t;
    smProducerRef m_sm;
    channel_images_t m_images;
    std::vector<channel_images_t> m_channel_images;
    std::vector<Rectf> m_rois;
    Rectf m_all;
    std::shared_ptr<sm_filter> m_smfilterRef;
    std::deque<double> m_medianLevel;
    std::shared_ptr<vector_of_trackD1s_t>  m_tracksRef;
};



#endif
