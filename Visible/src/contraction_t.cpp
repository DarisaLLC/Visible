//
//  contraction.cpp
//  Visible
//
//  Created by Arman Garakani on 8/19/18.
//

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcomma"
#pragma GCC diagnostic ignored "-Wunused-private-field"
#pragma GCC diagnostic ignored "-Wint-in-bool-context"

#include <stdio.h>
#include <algorithm>
#include <cmath>
#include <vector>
#include "cinder_cv/cinder_opencv.h"

#include "contraction.hpp"
#include "sg_filter.h"
#include "core/core.hpp"
#include "core/stl_utils.hpp"
#include "logger/logger.hpp"
#include "cardiomyocyte_model_detail.hpp"
#include "core/boost_units.hpp"
#include "core/simple_timing.hpp"
#include "core/moreMath.h"
#include "core/pf.h"
#include "vision/correlation1d.hpp"
#include "core/boost_stats.hpp"
#include "logger/logger.hpp"
#include "cpp-perf.hpp"
#include "boost/filesystem.hpp"

using namespace perf;
using namespace boost;

namespace bfs=boost::filesystem;

namespace anonymous
{
    template<typename P, template<typename ELEM, typename ALLOC = std::allocator<ELEM>> class CONT = std::deque >
    void norm_scale (const CONT<P>& src, CONT<P>& dst, double pw)
    {
        auto bot = std::min_element (src.begin (), src.end() );
        auto top = std::max_element (src.begin (), src.end() );
        
        P scaleBy = *top - *bot;
        dst.resize (src.size ());
        for (int ii = 0; ii < src.size (); ii++)
        {
            P dd = (src[ii] - *bot) / scaleBy;
            dst[ii] = pow (dd, 1.0 / pw);
        }
    }
    
    
    void norm_scale (const std::vector<double>& src, std::vector<double>& dst)
    {
        vector<double>::const_iterator bot = std::min_element (src.begin (), src.end() );
        vector<double>::const_iterator top = std::max_element (src.begin (), src.end() );
        
        if (svl::equal(*top, *bot)) return;
        double scaleBy = *top - *bot;
        dst.resize (src.size ());
        for (int ii = 0; ii < src.size (); ii++)
            dst[ii] = (src[ii] - *bot) / scaleBy;
    }
    
    void exponential_smoother (const std::vector<double>& src, std::vector<double>& dst)
    {
        vector<double>::const_iterator bot = std::min_element (src.begin (), src.end() );
        vector<double>::const_iterator top = std::max_element (src.begin (), src.end() );
        
        if (svl::equal(*top, *bot)) return;
        double scaleBy = *top - *bot;
        dst.resize (src.size ());
        for (int ii = 0; ii < src.size (); ii++)
            dst[ii] = (src[ii] - *bot) / scaleBy;
    }
}




std::shared_ptr<contractionLocator> contractionLocator::create(const input_section_selector_t& in, const uint32_t& body_id, const contractionLocator::params& params){
    return std::shared_ptr<contractionLocator>(new contractionLocator(in, body_id, params));
}

std::shared_ptr<contractionLocator> contractionLocator::getShared () {
    return shared_from_this();
}

contractionLocator::contractionLocator(const input_section_selector_t& in,  const uint32_t& body_id, const contractionLocator::params& params) :
m_cached(false),  m_in(in), mValidInput (false), m_params(params)
{
    m_median_levelset_frac = m_params.median_levelset_fraction();
    m_peaks.resize(0);
    m_id = body_id;
    m_in = in;
    // Signals we provide
    signal_contraction_ready = createSignal<contractionLocator::sig_cb_contraction_ready> ();
    signal_pci_available = createSignal<contractionLocator::sig_cb_pci_available> ();
    total_reactive_force_ready = createSignal<contractionLocator::sig_cb_force_ready>();
    cell_length_ready = createSignal<contractionLocator::sig_cb_cell_length_ready>();
}

void contractionLocator::load(const vector<double>& entropies, const vector<vector<double>>& mmatrix)
{
    m_entropies = entropies;
    m_SMatrix = mmatrix;
    mNoSMatrix = m_SMatrix.empty();
    
    m_entsize = m_entropies.size();
    if (isPreProcessed()) m_signal = m_entropies;
    else{
        m_ranks.resize (m_entsize);
        m_signal.resize (m_entsize);
    }
    m_peaks.resize(0);
    mValidInput = verify_input ();
    auto msg = "Contraction Locator Initialized For " + to_string(m_id);
    vlogger::instance().console()->info(msg);
}

/* Compute rank for all entropies
 * Steps:
 * 1. Copy and Calculate Median Entropy
 * 2. Subtract Median from the Copy
 * 3. Sort according to distance to Median
 * 4. Fill up rank vector
 */
double contractionLocator::Median_levelsets (const vector<double>& entropies,  std::vector<int>& ranks )
{
 
    vector<double> entcpy;
    
    for (const auto val : entropies)
        entcpy.push_back(val);
    auto median_value = svl::Median(entcpy);
    // Subtract median from all
    for (double& val : entcpy)
        val = std::abs (val - median_value );
    
    // Sort according to distance to the median. small to high
    ranks.resize(entropies.size());
    std::iota(ranks.begin(), ranks.end(), 0);
    auto comparator = [&entcpy](double a, double b){ return entcpy[a] < entcpy[b]; };
    std::sort(ranks.begin(), ranks.end(), comparator);
    return median_value;
}

size_t contractionLocator::recompute_signal () const
{

    size_t count = std::floor (m_entropies.size () * m_median_levelset_frac);
    assert(count < m_ranks.size());
    m_signal.resize(m_entropies.size (), 0.0);
    for (auto ii = 0; ii < m_signal.size(); ii++)
    {
        double val = 0;
        for (int index = 0; index < count; index++)
        {
            auto jj = m_ranks[index]; // get the actual index from rank
            val += m_SMatrix[jj][ii]; // fetch the cross match value for
        }
        val = val / count;
        m_signal[ii] = val;
    }
    
    return count;
}
void contractionLocator::update() const{
    perf::timer timeit;
    timeit.start();
     
    // Cache Rank Calculations
    compute_median_levelsets ();
    // If the fraction of entropies values expected is zero, then just find the minimum and call it contraction
    size_t count = recompute_signal();
    if (count == 0) m_signal = m_entropies;
    timeit.stop();
    auto timestr = toString(std::chrono::duration_cast<milliseconds>(timeit.duration()).count());
}

bool contractionLocator::get_contraction_at_point (int src_peak_index, const std::vector<int>& peak_indices, contraction_t& m_contraction) const{
    perf::timer timeit;
    timeit.start();
      
    if (! mValidInput) return false;
    contraction_t::clear(m_contraction);
    
    auto maxima = [](std::vector<double>::const_iterator a,
                     std::vector<double>::const_iterator b){
        std::vector<double> coeffs;
        polyfit(a, b, 1.0, coeffs, 2);
        assert(coeffs.size() == 3);
        return (-coeffs[1] / (2 * coeffs[2]));
    };
    const std::vector<double>& m_fder = m_signal;
    
    // Find distance between this peak and its previous and next peak or the begining or the end of the signal
    
    /* Contraction Start
     * 1. Find the location of maxima of a quadratic fit to the data before
     *    Maxima is at -b / 2c (y = a + bx + cx^2 => dy/dx = b + 2cx at Maxima dy/dx = 0 -> x = -b/2c
     * 1. Find the location of the minima of the first derivative using first differences
     *    dy = y(x-1) - y(x). x at minimum dy
     *
     */
    bool edge_case = peak_indices[src_peak_index] < m_params.minimum_contraction_frames() ||
        (m_fder.size() - peak_indices[src_peak_index]) < m_params.minimum_contraction_frames() ;
    if (edge_case) return false;
    
    m_contraction.contraction_peak.first = peak_indices[src_peak_index];
    std::vector<double>::const_iterator cpt = m_fder.begin() + peak_indices[src_peak_index];
    std::vector<double> results;
    // Left Boundary: If there is a peak behind us, use that other wise signal start
    auto left_boundary = src_peak_index > 0 ? peak_indices[src_peak_index - 1] : 0;
    results.reserve(m_contraction.contraction_peak.first - left_boundary);
    std::adjacent_difference (m_fder.begin()+left_boundary, cpt, std::back_inserter(results));
    auto min_elem = std::min_element(results.begin(),results.end());
    auto loc = std::distance(results.begin(), min_elem);
    
    auto loc_contraction_quadratic = maxima(m_fder.begin()+left_boundary, cpt);
    auto value = (! std::signbit(loc_contraction_quadratic))
        ? std::max(size_t(loc_contraction_quadratic), size_t(loc)) - std::min(size_t(loc_contraction_quadratic), size_t(loc)) / 2
        : loc;
    m_contraction.contraction_start.first = value;
    
    auto right_boundary = (src_peak_index == (peak_indices.size() - 1))
        ? m_fder.size() - 1
        : peak_indices[src_peak_index + 1];
    /* relaxation End
     * 1. Find the location of maxima of a quadratic fit to the data before
     *    Maxima is at -b / 2c (y = a + bx + cx^2 => dy/dx = b + 2cx at Maxima dy/dx = 0 -> x = -b/2c
     */
    auto loc_relaxation_quadratic = maxima(cpt, m_fder.begin()+right_boundary);
    m_contraction.relaxation_end.first =  m_contraction.contraction_peak.first + loc_relaxation_quadratic;
    
    /*
     * Use Median visual rank value for relaxation visual rank
     */
    m_contraction.relaxation_visual_rank = svl::Median(m_fder);
    timeit.stop();
    auto timestr = toString(std::chrono::duration_cast<milliseconds>(timeit.duration()).count());
    vlogger::instance().console()->info("get_contraction_at_point (ms): " + timestr);
    vlogger::instance().console()->info(" Peak Index: " + toString(src_peak_index));
    
    return true;
}



bool contractionLocator::locate_contractions (){
    perf::timer timeit;
    timeit.start();
     
    assert(verify_input());
    if(! isPreProcessed())
        update ();
    
    //
    m_ac.resize(0);
    f1dAutoCorr(m_signal.begin(), m_signal.end(), m_ac);
    
    m_bac.resize(m_ac.size());
    cv::GaussianBlur(m_ac,m_bac,cv::Size(0,0), 4.0);
    svl::boostStatistics bstats;
    for (const auto& val : m_bac) bstats.update(val);
    std::cout << std::endl
    << "(time in ms)" << std::endl
    << "Count:  " << bstats.get_n() << std::endl
    << "Min:    " << bstats.get_min() << std::endl
    << "Max:    " << bstats.get_max() << std::endl
    << "Med:    " << bstats.get_median() << std::endl
    << "x75:    " << bstats.get_quantile(0.75) << std::endl
    << "x85:    " << bstats.get_quantile(0.85) << std::endl
    << "x95:    " << bstats.get_quantile(0.95) << std::endl
    << "x99:    " << bstats.get_quantile(0.99) << std::endl
    << "x99.9:  " << bstats.get_quantile(0.999) << std::endl
    << "x99.99: " << bstats.get_quantile(0.9999) << std::endl
    
    
    << std::endl;
    clear_outputs ();
    std::vector<int> peaks_idx;
    std::vector<double> valleys(m_signal.size());
    std::transform(m_signal.begin(), m_signal.end(), valleys.begin(), [](double f)->double { return 1.0 - f; });
    
    svl::findPeaks(valleys, peaks_idx);
    stringstream ss;
    for (auto idx : peaks_idx) ss << idx << ",";
    vlogger::instance().console()->info(ss.str());
    
    auto save_csv = [](const std::shared_ptr<contractionProfile>& cp, bfs::path& root_path){
        auto folder = stl_utils::now_string();
        auto folder_path = root_path / folder;
        boost::system::error_code ec;
        if(!bfs::exists(folder_path)){
            bfs::create_directory(folder_path, ec);
            if (ec != boost::system::errc::success){
                std::string msg = "Could not create " + folder_path.string() ;
                vlogger::instance().console()->error(msg);
                return false;
            }
            std::string basefilename = folder_path.string() + boost::filesystem::path::preferred_separator;
            auto fn = basefilename + "force.csv";
            stl_utils::save_csv(cp->force(), fn);
            fn = basefilename + "interpolatedLength.csv";
            stl_utils::save_csv(cp->interpolatedLength(), fn);
            fn = basefilename + "elongation.csv";
            stl_utils::save_csv(cp->elongation(), fn);
              return true;
        }
        return false;
    };
    
    for (auto pp = 0; pp < peaks_idx.size(); pp++){
        m_peaks.emplace_back(peaks_idx[pp], m_signal[peaks_idx[pp]]);
    }
    
    for (auto pp = 0; pp < peaks_idx.size(); pp++){
        
        contraction_t ct;
        
        // Contraction gets a unique id by contraction profiler
        if(get_contraction_at_point(pp, peaks_idx, ct)){
            auto profile = std::make_shared<contractionProfile>(ct, m_id);
            profile->compute_interpolated_geometries_and_force(m_signal);
            m_contractions.emplace_back(profile->contraction());
            
            //@todo use cache_root for this
            bfs::path sp ("/Volumes/medvedev/Users/arman/tmp/");
            save_csv(profile, sp);
            auto msg = "Stored " + stl_utils::tostr(pp);
            vlogger::instance().console()->info(msg);
            break;
        }
    }
    
    if (signal_contraction_ready && signal_contraction_ready->num_slots() > 0)
        signal_contraction_ready->operator()(m_contractions, m_in);

    timeit.stop();
    auto timestr = toString(std::chrono::duration_cast<milliseconds>(timeit.duration()).count());
    vlogger::instance().console()->info("Update (ms): " + timestr);
    vlogger::instance().console()->info(" Number of Peaks: " + toString(peaks_idx.size()));
    
    return true;
}


void contractionLocator::compute_median_levelsets () const
{
    if (m_cached) return;
    m_median_value = contractionLocator::Median_levelsets (m_entropies, m_ranks);
    m_cached = true;
}


void contractionLocator::clear_outputs () const
{
    m_peaks.resize(0);
    m_contractions.resize(0);
}
bool contractionLocator::verify_input () const
{
    bool ok = ! m_entropies.empty();
    if (!ok)return false;
    
    if ( ! m_SMatrix.empty () ){
        ok = m_entropies.size() == m_entsize;
        if (!ok) return ok;
        ok &= m_SMatrix.size() == m_entsize;
        if (!ok) return ok;
        for (int row = 0; row < m_entsize; row++)
        {
            //      std::cout << "[" << row << "]" << m_SMatrix[row].size () << std::endl;
            ok &= m_SMatrix[row].size () == m_entsize;
            if (!ok) return ok;
        }
    }
    return ok;
}






template boost::signals2::connection contractionLocator::registerCallback(const std::function<contractionLocator::sig_cb_contraction_ready>&);

contractionProfile::contractionProfile (contraction_t& ct, cell_id_t cid ):
m_ctr(ct), m_id(cid)
{
    
}




/*
 * Compute interpolated length and force for this contraction
 *
 */

void contractionProfile::compute_interpolated_geometries_and_force(const std::vector<double>& signal){
    perf::timer timeit;
    timeit.start();
     
    const double relaxed_length = contraction().relaxation_visual_rank;
    for (auto dd : signal) m_fder.push_back((float)dd);

    /*
     * Compute everything within contraction interval
     */
    auto c_start = m_ctr.contraction_start.first;
    auto c_end = m_ctr.relaxation_end.first;
    assert(!m_fder.empty());
    assert(c_start >= 0 && c_start < c_end && c_end < m_fder.size());
    
    // Compute force using cardio model
    m_elongation.clear();
    m_interpolated_length.clear();
    m_force.clear();
    cardio_model cmm;
    cmm.shear_control(1.0f);
    cmm.shear_velocity(200.0_cm_s);
    m_ctr.cardioModel = cmm;
    
    for (auto ii = c_start; ii < c_end; ii++)
    {
        const auto & reading = m_fder[ii];
        auto linMul = clampValue(reading/relaxed_length, 0.0, 1.0);
        auto elon = relaxed_length - linMul;
        
        cmm.length(linMul * boost::units::cgs::micron);
        cmm.elongation(elon * boost::units::cgs::micron);
        cmm.width((linMul/3.0) * boost::units::cgs::micron);
        cmm.thickness((linMul/100.0)* boost::units::cgs::micron);
        cmm.run();
        m_force.push_back(cmm.result().total_reactive.value());
        m_interpolated_length.push_back(linMul);
        m_elongation.push_back(elon);
    }
    
    // Copy Results Out.
    m_ctr.force = m_force;
    m_ctr.elongation = m_elongation;
    m_ctr.normalized_length = m_interpolated_length;
    
    // Add uid for this contraction
    m_ctr.m_uid = m_id;
    
    timeit.stop();
    auto timestr = toString(std::chrono::duration_cast<milliseconds>(timeit.duration()).count());
    vlogger::instance().console()->info("Force (ms): " + timestr);
    vlogger::instance().console()->info("Id " + toString(m_ctr.m_uid));
    
    
}

#pragma GCC diagnostic pop
