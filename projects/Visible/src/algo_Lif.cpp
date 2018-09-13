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
#include "core/signaler.h"
#include "sm_producer.h"
#include "cinder_xchg.hpp"
#include "vision/histo.h"
#include "vision/opencv_utils.hpp"
#include "getLuminanceAlgo.hpp"
#include "contraction.hpp"
#include "vision/localvariance.h"
#include "algo_Lif.hpp"
#include "logger.hpp"

lif_browser::lif_browser(const boost::filesystem::path&  fqfn_path) : mPath(fqfn_path){
    
    if ( ! mPath.empty () )
    {
        std::string msg = mPath.string() + " Loaded ";
        vlogger::instance().console()->info(msg);
        
        try {
            
            m_lifRef =  std::shared_ptr<lifIO::LifReader> (new lifIO::LifReader (mPath.string()));
            get_series_info (m_lifRef);
            m_series_posters.clear ();
            BOOST_FOREACH(serie_info& si, m_series_book){
                cv::Mat mat;
                get_first_frame(si,0, mat);
                si.poster = mat.clone();
            }
            auto msg = tostr(m_series_book.size()) + "  Series  ";
            vlogger::instance().console()->info(msg);
        }
        catch( ... ) {
            vlogger::instance().console()->debug("Unable to load LIF file");
            return;
        }
        
    }
}

/*
 * LIF files are plane organized. 3 Channel LIF file is 3 * rows by cols by ONE byte. 
 */

void lif_browser::get_first_frame (serie_info& si,  const int frameCount, cv::Mat& out)
{
    auto serie_ref = std::shared_ptr<lifIO::LifSerie>(&m_lifRef->getSerie(si.index), stl_utils::null_deleter());
    // opencv rows, cols
    cv::Mat dst (si.dimensions[1] * si.channelCount , si.dimensions[0], CV_8U);
    serie_ref->fill2DBuffer(dst.ptr(0), 0);
    out = dst;
}

void  lif_browser::get_series_info (const std::shared_ptr<lifIO::LifReader>& lifer)
{
    m_series_book.clear ();
    for (unsigned ss = 0; ss < lifer->getNbSeries(); ss++)
    {
        serie_info si;
        
        si.index = ss;
        si.name = lifer->getSerie(ss).getName();
        si.timesteps = lifer->getSerie(ss).getNbTimeSteps();
        si.pixelsInOneTimestep = lifer->getSerie(ss).getNbPixelsInOneTimeStep();
        si.dimensions = lifer->getSerie(ss).getSpatialDimensions();
        si.channelCount = lifer->getSerie(ss).getChannels().size();
        si.channels.clear ();
        for (lifIO::ChannelData cda : lifer->getSerie(ss).getChannels())
        {
            si.channel_names.push_back(cda.getName());
            si.channels.emplace_back(cda);
        }
        
        // Get timestamps in to time_spec_t and store it in info
        si.timeSpecs.resize (lifer->getSerie(ss).getTimestamps().size());
        
        // Adjust sizes based on the number of bytes
        std::transform(lifer->getSerie(ss).getTimestamps().begin(), lifer->getSerie(ss).getTimestamps().end(),
                       si.timeSpecs.begin(), [](lifIO::LifSerie::timestamp_t ts) { return time_spec_t ( ts / 10000.0); });
        
        si.length_in_seconds = lifer->getSerie(ss).total_duration ();
        
        m_series_book.emplace_back (si);
    }
}

lif_processor::lif_processor ()
{
    // Signals we provide
    signal_content_loaded = createSignal<lif_processor::sig_cb_content_loaded>();
    signal_flu_available = createSignal<lif_processor::sig_cb_flu_stats_available>();
    signal_frame_loaded = createSignal<lif_processor::sig_cb_frame_loaded>();
    signal_sm1d_available = createSignal<lif_processor::sig_cb_sm1d_available>();
    signal_sm1dmed_available = createSignal<lif_processor::sig_cb_sm1dmed_available>();
    signal_contraction_available = createSignal<lif_processor::sig_cb_contraction_available>();
    signal_3dstats_available = createSignal<lif_processor::sig_cb_3dstats_available>();
    signal_channelmats_available = createSignal<lif_processor::sig_cb_channelmats_available>();
    
    // semilarity producer
    m_sm = std::shared_ptr<sm_producer> ( new sm_producer () );
    
    // Signals we support
    // support Similarity::Content Loaded
    std::function<void ()> sm_content_loaded_cb = boost::bind (&lif_processor::sm_content_loaded, this);
    boost::signals2::connection ml_connection = m_sm->registerCallback(sm_content_loaded_cb);
    
    // Create a contraction object
    m_caRef = contraction_analyzer::create ();
    
    // Suport lif_processor::Contraction Analyzed
    std::function<void (contractionContainer_t&)>ca_analyzed_cb = boost::bind (&lif_processor::contraction_analyzed, this, _1);
    boost::signals2::connection ca_connection = m_caRef->registerCallback(ca_analyzed_cb);
    
    // Support channel mats available
    std::function<void (int&)>mats_available_cb = boost::bind(&lif_processor::channelmats_available, this, _1);
    boost::signals2::connection mat_connection = registerCallback(mats_available_cb);
    
    // Signal us when we have pci mat channels are ready to run contraction analysis
    //std::function<void (int&)> sm1dmed_available_cb = boost::bind (&lif_processor::pci_done, this);
    //boost::signals2::connection nl_connection = registerCallback(sm1dmed_available_cb);
    
    // Signal us when 3d stats are ready
    std::function<void ()>_3dstats_done_cb = boost::bind (&lif_processor::stats_3d_computed, this);
    boost::signals2::connection _3d_stats_connection = registerCallback(_3dstats_done_cb);
    
}

// @note Specific to ID Lab Lif Files
// @note Specific to ID Lab Lif Files
// 3 channels: 2 flu one visible
// 1 channel: visible
void lif_processor::create_named_tracks (const std::vector<std::string>& names)
{
    m_tracksRef = std::make_shared<vectorOfnamedTrackOfdouble_t> ();
    m_pci_tracksRef = std::make_shared<vectorOfnamedTrackOfdouble_t> ();
    
    switch(names.size()){
        case 3:
            m_tracksRef->resize (2);
            m_pci_tracksRef->resize (1);
            for (auto tt = 0; tt < names.size()-1; tt++)
                m_tracksRef->at(tt).first = names[tt];
            m_pci_tracksRef->at(0).first = names[2];
            break;
        case 1:
            m_pci_tracksRef->resize (1);
            m_pci_tracksRef->at(0).first = names[0];
            break;
        default:
            assert(false);
            
    }
}


const smProducerRef lif_processor::sm () const {
    return m_sm;
    
}

// Check if the returned has expired
std::weak_ptr<contraction_analyzer> lif_processor::contractionWeakRef ()
{
    std::weak_ptr<contraction_analyzer> wp (m_caRef);
    return wp;
}


int64_t lif_processor::load (const std::shared_ptr<seqFrameContainer>& frames,const std::vector<std::string>& names)
{
    create_named_tracks(names);
    load_channels_from_images(frames);
    int channel_to_use = m_channel_count - 1;
    run_volume_sum_sumsq_count(channel_to_use);
    return m_frameCount;
}



/*
 * 1 monchrome channel. Compute volume stats of each on a thread
 */

svl::stats<int64_t> lif_processor::run_volume_sum_sumsq_count (const int channel_index){
    m_3d_stats_done = false;
    std::vector<std::tuple<int64_t,int64_t,uint32_t>> cts;
    std::vector<std::tuple<uint8_t,uint8_t>> rts;
    std::vector<std::thread> threads(1);
    threads[0] = std::thread(IntensityStatisticsPartialRunner(),std::ref(m_all_by_channel[channel_index]), std::ref(cts), std::ref(rts));
    std::for_each(threads.begin(), threads.end(), std::mem_fn(&std::thread::join));
    auto res = std::accumulate(cts.begin(), cts.end(), std::make_tuple(int64_t(0),int64_t(0), uint32_t(0)), stl_utils::tuple_sum<int64_t,uint32_t>());
    auto mes = std::accumulate(rts.begin(), rts.end(), std::make_tuple(uint8_t(255),uint8_t(0)), stl_utils::tuple_minmax<uint8_t, uint8_t>());
    m_3d_stats = svl::stats<int64_t> (std::get<0>(res), std::get<1>(res), std::get<2>(res), int64_t(std::get<0>(mes)), int64_t(std::get<1>(mes)));
    if (signal_3dstats_available && signal_3dstats_available->num_slots() > 0)
        signal_3dstats_available->operator()();
    return m_3d_stats;
    
}

void lif_processor::stats_3d_computed(){
    
}
/*
 * 2 flu channels. Compute stats of each using its own threaD
 */

std::shared_ptr<vectorOfnamedTrackOfdouble_t> lif_processor::run_flu_statistics (const std::vector<int>& channels)
{
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
// PCI track is being used for initial emtropy and median leveled 
std::shared_ptr<vectorOfnamedTrackOfdouble_t>  lif_processor::run_pci (const int channel_index)
{
    int channel_to_use = channel_index % m_channel_count;
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

// Update. Called also when cutoff offset has changed
void lif_processor::update ()
{
    if(m_pci_tracksRef && !m_pci_tracksRef->empty() && m_caRef && m_caRef->isValid())
        entropiesToTracks(m_pci_tracksRef->at(0));
}


void lif_processor::contraction_analyzed (contractionContainer_t& contractions)
{
    // Call the contraction available cb if any
    if (signal_contraction_available && signal_contraction_available->num_slots() > 0)
    {
        contractionContainer_t copied = m_caRef->contractions();
        signal_contraction_available->operator()(copied);
    }
     vlogger::instance().console()->info(" Contractions Analyzed: ");
}

void lif_processor:: channelmats_available (int& channel_index){
    
    if (m_all_by_channel[channel_index].size() == m_channel_mats[channel_index].size() &&
        signal_channelmats_available && signal_channelmats_available->num_slots() > 0){
        int ci = channel_index;
        signal_channelmats_available->operator()(ci);
    }
     vlogger::instance().console()->info(" Channels Available: ");
}
void lif_processor::sm_content_loaded ()
{
    vlogger::instance().console()->info(" Similarity Image Data Loaded ");
    int channel_to_use = m_channel_count - 1;
    loadImagesToMats(channel_to_use);
    vlogger::instance().console()->info(" cv::Mat generation dispatched ");
}



void lif_processor::loadImagesToMats (const int channel_index)
{
    int ci = channel_index;
    if (ci < 0){
        std::cout << " All channels is not implemented yet ";
        return;
    }
    if (m_all_by_channel.empty() || ci > (m_all_by_channel.size() - 1) ){
        std::cout << " Empty or channel does not exist ";
        return;
    }
    channel_images_t c2 = m_all_by_channel[channel_index];
    
    m_channel_mats[channel_index].resize(0);
    vector<roiWindow<P8U> >::const_iterator vitr = c2.begin();
    do
    {
        m_channel_mats[channel_index].emplace_back(vitr->height(), vitr->width(), CV_8UC(1), vitr->pelPointer(0,0), size_t(vitr->rowUpdate()));
    }
    while (++vitr != c2.end());

    channelmats_available(ci);

}

//  labelBlob::ref get_blobCachedObject (const index_time_t& ti)
//{
//    static std::mutex m;
//    
//    std::lock_guard<std::mutex> hold(m);
//    auto sp = m_blob_cache [ti].lock();
//    if(!sp)
//    {
//        m_blob_cache[ti] = sp = labelBlob::create(gray, tout, index);
//    }
//    return sp;
//}
// Assumes LIF data -- use multiple window.
// @todo condider creating cv::Mats and convert to roiWindow when needed.
void lif_processor::load_channels_from_images (const std::shared_ptr<seqFrameContainer>& frames)
{
    
    m_frameCount = 0;
    m_all_by_channel.clear();
    m_channel_count = frames->media_info().getNumChannels();
    m_all_by_channel.resize (m_channel_count);
    m_rois.resize (0);
    std::vector<std::string> names = {"Red", "Green","Blue"};
    
    while (frames->checkFrame(m_frameCount))
    {
        auto su8 = frames->getFrame(m_frameCount);
        auto ts = m_frameCount;
        auto m1 = svl::NewRefSingleFromSurface (su8, names, ts);
        
        switch (m_channel_count)
        {
                // @todo support general case
                // ID Lab 3 vertical roi 512 x (128x3)
            case 3  :
            {
                auto m3 = roiMultiWindow<P8UP3>(*m1, names, ts);
                for (auto cc = 0; cc < m3.planes(); cc++)
                    m_all_by_channel[cc].emplace_back(m3.plane(cc));
                
                if (m_rois.empty())
                {
                    for (auto cc = 0; cc < m3.planes(); cc++)
                    {
                        const iRect& ir = m3.roi(cc);
                        m_rois.emplace_back(vec2(ir.ul().first, ir.ul().second), vec2(ir.lr().first, ir.lr().second));
                    }
                }
                break;
            }
            case 1  :
            {
                m_all_by_channel[0].emplace_back(*m1);
                const iRect& ir = m1->frame();
                m_rois.emplace_back(vec2(ir.ul().first, ir.ul().second), vec2(ir.lr().first, ir.lr().second));
                break;
            }
        }
        m_frameCount++;
    }
    
    // Call the content loaded cb if any
    if (signal_content_loaded && signal_content_loaded->num_slots() > 0)
        signal_content_loaded->operator()(m_frameCount);
    
}

// Note tracks contained timed data.
// Each call to find_best can be with different median cut-off
void lif_processor::entropiesToTracks (namedTrackOfdouble_t& track)
{
    try{
        std::weak_ptr<contraction_analyzer> weakCaPtr (m_caRef);
        if (weakCaPtr.expired())
            return;
        
        if (! m_caRef->find_best  ()) return;
        m_medianLevel = m_caRef->filtered();
        track.second.clear();
        auto mee = m_caRef->filtered().begin();
        for (auto ss = 0; mee != m_caRef->filtered().end() && ss < frame_count(); ss++, mee++)
        {
            index_time_t ti;
            ti.first = ss;
            timed_double_t res;
            track.second.emplace_back(ti,*mee);
        }
    }
    catch(const std::exception & ex)
    {
        std::cout <<  ex.what() << std::endl;
    }
    // Signal we are done with median level set
    static int dummy;
    if (signal_sm1dmed_available && signal_sm1dmed_available->num_slots() > 0)
        signal_sm1dmed_available->operator()(dummy, dummy);
}


const int64_t& lif_processor::frame_count () const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    static int64_t inconsistent (0);
    
    if (m_all_by_channel.empty()) return inconsistent;
    
    const auto cs = m_all_by_channel[0].size();
    if (cs != m_frameCount) return inconsistent;
    for (const auto cc : m_all_by_channel){
        if (cc.size() != cs) return inconsistent;
    }
    return m_frameCount;
}

const int64_t lif_processor::channel_count () const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_channel_count;
}



