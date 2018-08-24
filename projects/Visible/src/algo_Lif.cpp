//
//  algo_Lif.cpp
//  Visible
//
//  Created by Arman Garakani on 8/20/18.
//

#include <stdio.h>

#include <mutex>
#include <iostream>
#include <string>
#include <typeinfo>
#include <vector>
#include <deque>
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
#include "vision/opencv_utils.hpp"
#include "getLuminanceAlgo.hpp"
#include "contraction.hpp"

#include "algo_Lif.hpp"
lif_processor::lif_processor ()
{
    // Signals we provide
    signal_content_loaded = createSignal<lif_processor::sig_cb_content_loaded>();
    signal_flu_available = createSignal<lif_processor::sig_cb_flu_stats_available>();
    signal_frame_loaded = createSignal<lif_processor::sig_cb_frame_loaded>();
    signal_sm1d_available = createSignal<lif_processor::sig_cb_sm1d_available>();
    signal_sm1dmed_available = createSignal<lif_processor::sig_cb_sm1dmed_available>();
    signal_contraction_available = createSignal<lif_processor::sig_cb_contraction_available>();
    
    // Signals we support
    m_sm = std::shared_ptr<sm_producer> ( new sm_producer () );
    std::function<void ()> sm_content_loaded_cb = boost::bind (&lif_processor::sm_content_loaded, this);
    boost::signals2::connection ml_connection = m_sm->registerCallback(sm_content_loaded_cb);
    
    // Create a contraction object and register our call back for contraction available
    m_caRef = contraction_analyzer::create ();
    std::function<void (contractionContainer_t&)>ca_analyzed_cb = boost::bind (&lif_processor::contraction_analyzed, this, _1);
    boost::signals2::connection ca_connection = m_caRef->registerCallback(ca_analyzed_cb);
}

const smProducerRef lif_processor::sm () const { return m_sm; }

// Check if the returned has expired
std::weak_ptr<contraction_analyzer> lif_processor::contractionWeakRef ()
{
    std::weak_ptr<contraction_analyzer> wp (m_caRef);
    return wp;
}


int64_t lif_processor::load (const std::shared_ptr<qTimeFrameCache>& frames,const std::vector<std::string>& names)
{
    create_named_tracks(names);
    load_channels_from_images(frames);
    return m_frameCount;
}



void lif_processor::compute_oflow_threaded (timed_mat_vec_t& res)
{
    std::vector<std::thread> threads(1);
    threads[0] = std::thread(fbFlowRunner(),std::ref(m_all_by_channel[2]), std::ref(res));
    std::for_each(threads.begin(), threads.end(), std::mem_fn(&std::thread::join));
}


/*
 * 1 monchrome channel. Compute volume stats of each on a thread
 */

svl::stats<int64_t> lif_processor::run_volume_sum_sumsq_count (){
    int channel_to_use = m_channel_count - 1;
    std::vector<std::tuple<int64_t,int64_t,uint32_t>> cts;
    std::vector<std::tuple<uint8_t,uint8_t>> rts;
    std::vector<std::thread> threads(1);
    threads[0] = std::thread(IntensityStatisticsPartialRunner(),std::ref(m_all_by_channel[channel_to_use]), std::ref(cts), std::ref(rts));
    std::for_each(threads.begin(), threads.end(), std::mem_fn(&std::thread::join));
    auto res = std::accumulate(cts.begin(), cts.end(), std::make_tuple(int64_t(0),int64_t(0), uint32_t(0)), stl_utils::tuple_sum<int64_t,uint32_t>());
    auto mes = std::accumulate(rts.begin(), rts.end(), std::make_tuple(uint8_t(255),uint8_t(0)), stl_utils::tuple_minmax<uint8_t, uint8_t>());
    
    return svl::stats<int64_t> (std::get<0>(res), std::get<1>(res), std::get<2>(res), int64_t(std::get<0>(mes)), int64_t(std::get<1>(mes)));
}

/*
 * 2 flu channels. Compute stats of each using its own threaD
 */

std::shared_ptr<vectorOfnamedTrackOfdouble_t> lif_processor::run_flu_statistics ()
{
    auto channels = std::vector<int> {0};
    if (m_channel_count == 3) channels.push_back(1);
    
    std::vector<timed_double_vec_t> cts (channels.size());
    std::vector<std::thread> threads(channels.size());
    for (auto tt = 0; tt < channels.size(); tt++)
    {
        threads[tt] = std::thread(IntensityStatisticsRunner(),
                                  std::ref(m_all_by_channel[tt]), std::ref(cts[tt]));
    }
    std::for_each(threads.begin(), threads.end(), std::mem_fn(&std::thread::join));
    
    for (auto tt = 0; tt < channels.size(); tt++)
        m_tracksRef->at(tt).second = cts[tt];
    
    // Call the content loaded cb if any
    if (signal_flu_available && signal_flu_available->num_slots() > 0)
        signal_flu_available->operator()();
    
    return m_tracksRef;
}

// Run to get Entropies and Median Level Set
std::shared_ptr<vectorOfnamedTrackOfdouble_t>  lif_processor::run_pci ()
{
    int channel_to_use = m_channel_count - 1;
    channel_images_t c2 = m_all_by_channel[channel_to_use];
    auto sp =  sm();
    sp->load_images (c2);
    std::packaged_task<bool()> task([sp](){ return sp->operator()(0, 0);}); // wrap the function
    std::future<bool>  future_ss = task.get_future();  // get a future
    std::thread(std::move(task)).join(); // Finish on a thread
    if (future_ss.get())
    {
        m_entropies = sp->shannonProjection ();
        m_smat = sp->similarityMatrix();
        m_caRef->load(m_entropies,m_smat);
        update ();

        // Signal we are done with ACI
        static int dummy;
        if (signal_sm1d_available && signal_sm1d_available->num_slots() > 0)
            signal_sm1d_available->operator()(dummy);
    }
    return m_pci_tracksRef;
}



const std::vector<Rectf>& lif_processor::rois () const { return m_rois; }
const namedTrackOfdouble_t& lif_processor::similarity_track () const { return m_tracksRef->at(2); }
const std::shared_ptr<vectorOfnamedTrackOfdouble_t>& lif_processor::all_track () const { return m_tracksRef; }

// Update. Called also when cutoff offset has changed
void lif_processor::update ()
{
    if(m_pci_tracksRef && !m_pci_tracksRef->empty() && m_caRef && m_caRef->isValid())
        entropiesToTracks(m_pci_tracksRef->at(0));
}


void lif_processor::contraction_analyzed (contractionContainer_t& contractions)
{
//    std::cout << " Lif Processor " << contractions.size() << std::endl;
    // Call the contraction available cb if any
    if (signal_contraction_available && signal_contraction_available->num_slots() > 0)
    {
//        std::cout << "Slots : " << signal_contraction_available->num_slots() << std::endl;
        contractionContainer_t copied = m_caRef->contractions();
        signal_contraction_available->operator()(copied);
    }
}
void lif_processor::sm_content_loaded ()
{
    std::cout << "----------> sm content_loaded\n";
}
// Assumes LIF data -- use multiple window.
void lif_processor::load_channels_from_images (const std::shared_ptr<qTimeFrameCache>& frames)
{
    m_frameCount = 0;
    m_all_by_channel.clear();
    m_channel_count = frames->media_info().getNumChannels();
    m_all_by_channel.resize (m_channel_count);
    m_rois.resize (0);
    std::vector<std::string> names = {"Red", "Green","Blue"};
    
    while (frames->checkFrame(m_frameCount))
    {
        auto su8 = frames->getFrame(m_frameCount++);
        switch (m_channel_count)
        {
            case 3  :
            {
                auto m3 = svl::NewRefMultiFromSurface (su8, names, m_frameCount);
                for (auto cc = 0; cc < m3->planes(); cc++)
                    m_all_by_channel[cc].emplace_back(m3->plane(cc));
                
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
                break;
            }
            case 1  :
            {
                auto m1 = svl::NewRefSingleFromSurface (su8, names, m_frameCount);
                m_all_by_channel[0].emplace_back(*m1);
                const iRect& ir = m1->frame();
                m_rois.emplace_back(vec2(ir.ul().first, ir.ul().second), vec2(ir.lr().first, ir.lr().second));
                break;
            }
        }
        
    }
    
    // Call the content loaded cb if any
    if (signal_content_loaded && signal_content_loaded->num_slots() > 0)
        signal_content_loaded->operator()(m_frameCount);
    
}

// Note tracks contained timed data.
// Each call to find_best can be with different median cut-off
void lif_processor::entropiesToTracks (namedTrackOfdouble_t& track)
{
    std::weak_ptr<contraction_analyzer> weakCaPtr (m_caRef);
    if (weakCaPtr.expired())
        return;
    
    if (! m_caRef->find_best  ()) return;
    track.second.clear();
    auto mee = m_caRef->filtered().begin();
    for (auto ss = 0; mee != m_caRef->filtered().end() && ss < frame_count(); ss++, mee++)
    {
        index_time_t ti;
        ti.first = ss;
        timed_double_t res;
        track.second.emplace_back(ti,*mee);
    }
    
    // Signal we are done with median level set
    static int dummy;
    if (signal_sm1dmed_available && signal_sm1dmed_available->num_slots() > 0)
        signal_sm1dmed_available->operator()(dummy, dummy);
}

const int64_t lif_processor::frame_count () const
{
    if (m_all_by_channel[0].size() == m_all_by_channel[1].size() && m_all_by_channel[1].size() == m_all_by_channel[2].size() &&
        m_frameCount == m_all_by_channel[0].size())
        return m_frameCount;
    else return 0;
}

// @note Specific to ID Lab Lif Files
void lif_processor::create_named_tracks (const std::vector<std::string>& names)
{
    m_tracksRef = std::make_shared<vectorOfnamedTrackOfdouble_t> ();
    m_pci_tracksRef = std::make_shared<vectorOfnamedTrackOfdouble_t> ();
    
    m_tracksRef->resize (2);
    m_pci_tracksRef->resize (1);
    for (auto tt = 0; tt < names.size()-1; tt++)
        m_tracksRef->at(tt).first = names[tt];
    m_pci_tracksRef->at(0).first = names[2];
    
}



