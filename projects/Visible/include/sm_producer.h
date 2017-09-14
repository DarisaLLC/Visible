
#ifndef _SIMILARITY_PRODUCER_H
#define _SIMILARITY_PRODUCER_H

#include <stdio.h>
#include <deque>
#include <vector>
#include <memory>
#include "qtime_frame_cache.hpp"
#include <typeinfo>
#include <string>
#include <tuple>
#include "base_signaler.h"
#include "roiWindow.h"
#include <chrono>
#include "core/singleton.hpp"
#include "core/stats.hpp"

using namespace std;
using namespace boost;
using namespace svl;


typedef std::shared_ptr<class sm_producer> smProducerRef;


class sm_producer
{
public:
    
    enum sizeMappingOption : int { dontCare = 0,  mostCommon = 1, reportFail = 2 };
    enum outputOrderOption : int { input = 0,  sorted = 1, binned = 2 };
    
    typedef roiWindow<P8U> image_t;
    typedef std::vector<image_t> images_vector_t;
    typedef std::deque<double> sMatrixProjection_t;
    typedef std::deque< std::deque<double> > sMatrix_t;
    typedef std::tuple<size_t, double, fs::path, image_t> outuple_t;
    typedef std::vector<outuple_t> ordered_outuple_t;
    
    typedef void (sig_cb_content_loaded) ();
    typedef void (sig_cb_frame_loaded) (int, double);
    typedef void (sig_cb_frames_cached) ();
    typedef void (sig_cb_sm1d_available) ();
    typedef void (sig_cb_sm2d_available) ();
    sm_producer (bool auto_on_off = false);
    
    bool load_content_file (const string& fq_path);
    bool load_image_directory (const string& fq_path, sizeMappingOption szmap = dontCare);
    void load_images (const images_vector_t&);
    
    bool operator () (int start_frame = 0, int frames = 0) const;
    
  
    
    bool set_auto_run_on () const;
    bool set_auto_run_off () const;
    
    bool has_content () const;
    int bytes_per_pixel () const;
    int pixels_per_channel () const;
    int channels_per_plane () const;
    int planes_per_image () const;
    int frames_in_content () const;
    range_error last_error () const;
    
    
    const std::string& content_file () const;
    int process_start_frame () const;
    int process_last_frame () const;
    int process_count () const;
    
    /**
     *  Results Output & Options
     */
    const sMatrix_t& similarityMatrix () const;
    
    const sMatrixProjection_t& meanProjection (outputOrderOption ooo = input) const;
    
    const sMatrixProjection_t& shannonProjection (outputOrderOption ooo = input) const;
    
    const sMatrixProjection_t& medianLeveledProjection () const;
    
    /**
     *  Image Directory Output & Options
     *  The first two functions return the natural order ( i.e. input order )
     *
     */
    images_vector_t& images () const;
    const std::vector<fs::path >& paths () const;
    const ordered_outuple_t& ordered_input_output (const outputOrderOption) const;
    
    /**
     *  Callback Registration
     */
    /** \brief registers a callback function/method to a signal with the corresponding signature
     * \param[in] callback: the callback function/method
     * \return Connection object, that can be used to disconnect the callback method from the signal again.
     */
    template<typename T>
    boost::signals2::connection registerCallback (const std::function<T>& callback);
    
    /** \brief indicates whether a signal with given parameter-type exists or not
     * \return true if signal exists, false otherwise
     */
    template<typename T>
    bool providesCallback () const;
    
    
private:
    class spImpl;
    std::shared_ptr<spImpl> _impl;
    
};

/*
 * in Tandem with sm_producer
 *
 *
 *
 */

class sm_filter
{
public:
    sm_filter(const deque<double>& entropies, const deque<deque<double>>& mmatrix) :
    m_entropies (entropies), m_SMatrix (mmatrix), m_cached(false), mValid (false), m_median_levelset_frac (0.1)
    {
        m_matsize = m_entropies.size();
        m_ranks.resize (m_matsize);
        mValid = verify_input ();
    }
    
    bool isValid () const { return mValid; }
    size_t size () const { return m_matsize; }
    
    const deque<double>& entropies () { return m_entropies; }
    
    bool median_levelset_similarities (deque<double>& signal) const
    {
        if (verify_input())
        {
            compute_median_levelsets ();
            
            size_t count = std::floor (m_entropies.size () * m_median_levelset_frac);
            signal.resize(m_entropies.size (), 0.0);
            for (auto ii = 0; ii < signal.size(); ii++)
            {
                double val = 0;
                for (int index = 0; index < count; index++)
                {
                    // get the index
                    auto jj = m_ranks[index];
                    // fetch the cross match value for
                    val += m_SMatrix[jj][ii];
                }
                signal[ii] = val;
            }
            
            return true;
        }
        return false;
    }
    
    void set_median_levelset_pct (float frac) const { m_median_levelset_frac = frac; }
    float get_median_levelset_pct () const { return m_median_levelset_frac; }
    
private:
    void compute_median_levelsets () const
    {
        if (m_cached) return;
        
        vector<double> entcpy;
        
        for (const auto val : m_entropies)
            entcpy.push_back(1.0 - val);
        
        auto median_value = svl::Median(entcpy);
        
        // Subtract median from all
        for (double& val : entcpy)
            val = std::abs(val - median_value );
        
        // Sort according to distance to the median. small to high
        m_ranks.resize(m_matsize);
        std::iota(m_ranks.begin(), m_ranks.end(), 0);
        auto comparator = [&entcpy](double a, double b){ return entcpy[a] < entcpy[b]; };
        std::sort(m_ranks.begin(), m_ranks.end(), comparator);
        m_cached = true;
    }
    
    bool verify_input () const
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
    
    mutable float m_median_levelset_frac;    
    mutable deque<deque<double>>        m_SMatrix;   // Used in eExhaustive and
    deque<double>               m_entropies;
    mutable std::vector<int>            m_ranks;
    size_t m_matsize;
    mutable std::atomic<bool> m_cached;
    mutable bool mValid;
    
    
};



#endif /* __VF_SIMILARITY_H */





