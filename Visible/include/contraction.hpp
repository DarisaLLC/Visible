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
#include <numeric>
#include <iterator>
#include <boost/ptr_container/ptr_vector.hpp>

#include "cardiomyocyte_model.hpp"
#include "core/stats.hpp"
#include "core/stl_utils.hpp"
#include "core/signaler.h"
#include "core/lineseg.hpp"
#include "eigen_utils.hpp"

using namespace std;
using namespace boost;
using namespace svl;


struct contractionMesh
{
public:
    //@note: pair representing frame number and frame time
    typedef std::pair<size_t,double> index_val_t;
    
    index_val_t contraction_start;
    index_val_t contraction_max_acceleration;
    index_val_t contraction_peak;
    index_val_t relaxation_max_acceleration;
    index_val_t relaxation_end;
    
    double relaxation_visual_rank;
    double max_length;

    vector<double>             m_elongation;
    vector<double>             m_interpolated_length;
    vector<double>             m_force;
    
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

class contractionProfile;

class contractionLocator : public ca_signaler, std::enable_shared_from_this<contractionLocator>
{
public:
    
    class params{
    public:
        params ():m_median_levelset_fraction(0.07) {}
        float median_levelset_fraction()const {return m_median_levelset_fraction; }
        
    private:
        float m_median_levelset_fraction;
    };
    
    using contraction_t = contractionMesh;
    using index_val_t = contractionMesh::index_val_t;

    typedef std::shared_ptr<contractionLocator> Ref;
    typedef std::vector<contractionProfile::Ref> contractionContainer_t;

    // Signals we provide
    // signal_contraction_available
    typedef void (sig_cb_contraction_analyzed) (contractionContainer_t&);
    typedef void (sig_cb_contraction_interpolated_length) (contractionContainer_t&);

    // Factory create method
    static Ref create();
    Ref getShared() const;
    
    // Load raw entropies and the self-similarity matrix
    // If no self-similarity matrix is given, entropies are assumed to be filtered and used directly
    void load (const vector<double>& entropies, const vector<vector<double>>& mmatrix = vector<vector<double>>());
    // @todo: add multi-contraction
    bool find_best () const;
    
    bool isValid () const { return mValidInput; }
    bool isOutputValid () const { return mValidOutput; }
    bool isPreProcessed () const { return mNoSMatrix; }
    size_t size () const { return m_entsize; }
    
    // Original
    const vector<double>& entropies () { return m_entropies; }
    
    // LevelSet corresponding to last coverage pct setting
    const vector<double>& leveled () { return m_signal; }
    const std::pair<double,double>& leveled_min_max () { return m_leveled_min_max; };
  
    
    const std::vector<index_val_t>& low_peaks () const { return m_peaks; }
    const contractionContainer_t & contractions () const { return m_contractions; }
    
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
private:
    contractionLocator();
    contractionLocator::params m_params;
    
    void regenerate () const;
    void compute_median_levelsets () const;
    size_t recompute_signal () const;
    void clear_outputs () const;
    bool verify_input () const;
    bool savgol_filter () const;
    bool get_contraction_at_point (const size_t p_index, contraction_t& ) const;
    mutable double m_median_value;
    mutable std::pair<double,double> m_leveled_min_max;
    mutable float m_median_levelset_frac;
    mutable vector<vector<double>>        m_SMatrix;   // Used in eExhaustive and
    vector<double>               m_entropies;
    vector<double>               m_accum;
    mutable vector<double>              m_signal;
    mutable vector<double>              m_interpolation;

    mutable std::vector<int>            m_ranks;
    size_t m_entsize;
    mutable std::atomic<bool> m_cached;
    mutable bool mValidInput;
    mutable bool mValidOutput;
    mutable bool mNoSMatrix;
    mutable std::vector<index_val_t> m_peaks;
    mutable contractionContainer_t m_contractions;

 
protected:
    
    boost::signals2::signal<contractionLocator::sig_cb_contraction_analyzed>* signal_contraction_analyzed;
};

class cp_signaler : public base_signaler
{
    virtual std::string
    getName () const { return "cpSignaler"; }
};



class contractionProfile : public cp_signaler
{
public:
    using contraction_t = contractionMesh;
    using index_val_t = contractionMesh::index_val_t;

    typedef std::shared_ptr<contractionProfile> Ref;
    typedef std::vector<double> sigContainer_t;
    typedef std::vector<double> forceContainer_t;
    typedef std::vector<double>::const_iterator sigIterator_t;
    typedef std::vector<double>::const_iterator forceIterator_t;
    
    // Signals we provide
    typedef void (sig_cb_cell_length_ready) (sigContainer_t&);
    typedef void (sig_cb_force_ready) (sigContainer_t&);
    
    contractionProfile (contraction_t& , const std::shared_ptr<contractionLocator>& );

    static Ref create(contraction_t& ct, const contractionLocator::Ref& parent){
        return Ref(new contractionProfile(ct, parent ));
    }

    
    // Compute Length Interpolation for measured contraction
    void compute_interpolated_geometries_and_force(const double relaxed_length);
    
    const contraction_t& contraction () const { return m_ctr; }
    const double& relaxed_length () const { return m_relaxed_length; }
    const std::vector<double>& profiled () const { return m_fder; }
    
    
private:
    typedef vector<double>::iterator dItr_t;
    lsFit<double> m_line_fit;
    std::pair<uRadian,double> m_ls_result;
    mutable contraction_t m_ctr;
    double m_median;
    double m_relaxed_length;
    mutable std::vector<double> m_fder;
    mutable std::vector<double> m_interpolated_length;
    mutable std::vector<double> m_elongation;
    mutable std::vector<double> m_force;


    std::weak_ptr<contractionLocator> m_connect;
    
protected:
    boost::signals2::signal<contractionProfile::sig_cb_cell_length_ready>* cell_length_ready;
    boost::signals2::signal<contractionProfile::sig_cb_force_ready>* total_reactive_force_ready;
};



#endif





