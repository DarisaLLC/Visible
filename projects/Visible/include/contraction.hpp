
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
#include "core/stats.hpp"
#include "core/stl_utils.hpp"
#include "core/signaler.h"

using namespace std;
using namespace boost;
using namespace svl;


struct contractionMesh
{
    //@note: pair representing frame number and frame time
    typedef std::pair<size_t,double> index_val_t;
    
    index_val_t contraction_start;
    index_val_t contraction_max_acceleration;
    index_val_t contraction_peak;
    index_val_t relaxation_max_acceleration;
    index_val_t relaxation_end;
    
    
    static bool equal (const contractionMesh& left, const contractionMesh& right)
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
    
    static void clear (contractionMesh& ct)
    {
        std::memset(&ct, 0, sizeof(contractionMesh));
    }
    
    static void fill (const index_val_t& peak, const std::pair<size_t,size_t>& start_end, contractionMesh& fillthis)
    {
        fillthis.contraction_peak = peak;
        fillthis.contraction_start.first = peak.first + start_end.first;
        fillthis.relaxation_end.first = peak.first + start_end.second;
    }

};

class ca_signaler : public base_signaler
{
    virtual std::string
    getName () const { return "caSignaler"; }
};


class contraction_analyzer : public ca_signaler, std::enable_shared_from_this<contraction_analyzer>
{
public:
    
    using contraction = contractionMesh;
    using index_val_t = contractionMesh::index_val_t;
    
    // Signals we provide
    // signal_contraction_available
    typedef std::vector<contraction> contractionContainer_t;
    typedef void (sig_cb_contraction_analyzed) (contractionContainer_t&);
 
    
    static std::shared_ptr<contraction_analyzer> create();
    
    void load (const deque<double>& entropies, const deque<deque<double>>& mmatrix);
    // @todo: add multi-contraction
    bool find_best () const;
    
    bool isValid () const { return mValid; }
    size_t size () const { return m_matsize; }
    
    // Original
    const deque<double>& entropies () { return m_entropies; }
    
    // Filtered 
    const deque<double>& filtered () { return m_signal; }
    
    void set_median_levelset_pct (float frac) const { m_median_levelset_frac = frac;  }
    float get_median_levelset_pct () const { return m_median_levelset_frac; }
    
    const std::vector<index_val_t>& low_peaks () const { return m_peaks; }
    const std::vector<contraction>& contractions () const { return m_contractions; }
    
    /* @note: left in public interface as the UT assumes that.
     * @todo: refactor
     * Compute rank for all entropies
     * Steps:
     * 1. Copy and Calculate Median Entropy
     * 2. Subtract Median from the Copy
     * 3. Sort according to distance to Median
     * 4. Fill up rank vector
     */
    static double Median_levelsets (const deque<double>& entropies,  std::vector<int>& ranks );
    static void savgol (const deque<double>& signal, deque<double>& dst, int winlen);
 
    
    
private:
    contraction_analyzer();
    void compute_median_levelsets () const;
    void clear_outputs () const;
    bool verify_input () const;
    bool savgol_filter () const;
  
    
    mutable float m_median_levelset_frac;
    mutable deque<deque<double>>        m_SMatrix;   // Used in eExhaustive and
    deque<double>               m_entropies;
    deque<double>               m_accum;
    mutable deque<double>              m_signal;
    mutable std::vector<int>            m_ranks;
    size_t m_matsize;
    mutable std::atomic<bool> m_cached;
    mutable bool mValid;
    mutable std::vector<index_val_t> m_peaks;
    mutable std::vector<contraction> m_contractions;
    
protected:
    
    boost::signals2::signal<contraction_analyzer::sig_cb_contraction_analyzed>* signal_contraction_analyzed;
    
};


class contraction_profile_analyzer
{
public:
  
    
    contraction_profile_analyzer ( unsigned long min_contraction_width = 7,
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
        auto medianD = stl_utils::median1D<double>(m_median_window);
        m_fder = medianD.filter(m_fder);
        std::transform(m_fder.begin(), m_fder.end(), m_fder_filtered.begin(),
                       [](double f)->double { return f * f; });
        m_valid = true;
        
    }
    
    void check_contraction_point (size_t p_index) {}
    
    bool measure_contraction_at_point (const size_t p_index, contractionMesh& mctr)
    {
        if (! m_valid) return false;
        contractionMesh::clear(mctr);
        mctr.contraction_peak.first = p_index;
        
        // find first element greater than 0.1
        auto pos = find_if (m_fder_filtered.begin(), m_fder_filtered.end(),    // range
                            bind2nd(greater<double>(),0.1));  // criterion
        
        mctr.contraction_start.first = std::distance(m_fder_filtered.begin(),pos);
        auto max_accel = std::min_element(m_fder.begin()+ mctr.contraction_start.first ,m_fder.begin()+mctr.contraction_peak.first);
        mctr.contraction_max_acceleration.first = std::distance(m_fder.begin()+ mctr.contraction_start.first, max_accel);
        mctr.contraction_max_acceleration.first += mctr.contraction_start.first;
        auto max_relax = std::max_element(m_fder.begin()+ mctr.contraction_peak.first ,m_fder.end());
        mctr.relaxation_max_acceleration.first = std::distance(m_fder.begin()+ mctr.contraction_peak.first, max_relax);
        mctr.relaxation_max_acceleration.first += mctr.contraction_peak.first;
        
        // Initialize rpos to point to the element following the last occurance of a value greater than 0.1
        // If there is no such value, initialize rpos = to begin
        // If the last occurance is the last element, initialize this it to end
        dItr_t rpos = find_if (m_fder_filtered.rbegin(), m_fder_filtered.rend(),    // range
                               bind2nd(greater<double>(),0.1)).base();  // criterion
        mctr.relaxation_end.first = std::distance (m_fder_filtered.begin(), rpos);
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
        m_ctr.contraction_peak.first = get_most_contracted(acid);
        load(acid);
        measure_contraction_at_point(m_ctr.contraction_peak.first, m_ctr);
    }
    
    const contractionMesh& pols () const { return m_ctr; }
    
private:
    typedef vector<double>::iterator dItr_t;
    double m_start_thr;
    double m_end_thr;
    unsigned long m_median_window;
    size_t m_minimum_contraction_width;
    
    mutable contractionMesh m_ctr;
    mutable std::vector<double> m_fder;
    mutable std::vector<double> m_fder_filtered;
    mutable bool m_valid;
};


#endif





