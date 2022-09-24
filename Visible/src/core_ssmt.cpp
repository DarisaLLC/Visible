//
//  core_ssmt.cpp
//  Visible
//
//  Created by Arman Garakani on 5/26/19.
//

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcomma"
#pragma GCC diagnostic ignored "-Wunused-private-field"
#pragma GCC diagnostic ignored "-Wint-in-bool-context"
#pragma GCC diagnostic ignored "-Wshorten-64-to-32"
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#pragma GCC diagnostic ignored "-Wunused-local-typedef"

#define float16_t opencv_broken_float16_t
#include "opencv2/stitching.hpp"
#undef float16_t

#include <stdio.h>
#include <iostream>
#include <string>
#include <vector>
#include <deque>
#include <sstream>
#include <map>
#include "oiio_utils.hpp"
#include "timed_types.h"
#include "core/signaler.h"
#include "sm_producer.h"
#include "vision/histo.h"
#include "vision/opencv_utils.hpp"
#include "ssmt.hpp"
#include "logger/logger.hpp"
#include "result_serialization.h"


/**
 ssmt_processor
 
 @param path to the Cache Folder
 
 */
ssmt_processor::ssmt_processor (const mediaSpec& ms, const bfs::path& serie_cache_folder,  const ssmt_processor::params& params):
mCurrentCachePath(serie_cache_folder), m_params(params), m_media_spec(ms)
{
    // Signals we provide
    signal_content_loaded = createSignal<ssmt_processor::sig_cb_content_loaded>();
    intensity_over_time_ready = createSignal<ssmt_processor::sig_intensity_over_time_ready>();
    signal_frame_loaded = createSignal<ssmt_processor::sig_cb_frame_loaded>();
    signal_root_pci_ready = createSignal<ssmt_processor::sig_cb_root_pci_ready>();
    signal_root_pci_med_reg_ready = createSignal<ssmt_processor::sig_cb_root_pci_median_regularized_ready>();
    signal_contraction_ready = createSignal<ssmt_processor::sig_cb_contraction_ready>();
    signal_volume_ready = createSignal<ssmt_processor::sig_cb_volume_ready>();
    signal_geometry_ready = createSignal<ssmt_processor::sig_cb_geometry_ready>();
    signal_segmented_view_ready = createSignal<ssmt_processor::sig_cb_segmented_view_ready>();
    signal_ss_voxel_ready = createSignal<ssmt_processor::sig_cb_ss_voxel_ready>();
    
    // semilarity producer
    m_sm_producer = std::shared_ptr<sm_producer> ( new sm_producer () );
	m_leveler.initialize(params.channel_to_use_root());

	
    // Signal us when volume stats are ready
    std::function<void ()>_volume_done_cb = boost::bind (&ssmt_processor::volume_stats_computed, this);
    boost::signals2::connection _volume_stats_connection = registerCallback(_volume_done_cb);
    
    // Signal us when ss segmentation surface is ready
    std::function<sig_cb_ss_voxel_ready> _ss_voxel_done_cb = boost::bind (&ssmt_processor::create_voxel_surface,this, boost::placeholders::_1);
    boost::signals2::connection _ss_voxel_connection = registerCallback(_ss_voxel_done_cb);

    // Signal us when ss segmentation is ready
    std::function<sig_cb_segmented_view_ready> _ss_segmentation_done_cb = boost::bind (&ssmt_processor::finalize_segmentation,
                                                                                       this, boost::placeholders::_1, boost::placeholders::_2);
    boost::signals2::connection _ss_image_connection = registerCallback(_ss_segmentation_done_cb);
    
    // Support lifProcessor::geometry_ready
    std::function<void (int, const result_index_channel_t&)> geometry_ready_cb = boost::bind (&ssmt_processor::signal_geometry_done, this, boost::placeholders::_1, boost::placeholders::_2);
    boost::signals2::connection geometry_connection = registerCallback(geometry_ready_cb);
}

// @note Specific to ID Lab Lif Files
// @note Specific to ID Lab Lif Files
// 3 channels: 2 flu one visible
// 1 channel: visible


const smProducerRef ssmt_processor::similarity_producer () const {
    return m_sm_producer;
    
}


bfs::path ssmt_processor::get_cache_location (const int channel_index,const int input){
    // Check input
    bool isEntire = input == -1;
    bool isMobj = input >= 0 && input < m_results.size();
    // Check index
    bool channel_index_ok = channel_index >= 0 && channel_index < channel_count();
    if (isEntire == isMobj || ! channel_index_ok)
        return bfs::path ();
    
    // Get Cache path
    auto cache_path = mCurrentCachePath;
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
        if(bfs::exists(mCurrentCachePath)){
            std::string subdir = to_string(sr->id());
            auto save_path = mCurrentCachePath / subdir;
            boost::system::error_code ec;
            if(!bfs::exists(save_path)){
                bfs::create_directory(save_path, ec);
                
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
void ssmt_processor::load_channels_from_ImageBuf(const std::shared_ptr<ImageBuf>& frames,const ustring& contentName, const mediaSpec& mspec)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    m_loaded_spec = mspec;
    m_voxel_sample.first = m_params.voxel_sample().first;
    m_expected_segmented_size.first = mspec.getSectionSize().first / m_params.voxel_sample().first;
    m_voxel_sample.second = m_params.voxel_sample().second;
    m_expected_segmented_size.second = mspec.getSectionSize().second / m_params.voxel_sample().second;
    
    /*
     Creates named tracks @todo move out of here
     Loads channels from images @note uses last channel for visible processing
     i.e 2nd channel from 2 channel LIF file and 3rd channel from a 3 channel LIF file or media file
     
     */
    internal_load_channels_from_lif_buffer2d(frames, contentName, mspec);
    lock.unlock();
    
    // Call the content loaded cb if any
    if (signal_content_loaded && signal_content_loaded->num_slots() > 0)
        signal_content_loaded->operator()(m_frameCount);
	
	// Dispatch a thread to perform ss on entire -- root -- image
	result_index_channel_t entire(-1,0);
	assert(entire.isEntire());
	auto ss_thread = std::thread(&ssmt_processor::run_selfsimilarity_on_selected_input,this, entire,nullptr);
	ss_thread.detach();
	
}

// @todo condider creating cv::Mats and convert to roiWindow when needed.
// @todo consider passing ImageBuf to similarity so that it can fetch image directly and does not need all images in memory
// 16bit is converted to 8 bit for now using normalize

void ssmt_processor::internal_load_channels_from_lif_buffer2d (const std::shared_ptr<ImageBuf>& frames, const ustring& contentName,
                                                      const mediaSpec& mspec)
{
    m_frameCount = 0;
    m_all_by_channel.clear();
    m_channel_count = mspec.getSectionCount();
    m_all_by_channel.resize (m_channel_count);
    
    auto nsubs = frames->nsubimages();
    int width = mspec.getSectionSize().first;
    int height = mspec.getSectionSize().second;
    
    auto format = m_params.content_type();
    if (format == TypeUInt8){
            for (auto ii = 0; ii < nsubs; ii++){
                auto cvb = getRootFrame(frames, contentName, ii);
                assert(cvb.type() == CV_8U);
                roiWindow<P8U> r8;
                cpCvMatToRoiWindow8U (cvb, r8);
                
                for (auto cc = 0; cc < mspec.getSectionCount(); cc++){
                    auto tl_f_x = mspec.getROIxRanges()[cc][0];
                    auto tl_f_y = mspec.getROIyRanges()[cc][0];
                    m_all_by_channel[cc].emplace_back(r8.frameBuf(),tl_f_x,tl_f_y,width,height);
                }
                m_frameCount++;
            }
    }
    else if (format == TypeUInt16){
            for (auto ii = 0; ii < nsubs; ii++){
                auto cvb = getRootFrame(frames, contentName, ii);
                assert(cvb.type() == CV_16U);
                cv::Mat cvb8 (cvb.rows, cvb.cols, CV_8U);
                cv::normalize(cvb, cvb8, 0, 255, NORM_MINMAX, CV_8UC1);
                roiWindow<P8U> r8;
                cpCvMatToRoiWindow8U (cvb8, r8);
                
                for (auto cc = 0; cc < mspec.getSectionCount(); cc++){
                    auto tl_f_x = mspec.getROIxRanges()[cc][0];
                    auto tl_f_y = mspec.getROIyRanges()[cc][0];
                    m_all_by_channel[cc].emplace_back(r8.frameBuf(),tl_f_x,tl_f_y,width,height);
                }
                m_frameCount++;
            }
    }
    else{
        assert(false);
    }
}


/*
 * 1 monchrome channel. Compute volume stats of each on a thread
 */

svl::stats<int64_t> ssmt_processor::run_volume_stats (std::vector<roiWindow<P8U>>& images){
    std::lock_guard<std::mutex> lock(m_mutex);
    
    std::vector<std::tuple<int64_t,int64_t,uint32_t>> cts;
    std::vector<std::tuple<uint8_t,uint8_t>> rts;
    std::vector<std::thread> threads(1);
    threads[0] = std::thread(IntensityStatisticsPartialRunner(),std::ref(images), std::ref(cts), std::ref(rts));
    std::for_each(threads.begin(), threads.end(), std::mem_fn(&std::thread::join));
    auto res = std::accumulate(cts.begin(), cts.end(), std::make_tuple(int64_t(0),int64_t(0), uint32_t(0)), stl_utils::tuple_sum<int64_t,uint32_t>());
    auto mes = std::accumulate(rts.begin(), rts.end(), std::make_tuple(uint8_t(255),uint8_t(0)), stl_utils::tuple_minmax<uint8_t, uint8_t>());
    m_volume_stats = svl::stats<int64_t> (std::get<0>(res), std::get<1>(res), std::get<2>(res), int64_t(std::get<0>(mes)), int64_t(std::get<1>(mes)));
    
    // Signal to listeners
    if (signal_volume_ready && signal_volume_ready->num_slots() > 0)
        signal_volume_ready->operator()();
    return m_volume_stats;
    
}


svl::stats<int64_t> ssmt_processor::run_volume_stats (const int channel_index){
    return run_volume_stats(m_all_by_channel[channel_index]);
}


void ssmt_processor::volume_stats_computed(){
    
}
/*
 * 2 flu channels. Compute stats of each using its own threaD
 */

/**
 run_flu_statistics
 
 @param channels vector of channels containing fluorescence images
 @return vecor of named tracks containing of output time series
 */
//std::shared_ptr<vecOfNamedTrack_t> ssmt_processor::run_intensity_statistics (const std::vector<int>& channels)
//{
//    vlogger::instance().console()->info("run_intensity_stats started ");
//    std::vector<std::thread> threads(channels.size());
//    for (auto tt = 0; tt < channels.size(); tt++)
//    {
//        auto channel = channels[tt];
//        threads[tt] = std::thread(IntensityStatisticsRunner(),
//                                  std::ref(m_all_by_channel[channel]), std::ref(m_moment_tracksRef->at(channel).second));
//    }
//    std::for_each(threads.begin(), threads.end(), std::mem_fn(&std::thread::join));
//
//    vlogger::instance().console()->info("run_intensity_stats ended ");
//
//    // Call the content loaded cb if any
//    if (intensity_over_time_ready && intensity_over_time_ready->num_slots() > 0)
//        intensity_over_time_ready->operator()();
//
//    return m_moment_tracksRef;
//}

// This is called from GUI side.
// @todo add params
// Run to get Entropies and Median Level Set
// PCI track is being used for initial emtropy and median leveled
void ssmt_processor::internal_run_selfsimilarity_on_selected_input (const std::vector<roiWindow<P8U>>& images,
                                                                    const result_index_channel_t& in,
                                                                    const progress_fn_t& reporter)
{
    bool cache_ok = false;
    size_t dim = images.size();
    std::string ss = " internal run ss started " + toString(in.region());
    vlogger::instance().console()->info(ss);
    std::shared_ptr<ssResultContainer> ssref;
    auto cache_path = get_cache_location(in.section(), in.region());
    if(bfs::exists(cache_path)){
        ssref = ssResultContainer::create(cache_path);
    }
    cache_ok = ssref && ssref->size_check(dim);
    
    // Create a contraction object for entire view processing.
    // @todo: add params
	m_entropies.clear();
	m_entropies_F.clear();
	m_smat.clear();
    
    if(cache_ok){
        vlogger::instance().console()->info(" SS result container cache : Hit ");
		m_entropies.insert(m_entropies.end(), ssref->entropies().begin(), ssref->entropies().end());
		for (auto row : ssref->smatrix()){
			vector<double> rowv;
			rowv.insert(rowv.end(), row.begin(), row.end());
			m_smat.push_back(rowv);
		}
    }else{
        auto sp =  similarity_producer();
        sp->load_images (images);
        std::future<bool>  future_ss = sp->launch_async(0, reporter);
        vlogger::instance().console()->info(" async ss submitted ");
        if (future_ss.get()){
            vlogger::instance().console()->info(" async ss finished ");
			m_entropies.insert(m_entropies.end(), sp->shannonProjection ().begin(),sp->shannonProjection ().end());
			for (auto row : sp->similarityMatrix()){
				vector<double> rowv;
				rowv.insert(rowv.end(), row.begin(), row.end());
				m_smat.push_back(rowv);
			}
        }
    }
    m_entropies_F.insert(m_entropies_F.end(), m_entropies.begin(), m_entropies.end());
	m_leveler.load(m_entropies, m_smat);
	
	bool ok = ssResultContainer::store(cache_path,m_entropies , m_smat );
	if(ok)
		vlogger::instance().console()->info(" SS result container cache : filled ");
	else
		vlogger::instance().console()->info(" SS result container cache : failed ");
	
    assert(images.size() == m_entropies.size() && m_smat.size() == images.size());
    for (auto row : m_smat) assert(row.size() == images.size());
    assert(images.size() == m_entropies_F.size());
    
    // Signal we are done with ACI
    if (signal_root_pci_ready && signal_root_pci_ready->num_slots() > 0){
        signal_root_pci_ready->operator()(m_entropies_F, in);
    }
}


// channel_index which channel of multi-channel input. Usually visible channel is the last one
// input is -1 for the entire root or index of moving object area in results container

void ssmt_processor::run_selfsimilarity_on_selected_input (const result_index_channel_t& in, const progress_fn_t& reporter){
    // protect fetching image data
    std::lock_guard<std::mutex> lock(m_mutex);
    const auto& _content = in.isEntire() ? content()[in.section()] : m_results[in.region()]->content()[in.section()];
    internal_run_selfsimilarity_on_selected_input(std::move(_content), in, reporter);
}


void ssmt_processor::signal_geometry_done (int count, const result_index_channel_t& in){
    auto msg = "ssmt_processor called geom cb " + toString(count);
    vlogger::instance().console()->info(msg);
}

/* UnUsed
 * 1 monchrome channel. Compute 3D Standard Dev. per pixel
 * the atomic bool m_3d_stats_done is set to true
 */

void ssmt_processor::volume_variance_peak_promotion (std::vector<roiWindow<P8U>>& images){
    std::lock_guard<std::mutex> lock(m_mutex);
    
    cv::Mat m_sum, m_sqsum;
    int image_count = 0;
    std::vector<std::thread> threads(1);
    threads[0] = std::thread(SequenceAccumulator(),std::ref(images),
                             std::ref(m_sum), std::ref(m_sqsum), std::ref(image_count), std::ref(m_variance_peak_detection_done));
    
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
    m_variance_peak_detection_done = true;
    
}

/**
 @Check if needed
 median_leveled_pci
 Note tracks contained timed data.
 Each call to find_best can be with different median cut-off
 @param track track to be filled
 */
//void ssmt_processor::median_leveled_pci (namedTrack_t& track, const input_section_selector_t& in)
//{
//    try{
//        vector<double>& leveled = m_entropies[in.region()];
//        std::weak_ptr<contractionLocator> weakCaPtr (in.isEntire() ? m_entireCaRef : m_results[in.region()]->locator());
//        if (weakCaPtr.expired())
//            return;
//        auto caRef = weakCaPtr.lock();
//        caRef->update ();
//        leveled = caRef->leveled();
//        
//        track.second.clear();
//        auto mee = leveled.begin();
//        for (auto ss = 0; mee != leveled.end() || ss < frame_count(); ss++, mee++)
//        {
//            index_time_t ti;
//            ti.first = ss;
//            timedVal_t res;
//            track.second.emplace_back(ti,*mee);
//        }
//    }
//    catch(const std::exception & ex)
//    {
//        stringstream ss;
//        ss << __FILE__  << ":::" << __LINE__ << ex.what();
//        vlogger::instance().console()->info(ss.str());
//    }
//    // Signal we are done with median level set
//    if (signal_root_pci_med_reg_ready && signal_root_pci_med_reg_ready->num_slots() > 0)
//        signal_root_pci_med_reg_ready->operator()(in);
//}

const int64_t& ssmt_processor::frame_count () const
{
    //    std::lock_guard<std::mutex> lock(m_mutex);
    static int64_t inconsistent (0);
    
    if (m_all_by_channel.empty()) return inconsistent;
    
    const auto cs = m_all_by_channel[0].size();
    if (cs != m_frameCount) return inconsistent;
    for (const auto &cc : m_all_by_channel){
        if (cc.size() != cs) return inconsistent;
    }
    return m_frameCount;
}

const uint32_t ssmt_processor::channel_count () const
{
    return m_channel_count;
}


const std::vector<moving_region>& ssmt_processor::moving_regions ()const {
    return m_regions;
}

const ssmt_processor::channel_vec_t& ssmt_processor::content () const{
    return m_all_by_channel;
}


#ifdef notYet

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

int ssmt_processor::save_channel_images (const input_section_selector_t& in, const std::string& dir_fqfn){
    std::lock_guard<std::mutex> lock(m_mutex);
    bfs::path _path (dir_fqfn);
    if (! bfs::exists(_path)){
        return -1;
    }
    const auto& _content = in.isEntire() ? content() : m_results[in.region()]->content();
    int channel_to_use = in.section() % m_channel_count;
    channel_images_t c2 = _content[channel_to_use];
    auto writer = get_image_writer();
    if (writer) {
        writer->operator()(dir_fqfn, c2);
    }
    
    return -1;
}

#endif

