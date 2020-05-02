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
#include "core/pair.hpp"
#include "cardiomyocyte_model.hpp"
#include "core/stats.hpp"
#include "core/stl_utils.hpp"
#include "core/signaler.h"
#include "core/lineseg.hpp"
#include "eigen_utils.hpp"
#include "input_selector.hpp"
#include "time_index.h"
#include "timed_value_containers.h"

using namespace std;
using namespace boost;
using namespace svl;

/* contractionMesh contains all measured data for a contraction
 * duration of a contraction is relaxaton_end - contraction_start
 * peak contraction is at contraction peak
 */

struct contractionMesh : public std::pair<size_t,size_t>
{
    //@note: pair representing frame number and frame time
    typedef std::pair<size_t,double> index_val_t;
    // container
    typedef std::vector<double> sigContainer_t;

    index_val_t contraction_start;
    index_val_t contraction_max_acceleration;
    index_val_t contraction_peak;
    index_val_t relaxation_max_acceleration;
    index_val_t relaxation_end;
    
    double relaxation_visual_rank;
    double max_length;
    cell_id_t m_uid;
 
   sigContainer_t             elongation;
   sigContainer_t             interpolated_length;
   sigContainer_t             force;
    
    cm::cardio_model               cardioModel;

    static size_t timeCount(const contractionMesh& a){
        return a.relaxation_end.first - a.contraction_start.first;
    }
    static size_t first(const contractionMesh& a){
        return a.contraction_start.first;
    }
    static size_t last (const contractionMesh& a){
        return a.relaxation_end.first;
    }
    
    
    static bool selfCheck (const contractionMesh& a){
        bool ok = a.relaxation_end.first > a.contraction_start.first;
        ok &=  a.contraction_peak.first > a.contraction_start.first;
        ok &=  a.contraction_peak.first < a.relaxation_end.first;
        auto fc = ok ? (a.relaxation_end.first - a.contraction_start.first) : 0;
        ok &= a.force.size() == size_t(fc);
        ok &= a.elongation.size() == size_t(fc);
        ok &= a.interpolated_length.size() == size_t(fc);
        return ok;
        
    }
    
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
typedef std::shared_ptr<contractionProfile> profileRef;



class contractionLocator : public ca_signaler, std::enable_shared_from_this<contractionLocator>
{
public:
    
    class params{
    public:
        params ():m_median_levelset_fraction(0.07), m_minimum_contraction_time (0.32), m_frame_duration(0.020){
            update ();
        }
        float median_levelset_fraction()const {return m_median_levelset_fraction; }
        void median_levelset_fraction(const float newval)const {m_median_levelset_fraction = newval; }
        float minimum_contraction_time () const { return m_minimum_contraction_time; }
        void minimum_contraction_time (const float newval) const {
            m_minimum_contraction_time = newval;
            update ();
        }
        float frame_duration () const { return m_frame_duration; }
        void frame_duration (float newval) const {
            m_frame_duration = newval;
            update ();
        }
        
        uint32_t minimum_contraction_frames () const { return m_pad_frames; }
        
    private:
        void update () const {
            m_pad_frames = m_minimum_contraction_time / m_frame_duration;
        }
        mutable float m_median_levelset_fraction;
        mutable float m_minimum_contraction_time;
        mutable float m_frame_duration;
        mutable uint32_t m_pad_frames;
    };
    
    using contraction_t = contractionMesh;
    using index_val_t = contractionMesh::index_val_t;
    using sigContainer_t = contractionMesh::sigContainer_t;
  
    typedef std::shared_ptr<contractionLocator> Ref;
    typedef std::vector<contraction_t> contractionContainer_t;

    // Signals we provide
    // signal_contraction_available
    // signal pci is median level processed
    typedef void (sig_cb_pci_available) (std::vector<double>&);
    typedef void (sig_cb_contraction_ready) (contractionContainer_t&, const input_channel_selector_t& );
    typedef void (sig_cb_cell_length_ready) (sigContainer_t&);
    typedef void (sig_cb_force_ready) (sigContainer_t&);
    
    // Factory create method
    static Ref create(const input_channel_selector_t&,  const uint32_t& body_id, const contractionLocator::params& params = contractionLocator::params());
    Ref getShared();
    // Load raw entropies and the self-similarity matrix
    // If no self-similarity matrix is given, entropies are assumed to be filtered and used directly
    // // input selector -1 entire index mobj index
    void load (const vector<double>& entropies, const vector<vector<double>>& mmatrix = vector<vector<double>>());

    // Update with most recent median level set
    void update () const;
    
    // @todo: add multi-contraction
    bool locate_contractions () ;
    
    bool isValid () const { return mValidInput; }
    bool isOutputValid () const { return mValidOutput; }
    bool isPreProcessed () const { return mNoSMatrix; }
    size_t size () const { return m_entsize; }
    const uint32_t id () const { return m_id; }
    const input_channel_selector_t& input () const { return m_in; }
    
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
    contractionLocator(const input_channel_selector_t&,  const uint32_t& body_id, const contractionLocator::params& params = contractionLocator::params ());
    contractionLocator::params m_params;
    

    void compute_median_levelsets () const;
    size_t recompute_signal () const;
    void clear_outputs () const;
    bool verify_input () const;
    bool savgol_filter () const;
    bool get_contraction_at_point (int src_peak_index, const std::vector<int>& peak_indices, contraction_t& ) const;
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
    mutable bool mValidInput;
    mutable bool mValidOutput;
    mutable bool mNoSMatrix;
    mutable std::vector<index_val_t> m_peaks;
    mutable contractionContainer_t m_contractions;
    mutable int m_input; // input source
    mutable std::atomic<bool> m_cached;
    mutable int m_id;
    mutable std::vector<double> m_ac;
    mutable std::vector<double> m_bac;
    input_channel_selector_t m_in;
    
protected:
    boost::signals2::signal<contractionLocator::sig_cb_cell_length_ready>* cell_length_ready;
    boost::signals2::signal<contractionLocator::sig_cb_force_ready>* total_reactive_force_ready;
    boost::signals2::signal<contractionLocator::sig_cb_contraction_ready>* signal_contraction_ready;
    boost::signals2::signal<contractionLocator::sig_cb_pci_available>* signal_pci_available;
    
};



class contractionProfile
{
public:
    using contraction_t = contractionMesh;
    using index_val_t = contractionMesh::index_val_t;
    using sigContainer_t = contractionMesh::sigContainer_t;
    
    contractionProfile (contraction_t&);

    static profileRef create(contraction_t& ct){
        return profileRef(new contractionProfile(ct));
    }
    // Compute Length Interpolation for measured contraction
    void compute_interpolated_geometries_and_force(const std::vector<double>& );
    
    const contraction_t& contraction () const { return m_ctr; }
    const double& relaxed_length () const { return m_relaxed_length; }
    const sigContainer_t& profiled () const { return m_fder; }
    const sigContainer_t& force () const { return m_force; }
    const sigContainer_t& interpolatedLength () const { return m_interpolated_length; }
    const sigContainer_t& elongation () const { return m_elongation; }
    
    
private:
    typedef vector<double>::iterator dItr_t;
    lsFit<double> m_line_fit;
    std::pair<uRadian,double> m_ls_result;
    mutable contraction_t m_ctr;
    double m_relaxed_length;
    mutable std::vector<double> m_fder;
 
    mutable sigContainer_t m_interpolated_length;
    mutable sigContainer_t m_elongation;
    mutable sigContainer_t m_force;

};



#endif





