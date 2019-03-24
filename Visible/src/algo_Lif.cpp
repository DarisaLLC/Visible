//
//  algo_Lif.cpp
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
#include "algo_cardiac.hpp"
#include "contraction.hpp"
#include "vision/localvariance.h"
#include "algo_Lif.hpp"
#include "logger.hpp"
#include "cpp-perf.hpp"
#include "vision/localvariance.h"
#include "vision/labelBlob.hpp"

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

// Check if the returned has expired
std::weak_ptr<contraction_analyzer> lif_serie_processor::contractionWeakRef ()
{
    std::weak_ptr<contraction_analyzer> wp (m_caRef);
    return wp;
}


int64_t lif_serie_processor::load (const std::shared_ptr<seqFrameContainer>& frames,const std::vector<std::string>& names, const std::vector<std::string>& plot_names)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    create_named_tracks(names, plot_names);
    load_channels_from_images(frames);
    lock.unlock();
    int channel_to_use = m_channel_count - 1;
    run_volume_variances (channel_to_use);
    
    // Call the content loaded cb if any
    if (signal_content_loaded && signal_content_loaded->num_slots() > 0)
        signal_content_loaded->operator()(m_frameCount);
    
    return m_frameCount;
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
    m_var_display_image.create(m_var_image.rows, m_var_image.cols, CV_8UC1);
    cv::normalize(m_var_image, m_var_display_image, 0, 255, NORM_MINMAX, CV_8UC1);

    cv::Scalar lmean, lstd;
    Mat threshold_output;
    vector<vector<cv::Point> > contours;
    vector<cv::Point> all_contours;
    vector<Vec4i> hierarchy;

    cv::meanStdDev(m_var_display_image, lmean, lstd);
    auto thr = leftTailPost (m_var_display_image, 0.95);
    threshold(m_var_display_image, threshold_output, thr, 255, THRESH_BINARY );
    cv::findContours( threshold_output, contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, cv::Point(0, 0) );

    for( int i = 0; i < contours.size(); i++ )
    {
        const vector<cv::Point>& cc = contours[i];
        for (const cv::Point& point : cc)
            all_contours.push_back(point);
    }
    RotatedRect box = fitEllipse(all_contours);
    m_motion_mass = box;
    
    auto dims = box.size;
    float minor_dim = std::min(dims.width, dims.height);
    float major_dim = std::max(dims.width, dims.height);
    std::cout << "Center @ " << box.center.x << "," << box.center.y << "   ";
    std::cout << minor_dim << "  "  << major_dim << " angle: " << box.angle << std::endl;
    
    
    // Signal to listeners
    if (signal_volume_var_available && signal_volume_var_available->num_slots() > 0)
        signal_volume_var_available->operator()();
    
    // Now generate voxels at 1/3 resolution and generate temporal ss
    generateVoxelSelfSimilarities_on_channel(2, 3, 3);
    
    
}

void lif_serie_processor::run_volume_variances (const int channel_index){
    return run_volume_variances(m_all_by_channel[channel_index]);
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
    auto sp =  similarity_producer();
    sp->load_images (images);
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
    return m_contraction_pci_tracksRef;
}

// @todo even though channel index is an argument, but only results for one channel is kept in lif_processor.
// 
std::shared_ptr<vecOfNamedTrack_t>  lif_serie_processor::run_contraction_pci_on_channel (const int channel_index)
{
    return run_contraction_pci(std::move(m_all_by_channel[channel_index]));
}


const std::vector<Rectf>& lif_serie_processor::rois () const { return m_rois; }


const cv::RotatedRect& lif_serie_processor::motion_surface() const { return m_motion_mass; }

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
void lif_serie_processor::generateVoxels_on_channel (const int channel_index, std::vector<std::vector<roiWindow<P8U>>>& rvs,
                                                     uint32_t sample_x, uint32_t sample_y){
    std::lock_guard<std::mutex> lock(m_mutex);
    generateVoxels(m_all_by_channel[channel_index], rvs, sample_x, sample_y);
}


// Return 2D latice of voxel self-similarity

void lif_serie_processor::generateVoxelSelfSimilarities_on_channel (const int channel_index, uint32_t sample_x, uint32_t sample_y){
    std::vector<std::vector<roiWindow<P8U>>> rvs;
    std::string msg = " Generating Voxels @ (" + to_string(sample_x) + "," + to_string(sample_y) + ")";
    vlogger::instance().console()->info("starting " + msg);
    generateVoxels(m_all_by_channel[channel_index], rvs, sample_x, sample_y);
    vlogger::instance().console()->info("finished " + msg);
    generateVoxelSelfSimilarities(rvs);
    m_voxel_xy.first = sample_x;
    m_voxel_xy.second = sample_y;
}



void lif_serie_processor::generateVoxels (const std::vector<roiWindow<P8U>>& images,
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



void lif_serie_processor::generateVoxelSelfSimilarities (std::vector<std::vector<roiWindow<P8U>>>& voxels){
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
#if 1
        std::string imagename = svl::toString(std::clock()) + ".png";
        std::string image_path = "/Users/arman/tmp/" + imagename;
        cv::imwrite(image_path, m_temporal_ss);
#endif
    }

}


#pragma GCC diagnostic pop


