//
//  algo_Lif.cpp
//  Visible
//
//  Created by Arman Garakani on 8/20/18.
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
#include "cinder_xchg.hpp"
#include "vision/histo.h"
#include "vision/opencv_utils.hpp"
#include "algo_cardiac.hpp"
#include "contraction.hpp"
#include "vision/localvariance.h"
#include "algo_Lif.hpp"
#include "logger/logger.hpp"
#include "cpp-perf.hpp"
#include "vision/localvariance.h"
#include "vision/labelBlob.hpp"
#include "result_serialization.h"

using namespace std;




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
    
    if(m_dimensions[0] == 512 && m_dimensions[1] == 128 && m_channels.size() == 3 &&
       serie_ref->content_type() == ""){
        m_contentType = "IDLab_0";
        serie_ref->set_content_type(m_contentType);
    }
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
                                                                   this, _1, _2);
    boost::signals2::connection _ss_image_connection = registerCallback(_ss_image_done_cb);
    
    // Signal us when ss segmentation is ready
    std::function<sig_cb_ss_voxel_available> _ss_voxel_done_cb = boost::bind (&lif_serie_processor::create_voxel_surface, this, _1);
    boost::signals2::connection _ss_voxel_connection = registerCallback(_ss_voxel_done_cb);
    
}

// @note Specific to ID Lab Lif Files
// @note Specific to ID Lab Lif Files
// 3 channels: 2 flu one visible
// 1 channel: visible
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

const fPair& lif_serie_processor::ellipse_ab () const { return m_ab; }

const Rectf& lif_serie_processor::measuredArea() const { return m_measured_area; }

// Check if the returned has expired
std::weak_ptr<contraction_analyzer> lif_serie_processor::contractionWeakRef ()
{
    std::weak_ptr<contraction_analyzer> wp (m_caRef);
    return wp;
}



const std::vector<sides_length_t>& lif_serie_processor::cell_ends() const{
    return m_cell_ends;
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
 * 1 monchrome channel. Compute 3D Standard Dev. per pixel
 */

void lif_serie_processor::run_detect_geometry (std::vector<roiWindow<P8U>>& images){
    std::lock_guard<std::mutex> lock(m_mutex);
    m_3d_stats_done = false;
    
    std::vector<std::thread> threads(1);
    threads[0] = std::thread(&lif_serie_processor::generateVoxelsAndSelfSimilarities, this, images);
    std::for_each(threads.begin(), threads.end(), std::mem_fn(&std::thread::join));
}

void lif_serie_processor::finalize_segmentation (cv::Mat& mono, cv::Mat& bi_level){
    
    std::lock_guard<std::mutex> lock(m_segmentation_mutex);
 
    m_main_blob = labelBlob::create(mono, bi_level, m_params.min_seqmentation_area(), 666);
    std::function<labelBlob::results_ready_cb> res_ready_lambda = [this](int64_t& cbi)
    {
        const std::vector<blob>& blobs = m_main_blob->results();
        if (! blobs.empty()){
            m_motion_mass = blobs[0].rotated_roi();
            std::vector<cv::Point2f> mid_points;
            svl::get_mid_points(m_motion_mass, mid_points);

            auto two_pt_dist = [mid_points](int a, int b){
                const cv::Point2f& pt1 = mid_points[a];
                const cv::Point2f& pt2 = mid_points[b];
                return sqrt(pow(pt1.x-pt2.x,2)+pow(pt1.y-pt2.y,2));
            };
            
            if (mid_points.size() == 4){
                float dists[2];
                float d = two_pt_dist(0,2);
                dists[0] = d;
                d = two_pt_dist(1,3);
                dists[1] = d;
                if (dists[0] >= dists[1]){
                    m_cell_ends[0].first = vec2(mid_points[0].x,mid_points[0].y);
                    m_cell_ends[0].second = vec2(mid_points[2].x,mid_points[2].y);
                    m_cell_ends[1].first = vec2(mid_points[1].x,mid_points[1].y);
                    m_cell_ends[1].second = vec2(mid_points[3].x,mid_points[3].y);
                }
                else{
                    m_cell_ends[1].first = vec2(mid_points[0].x,mid_points[0].y);
                    m_cell_ends[1].second = vec2(mid_points[2].x,mid_points[2].y);
                    m_cell_ends[0].first = vec2(mid_points[1].x,mid_points[1].y);
                    m_cell_ends[0].second = vec2(mid_points[3].x,mid_points[3].y);
                }
            }
            m_ab = blobs[0].moments().getEllipseAspect ();
            
            // Signal to listeners
            if (signal_geometry_available && signal_geometry_available->num_slots() > 0)
                signal_geometry_available->operator()();
        }
        
        
    };
 
    boost::signals2::connection results_ready_ = m_main_blob->registerCallback(res_ready_lambda);
    m_main_blob->run_async();
  

    
}

void lif_serie_processor::run_detect_geometry (const int channel_index){
    return run_detect_geometry(m_all_by_channel[channel_index]);
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



void lif_serie_processor::compute_shortterm (const uint32_t halfWinSz) const{
    
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



void lif_serie_processor::shortterm_pci (const uint32_t& halfWinSz) {
    
    
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


const cv::RotatedRect& lif_serie_processor::motion_surface() const { return m_motion_mass; }
const cv::RotatedRect& lif_serie_processor::motion_surface_bottom() const { return m_motion_mass_top; }


// Update. Called also when cutoff offset has changed
void lif_serie_processor::update ()
{
    if(m_contraction_pci_tracksRef && !m_contraction_pci_tracksRef->empty() && m_caRef && m_caRef->isValid())
        median_leveled_pci(m_contraction_pci_tracksRef->at(0));
}


void lif_serie_processor::contraction_analyzed (contractionContainer_t& contractions)
{
    std::lock_guard<std::mutex> lock(m_mutex);
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

// Note tracks contained timed data.
// Each call to find_best can be with different median cut-off
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


// Return 2D latice of pixels over time
void lif_serie_processor::generateVoxels_on_channel (const int channel_index){
    generateVoxels(m_all_by_channel[channel_index]);
}


// Return 2D latice of voxel self-similarity

void lif_serie_processor::generateVoxelsAndSelfSimilarities (const std::vector<roiWindow<P8U>>& images){
    
    generateVoxels(images);
    generateVoxelSelfSimilarities();

//
//    bool cache_ok = false;
//    assert(!images.empty());
//    auto expected_width = m_expected_segmented_size.first;
//    auto expected_height = m_expected_segmented_size.second;
//    if(fs::exists(mCurrentSerieCachePath)){
//        auto image_path = mCurrentSerieCachePath / m_params.image_cache_name ();
//        if(fs::exists(image_path)){
//            try{
//                m_temporal_ss = imread(image_path.string(), IMREAD_GRAYSCALE);
//            } catch( std::exception& ex){
//                auto msg = m_params.image_cache_name() + " Exists but could not be read ";
//                vlogger::instance().console()->info(msg);
//            }
//        }
//        cache_ok = expected_width == m_temporal_ss.cols;
//        cache_ok = cache_ok && expected_height == m_temporal_ss.rows;
//    }
//    if ( cache_ok ){
//        // Call the content loaded cb if any
//        if (signal_ss_image_available && signal_ss_image_available->num_slots() > 0)
//            signal_ss_image_available->operator()(m_temporal_ss);
//    }
//    else {

    
}



void lif_serie_processor::generateVoxels (const std::vector<roiWindow<P8U>>& images){
    size_t t_d = images.size();
    auto half_offset = m_voxel_sample / 2;
    uint32_t width = images[0].width() - half_offset.first;
    uint32_t height = images[0].height() - half_offset.second;
    uint32_t expected_width = m_expected_segmented_size.first;
    uint32_t expected_height = m_expected_segmented_size.second;
    
    m_voxels.resize(0);
    std::string msg = " Generating Voxels @ (" + to_string(m_voxel_sample.first) + "," + to_string(m_voxel_sample.second) + ")";
    msg += "Expected Size: " + to_string(expected_width) + " x " + to_string(expected_height);
    vlogger::instance().console()->info("starting " + msg);

    int count = 0;
    int row, col;
    for (row = half_offset.second; row < height; row+=m_voxel_sample.second){
        for (col = half_offset.first; col < width; col+=m_voxel_sample.first){
            std::vector<uint8_t> voxel(t_d);
            for (auto tt = 0; tt < t_d; tt++){
                voxel[tt] = images[tt].getPixel(col, row);
            }
            count++;
            m_voxels.emplace_back(voxel);
        }
        expected_height -= 1;
    }
    expected_width -= (count / m_expected_segmented_size.second);
    
    bool ok = expected_width == expected_height && expected_width == 0;
    msg = " remainders: " + to_string(expected_width) + " x " + to_string(expected_height);
    vlogger::instance().console()->info(ok ? "finished ok " : "finished with error " + msg);
    
    
}

void lif_serie_processor::create_voxel_surface(std::vector<float>& ven){

    uint32_t width = m_expected_segmented_size.first;
    uint32_t height = m_expected_segmented_size.second;
    assert(width*height == ven.size());
    auto extremes = svl::norm_min_max(ven.begin(),ven.end());
    std::string msg = to_string(extremes.first) + "," + to_string(extremes.second);
    vlogger::instance().console()->info(msg);
 
    cv::Mat tmp = cv::Mat(height, width, CV_8U);
    std::vector<float>::const_iterator start = ven.begin();

    for (auto row =0; row < height; row++){
        uint8_t* row_ptr = tmp.ptr<uint8_t>(row);
        auto end = start + width; // point to one after
        for (; start < end; start++, row_ptr++){
            if(std::isnan(*start)) continue; // nan is set to 1, the pre-set value
            *row_ptr = uint8_t((*start)*255);
        }
        start = end;
    }

    // Median Blur before up sizeing.
    cv::medianBlur(tmp, tmp, 5);
    cv::Size bot(tmp.cols * m_voxel_sample.first, tmp.rows * m_voxel_sample.second);
    m_temporal_ss = cv::Mat(bot, CV_8U);
    cv::resize(tmp, m_temporal_ss, bot, 0, 0, INTER_NEAREST);
    m_measured_area = Rectf(0,0,bot.width, bot.height);
    assert(m_temporal_ss.cols == m_measured_area.getWidth());
    assert(m_temporal_ss.rows == m_measured_area.getHeight());

    cv::Mat bi_level;
    
    threshold(m_temporal_ss, bi_level, 126, 255, THRESH_BINARY | THRESH_OTSU);
    
    vlogger::instance().console()->info("finished voxel surface");
    if(fs::exists(mCurrentSerieCachePath)){
        std::string imagename = "voxel_ss_.png";
        auto image_path = mCurrentSerieCachePath / imagename;
        cv::imwrite(image_path.string(), m_temporal_ss);
    }
    
    // Call the voxel ready cb if any
    if (signal_ss_image_available && signal_ss_image_available->num_slots() > 0){
         signal_ss_image_available->operator()(m_temporal_ss, bi_level);
    }
        
    
}

void lif_serie_processor::generateVoxelSelfSimilarities (){
 
    bool cache_ok = false;
    std::shared_ptr<internalContainer> ssref;
    
    if(fs::exists(mCurrentSerieCachePath)){
        auto cache_path = mCurrentSerieCachePath / m_params.internal_container_cache_name ();
        if(fs::exists(cache_path)){
            ssref = internalContainer::create(cache_path);
        }
        cache_ok = ssref && ssref->size_check(m_expected_segmented_size.first*m_expected_segmented_size.second);
    }
    
    if(cache_ok){
        vlogger::instance().console()->info(" IC container cache : Hit ");
        m_voxel_entropies.insert(m_voxel_entropies.end(), ssref->data().begin(),
                           ssref->data().end());
        // Call the voxel ready cb if any
        if (signal_ss_voxel_available && signal_ss_voxel_available->num_slots() > 0)
            signal_ss_voxel_available->operator()(m_voxel_entropies);
        
    }else{
        
        auto cache_path = mCurrentSerieCachePath / m_params.internal_container_cache_name ();
        
        vlogger::instance().console()->info("starting generating voxel self-similarity");
        auto sp =  similarity_producer();
        sp->load_images (m_voxels);
        std::packaged_task<bool()> task([sp](){ return sp->operator()(0);}); // wrap the function
        std::future<bool>  future_ss = task.get_future();  // get a future
        std::thread(std::move(task)).join(); // Finish on a thread
        vlogger::instance().console()->info("dispatched voxel self-similarity");
        if (future_ss.get())
        {
            vlogger::instance().console()->info("copying results of voxel self-similarity");
            const deque<double>& entropies = sp->shannonProjection ();
            m_voxel_entropies.insert(m_voxel_entropies.end(), entropies.begin(), entropies.end());
            
            // Call the voxel ready cb if any
            if (signal_ss_voxel_available && signal_ss_voxel_available->num_slots() > 0)
                signal_ss_voxel_available->operator()(m_voxel_entropies);
            
            bool ok = internalContainer::store(cache_path, m_voxel_entropies);
            if(ok)
                vlogger::instance().console()->info(" SS result container cache : filled ");
            else
                vlogger::instance().console()->info(" SS result container cache : failed ");
            
     
        }
    }
}


#pragma GCC diagnostic pop


