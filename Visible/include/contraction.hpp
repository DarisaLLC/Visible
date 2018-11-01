
#ifndef _CONTRACTION_H
#define _CONTRACTION_H

#include <stdio.h>
#include <algorithm>
#include <vector>
#include <memory>
#include <typeinfo>
#include <string>
#include <tuple>
#include <chrono>
#include "core/stats.hpp"
#include "core/stl_utils.hpp"
#include "core/signaler.h"
//#include "cardiomyocyte_model.hpp"

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

    typedef std::vector<contraction> contractionContainer_t;
    typedef std::vector<double> sigContainer_t;
    typedef std::vector<double>::iterator sigIterator_t;
    
    // Signals we provide
    // signal_contraction_available
    typedef void (sig_cb_contraction_analyzed) (contractionContainer_t&);

    // Factory create method
    static std::shared_ptr<contraction_analyzer> create();
    
    // Load raw entropies and the self-similarity matrix
    // If no self-similarity matrix is given, entropies are assumed to be filtered and used directly
    void load (const vector<double>& entropies, const vector<vector<double>>& mmatrix = vector<vector<double>>());
    
    // Compute Length Interpolation 
    void compute_interpolated_geometries( float min_length, float max_length);
    
    // @todo: add multi-contraction
    bool find_best () const;
    
    bool isValid () const { return mValidInput; }
    bool isOutputValid () const { return mValidOutput; }
    bool isPreProcessed () const { return mNoSMatrix; }
    size_t size () const { return m_entsize; }
    
    // Original
    const vector<double>& entropies () { return m_entropies; }
    
    // Filtered 
    const vector<double>& filtered () { return m_signal; }
    const std::pair<double,double>& filtered_min_max () { return m_filtered_min_max; };
  
    
    // Elongstion Over time
    const vector<double>& interpolated_length () { return m_interpolated_length; }
    const vector<double>& elongations () { return m_elongation; }
  
    
    const std::vector<index_val_t>& low_peaks () const { return m_peaks; }
    const std::vector<contraction>& contractions () const { return m_contractions; }
    
   
    
    // Adjusting Median Level
    /* @note: left in public interface as the UT assumes that.
     * @todo: refactor
     * Compute rank for all entropies
     * Steps:
     * 1. Copy and Calculate Median Entropy
     * 2. Subtract Median from the Copy
     * 3. Sort according to distance to Median
     * 4. Fill up rank vector
     */
    void set_median_levelset_pct (float frac) const { m_median_levelset_frac = frac;  }
    float get_median_levelset_pct () const { return m_median_levelset_frac; }
    
    // Static public functions. Enabling testing @todo move out of here
    static double Median_levelsets (const vector<double>& entropies,  std::vector<int>& ranks );
    static void savgol (const vector<double>& signal, vector<double>& dst, int winlen);
 
    
    
private:
    contraction_analyzer();
    void regenerate () const;
    void compute_median_levelsets () const;
    size_t recompute_signal () const;
    void clear_outputs () const;
    bool verify_input () const;
    bool savgol_filter () const;
  
    float entropy_compute_interpolated_geometries (sigIterator_t , float min_length, float max_length);
    
    mutable double m_median_value;
    mutable std::pair<double,double> m_filtered_min_max;
    mutable float m_median_levelset_frac;
    mutable vector<vector<double>>        m_SMatrix;   // Used in eExhaustive and
    vector<double>               m_entropies;
    vector<double>               m_accum;
    mutable vector<double>              m_signal;
    mutable vector<double>              m_interpolation;
    mutable vector<double>              m_elongation;
    mutable vector<double>              m_interpolated_length;
    mutable std::vector<int>            m_ranks;
    size_t m_entsize;
    mutable std::atomic<bool> m_cached;
    mutable bool mValidInput;
    mutable bool mValidOutput;
    mutable bool mNoSMatrix;
    mutable std::vector<index_val_t> m_peaks;
    mutable std::vector<contraction> m_contractions;
    
    double exponentialMovingAverageIrregular(
                                             double alpha, double sample, double prevSample,
                                             double deltaTime, double emaPrev )
    {
        double a = deltaTime / alpha;
        double u = exp( a * -1 );
        double v = ( 1 - u ) / a;
        
        double emaNext = ( u * emaPrev ) + (( v - u ) * prevSample ) +
        (( 1.0 - v ) * sample );
        return emaNext;
    }
    double exponentialMovingAverage( double sample, double alpha )
    {
        static double cur = 0;
        cur = ( sample * alpha ) + (( 1-alpha) * cur );
        return cur;
    }
    
protected:
    
    boost::signals2::signal<contraction_analyzer::sig_cb_contraction_analyzed>* signal_contraction_analyzed;
    
};




class contraction_profile_analyzer
{
public:
  
    
    contraction_profile_analyzer (double contraction_start_threshold = 0.1,
                          double relaxation_end_threshold = 0.1,
                         uint32_t movingMedianFilterSize = 7):
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
    
    const contractionMesh& contraction () const { return mctr; }
    
    bool measure_contraction_at_point (const size_t p_index)
    {
        if (! m_valid) return false;
        contractionMesh::clear(mctr);
        mctr.contraction_peak.first = p_index;
        
        auto thr = std::min(m_start_thr, m_end_thr);
        
        // find first element greater than 0.1
        auto pos = find_if (m_fder_filtered.begin(), m_fder_filtered.end(),    // range
                            std::bind2nd(greater<double>(),thr));  // criterion
        
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
                               std::bind2nd(greater<double>(),thr)).base();  // criterion
        mctr.relaxation_end.first = std::distance (m_fder_filtered.begin(), rpos);
        
        // Check the length of the entire contraction
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
        auto mostc = get_most_contracted(acid);
        load(acid);
        measure_contraction_at_point(mostc);
    }
    
    const std::vector<double>& first_derivative () const { return m_fder; }
    const std::vector<double>& first_derivative_filtered () const { return m_fder_filtered; }
    
private:
    typedef vector<double>::iterator dItr_t;
    double m_start_thr;
    double m_end_thr;
    uint32_t m_median_window;
    
    mutable contractionMesh mctr;
    mutable std::vector<double> m_fder;
    mutable std::vector<double> m_fder_filtered;
    mutable bool m_valid;
};



#endif





