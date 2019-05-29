//
//  lif_core.cpp
//  Visible
//
//  Created by Arman Garakani on 5/26/19.
//

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcomma"
#pragma GCC diagnostic ignored "-Wunused-private-field"


#include <stdio.h>
#include <iostream>
#include <string>
#include <vector>
#include <deque>
#include <sstream>
#include <map>
#include "async_tracks.h"
#include "core/signaler.h"
#include "sm_producer.h"
#include "cinder_cv/cinder_xchg.hpp"
#include "cinder_cv/cinder_opencv.h"
#include "vision/histo.h"
#include "vision/opencv_utils.hpp"
#include "algo_Lif.hpp"
#include "logger/logger.hpp"
#include "result_serialization.h"


/**
 lif_serie_processor

 @param path to the Cache Folder

 */
lif_serie_processor::lif_serie_processor (const fs::path& serie_cache_folder):
mCurrentSerieCachePath(serie_cache_folder)
{
    // Signals we provide
    signal_content_loaded = createSignal<lif_serie_processor::sig_cb_content_loaded>();
    signal_flu_available = createSignal<lif_serie_processor::sig_cb_flu_stats_available>();
    signal_frame_loaded = createSignal<lif_serie_processor::sig_cb_frame_loaded>();
    signal_sm1d_available = createSignal<lif_serie_processor::sig_cb_sm1d_available>();
    signal_sm1dmed_available = createSignal<lif_serie_processor::sig_cb_sm1dmed_available>();
    signal_contraction_available = createSignal<lif_serie_processor::sig_cb_contraction_available>();
    signal_3dstats_available = createSignal<lif_serie_processor::sig_cb_3dstats_available>();
    signal_geometry_available = createSignal<lif_serie_processor::sig_cb_geometry_available>();
    signal_ss_image_available = createSignal<lif_serie_processor::sig_cb_ss_image_available>();
    signal_ss_voxel_available = createSignal<lif_serie_processor::sig_cb_ss_voxel_available>();
    
    // semilarity producer
    m_sm_producer = std::shared_ptr<sm_producer> ( new sm_producer () );
    
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
    
    // Signal us when ss segmentation surface is ready
    std::function<sig_cb_ss_image_available>_ss_image_done_cb = boost::bind (&lif_serie_processor::finalize_segmentation,
                                                                             this, _1, _2, _3);
    boost::signals2::connection _ss_image_connection = registerCallback(_ss_image_done_cb);
    
    // Signal us when ss segmentation is ready
    std::function<sig_cb_ss_voxel_available> _ss_voxel_done_cb = boost::bind (&lif_serie_processor::create_voxel_surface, this, _1);
    boost::signals2::connection _ss_voxel_connection = registerCallback(_ss_voxel_done_cb);
    
}

// @note Specific to ID Lab Lif Files
// @note Specific to ID Lab Lif Files
// 3 channels: 2 flu one visible
// 1 channel: visible
/**
 create named_tracks

 @param names <#names description#>
 @param plot_names <#plot_names description#>
 */
void lif_serie_processor::create_named_tracks (const std::vector<std::string>& names, const std::vector<std::string>& plot_names)
{
    m_flurescence_tracksRef = std::make_shared<vecOfNamedTrack_t> ();
    m_contraction_pci_tracksRef = std::make_shared<vecOfNamedTrack_t> ();
    
    switch(names.size()){
        case 3:
            m_flurescence_tracksRef->resize (2);
            m_contraction_pci_tracksRef->resize (1);
            m_shortterm_pci_tracks.resize (1);
            for (auto tt = 0; tt < names.size()-1; tt++)
                m_flurescence_tracksRef->at(tt).first = plot_names[tt];
            m_contraction_pci_tracksRef->at(0).first = plot_names[2];
            m_shortterm_pci_tracks.at(0).first = plot_names[3];
            break;
        case 1:
            m_contraction_pci_tracksRef->resize (1);
            m_shortterm_pci_tracks.resize (1);
            m_contraction_pci_tracksRef->at(0).first = plot_names[0];
            m_shortterm_pci_tracks.at(0).first = plot_names[1];
            break;
        default:
            assert(false);
            
    }
}


const smProducerRef lif_serie_processor::similarity_producer () const {
    return m_sm_producer;
    
}


/*
 * Note On Sampled Voxel Creation:
 * Top left of unsampled voxel is at pixel top left, i.e. vCols x vRows
 * The result of voxel operation is at 0.5, 0.5 of the voxel area
 * that is in vCols / 2 , vRows / 2
 * And vCols / 2 , vRows / 2 border
 */
int64_t lif_serie_processor::load (const std::shared_ptr<seqFrameContainer>& frames,const std::vector<std::string>& names, const std::vector<std::string>& plot_names)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    
    m_voxel_sample.first = m_params.voxel_sample().first;
    m_expected_segmented_size.first = frames->getChannelWidth() / m_params.voxel_sample().first;
    m_voxel_sample.second = m_params.voxel_sample().second;
    m_expected_segmented_size.second = frames->getChannelHeight() / m_params.voxel_sample().second;
    
    create_named_tracks(names, plot_names);
    load_channels_from_images(frames);
    lock.unlock();
    
    int channel_to_use = m_channel_count - 1;
    run_detect_geometry (channel_to_use);
    
    
    // Call the content loaded cb if any
    if (signal_content_loaded && signal_content_loaded->num_slots() > 0)
        signal_content_loaded->operator()(m_frameCount);
    
    return m_frameCount;
}



/*
 * 1 monchrome channel. Compute volume stats of each on a thread
 */

svl::stats<int64_t> lif_serie_processor::run_volume_sum_sumsq_count (std::vector<roiWindow<P8U>>& images){
    std::lock_guard<std::mutex> lock(m_mutex);
    m_3d_stats_done = false;
    std::vector<std::tuple<int64_t,int64_t,uint32_t>> cts;
    std::vector<std::tuple<uint8_t,uint8_t>> rts;
    std::vector<std::thread> threads(1);
    threads[0] = std::thread(IntensityStatisticsPartialRunner(),std::ref(images), std::ref(cts), std::ref(rts));
    std::for_each(threads.begin(), threads.end(), std::mem_fn(&std::thread::join));
    auto res = std::accumulate(cts.begin(), cts.end(), std::make_tuple(int64_t(0),int64_t(0), uint32_t(0)), stl_utils::tuple_sum<int64_t,uint32_t>());
    auto mes = std::accumulate(rts.begin(), rts.end(), std::make_tuple(uint8_t(255),uint8_t(0)), stl_utils::tuple_minmax<uint8_t, uint8_t>());
    m_3d_stats = svl::stats<int64_t> (std::get<0>(res), std::get<1>(res), std::get<2>(res), int64_t(std::get<0>(mes)), int64_t(std::get<1>(mes)));
    
    // Signal to listeners
    if (signal_3dstats_available && signal_3dstats_available->num_slots() > 0)
        signal_3dstats_available->operator()();
    return m_3d_stats;
    
}


svl::stats<int64_t> lif_serie_processor::run_volume_sum_sumsq_count (const int channel_index){
    return run_volume_sum_sumsq_count(m_all_by_channel[channel_index]);
}


void lif_serie_processor::stats_3d_computed(){
    
}
/*
 * 2 flu channels. Compute stats of each using its own threaD
 */

/**
 run_flu_statistics
 
 @param channels vector of channels containing fluorescence images
 @return vecor of named tracks containing of output time series
 */
std::shared_ptr<vecOfNamedTrack_t> lif_serie_processor::run_flu_statistics (const std::vector<int>& channels)
{
    std::vector<std::thread> threads(channels.size());
    for (auto tt = 0; tt < channels.size(); tt++)
    {
        threads[tt] = std::thread(IntensityStatisticsRunner(),
                                  std::ref(m_all_by_channel[tt]), std::ref(m_flurescence_tracksRef->at(tt).second));
    }
    std::for_each(threads.begin(), threads.end(), std::mem_fn(&std::thread::join));
    
    // Call the content loaded cb if any
    if (signal_flu_available && signal_flu_available->num_slots() > 0)
        signal_flu_available->operator()();
    
    return m_flurescence_tracksRef;
}


// Run to get Entropies and Median Level Set
// PCI track is being used for initial emtropy and median leveled
std::shared_ptr<vecOfNamedTrack_t>  lif_serie_processor::run_contraction_pci (const std::vector<roiWindow<P8U>>& images)
{
    bool cache_ok = false;
    size_t dim = images.size();
    std::shared_ptr<ssResultContainer> ssref;
    
    if(fs::exists(mCurrentSerieCachePath)){
        auto cache_path = mCurrentSerieCachePath / m_params.result_container_cache_name ();
        if(fs::exists(cache_path)){
            ssref = ssResultContainer::create(cache_path);
        }
        cache_ok = ssref && ssref->size_check(dim);
    }
    
    if(cache_ok){
        vlogger::instance().console()->info(" SS result container cache : Hit ");
        m_entropies.insert(m_entropies.end(), ssref->entropies().begin(),
                           ssref->entropies().end());
        const std::deque<deque<double>>& sm = ssref->smatrix();
        for (auto row : sm){
            vector<double> rowv;
            rowv.insert(rowv.end(), row.begin(), row.end());
            m_smat.push_back(rowv);
        }
        m_caRef->load(m_entropies,m_smat);
        update ();
        
    }else{
        
        auto cache_path = mCurrentSerieCachePath / m_params.result_container_cache_name ();
        auto sp =  similarity_producer();
        sp->load_images (images);
        std::packaged_task<bool()> task([sp](){ return sp->operator()(0);}); // wrap the function
        std::future<bool>  future_ss = task.get_future();  // get a future
        std::thread(std::move(task)).join(); // Finish on a thread
        if (future_ss.get())
        {
            const deque<double>& entropies = sp->shannonProjection ();
            const std::deque<deque<double>>& sm = sp->similarityMatrix();
            m_entropies.insert(m_entropies.end(), entropies.begin(), entropies.end());
            for (auto row : sm){
                vector<double> rowv;
                rowv.insert(rowv.end(), row.begin(), row.end());
                m_smat.push_back(rowv);
            }
            m_caRef->load(m_entropies,m_smat);
            update ();
            bool ok = ssResultContainer::store(cache_path, entropies, sm);
            if(ok)
                vlogger::instance().console()->info(" SS result container cache : filled ");
            else
                vlogger::instance().console()->info(" SS result container cache : failed ");
            
        }
    }
    // Signal we are done with ACI
    static int dummy;
    if (signal_sm1d_available && signal_sm1d_available->num_slots() > 0)
        signal_sm1d_available->operator()(dummy);
    
    return m_contraction_pci_tracksRef;
}

// @todo even though channel index is an argument, but only results for one channel is kept in lif_processor.
//
std::shared_ptr<vecOfNamedTrack_t>  lif_serie_processor::run_contraction_pci_on_channel (const int channel_index)
{
    return run_contraction_pci(std::move(m_all_by_channel[channel_index]));
}

/*
 * 1 monchrome channel. Compute 3D Standard Dev. per pixel
 */

void lif_serie_processor::run_volume_variances (std::vector<roiWindow<P8U>>& images){
    std::lock_guard<std::mutex> lock(m_mutex);
    m_3d_stats_done = false;
    cv::Mat m_sum, m_sqsum;
    int image_count = 0;
    std::vector<std::thread> threads(1);
    threads[0] = std::thread(SequenceAccumulator(),std::ref(images),
                             std::ref(m_sum), std::ref(m_sqsum), std::ref(image_count));
    
    std::for_each(threads.begin(), threads.end(), std::mem_fn(&std::thread::join));
    SequenceAccumulator::computeStdev(m_sum, m_sqsum, image_count, m_var_image);
    cv::normalize(m_var_image, m_var_image, 0, 255, NORM_MINMAX, CV_8UC1);
    
}

const std::vector<Rectf>& lif_serie_processor::channel_rois () const { return m_channel_rois; }




// Assumes LIF data -- use multiple window.
// @todo condider creating cv::Mats and convert to roiWindow when needed.

void lif_serie_processor::load_channels_from_images (const std::shared_ptr<seqFrameContainer>& frames)
{
    m_frameCount = 0;
    m_all_by_channel.clear();
    m_channel_count = frames->media_info().getNumChannels();
    m_all_by_channel.resize (m_channel_count);
    m_channel_rois.resize (0);
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
                
                if (m_channel_rois.empty())
                {
                    for (auto cc = 0; cc < m3.planes(); cc++)
                    {
                        const iRect& ir = m3.roi(cc);
                        m_channel_rois.emplace_back(vec2(ir.ul().first, ir.ul().second), vec2(ir.lr().first, ir.lr().second));
                    }
                }
                break;
            }
            case 1  :
            {
                m_all_by_channel[0].emplace_back(*m1);
                const iRect& ir = m1->frame();
                m_channel_rois.emplace_back(vec2(ir.ul().first, ir.ul().second), vec2(ir.lr().first, ir.lr().second));
                break;
            }
        }
        m_frameCount++;
    }
    
    
}


/**
 @Check if needed
 median_leveled_pci
 Note tracks contained timed data.
 Each call to find_best can be with different median cut-off
 @param track track to be filled
 */
void lif_serie_processor::median_leveled_pci (namedTrack_t& track)
{
    //  std::lock_guard<std::mutex> lock(m_mutex);
    
    try{
        std::weak_ptr<contraction_analyzer> weakCaPtr (m_caRef);
        if (weakCaPtr.expired())
            return;
        
        if (! m_caRef->find_best  ()) return;
        m_medianLevel = m_caRef->leveled();
        track.second.clear();
        auto mee = m_caRef->leveled().begin();
        for (auto ss = 0; mee != m_caRef->leveled().end() || ss < frame_count(); ss++, mee++)
        {
            index_time_t ti;
            ti.first = ss;
            timedVal_t res;
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

std::shared_ptr<OCVImageWriter>& lif_serie_processor::get_image_writer (){
    if (! m_writer){
        m_writer = std::make_shared<OCVImageWriter>();
    }
    return m_writer;
}

void lif_serie_processor::save_channel_images (int channel_index, std::string& dir_fqfn){
    std::lock_guard<std::mutex> lock(m_mutex);
    int channel_to_use = channel_index % m_channel_count;
    channel_images_t c2 = m_all_by_channel[channel_to_use];
    auto writer = get_image_writer();
    if (writer)
        writer->operator()(dir_fqfn, c2);
    
}

