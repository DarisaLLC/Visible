
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
        uint32_t contraction_start;
        uint32_t contraction_max_acceleration;
        uint32_t contraction_peak;
        uint32_t relaxation_max_acceleration;
        uint32_t relaxation_end;
        
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
    };
    
    contraction_analyzer ( double contraction_start_threshold = 0.1,
                          double relaxation_end_threshold = 0.1,
                          uint32_t movingMedianFilterSize = 7):
    m_start_thr(contraction_start_threshold ),
    m_end_thr(relaxation_end_threshold ),
    m_median_window (movingMedianFilterSize )
    {
        
    }
    
    void run (const std::vector<double>& acid)
    {
        typedef vector<double>::iterator dItr_t;
        
        double contraction_start_threshold = 0.1;
        
        std::vector<double> fder, fder2;
        auto bItr = acid.begin();
        fder.resize (acid.size());
        fder2.resize (acid.size());
        
        // Get contraction peak ( valley ) first
        auto min_iter = std::min_element(acid.begin(),acid.end());
        m_ctr.contraction_peak = std::distance(acid.begin(),min_iter);
        
        // Computer First Difference,
        adjacent_difference(acid.begin(),acid.end(), fder.begin());
        std::rotate(fder.begin(), fder.begin()+1, fder.end());
        fder.pop_back();
        auto medianD = stl_utils::NthElement<double>(7);
        fder = medianD.filter(fder);
        std::transform(fder.begin(), fder.end(), fder2.begin(), [](double f)->double { return f * f; });
        // find first element greater than 0.1
        auto pos = find_if (fder2.begin(), fder2.end(),    // range
                            bind2nd(greater<double>(),0.1));  // criterion
        
        m_ctr.contraction_start = std::distance(fder2.begin(),pos);
        auto max_accel = std::min_element(fder.begin()+ m_ctr.contraction_start ,fder.begin()+m_ctr.contraction_peak);
        m_ctr.contraction_max_acceleration = std::distance(fder.begin()+ m_ctr.contraction_start, max_accel);
        m_ctr.contraction_max_acceleration += m_ctr.contraction_start;
        auto max_relax = std::max_element(fder.begin()+ m_ctr.contraction_peak ,fder.end());
        m_ctr.relaxation_max_acceleration = std::distance(fder.begin()+ m_ctr.contraction_peak, max_relax);
        m_ctr.relaxation_max_acceleration += m_ctr.contraction_peak;
        
        // Initialize rpos to point to the element following the last occurance of a value greater than 0.1
        // If there is no such value, initialize rpos = to begin
        // If the last occurance is the last element, initialize this it to end
        dItr_t rpos = find_if (fder2.rbegin(), fder2.rend(),    // range
                               bind2nd(greater<double>(),0.1)).base();  // criterion
        m_ctr.relaxation_end = std::distance (fder2.begin(), rpos);
    }
    
    const contraction& pols () const { return m_ctr; }
    
private:
    double m_start_thr;
    double m_end_thr;
    uint32_t m_median_window;
    mutable contraction m_ctr;

};


#endif





