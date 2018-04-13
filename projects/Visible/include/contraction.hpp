
#ifndef _CONTRACTION_H
#define _CONTRACTION_H

#include <stdio.h>
#include <algorithm>
#include <deque>
#include <vector>
#include <memory>
#include <typeinfo>
#include <string>
#include <tuple>
#include <chrono>
#include "core/singleton.hpp"
#include "core/stats.hpp"
#include "core/stl_utils.hpp"

using namespace std;
using namespace boost;
using namespace svl;

class contraction_analyzer
{
public:
    
    struct contraction {
       size_t contraction_start;
       size_t contraction_max_acceleration;
       size_t contraction_peak;
       size_t relaxation_max_acceleration;
       size_t relaxation_end;
        
        static bool compare (const contraction& left, const contraction& right)
        {
            bool ok = left.contraction_start == right.contraction_start;
            if (! ok) return ok;
            ok &=  left.contraction_max_acceleration == right.contraction_max_acceleration;
            if (! ok) return ok;
            ok &=  left.contraction_peak == right.contraction_peak;
            if (! ok) return ok;
            ok &=  left.relaxation_max_acceleration == right.relaxation_max_acceleration;
            if (! ok) return ok;
            ok &=  left.relaxation_end == right.relaxation_end;
            return ok;
        }
        
        static void clear (contraction& ct)
        {
            std::memset(&ct, 0, sizeof(contraction));
        }
    };
    
    contraction_analyzer ( unsigned long min_contraction_width = 7,
                          double contraction_start_threshold = 0.1,
                          double relaxation_end_threshold = 0.1,
                         size_t movingMedianFilterSize = 7):
    m_minimum_contraction_width (min_contraction_width),
    m_start_thr(contraction_start_threshold ),
    m_end_thr(relaxation_end_threshold ),
    m_median_window (movingMedianFilterSize ), m_valid (false)
    {

    }
    
    void load (const std::vector<double>& acid) const
    {
        m_fder.clear ();
        m_fder.resize (acid.size());
        m_fder_filtered.resize (acid.size());

        // Computer First Difference,
        adjacent_difference(acid.begin(),acid.end(), m_fder.begin());
        std::rotate(m_fder.begin(), m_fder.begin()+1, m_fder.end());
        m_fder.pop_back();
        auto medianD = stl_utils::NthElement<double>(m_median_window);
        m_fder = medianD.filter(m_fder);
        std::transform(m_fder.begin(), m_fder.end(), m_fder_filtered.begin(),
                       [](double f)->double { return f * f; });
        m_valid = true;
        
    }
    
    void check_contraction_point (size_t p_index) {}
    
    bool measure_contraction_at_point (const size_t p_index, contraction& mctr)
    {
        if (! m_valid) return false;
        contraction::clear(mctr);
        mctr.contraction_peak = p_index;
        
        // find first element greater than 0.1
        auto pos = find_if (m_fder_filtered.begin(), m_fder_filtered.end(),    // range
                            bind2nd(greater<double>(),0.1));  // criterion
        
        mctr.contraction_start = std::distance(m_fder_filtered.begin(),pos);
        auto max_accel = std::min_element(m_fder.begin()+ mctr.contraction_start ,m_fder.begin()+mctr.contraction_peak);
        mctr.contraction_max_acceleration = std::distance(m_fder.begin()+ mctr.contraction_start, max_accel);
        mctr.contraction_max_acceleration += mctr.contraction_start;
        auto max_relax = std::max_element(m_fder.begin()+ mctr.contraction_peak ,m_fder.end());
        mctr.relaxation_max_acceleration = std::distance(m_fder.begin()+ mctr.contraction_peak, max_relax);
        mctr.relaxation_max_acceleration += mctr.contraction_peak;
        
        // Initialize rpos to point to the element following the last occurance of a value greater than 0.1
        // If there is no such value, initialize rpos = to begin
        // If the last occurance is the last element, initialize this it to end
        dItr_t rpos = find_if (m_fder_filtered.rbegin(), m_fder_filtered.rend(),    // range
                               bind2nd(greater<double>(),0.1)).base();  // criterion
        mctr.relaxation_end = std::distance (m_fder_filtered.begin(), rpos);
        return true;
    }
    
    size_t get_most_contracted (const std::vector<double>& acid) const
    {
        // Get contraction peak ( valley ) first
        auto min_iter = std::min_element(acid.begin(),acid.end());
        return std::distance(acid.begin(),min_iter);
    }
    
    void run (const std::vector<double>& acid)
    {
        m_ctr.contraction_peak = get_most_contracted(acid);
        load(acid);
        measure_contraction_at_point(m_ctr.contraction_peak, m_ctr);
    }
    
    const contraction& pols () const { return m_ctr; }
    
private:
    typedef vector<double>::iterator dItr_t;
    double m_start_thr;
    double m_end_thr;
    unsigned long m_median_window;
    size_t m_minimum_contraction_width;
    
    mutable contraction m_ctr;
    mutable std::vector<double> m_fder;
    mutable std::vector<double> m_fder_filtered;
    mutable bool m_valid;
};


#endif





