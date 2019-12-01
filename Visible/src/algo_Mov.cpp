//
//  algo_sequence.cpp
//  Visible
//
//  Created by Arman Garakani on 8/20/18.
//


#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcomma"

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
#include "cinder_xchg.hpp"
#include "vision/histo.h"
#include "vision/opencv_utils.hpp"
#include "vision/localvariance.h"
#include "algo_Mov.hpp"
#include "logger.hpp"
//#include "cpp-perf.hpp"
#include "vision/localvariance.h"
#include "vision/labelBlob.hpp"
#include "result_serialization.h"

/****
 
 sequence implementation
 
 ****/

static std::string s_image_cache_name = "voxel_ss_.png";
static std::string s_result_container_cache_name = "container_ss_";


/****
 
 sequence_processor implementation
 
 ****/

sequence_processor::sequence_processor (const fs::path& serie_cache_folder):
mCurrentCachePath(serie_cache_folder)
{
    // Signals we provide
    signal_content_loaded = createSignal<sequence_processor::sig_cb_content_loaded>();
    signal_frame_loaded = createSignal<sequence_processor::sig_cb_frame_loaded>();
    signal_sm1d_available = createSignal<sequence_processor::sig_cb_sm1d_available>();
    signal_3dstats_available = createSignal<sequence_processor::sig_cb_3dstats_available>();
    signal_geometry_available = createSignal<sequence_processor::sig_cb_geometry_available>();
    signal_ss_image_available = createSignal<sequence_processor::sig_cb_ss_image_available>();
    
    // semilarity producer
    m_sm_producer = std::shared_ptr<sm_producer> ( new sm_producer () );
    
    // Signal us when 3d stats are ready
    std::function<void ()>_3dstats_done_cb = boost::bind (&sequence_processor::stats_3d_computed, this);
    boost::signals2::connection _3d_stats_connection = registerCallback(_3dstats_done_cb);
    
    // Signal us when ss segmentation is ready
    std::function<void (cv::Mat&)>_ss_image_done_cb = boost::bind (&sequence_processor::finalize_segmentation, this, _1);
    boost::signals2::connection _ss_image_connection = registerCallback(_ss_image_done_cb);
    
}

// @note Specific to ID Lab sequence Files
// @note Specific to ID Lab sequence Files
// 3 channels: 2 flu one visible
// 1 channel: visible
void sequence_processor::create_named_tracks (const std::vector<std::string>& names, const std::vector<std::string>& plot_names)
{
    m_longterm_pci_tracksRef = std::make_shared<vecOfNamedTrack_t> ();
    
    switch(names.size()){
        case 3:
        case 4:
            m_longterm_pci_tracksRef->resize (1);
            m_shortterm_pci_tracks.resize (1);
            m_longterm_pci_tracksRef->at(0).first = "Long Term"; // plot_names[2];
            m_shortterm_pci_tracks.at(0).first = "Short Term"; // plot_names[3];
            break;
        case 1:
            m_longterm_pci_tracksRef->resize (1);
            m_shortterm_pci_tracks.resize (1);
            m_longterm_pci_tracksRef->at(0).first = plot_names[0];
            m_shortterm_pci_tracks.at(0).first = plot_names[1];
            break;
        default:
            assert(false);
            
    }
}


const smProducerRef sequence_processor::similarity_producer () const {
    return m_sm_producer;
    
}

const fPair& sequence_processor::ellipse_ab () const { return m_ab; }




int64_t sequence_processor::load (const std::shared_ptr<seqFrameContainer>& frames,const std::vector<std::string>& names, const std::vector<std::string>& plot_names)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    create_named_tracks(names, plot_names);
    load_channels_from_images(frames);
    lock.unlock();
   // int channel_to_use = m_channel_count - 1;
  //  run_detect_geometry (channel_to_use);
    
    // Call the content loaded cb if any
    if (signal_content_loaded && signal_content_loaded->num_slots() > 0)
        signal_content_loaded->operator()(m_frameCount);
    
    return m_frameCount;
}


/*
 * 1 monchrome channel. Compute 3D Standard Dev. per pixel
 */

void sequence_processor::run_detect_geometry (std::vector<roiWindow<P8U>>& images){
    std::lock_guard<std::mutex> lock(m_mutex);
    m_3d_stats_done = false;
    
    std::vector<std::thread> threads(1);
    threads[0] = std::thread(&sequence_processor::generateVoxelSelfSimilarities_on_channel, this, 2, 3, 3);
    std::for_each(threads.begin(), threads.end(), std::mem_fn(&std::thread::join));
}

void sequence_processor::finalize_segmentation (cv::Mat& space){
    
    std::lock_guard<std::mutex> lock(m_segmentation_mutex);
    
    cv::Point replicated_pad (5,5);
    cv::Mat mono, bi_level;
    copyMakeBorder(space,mono, replicated_pad.x,replicated_pad.y,
                   replicated_pad.x,replicated_pad.y, BORDER_REPLICATE, 0);
    threshold(mono, bi_level, 126, 255, THRESH_BINARY | THRESH_OTSU);
    m_main_blob = labelBlob::create(mono, bi_level, 666);
    std::function<labelBlob::results_ready_cb> res_ready_lambda = [this](int64_t& cbi)
    {
        const std::vector<labelBlob::blob>& blobs = m_main_blob->results();
        
        
        
        if (! blobs.empty()){
            
            
            m_motion_mass_bottom = blobs[0].rotated_roi();
            m_motion_mass = blobs[0].rotated_roi();
            m_motion_mass.center.x -= 5.0;
            m_motion_mass.center.y -= 5.0;
            m_motion_mass.center.x *= m_voxel_sample.first;
            m_motion_mass.center.y *= m_voxel_sample.second;
            m_motion_mass.size.width *= m_voxel_sample.first;
            m_motion_mass.size.height *= m_voxel_sample.second;
            
            m_ab = blobs[0].moments().getEllipseAspect ();
            
            // Signal to listeners
            if (signal_geometry_available && signal_geometry_available->num_slots() > 0)
                signal_geometry_available->operator()();
        }
        
        
    };
 
    boost::signals2::connection results_ready_ = m_main_blob->registerCallback(res_ready_lambda);
    m_main_blob->run_async();
  

    
}

void sequence_processor::run_detect_geometry (const int channel_index){
    return run_detect_geometry(m_all_by_channel[channel_index]);
}

/*
 * 1 monchrome channel. Compute volume stats of each on a thread
 */

svl::stats<int64_t> sequence_processor::run_volume_sum_sumsq_count (std::vector<roiWindow<P8U>>& images){
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


svl::stats<int64_t> sequence_processor::run_volume_sum_sumsq_count (const int channel_index){
    return run_volume_sum_sumsq_count(m_all_by_channel[channel_index]);
}


void sequence_processor::stats_3d_computed(){
    
}



void sequence_processor::compute_shortterm (const uint32_t halfWinSz) const{
    
    uint32_t tWinSz = 2 * halfWinSz + 1;
    
    auto shannon = [](double r) { return (-1.0 * r * log2 (r)); };
    double _log2MSz = log2(tWinSz);
    
    
    for (uint32_t diag = halfWinSz; diag < (m_frameCount - halfWinSz); diag++) {
        // tWinSz x tWinSz centered at diag,diag putting out a readount at diag
        // top left at row, col
        auto tl_row = diag - halfWinSz; auto br_row = tl_row + tWinSz;
        auto tl_col = diag - halfWinSz; auto br_col = tl_col + tWinSz;
        
        // Sum rows in to _sums @note possible optimization by constant time implementation
        std::vector<double> _sums (tWinSz, 0);
        std::vector<double> _entropies (tWinSz, 0);
        for (auto row = tl_row; row < br_row; row++){
            auto proj_row = row - tl_row;
            _sums[proj_row] = m_smat[row][row];
        }
        
        for (auto row = tl_row; row < br_row; row++)
            for (auto col = (row+1); col < br_col; col++){
                auto proj_row = row - tl_row;
                auto proj_col = col - tl_col;
                _sums[proj_row] += m_smat[row][col];
                _sums[proj_col] += m_smat[row][col];
            }
        
        
        for (auto row = tl_row; row < br_row; row++)
            for (auto col = row; col < br_col; col++){
                auto proj_row = row - tl_row;
                auto proj_col = col - tl_col;
                double rr = m_smat[row][col]/_sums[proj_row]; // Normalize for total energy in samples
                _entropies[proj_row] += shannon(rr);
                
                if ((proj_row) != (proj_col)) {
                    rr = m_smat[row][col]/_sums[proj_col];//Normalize for total energy in samples
                    _entropies[proj_col] += shannon(rr);
                }
                _entropies[proj_row] = _entropies[proj_row]/_log2MSz;// Normalize for cnt of samples
            }
        
        std::lock_guard<std::mutex> lk(m_shortterms_mutex);
        m_shortterms.push(1.0f - _entropies[halfWinSz]);
        m_shortterm_cv.notify_one();
    }
}



void sequence_processor::shortterm_pci (const uint32_t& halfWinSz) {
    // Check if full sm has been done
    m_shortterm_pci_tracks.at(0).second.clear();
    for (auto pp = 0; pp < halfWinSz; pp++){
        timedVal_t res;
        index_time_t ti;
        ti.first = pp;
        res.first = ti;
        res.second = -1.0f;
        m_shortterm_pci_tracks.at(0).second.emplace_back(res);
    }
    
    compute_shortterm(halfWinSz);
    //    auto twinSz = 2 * halfWinSz + 1;
    auto ii = halfWinSz;
    auto last = m_frameCount - halfWinSz;
    while(true){
        std::unique_lock<std::mutex> lock( m_shortterms_mutex);
        m_shortterm_cv.wait(lock,[this]{return !m_shortterms.empty(); });
        float val = m_shortterms.front();
        timedVal_t res;
        m_shortterms.pop();
        lock.unlock();
        index_time_t ti;
        ti.first = ii++;
        res.first = ti;
        res.second = static_cast<float>(val);
        m_shortterm_pci_tracks.at(0).second.emplace_back(res);
        //        std::cout << m_shortterm_pci_tracks.at(0).second.size() << "," << m_shortterms.size() << std::endl;
        
        if(ii == last)
            break;
    }
    // Fill the pad in front with first valid read
    for (auto pp = 0; pp < halfWinSz; pp++){
        m_shortterm_pci_tracks.at(0).second.at(pp) = m_shortterm_pci_tracks.at(0).second.at(halfWinSz);
    }
    // Fill the pad in the back with the last valid read
    for (auto pp = 0; pp < halfWinSz; pp++){
        m_shortterm_pci_tracks.at(0).second.emplace_back (m_shortterm_pci_tracks.at(0).second.back());
    }
    
}


// Note tracks contained timed data.
void  sequence_processor::fill_longterm_pci (namedTrack_t& track)
{
    try{
        track.second.clear();
        auto mee = m_entropies.begin();
        assert(m_entropies.size() == m_frameCount);
        for (auto ss = 0; mee != m_entropies.end() || ss < m_frameCount; ss++, mee++)
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
        return;
    }
    // Signal we are done with ACI
    static int dummy;
    if (signal_sm1d_available && signal_sm1d_available->num_slots() > 0)
        signal_sm1d_available->operator()(dummy);

}


// Run to get Entropies and Median Level Set
// PCI track is being used for initial emtropy and median leveled
std::shared_ptr<vecOfNamedTrack_t>  sequence_processor::run_longterm_pci (const std::vector<roiWindow<P8U>>& images)
{
    bool cache_ok = false;
    size_t dim = images.size();
    std::shared_ptr<ssResultContainer> ssref;
    
    if(fs::exists(mCurrentCachePath)){
        auto cache_path = mCurrentCachePath / s_result_container_cache_name;
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
        update ();
        
    }else{
        
        auto cache_path = mCurrentCachePath / s_result_container_cache_name;
        auto sp =  similarity_producer();
        sp->load_images (images);
        std::packaged_task<bool()> task([sp](){ return sp->operator()(0);}); // wrap the function
        std::future<bool>  future_ss = task.get_future();  // get a future
        std::thread(std::move(task)).join(); // Finish on a thread
        if (future_ss.get())
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            
            const deque<double>& entropies = sp->shannonProjection ();
            const std::deque<deque<double>>& sm = sp->similarityMatrix();
            m_entropies.assign(entropies.begin(), entropies.end());
            vector<double> rowv;
            for (auto row : sm){
                rowv.assign(row.begin(), row.end());
                m_smat.push_back(rowv);
            }
            update();
            
            std::async(std::launch::async, [&](){
                bool ok = ssResultContainer::store(cache_path, m_entropies, m_smat);
                if(ok)
                    vlogger::instance().console()->info(" SS result container cache : filled ");
                else
                    vlogger::instance().console()->info(" SS result container cache : failed ");
            });
        }
    }
 
    return m_longterm_pci_tracksRef;
}

// @todo even though channel index is an argument, but only results for one channel is kept in sequence_processor.
// 
std::shared_ptr<vecOfNamedTrack_t>  sequence_processor::run_longterm_pci_on_channel (const int channel_index)
{
    return run_longterm_pci(std::move(m_all_by_channel[channel_index]));
}


const std::vector<Rectf>& sequence_processor::channel_rois () const { return m_channel_rois; }


const cv::RotatedRect& sequence_processor::motion_surface() const { return m_motion_mass; }
const cv::RotatedRect& sequence_processor::motion_surface_bottom() const { return m_motion_mass_bottom; }


// Update.
void sequence_processor::update ()
{
  if(m_longterm_pci_tracksRef && !m_longterm_pci_tracksRef->empty())
   fill_longterm_pci(m_longterm_pci_tracksRef->at(0));
}





// Assumes sequence data -- use multiple window.
// @todo condider creating cv::Mats and convert to roiWindow when needed.

void sequence_processor::load_channels_from_images (const std::shared_ptr<seqFrameContainer>& frames)
{
    m_frameCount = 0;
    m_all_by_channel.clear();
    m_channel_count = frames->media_info().getNumChannels();
    m_all_by_channel.resize (m_channel_count);
    m_channel_rois.resize (0);
    std::vector<std::string> names = {"Blue", "Green","Red", "Alpha"};
    
    //@todo: seperate channels and add
    while (frames->checkFrame(m_frameCount))
    {
        auto su8 = frames->getFrame(m_frameCount);
        auto red = svl::NewRedFromSurface(su8);
        m_all_by_channel[0].emplace_back(red);
        m_all_by_channel[1].emplace_back(red);
        m_all_by_channel[2].emplace_back(red);
        m_all_by_channel[3].emplace_back(red);

        if (m_channel_rois.empty())
        {
            for (auto cc = 0; cc < 4; cc++)
            {
                const auto ir = su8->getBounds();
                
                m_channel_rois.emplace_back(ir.getX1(), ir.getY1(), ir.getX2(),ir.getY2());
            }
        }
        m_frameCount++;
    }
}



const int64_t& sequence_processor::frame_count () const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_frameCount;
}

const int64_t sequence_processor::channel_count () const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_channel_count;
}

std::shared_ptr<OCVImageWriter>& sequence_processor::get_image_writer (){
    if (! m_writer){
        m_writer = std::make_shared<OCVImageWriter>();
    }
    return m_writer;
}

void sequence_processor::save_channel_images (int channel_index, std::string& dir_fqfn){
    std::lock_guard<std::mutex> lock(m_mutex);
    int channel_to_use = channel_index % m_channel_count;
    channel_images_t c2 = m_all_by_channel[channel_to_use];
    auto writer = get_image_writer();
    if (writer)
        writer->operator()(dir_fqfn, c2);
    
}


// Return 2D latice of pixels over time
void sequence_processor::generateVoxels_on_channel (const int channel_index, std::vector<std::vector<roiWindow<P8U>>>& rvs,
                                                     uint32_t sample_x, uint32_t sample_y){
    std::lock_guard<std::mutex> lock(m_mutex);
    generateVoxels(m_all_by_channel[channel_index], rvs, sample_x, sample_y);
}


// Return 2D latice of voxel self-similarity

void sequence_processor::generateVoxelSelfSimilarities_on_channel (const int channel_index, uint32_t sample_x, uint32_t sample_y){
    
    bool cache_ok = false;
    m_voxel_sample.first = sample_x;
    m_voxel_sample.second = sample_y;
    //@todo add width and height so we do not have to do this
    auto images = m_all_by_channel[channel_index];
    assert(!images.empty());
    auto expected_width = sample_x / 2 + images[0].width() / sample_x;
    auto expected_height = sample_y / 2 + images[0].height() / sample_y;
    if(fs::exists(mCurrentCachePath)){
        auto image_path = mCurrentCachePath / s_image_cache_name;
        if(fs::exists(image_path)){
            try{
                m_temporal_ss = imread(image_path.string(), IMREAD_GRAYSCALE);
            } catch( std::exception& ex){
                auto msg = s_image_cache_name + " Exists but could not be read ";
                vlogger::instance().console()->info(msg);
            }
        }
        cache_ok = expected_width == m_temporal_ss.cols;
        cache_ok = cache_ok && expected_height == m_temporal_ss.rows;
    }
    if ( cache_ok ){
        // Call the content loaded cb if any
        if (signal_ss_image_available && signal_ss_image_available->num_slots() > 0)
            signal_ss_image_available->operator()(m_temporal_ss);
    }
    else {
        std::vector<std::vector<roiWindow<P8U>>> rvs;
        std::string msg = " Generating Voxels @ (" + to_string(sample_x) + "," + to_string(sample_y) + ")";
     //   vlogger::instance().console()->info("starting " + msg);
        generateVoxels(m_all_by_channel[channel_index], rvs, sample_x, sample_y);
        vlogger::instance().console()->info("finished " + msg);
        generateVoxelSelfSimilarities(rvs);
    }
    
}



void sequence_processor::generateVoxels (const std::vector<roiWindow<P8U>>& images,
                                          std::vector<std::vector<roiWindow<P8U>>>& output,
                                          uint32_t sample_x, uint32_t sample_y){
    output.resize(0);
    size_t t_d = images.size();
    uint32_t width = images[0].width();
    uint32_t height = images[0].height();
    
    for (auto row = 0; row < height; row+=sample_y){
        std::vector<roiWindow<P8U>> row_bufs;
        for (auto col = 0; col < width; col+=sample_x){
            std::vector<uint8_t> voxel(t_d);
            for (auto tt = 0; tt < t_d; tt++){
                voxel[tt] = images[tt].getPixel(col, row);
            }
            row_bufs.emplace_back(voxel);
        }
        output.emplace_back(row_bufs);
    }
}



void sequence_processor::generateVoxelSelfSimilarities (std::vector<std::vector<roiWindow<P8U>>>& voxels){
    int height = static_cast<int>(voxels.size());
    int width = static_cast<int>(voxels[0].size());
    
    
    vlogger::instance().console()->info("starting generating voxel self-similarity");
    // Create a single vector of all roi windows
    std::vector<roiWindow<P8U>> all;
    auto sp =  similarity_producer();
    for(std::vector<roiWindow<P8U>>& row: voxels){
        for(roiWindow<P8U>& voxel : row){
            all.emplace_back(voxel.frameBuf(), voxel.bound());
        }
    }
    
    sp->load_images (all);
    std::packaged_task<bool()> task([sp](){ return sp->operator()(0);}); // wrap the function
    std::future<bool>  future_ss = task.get_future();  // get a future
    std::thread(std::move(task)).join(); // Finish on a thread
    vlogger::instance().console()->info("dispatched voxel self-similarity");
    if (future_ss.get())
    {
        vlogger::instance().console()->info("copying results of voxel self-similarity");
        //@todo remove extra copy here
        cv::Mat m_temporal_ss (height,width, CV_8UC1);
        m_temporal_ss.setTo(0);
        
        const deque<double>& entropies = sp->shannonProjection ();
        vector<float> m_entropies;
        m_entropies.insert(m_entropies.end(), entropies.begin(), entropies.end());
        vector<float>::const_iterator start = m_entropies.begin();
        for (auto row =0; row < height; row++){
            uint8_t* row_ptr = m_temporal_ss.ptr<uint8_t>(row);
            auto end = start + width; // point to one after
            for (; start < end; start++, row_ptr++){
                if(std::isnan(*start)) continue; // nan is set to 1, the pre-set value
                *row_ptr = uint8_t((*start)*255);
            }
            start = end;
        }
        cv::medianBlur(m_temporal_ss, m_temporal_ss, 5);
        
        vlogger::instance().console()->info("finished voxel self-similarity");
        if(fs::exists(mCurrentCachePath)){
            std::string imagename = "voxel_ss_.png";
            auto image_path = mCurrentCachePath / imagename;
            cv::imwrite(image_path.string(), m_temporal_ss);
        }
        
   
        
        
    }
    
}


#pragma GCC diagnostic pop


