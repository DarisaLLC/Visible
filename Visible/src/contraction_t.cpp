//
//  contraction.cpp
//  Visible
//
//  Created by Arman Garakani on 8/19/18.
//

#include <stdio.h>
#include <algorithm>
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




std::shared_ptr<contractionLocator> contractionLocator::create()
{
    return std::shared_ptr<contractionLocator>(new contractionLocator());
}

std::shared_ptr<contractionLocator> contractionLocator::getShared () {
        return shared_from_this();
}

contractionLocator::contractionLocator() :
m_cached(false),  mValidInput (false)
{
    m_median_levelset_frac = m_params.median_levelset_fraction();
    m_peaks.resize(0);
    m_id = -2; 
    // Signals we provide
    signal_contraction_ready = createSignal<contractionLocator::sig_cb_contraction_ready> ();
    signal_pci_available = createSignal<contractionLocator::sig_cb_pci_available> ();
    total_reactive_force_ready = createSignal<contractionLocator::sig_cb_force_ready>();
    cell_length_ready = createSignal<contractionLocator::sig_cb_cell_length_ready>();
}

void contractionLocator::load(const vector<double>& entropies, const vector<vector<double>>& mmatrix, int input)
{
    m_id = input;
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
    // If the fraction of entropies values expected is zero, then just find the minimum and call it contraction
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
      stl_utils::save_csv(m_signal,"/Volumes/medvedev/Users/arman/tmp/signal.csv");
    return count;
}
void contractionLocator::update() const{
    // Cache Rank Calculations
    compute_median_levelsets ();
    // If the fraction of entropies values expected is zero, then just find the minimum and call it contraction
    size_t count = recompute_signal();
    if (count == 0) m_signal = m_entropies;
}

bool contractionLocator::get_contraction_at_point (int src_peak_index, const std::vector<int>& peak_indices, contraction_t& m_contraction) const{
 
    if (! mValidInput) return false;
    contraction_t::clear(m_contraction);
    
    auto maxima = [](sigContainer_t::const_iterator a,
                     sigContainer_t::const_iterator b){
        std::vector<double> coeffs;
        polyfit(a, b, 1.0, coeffs, 2);
        assert(coeffs.size() == 3);
        return (-coeffs[1] / (2 * coeffs[2]));
    };
    const sigContainer_t& m_fder = m_signal;

    // Find distance between this peak and its previous and next peak or the begining or the end of the signal
    
    /* Contraction Start
     * 1. Find the location of maxima of a quadratic fit to the data before
     *    Maxima is at -b / 2c (y = a + bx + cx^2 => dy/dx = b + 2cx at Maxima dy/dx = 0 -> x = -b/2c
     * 1. Find the location of the minima of the first derivative using first differences
     *    dy = y(x-1) - y(x). x at minimum dy
     *
     */
    m_contraction.contraction_peak.first = peak_indices[src_peak_index];
    std::vector<double>::const_iterator cpt = m_fder.begin() + peak_indices[src_peak_index];
    std::vector<double> results;
    auto left_boundary = src_peak_index > 0 ? peak_indices[src_peak_index - 1] : 0;
    results.reserve(m_contraction.contraction_peak.first - left_boundary);
    std::adjacent_difference (m_fder.begin()+left_boundary, cpt, std::back_inserter(results));
    auto min_elem = std::min_element(results.begin(),results.end());
    auto loc = std::distance(results.begin(), min_elem);
    
    auto loc_contraction_quadratic = maxima(m_fder.begin()+left_boundary, cpt);
    auto range = std::max(size_t(loc_contraction_quadratic), size_t(loc)) -
        std::min(size_t(loc_contraction_quadratic), size_t(loc));
    m_contraction.contraction_start.first = range / 2;
    
    /* relaxation End
     * 1. Find the location of maxima of a quadratic fit to the data before
     *    Maxima is at -b / 2c (y = a + bx + cx^2 => dy/dx = b + 2cx at Maxima dy/dx = 0 -> x = -b/2c
     */
    auto loc_relaxation_quadratic = maxima(cpt, m_fder.end());
    m_contraction.relaxation_end.first =  m_contraction.contraction_peak.first + loc_relaxation_quadratic;
    
    /*
     * Use Median visual rank value for relaxation visual rank
     */
    m_contraction.relaxation_visual_rank = svl::Median(m_fder);

    return true;
}
    
// @todo: add multi-contraction
bool contractionLocator::find_best ()
{
    assert(verify_input());
    if(! isPreProcessed())
        update ();
    
    auto invalid = m_entropies.size ();
    index_val_t lowest;
    lowest.first = invalid;
    lowest.second = std::numeric_limits<double>::max ();
    auto result = std::minmax_element(m_signal.begin(), m_signal.end() );
    m_leveled_min_max.first = *result.first;
    m_leveled_min_max.second = *result.second;
    auto min_itr = result.first;
    if (min_itr != m_signal.end())
    {
        auto min_index = std::distance(m_signal.begin(), min_itr);
        if (min_index >= 0){
            lowest.first = static_cast<size_t>(min_index);
            lowest.second = *min_itr;
        }
    }
    
    //@todo: multiple contractions
    clear_outputs ();
    std::vector<int> peaks_idx;
    std::vector<double> valleys(m_signal.size());
    std::transform(m_signal.begin(), m_signal.end(), valleys.begin(), [](double f)->double { return 1.0 - f; });
    
    svl::findPeaks(valleys, peaks_idx);
    for (auto idx : peaks_idx){
        std::cout << idx << std::endl;
    }

    auto save_csv = [](const std::shared_ptr<contractionProfile>& cp, fs::path& root_path){
        
        auto folder = stl_utils::now_string();
        auto folder_path = root_path / folder;
        boost::system::error_code ec;
        if(!fs::exists(folder_path)){
            fs::create_directory(folder_path, ec);
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
    
    assert(lowest.first < invalid);
    m_peaks.emplace_back(peaks_idx[0], m_signal[peaks_idx[0]]);
    contraction_t ct;
 //   ct.first = 0;ct.second = size();
    if(get_contraction_at_point(0, peaks_idx, ct)){
        auto profile = std::make_shared<contractionProfile>(ct);
        profile->compute_interpolated_geometries_and_force(m_signal);
        m_contractions.emplace_back(profile->contraction());

        //@todo use cache_root for this
        fs::path sp ("/Volumes/medvedev/Users/arman/tmp/");
        save_csv(profile, sp);
        
        if (signal_contraction_ready && signal_contraction_ready->num_slots() > 0)
            signal_contraction_ready->operator()(m_contractions);
        std::string c0("Contraction Detected @ ");
        c0 = c0 + to_string(ct.contraction_peak.first);
        mValidOutput = true;
    }
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
            std::cout << "[" << row << "]" << m_SMatrix[row].size () << std::endl;
            ok &= m_SMatrix[row].size () == m_entsize;
            if (!ok) return ok;
        }
    }
    return ok;
}






template boost::signals2::connection contractionLocator::registerCallback(const std::function<contractionLocator::sig_cb_contraction_ready>&);

contractionProfile::contractionProfile (contraction_t& ct/*, const contractionLocator::Ref & cl */ ):
m_ctr(ct) /*, m_connect(cl) */
{

}




/*
 * Compute interpolated length and force for this contraction
 *
 */

void contractionProfile::compute_interpolated_geometries_and_force(const std::vector<double>& signal){

    const double relaxed_length = contraction().relaxation_visual_rank;
    m_fder = signal;
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
    m_ctr.interpolated_length = m_interpolated_length;
 
    
    

}

