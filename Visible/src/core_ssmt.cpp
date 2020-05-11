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
#include "timed_types.h"
#include "core/signaler.h"
#include "sm_producer.h"
#include "cinder_cv/cinder_xchg.hpp"
#include "cinder_cv/cinder_opencv.h"
#include "vision/histo.h"
#include "vision/opencv_utils.hpp"
#include "ssmt.hpp"
#include "logger/logger.hpp"
#include "result_serialization.h"


/**
 ssmt_processor

 @param path to the Cache Folder

 */
ssmt_processor::ssmt_processor (const fs::path& serie_cache_folder):
mCurrentSerieCachePath(serie_cache_folder)
{
    // Signals we provide
    signal_content_loaded = createSignal<ssmt_processor::sig_cb_content_loaded>();
    signal_flu_ready = createSignal<ssmt_processor::sig_cb_flu_stats_ready>();
    signal_frame_loaded = createSignal<ssmt_processor::sig_cb_frame_loaded>();
    signal_contraction_pci_ready = createSignal<ssmt_processor::sig_cb_sm1d_ready>();
    signal_sm1dmed_ready = createSignal<ssmt_processor::sig_cb_sm1dmed_ready>();
    signal_contraction_ready = createSignal<ssmt_processor::sig_cb_contraction_ready>();
    signal_3dstats_ready = createSignal<ssmt_processor::sig_cb_3dstats_ready>();
    signal_geometry_ready = createSignal<ssmt_processor::sig_cb_geometry_ready>();
    signal_segmented_view_ready = createSignal<ssmt_processor::sig_cb_segmented_view_ready>();
    signal_ss_voxel_ready = createSignal<ssmt_processor::sig_cb_ss_voxel_ready>();
    
    // semilarity producer
    m_sm_producer = std::shared_ptr<sm_producer> ( new sm_producer () );
    
    // Signals we support
    // support Similarity::Content Loaded
    // std::function<void ()> sm_content_loaded_cb = boost::bind (&ssmt_processor::sm_content_loaded, this);
    // boost::signals2::connection ml_connection = m_sm->registerCallback(sm_content_loaded_cb);
    
   
    // Signal us when 3d stats are ready
    std::function<void ()>_3dstats_done_cb = boost::bind (&ssmt_processor::stats_3d_computed, this);
    boost::signals2::connection _3d_stats_connection = registerCallback(_3dstats_done_cb);
    
    // Signal us when ss segmentation surface is ready
    std::function<sig_cb_segmented_view_ready> _ss_segmentation_done_cb = boost::bind (&ssmt_processor::finalize_segmentation,
                                                                             this, _1, _2);
    boost::signals2::connection _ss_image_connection = registerCallback(_ss_segmentation_done_cb);
    
    // Signal us when ss segmentation is ready
    std::function<sig_cb_ss_voxel_ready> _ss_voxel_done_cb = boost::bind (&ssmt_processor::create_voxel_surface,this, _1);
    boost::signals2::connection _ss_voxel_connection = registerCallback(_ss_voxel_done_cb);
    
    // Support lifProcessor::geometry_ready
    std::function<void (int, const input_channel_selector_t&)> geometry_ready_cb = boost::bind (&ssmt_processor::signal_geometry_done, this, _1, _2);
    boost::signals2::connection geometry_connection = registerCallback(geometry_ready_cb);
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
void ssmt_processor::create_named_tracks (const std::vector<std::string>& names, const std::vector<std::string>& plot_names)
{
    m_flurescence_tracksRef = std::make_shared<vecOfNamedTrack_t> ();
    m_contraction_pci_tracksRef = std::make_shared<vecOfNamedTrack_t> ();
    
    switch(names.size()){
        case 2:
            m_flurescence_tracksRef->resize (1);
            m_contraction_pci_tracksRef->resize (1);
            m_shortterm_pci_tracks.resize (1);
            for (auto tt = 0; tt < names.size()-1; tt++)
                m_flurescence_tracksRef->at(tt).first = plot_names[tt];
            m_contraction_pci_tracksRef->at(0).first = plot_names[1];
            m_shortterm_pci_tracks.at(0).first = plot_names[2];
            break;
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


const smProducerRef ssmt_processor::similarity_producer () const {
    return m_sm_producer;
    
}

// Check if the returned has expired
std::weak_ptr<contractionLocator> ssmt_processor::entireContractionWeakRef ()
{
    std::weak_ptr<contractionLocator> wp (m_entireCaRef);
    return wp;
}

fs::path ssmt_processor::get_cache_location (const int channel_index,const int input){
    // Check input
    bool isEntire = input == -1;
    bool isMobj = input >= 0 && input < m_results.size();
    // Check index
    bool channel_index_ok = channel_index >= 0 && channel_index < channel_count();
    if (isEntire == isMobj || ! channel_index_ok)
        return fs::path ();
    
    // Get Cache path
    auto cache_path = mCurrentSerieCachePath;
    if(! isEntire){
        const int count = create_cache_paths();
        if(count != (int)m_results.size()){
            vlogger::instance().console()->error(" miscount in cache_path  ");
        }
        std::string subdir = to_string(input);
        cache_path = cache_path / subdir;
    }
    cache_path = cache_path / m_params.result_container_cache_name ();
    return cache_path;
}

int ssmt_processor:: create_cache_paths (){

    int count = 0;
    // Create cache directories for each cell off of the top for now (@todo create a cells subdir )

    for(const ssmt_result::ref_t& sr : m_results){
        if(fs::exists(mCurrentSerieCachePath)){
            std::string subdir = to_string(sr->id());
            auto save_path = mCurrentSerieCachePath / subdir;
            boost::system::error_code ec;
            if(!fs::exists(save_path)){
                fs::create_directory(save_path, ec);
                
                switch( ec.value() ) {
                    case boost::system::errc::success: {
                        count++;
                        std::string msg = "Created " + save_path.string() ;
                        vlogger::instance().console()->info(msg);
                    }
                        break;
                    default:
                        std::string msg = "Failed to create " + save_path.string() + ec.message();
                        vlogger::instance().console()->info(msg);
                        return -1;
                }
            } // tried creating it if was not already
            else{
                count++; // exists
            }
            // Save path exists, try updating the map
            int last_count = count-1;
            assert(last_count >= 0 && last_count < m_results.size());
            if(m_result_index_to_cache_path.find(last_count) == m_result_index_to_cache_path.end()){
                m_result_index_to_cache_path[last_count] = save_path;
            }
        }
    }
    return count;
    
}
//! Load and start async processing of Visible channel
/*!
 *
 * Note On Sampled Voxel Creation:
 * Top left of unsampled voxel is at pixel top left, i.e. vCols x vRows
 * The result of voxel operation is at 0.5, 0.5 of the voxel area
 * that is in vCols / 2 , vRows / 2
 * And vCols / 2 , vRows / 2 border
 */
int64_t ssmt_processor::load (const std::shared_ptr<seqFrameContainer>& frames,const std::vector<std::string>& names, const std::vector<std::string>& plot_names)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    
    m_voxel_sample.first = m_params.voxel_sample().first;
    m_expected_segmented_size.first = frames->getChannelWidth() / m_params.voxel_sample().first;
    m_voxel_sample.second = m_params.voxel_sample().second;
    m_expected_segmented_size.second = frames->getChannelHeight() / m_params.voxel_sample().second;
    
    /*
     Creates named tracks @todo move out of here
     Loads channels from images @note uses last channel for visible processing
        i.e 2nd channel from 2 channel LIF file and 3rd channel from a 3 channel LIF file or media file
     
     */
    create_named_tracks(names, plot_names);
    load_channels_from_images(frames);
    lock.unlock();
    
    int channel_to_use = m_channel_count - 1;
    m_3d_stats_done = false;
    run_volume_variances(m_all_by_channel[channel_to_use]);
    while(!m_3d_stats_done){ std::this_thread::yield();}
    m_instant_input = input_channel_selector_t(-1, channel_to_use);
    find_moving_regions (channel_to_use);
    
    
    // Call the content loaded cb if any
    if (signal_content_loaded && signal_content_loaded->num_slots() > 0)
        signal_content_loaded->operator()(m_frameCount);
    
    return m_frameCount;
}



/*
 * 1 monchrome channel. Compute volume stats of each on a thread
 */

svl::stats<int64_t> ssmt_processor::run_volume_sum_sumsq_count (std::vector<roiWindow<P8U>>& images){
    std::lock_guard<std::mutex> lock(m_mutex);

    std::vector<std::tuple<int64_t,int64_t,uint32_t>> cts;
    std::vector<std::tuple<uint8_t,uint8_t>> rts;
    std::vector<std::thread> threads(1);
    threads[0] = std::thread(IntensityStatisticsPartialRunner(),std::ref(images), std::ref(cts), std::ref(rts));
    std::for_each(threads.begin(), threads.end(), std::mem_fn(&std::thread::join));
    auto res = std::accumulate(cts.begin(), cts.end(), std::make_tuple(int64_t(0),int64_t(0), uint32_t(0)), stl_utils::tuple_sum<int64_t,uint32_t>());
    auto mes = std::accumulate(rts.begin(), rts.end(), std::make_tuple(uint8_t(255),uint8_t(0)), stl_utils::tuple_minmax<uint8_t, uint8_t>());
    m_3d_stats = svl::stats<int64_t> (std::get<0>(res), std::get<1>(res), std::get<2>(res), int64_t(std::get<0>(mes)), int64_t(std::get<1>(mes)));
    
    // Signal to listeners
    if (signal_3dstats_ready && signal_3dstats_ready->num_slots() > 0)
        signal_3dstats_ready->operator()();
    return m_3d_stats;
    
}


svl::stats<int64_t> ssmt_processor::run_volume_sum_sumsq_count (const int channel_index){
    return run_volume_sum_sumsq_count(m_all_by_channel[channel_index]);
}


void ssmt_processor::stats_3d_computed(){
    
}
/*
 * 2 flu channels. Compute stats of each using its own threaD
 */

/**
 run_flu_statistics
 
 @param channels vector of channels containing fluorescence images
 @return vecor of named tracks containing of output time series
 */
std::shared_ptr<vecOfNamedTrack_t> ssmt_processor::run_flu_statistics (const std::vector<int>& channels)
{
    std::vector<std::thread> threads(channels.size());
    for (auto tt = 0; tt < channels.size(); tt++)
    {
        auto channel = channels[tt];
        threads[tt] = std::thread(IntensityStatisticsRunner(),
                                  std::ref(m_all_by_channel[channel]), std::ref(m_flurescence_tracksRef->at(channel).second));
    }
    std::for_each(threads.begin(), threads.end(), std::mem_fn(&std::thread::join));
    
    // Call the content loaded cb if any
    if (signal_flu_ready && signal_flu_ready->num_slots() > 0)
        signal_flu_ready->operator()();
    
    return m_flurescence_tracksRef;
}

// This is called from GUI side.
// @todo add params
// Run to get Entropies and Median Level Set
// PCI track is being used for initial emtropy and median leveled
std::shared_ptr<vecOfNamedTrack_t>  ssmt_processor::run_contraction_pci (const std::vector<roiWindow<P8U>>& images,
                                                                         const input_channel_selector_t& in, const progress_fn_t& reporter)
{
    bool cache_ok = false;
    size_t dim = images.size();
    std::shared_ptr<ssResultContainer> ssref;
    auto cache_path = get_cache_location(in.channel(), in.region());
    if(fs::exists(cache_path)){
        ssref = ssResultContainer::create(cache_path);
    }
    cache_ok = ssref && ssref->size_check(dim);

    // Create a contraction object for entire view processing.
    // @todo: add params
    m_entireCaRef = contractionLocator::create (in, -1);
    std::vector<float> fout;
    
    if(cache_ok){
        vlogger::instance().console()->info(" SS result container cache : Hit ");
        std::vector<double> entmp;
        std::vector<std::vector<double>> smtmp;
        
        entmp.insert(entmp.end(), ssref->entropies().begin(),
                     ssref->entropies().end());
        fout.insert(fout.end(), entmp.begin(), entmp.end());
        m_entropies[in.region()] = entmp;
        const std::deque<deque<double>>& sm = ssref->smatrix();
        for (auto row : sm){
            vector<double> rowv;
            rowv.insert(rowv.end(), row.begin(), row.end());
            smtmp.push_back(rowv);
        }
        m_smat[in.region()] = smtmp;

        if( ! in.isEntire())
            m_results[in.region()]->locator()->load(m_entropies[in.region()], m_smat[in.region()]);
        else{
            m_entireCaRef->load(m_entropies[in.region()], m_smat[in.region()]);
        }
        update (in);
        
    }else{
        auto sp =  similarity_producer();
        sp->load_images (images);
//        std::packaged_task<bool(int,sm_producer::progress_fn_t)> task([sp](int a, const sm_producer::progress_fn_t& reporter){ return sp->operator()(a, reporter);}); // wrap the function
        
        std::future<bool>  future_ss = sp->launch_async(0, reporter);
//        std::thread(std::move(task)).join(); // Finish on a thread
  
        if (future_ss.get())
        {
            const deque<double>& entropies = sp->shannonProjection ();
            const std::deque<deque<double>>& sm = sp->similarityMatrix();
            assert(images.size() == entropies.size() && sm.size() == images.size());
            for (auto row : sm) assert(row.size() == images.size());
            
            std::vector<double> entmp;
            std::vector<std::vector<double>> smtmp;
            
            entmp.insert(entmp.end(), entropies.begin(), entropies.end());
            fout.insert(fout.end(), entmp.begin(), entmp.end());
            m_entropies[in.region()] = entmp;
            for (auto row : sm){
                vector<double> rowv;
                rowv.insert(rowv.end(), row.begin(), row.end());
                smtmp.push_back(rowv);
            }
            m_smat[in.region()] = smtmp;
            
            if(! in.isEntire() )
                m_results[in.region()]->locator()->load(m_entropies[in.region()] , m_smat[in.region()] );
            else{
                m_entireCaRef->load(m_entropies[in.region()] , m_smat[in.region()] );
            }
            update (in);
            bool ok = ssResultContainer::store(cache_path,m_entropies[in.region()] , m_smat[in.region()] );
            if(ok)
                vlogger::instance().console()->info(" SS result container cache : filled ");
            else
                vlogger::instance().console()->info(" SS result container cache : failed ");
            
        }
    }
    // Signal we are done with ACI
    if (signal_contraction_pci_ready && signal_contraction_pci_ready->num_slots() > 0){
        signal_contraction_pci_ready->operator()(fout, in);
    }
    
    return m_contraction_pci_tracksRef;
}


// channel_index which channel of multi-channel input. Usually visible channel is the last one
// input is -1 for the entire root or index of moving object area in results container

std::shared_ptr<vecOfNamedTrack_t>  ssmt_processor::run_contraction_pci_on_selected_input (const input_channel_selector_t& in, const progress_fn_t& reporter){
    auto cache_path = get_cache_location(in.channel(),in.region());
    if (cache_path == fs::path()){
        return std::shared_ptr<vecOfNamedTrack_t> ();
    }
    // protect fetching image data 
    std::lock_guard<std::mutex> lock(m_mutex);
    const auto& _content = in.isEntire() ? content()[in.channel()] : m_results[in.region()]->content()[in.channel()];
    return run_contraction_pci(std::move(_content), in, reporter);
}


void ssmt_processor::signal_geometry_done (int count, const input_channel_selector_t& in){
    auto msg = "ssmt_processor called geom cb " + toString(count);
    vlogger::instance().console()->info(msg);
}

/*
 * 1 monchrome channel. Compute 3D Standard Dev. per pixel
 * the atomic bool m_3d_stats_done is set to true
 */

void ssmt_processor::run_volume_variances (std::vector<roiWindow<P8U>>& images){
    std::lock_guard<std::mutex> lock(m_mutex);

    cv::Mat m_sum, m_sqsum;
    int image_count = 0;
    std::vector<std::thread> threads(1);
    threads[0] = std::thread(SequenceAccumulator(),std::ref(images),
                             std::ref(m_sum), std::ref(m_sqsum), std::ref(image_count), std::ref(m_3d_stats_done));
    
    std::for_each(threads.begin(), threads.end(), std::mem_fn(&std::thread::join));
    SequenceAccumulator::computeStdev(m_sum, m_sqsum, image_count, m_var_image);
    /*
     * Save Only local maximas in the var field
     */
    m_var_peaks.resize(0);
    svl::PeakDetect(m_var_image, m_var_peaks);
    cv::Mat tmp = m_var_image.clone();
    tmp = 0.0;
    for(cv::Point& peak : m_var_peaks){
        tmp.at<float>(peak.y,peak.x) = m_var_image.at<float>(peak.y,peak.x);
    }
    cv::normalize(tmp, m_var_image, 0, 255, NORM_MINMAX, CV_8UC1);
    m_3d_stats_done = true;
    
}


// Assumes LIF data -- use multiple window.
// @todo condider creating cv::Mats and convert to roiWindow when needed.

void ssmt_processor::load_channels_from_images (const std::shared_ptr<seqFrameContainer>& frames)
{
    // Copy deep time / index maps from seqFrameContainer 
    m_2TimeMap = indexToTime_t (frames->index2TimeMap());
    m_2IndexMap = timeToIndex_t (frames->time2IndexMap());
    
    m_frameCount = 0;
    m_all_by_channel.clear();
    m_channel_count = frames->media_info().getNumChannels();
    m_all_by_channel.resize (m_channel_count);
    std::vector<std::string> names = {"Red", "Green","Blue"};
    
    while (frames->checkFrame(m_frameCount))
    {
        auto su8 = frames->getFrame(m_frameCount);
        auto ts = m_frameCount;
        std::shared_ptr<roiWindow<P8U>> m1 = svl::NewRefSingleFromSurface (su8, names, ts);
        
        switch (m_channel_count)
        {
                // @todo support general case
                // ID Lab 3 vertical roi 512 x (128x3)
            case 3  :
            {
                auto m3 = roiFixedMultiWindow<P8UP3>(*m1, names, ts);
                for (auto cc = 0; cc < m3.planes(); cc++)
                    m_all_by_channel[cc].emplace_back(m3.plane(cc));
                break;
            }
            case 2  :
            {
                auto m2 = roiFixedMultiWindow<P8UP2,512,128,2>(*m1, names, ts);
                for (auto cc = 0; cc < m2.planes(); cc++)
                    m_all_by_channel[cc].emplace_back(m2.plane(cc));
                break;
            }
            case 1  :
            {
                m_all_by_channel[0].emplace_back(*m1);
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
void ssmt_processor::median_leveled_pci (namedTrack_t& track, const input_channel_selector_t& in)
{
    try{
        vector<double>& leveled = m_entropies[in.region()];
        std::weak_ptr<contractionLocator> weakCaPtr (in.isEntire() ? m_entireCaRef : m_results[in.region()]->locator());
        if (weakCaPtr.expired())
            return;
        auto caRef = weakCaPtr.lock();
        caRef->update ();
        leveled = caRef->leveled();

        track.second.clear();
        auto mee = leveled.begin();
        for (auto ss = 0; mee != leveled.end() || ss < frame_count(); ss++, mee++)
        {
            index_time_t ti;
            ti.first = ss;
            timedVal_t res;
            track.second.emplace_back(ti,*mee);
        }
    }
    catch(const std::exception & ex)
    {
        stringstream ss;
        ss << __FILE__  << ":::" << __LINE__ << ex.what();
        vlogger::instance().console()->info(ss.str());
    }
    // Signal we are done with median level set
    if (signal_sm1dmed_ready && signal_sm1dmed_ready->num_slots() > 0)
        signal_sm1dmed_ready->operator()(in);
}

const int64_t& ssmt_processor::frame_count () const
{
//    std::lock_guard<std::mutex> lock(m_mutex);
    static int64_t inconsistent (0);
    
    if (m_all_by_channel.empty()) return inconsistent;
    
    const auto cs = m_all_by_channel[0].size();
    if (cs != m_frameCount) return inconsistent;
    for (const auto cc : m_all_by_channel){
        if (cc.size() != cs) return inconsistent;
    }
    return m_frameCount;
}

const uint32_t ssmt_processor::channel_count () const
{
    return m_channel_count;
}

std::shared_ptr<ioImageWriter>& ssmt_processor::get_image_writer (){
    if (! m_image_writer){
        m_image_writer = std::make_shared<ioImageWriter>();
    }
    return m_image_writer;
}


std::shared_ptr<ioImageWriter>& ssmt_processor::get_csv_writer (){
    if (! m_csv_writer){
        m_csv_writer = std::make_shared<ioImageWriter>();
    }
    return m_csv_writer;
}

int ssmt_processor::save_channel_images (const input_channel_selector_t& in, const std::string& dir_fqfn){
    std::lock_guard<std::mutex> lock(m_mutex);
    fs::path _path (dir_fqfn);
    if (! fs::exists(_path)){
        return -1;
    }
    const auto& _content = in.isEntire() ? content() : m_results[in.region()]->content();
    int channel_to_use = in.channel() % m_channel_count;
    channel_images_t c2 = _content[channel_to_use];
    auto writer = get_image_writer();
    if (writer) {
        writer->operator()(dir_fqfn, c2);
    }

    return -1;
}

const std::vector<moving_region>& ssmt_processor::moving_regions ()const {
    return m_regions;
}

const ssmt_processor::channel_vec_t& ssmt_processor::content () const{
    return m_all_by_channel;
}
