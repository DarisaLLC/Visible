//
//  contraction.cpp
//  Visible
//
//  Created by Arman Garakani on 8/19/18.
//

#include <stdio.h>
#include "contraction.hpp"
#include "sg_filter.h"
#include "core/core.hpp"
#include "core/stl_utils.hpp"
#include "logger.hpp"

namespace anonymous
{
    void norm_scale (const std::deque<double>& src, std::deque<double>& dst)
    {
        deque<double>::const_iterator bot = std::min_element (src.begin (), src.end() );
        deque<double>::const_iterator top = std::max_element (src.begin (), src.end() );
        
        if (svl::equal(*top, *bot)) return;
        double scaleBy = *top - *bot;
        dst.resize (src.size ());
        for (int ii = 0; ii < src.size (); ii++)
            dst[ii] = (src[ii] - *bot) / scaleBy;
    }
}

std::shared_ptr<contraction_analyzer> contraction_analyzer::create()
{
    return std::shared_ptr<contraction_analyzer>(new contraction_analyzer());
}

contraction_analyzer::contraction_analyzer() :
m_cached(false),  mValidInput (false), m_median_levelset_frac (0.0)
{
    m_peaks.resize(0);
    
    // Signals we provide
    signal_contraction_analyzed = createSignal<contraction_analyzer::sig_cb_contraction_analyzed> ();
    
}

void contraction_analyzer::load(const deque<double>& entropies, const deque<deque<double>>& mmatrix)
{
    m_entropies = entropies;
    m_SMatrix = mmatrix;
    m_matsize = m_entropies.size();
    m_ranks.resize (m_matsize);
    m_signal.resize (m_matsize);
    m_peaks.resize(0);
    mValidInput = verify_input ();
}

// @todo: add multi-contraction
bool contraction_analyzer::find_best () const
{
    if (verify_input())
    {
        auto invalid = m_entropies.size ();
        index_val_t lowest;
        lowest.first = invalid;
        lowest.second = std::numeric_limits<double>::max ();
        // Cache Rank Calculations
        compute_median_levelsets ();
        // If the fraction of entropies values expected is zero, then just find the minimum and call it contraction
        size_t count = recompute_signal();
        if (count == 0) m_signal = m_entropies;
        auto result = std::minmax_element(m_signal.begin(), m_signal.end() );
        m_filtered_min_max.first = *result.first;
        m_filtered_min_max.second = *result.second;
        auto min_itr = result.first;
        if (min_itr != m_signal.end())
        {
            auto min_index = std::distance(m_signal.begin(), min_itr);
            if (min_index >= 0){
                lowest.first = static_cast<size_t>(min_index);
                lowest.second = *min_itr;
            }
        }
        
        //@ note: clear and fetch the peak. Set cotraction envelope in frames
        //@todo: multiple contractions
        clear_outputs ();
        if (lowest.first < invalid)
        {
            m_peaks.emplace_back(lowest.first, m_signal[lowest.first]);
            contraction one;
            contraction::fill(m_peaks.back(), std::pair<size_t,size_t>(-20,30), one);
            m_contractions.emplace_back(one);
            if (signal_contraction_analyzed && signal_contraction_analyzed->num_slots() > 0)
                signal_contraction_analyzed->operator()(m_contractions);
            std::string c0("Contraction Detected @ ");
            c0 = c0 + to_string(one.contraction_peak.first);
            mValidOutput = true;
            vlogger::instance().console()->info(c0);
            return true;
        }
    }
    return false;
}

/* Compute rank for all entropies
 * Steps:
 * 1. Copy and Calculate Median Entropy
 * 2. Subtract Median from the Copy
 * 3. Sort according to distance to Median
 * 4. Fill up rank vector
 */
double contraction_analyzer::Median_levelsets (const deque<double>& entropies,  std::vector<int>& ranks )
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

size_t contraction_analyzer::recompute_signal () const
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
    return count;
}
void contraction_analyzer::compute_median_levelsets () const
{
    if (m_cached) return;
    m_median_value = contraction_analyzer::Median_levelsets (m_entropies, m_ranks);
    m_cached = true;
}

float contraction_analyzer::entropy_interpolated_length (sigIterator_t , float min_length, float max_length){
    return -1.0f;
    
}

void contraction_analyzer::interpolated_length(vector<double>& dst, float min_length, float max_length){
    if(!isOutputValid() || max_length < 1.0f || min_length >= max_length ){
        dst = vector<double> ();
        return;
    }
    double entOlenScale = (1.0 - m_filtered_min_max.first / m_filtered_min_max.second) / (1.0 - min_length / max_length);
    
    dst.resize(m_signal.size());
    for (auto ii = 0; ii < m_signal.size(); ii++)
    {
        auto nn = 1.0 - m_signal[ii] / m_filtered_min_max.second;
        nn = 1.0 - nn / entOlenScale;
        dst[ii] = nn * max_length;
    }
    
}
void contraction_analyzer::clear_outputs () const
{
    m_peaks.resize(0);
    m_contractions.resize(0);
}
bool contraction_analyzer::verify_input () const
{
    if (m_entropies.empty() || m_SMatrix.empty ())
        return false;
    
    bool ok = m_entropies.size() == m_matsize;
    if (!ok) return ok;
    ok &= m_SMatrix.size() == m_matsize;
    if (!ok) return ok;
    for (int row = 0; row < m_matsize; row++)
    {
        ok &= m_SMatrix[row].size () == m_matsize;
        if (!ok) return ok;
    }
    return ok;
}




void contraction_analyzer::savgol (const deque<double>& signal, deque<double>& dst, int winlen)
{
    // for scalar data:
    int order = 4;
    SGF::real sample_time = 0; // this is simply a float or double, you can change it in the header sg_filter.h if yo u want
    SGF::ScalarSavitzkyGolayFilter filter(order, winlen, sample_time);
    dst.resize(signal.size());
    SGF::real output;
    for (auto ii = 0; ii < signal.size(); ii++)
    {
        filter.AddData(signal[ii]);
        if (! filter.IsInitialized()) continue;
        dst[ii] = 0;
        int ret_code;
        ret_code = filter.GetOutput(0, output);
        dst[ii] = output;
    }
    
}

bool contraction_analyzer::savgol_filter () const
{
    m_signal.resize(m_entropies.size (), 0.0);
    int wlen = std::floor (100 * m_median_levelset_frac);
    
    savgol (m_entropies, m_signal, wlen);
    return true;
}

template boost::signals2::connection contraction_analyzer::registerCallback(const std::function<contraction_analyzer::sig_cb_contraction_analyzed>&);

