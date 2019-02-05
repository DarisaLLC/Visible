//
//  algo_Lif.cpp
//  Visible
//
//  Created by Arman Garakani on 8/20/18.
//


#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcomma"

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
#include "async_tracks.h"
#include "core/signaler.h"
#include "sm_producer.h"
#include "cinder_xchg.hpp"
#include "vision/histo.h"
#include "vision/opencv_utils.hpp"
#include "algo_cardiac.hpp"
#include "contraction.hpp"
#include "vision/localvariance.h"
#include "algo_Lif.hpp"
#include "logger.hpp"
#include "cpp-perf.hpp"

/****
 
 lif_serie implementation
 
 ****/


lif_serie_data:: lif_serie_data () : m_index (-1) {}

lif_serie_data:: lif_serie_data (const lifIO::LifReader::ref& m_lifRef, const unsigned index, const lifIO::ContentType_t& ct):
m_index(-1), m_contentType(ct) {
    
    if (index >= m_lifRef->getNbSeries()) return;
    
    m_index = index;
    m_name = m_lifRef->getSerie(m_index).getName();
    m_timesteps = static_cast<uint32_t>(m_lifRef->getSerie(m_index).getNbTimeSteps());
    m_pixelsInOneTimestep = static_cast<uint32_t>(m_lifRef->getSerie(m_index).getNbPixelsInOneTimeStep());
    m_dimensions = m_lifRef->getSerie(m_index).getSpatialDimensions();
    m_channelCount = static_cast<uint32_t>(m_lifRef->getSerie(m_index).getChannels().size());
    m_channels.clear ();
    for (lifIO::ChannelData cda : m_lifRef->getSerie(m_index).getChannels())
    {
        m_channel_names.push_back(cda.getName());
        m_channels.emplace_back(cda);
    }
    
    // Get timestamps in to time_spec_t and store it in info
    m_timeSpecs.resize (m_lifRef->getSerie(m_index).getTimestamps().size());
    
    // Adjust sizes based on the number of bytes
    std::transform(m_lifRef->getSerie(m_index).getTimestamps().begin(), m_lifRef->getSerie(m_index).getTimestamps().end(),
                   m_timeSpecs.begin(), [](lifIO::LifSerie::timestamp_t ts) { return time_spec_t ( ts / 10000.0); });
    
    m_length_in_seconds = m_lifRef->getSerie(m_index).total_duration ();
    
    auto serie_ref = std::shared_ptr<lifIO::LifSerie>(&m_lifRef->getSerie(m_index), stl_utils::null_deleter());
    // opencv rows, cols
    uint64_t rows (m_dimensions[1] * m_channelCount);
    uint64_t cols (m_dimensions[0]);
    cv::Mat dst ( static_cast<uint32_t>(rows),static_cast<uint32_t>(cols), CV_8U);
    serie_ref->fill2DBuffer(dst.ptr(0), 0);
    m_poster = dst;

    m_lifWeakRef = m_lifRef;
    
}

// Check if the returned has expired
const lifIO::LifReader::weak_ref& lif_serie_data::readerWeakRef () const{
    return m_lifWeakRef;
}



/****
 
 lif_browser implementation
 
 ****/


lif_browser::lif_browser (const std::string&  fqfn_path, const lifIO::ContentType_t& ct) :
    mFqfnPath(fqfn_path), m_content_type(ct), m_data_ready(false) {
    if ( boost::filesystem::exists(boost::filesystem::path(mFqfnPath)) )
    {
        std::string msg = " Loaded Series Info for " + mFqfnPath ;
        vlogger::instance().console()->info(msg);
        internal_get_series_info();
        
    }
}
const lif_serie_data  lif_browser::get_serie_by_index (unsigned index){
    lif_serie_data si;
    get_series_info();
    if (index < m_series_book.size())
        si = m_series_book[index];
    return si;
    
}

const std::vector<lif_serie_data>& lif_browser::get_all_series  () const{
    get_series_info();
    return m_series_book;
}

const std::vector<std::string>& lif_browser::names () const{
    get_series_info();
    return m_series_names;
    
}
/*
 * LIF files are plane organized. 3 Channel LIF file is 3 * rows by cols by ONE byte. 
 */

void lif_browser::get_first_frame (lif_serie_data& si,  const int frameCount, cv::Mat& out)
{
    get_series_info();
    
    auto serie_ref = std::shared_ptr<lifIO::LifSerie>(&m_lifRef->getSerie(si.index()), stl_utils::null_deleter());
    // opencv rows, cols
    uint64_t rows (si.dimensions()[1] * si.channelCount());
    uint64_t cols (si.dimensions()[0]);
    cv::Mat dst ( static_cast<uint32_t>(rows),static_cast<uint32_t>(cols), CV_8U);
    serie_ref->fill2DBuffer(dst.ptr(0), 0);
    out = dst;
}


void  lif_browser::internal_get_series_info () const
{
    if ( exists(boost::filesystem::path(mFqfnPath)))
    {
        m_lifRef =  lifIO::LifReader::create(mFqfnPath, m_content_type);
        m_series_book.clear ();
        m_series_names.clear();
        
        for (unsigned ss = 0; ss < m_lifRef->getNbSeries(); ss++)
        {
            lif_serie_data si(m_lifRef, ss, m_content_type);
            m_series_book.emplace_back (si);
            m_series_names.push_back(si.name());
            // Fill up names / index map -- convenience
            auto index = static_cast<int>(ss);
            m_name_to_index[si.name()] = index;
            m_index_to_name[index] = si.name();
        }
        m_data_ready.store(true, std::memory_order_release);
    }
}

// Yield while finishing up
void  lif_browser::get_series_info () const
{
    while(!m_data_ready.load(std::memory_order_acquire)){
        std::this_thread::yield();
    }
}



/****
 
    lif_processor implementation
 
 ****/

lif_serie_processor::lif_serie_processor ()
{
    // Signals we provide
    signal_content_loaded = createSignal<lif_serie_processor::sig_cb_content_loaded>();
    signal_flu_available = createSignal<lif_serie_processor::sig_cb_flu_stats_available>();
    signal_frame_loaded = createSignal<lif_serie_processor::sig_cb_frame_loaded>();
    signal_sm1d_available = createSignal<lif_serie_processor::sig_cb_sm1d_available>();
    signal_sm1dmed_available = createSignal<lif_serie_processor::sig_cb_sm1dmed_available>();
    signal_contraction_available = createSignal<lif_serie_processor::sig_cb_contraction_available>();
    signal_3dstats_available = createSignal<lif_serie_processor::sig_cb_3dstats_available>();
    signal_volume_var_available = createSignal<lif_serie_processor::sig_cb_volume_var_available>();
    
    // semilarity producer
    m_sm = std::shared_ptr<sm_producer> ( new sm_producer () );
    
    // Signals we support
    // support Similarity::Content Loaded
   // std::function<void ()> sm_content_loaded_cb = boost::bind (&lif_serie_processor::sm_content_loaded, this);
   // boost::signals2::connection ml_connection = m_sm->registerCallback(sm_content_loaded_cb);
    
    // Create a contraction object
    m_caRef = contraction_analyzer::create ();
    
    // Suport lif_processor::Contraction Analyzed
    std::function<void (contractionContainer_t&)>ca_analyzed_cb = boost::bind (&lif_serie_processor::contraction_analyzed, this, _1);
    boost::signals2::connection ca_connection = m_caRef->registerCallback(ca_analyzed_cb);
    
    // Signal us when we have pci mat channels are ready to run contraction analysis
    //std::function<void (int&)> sm1dmed_available_cb = boost::bind (&lif_processor::pci_done, this);
    //boost::signals2::connection nl_connection = registerCallback(sm1dmed_available_cb);
    
    // Signal us when 3d stats are ready
    std::function<void ()>_3dstats_done_cb = boost::bind (&lif_serie_processor::stats_3d_computed, this);
    boost::signals2::connection _3d_stats_connection = registerCallback(_3dstats_done_cb);
    
}

// @note Specific to ID Lab Lif Files
// @note Specific to ID Lab Lif Files
// 3 channels: 2 flu one visible
// 1 channel: visible
void lif_serie_processor::create_named_tracks (const std::vector<std::string>& names)
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


const smProducerRef lif_serie_processor::sm () const {
    return m_sm;
    
}

// Check if the returned has expired
std::weak_ptr<contraction_analyzer> lif_serie_processor::contractionWeakRef ()
{
    std::weak_ptr<contraction_analyzer> wp (m_caRef);
    return wp;
}


int64_t lif_serie_processor::load (const std::shared_ptr<seqFrameContainer>& frames,const std::vector<std::string>& names)
{
    create_named_tracks(names);
    load_channels_from_images(frames);
    int channel_to_use = m_channel_count - 1;
    run_volume_variances (channel_to_use);
    return m_frameCount;
}


/*
 * 1 monchrome channel. Compute 3D Standard Dev. per pixel
 */

void lif_serie_processor::run_volume_variances (const int channel_index){
    m_3d_stats_done = false;
    cv::Mat m_sum, m_sqsum;
    int image_count = 0;
    std::vector<std::thread> threads(1);
    threads[0] = std::thread(SequenceAccumulator(),std::ref(m_all_by_channel[channel_index]),
                             std::ref(m_sum), std::ref(m_sqsum), std::ref(image_count));
    
    std::for_each(threads.begin(), threads.end(), std::mem_fn(&std::thread::join));
    SequenceAccumulator::computeStdev(m_sum, m_sqsum, image_count, m_var_image);
    m_var_display_image.create(m_var_image.rows, m_var_image.cols, CV_8UC1);
    cv::normalize(m_var_image, m_var_display_image, 0, 255, NORM_MINMAX, CV_8UC1);
   // cv::imwrite("/Users/arman/tmp/var.png", m_var_display_image);
    
    // Signal to listeners
    if (signal_volume_var_available && signal_volume_var_available->num_slots() > 0)
        signal_volume_var_available->operator()();
}


/*
 * 1 monchrome channel. Compute volume stats of each on a thread
 */

svl::stats<int64_t> lif_serie_processor::run_volume_sum_sumsq_count (const int channel_index){
    m_3d_stats_done = false;
    std::vector<std::tuple<int64_t,int64_t,uint32_t>> cts;
    std::vector<std::tuple<uint8_t,uint8_t>> rts;
    std::vector<std::thread> threads(1);
    threads[0] = std::thread(IntensityStatisticsPartialRunner(),std::ref(m_all_by_channel[channel_index]), std::ref(cts), std::ref(rts));
    std::for_each(threads.begin(), threads.end(), std::mem_fn(&std::thread::join));
    auto res = std::accumulate(cts.begin(), cts.end(), std::make_tuple(int64_t(0),int64_t(0), uint32_t(0)), stl_utils::tuple_sum<int64_t,uint32_t>());
    auto mes = std::accumulate(rts.begin(), rts.end(), std::make_tuple(uint8_t(255),uint8_t(0)), stl_utils::tuple_minmax<uint8_t, uint8_t>());
    m_3d_stats = svl::stats<int64_t> (std::get<0>(res), std::get<1>(res), std::get<2>(res), int64_t(std::get<0>(mes)), int64_t(std::get<1>(mes)));
    
    // Signal to listeners
    if (signal_3dstats_available && signal_3dstats_available->num_slots() > 0)
        signal_3dstats_available->operator()();
    return m_3d_stats;
    
}

void lif_serie_processor::stats_3d_computed(){
    
}
/*
 * 2 flu channels. Compute stats of each using its own threaD
 */

std::shared_ptr<vectorOfnamedTrackOfdouble_t> lif_serie_processor::run_flu_statistics (const std::vector<int>& channels)
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
std::shared_ptr<vectorOfnamedTrackOfdouble_t>  lif_serie_processor::run_pci (const int channel_index)
{
    int channel_to_use = channel_index % m_channel_count;
    channel_images_t c2 = m_all_by_channel[channel_to_use];
    auto sp =  sm();
    sp->load_images (c2);
    std::packaged_task<bool()> task([sp](){ return sp->operator()(0);}); // wrap the function
    std::future<bool>  future_ss = task.get_future();  // get a future
    std::thread(std::move(task)).join(); // Finish on a thread
    if (future_ss.get())
    {
        const deque<double>& entropies = sp->shannonProjection ();
        m_entropies.insert(m_entropies.end(), entropies.begin(), entropies.end());
        const std::deque<deque<double>>& sm = sp->similarityMatrix();
        for (auto row : sm){
            vector<double> rowv;
            rowv.insert(rowv.end(), row.begin(), row.end());
            m_smat.push_back(rowv);
        }
        m_caRef->load(m_entropies,m_smat);
        update ();
        
        // Signal we are done with ACI
        static int dummy;
        if (signal_sm1d_available && signal_sm1d_available->num_slots() > 0)
            signal_sm1d_available->operator()(dummy);
    }
    return m_pci_tracksRef;
}



const std::vector<Rectf>& lif_serie_processor::rois () const { return m_rois; }

// Update. Called also when cutoff offset has changed
void lif_serie_processor::update ()
{
    if(m_pci_tracksRef && !m_pci_tracksRef->empty() && m_caRef && m_caRef->isValid())
        entropiesToTracks(m_pci_tracksRef->at(0));
}


void lif_serie_processor::contraction_analyzed (contractionContainer_t& contractions)
{
    // Call the contraction available cb if any
    if (signal_contraction_available && signal_contraction_available->num_slots() > 0)
    {
        contractionContainer_t copied = m_caRef->contractions();
        signal_contraction_available->operator()(copied);
    }
     vlogger::instance().console()->info(" Contractions Analyzed: ");
}





// Assumes LIF data -- use multiple window.
// @todo condider creating cv::Mats and convert to roiWindow when needed.

void lif_serie_processor::load_channels_from_images (const std::shared_ptr<seqFrameContainer>& frames)
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
                auto m3 = roiFixedMultiWindow<P8UP3>(*m1, names, ts);
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
void lif_serie_processor::entropiesToTracks (namedTrackOfdouble_t& track)
{
    try{
        std::weak_ptr<contraction_analyzer> weakCaPtr (m_caRef);
        if (weakCaPtr.expired())
            return;
        
        if (! m_caRef->find_best  ()) return;
        m_medianLevel = m_caRef->leveled();
        track.second.clear();
        auto mee = m_caRef->leveled().begin();
        for (auto ss = 0; mee != m_caRef->leveled().end() && ss < frame_count(); ss++, mee++)
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


const int64_t& lif_serie_processor::frame_count () const
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

const int64_t lif_serie_processor::channel_count () const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_channel_count;
}

#pragma GCC diagnostic pop


