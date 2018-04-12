
#ifndef _SIMILARITY_PRODUCER_H
#define _SIMILARITY_PRODUCER_H

#include <stdio.h>
#include <algorithm>
#include <deque>
#include <vector>
#include <memory>
#include "qtime_frame_cache.hpp"
#include <typeinfo>
#include <string>
#include <tuple>
#include "signaler.h"
#include "roiWindow.h"
#include <chrono>
#include "core/singleton.hpp"
#include "core/stats.hpp"
#include "core/stl_utils.hpp"
#include "sg_filter.h"

using namespace std;
using namespace boost;
using namespace svl;
using namespace SGF;


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

typedef std::shared_ptr<sm_producer> smProducerRef;


class sm_filter
{
public:
    typedef std::pair<uint32_t,double> index_val_t;
    struct contraction
    {
        index_val_t contraction_start;
        index_val_t peak;
        index_val_t relaxation_end;
    };
    
    sm_filter(const deque<double>& entropies, const deque<deque<double>>& mmatrix) :
    m_entropies (entropies), m_SMatrix (mmatrix), m_cached(false),  mValid (false), m_median_levelset_frac (0.0)
    {
        m_matsize = m_entropies.size();
        m_ranks.resize (m_matsize);
        m_signal.resize (m_matsize);
        m_peaks.resize(0);
        mValid = verify_input ();
    }
    
    bool isValid () const { return mValid; }
    size_t size () const { return m_matsize; }
    
    const deque<double>& entropies () { return m_entropies; }
    const deque<double>& filtered () { return m_signal; }
    
    bool savgol_filter () const
    {
        m_signal.resize(m_entropies.size (), 0.0);
        int wlen = std::floor (100 * m_median_levelset_frac);
        
        savgol (m_entropies, m_signal, wlen);
        return true;
    }
    
    // @todo: add multi-contraction
    bool median_levelset_similarities () const
    {
        if (verify_input())
        {
            index_val_t lowest;
            lowest.first = -1;
            lowest.second = std::numeric_limits<double>::max ();
            compute_median_levelsets ();
            size_t count = std::floor (m_entropies.size () * m_median_levelset_frac);
            if (count == 0)
            {
                m_signal = m_entropies;
                auto min_itr = min_element( m_signal.begin(), m_signal.end() );
                lowest.first = std::distance(m_signal.begin(), min_itr);
                lowest.second = *min_itr;
            }
                
            else
            {
                m_signal.resize(m_entropies.size (), 0.0);
                for (auto ii = 0; ii < m_signal.size(); ii++)
                {
                    double val = 0;
                    for (int index = 0; index < count; index++)
                    {
                        // get the index
                        auto jj = m_ranks[index];
                        // fetch the cross match value for
                        val += m_SMatrix[jj][ii];
                    }
                    val = val / count;
                    m_signal[ii] = val;
                    if (val < lowest.second)
                    {
                        lowest.second = val;
                        lowest.first = ii;
                    }
                }
            }
            
            m_peaks.resize(0);
            m_peaks.emplace_back(lowest.first, m_signal[lowest.first]);
            contraction one;
            one.peak = m_peaks[0];
            m_contractions.emplace_back(one);
            
            std::cout << "[" << lowest.first << "] = " << lowest.second << " mfrac = " << m_median_levelset_frac << std::endl;
            
            
            //            auto iter_to_peak = m_signal.begin();
            //            std::advance (iter_to_peak, lowest.first);
            //            find_flat(signal.begin(), iter_to_peak);
            
            return true;
        }
        return false;
    }
    
    void set_median_levelset_pct (float frac) const { m_median_levelset_frac = frac;  }
    float get_median_levelset_pct () const { return m_median_levelset_frac; }
    
    const std::vector<index_val_t>& low_peaks () const
    {
        return m_peaks;
    }
    
    const std::vector<contraction>& contractions () const
    {
        return m_contractions;
    }
    
    static double Median_levelsets (const deque<double>& entropies,  std::vector<int>& ranks )
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
    
    
    
    static void savgol (const deque<double>& signal, deque<double>& dst, int winlen)
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
    
    
private:
    void norm_scale (const std::deque<double>& src, std::deque<double>& dst) const
    {
        deque<double>::const_iterator bot = std::min_element (src.begin (), src.end() );
        deque<double>::const_iterator top = std::max_element (src.begin (), src.end() );
        
        if (svl::equal(*top, *bot)) return;
        double scaleBy = *top - *bot;
        dst.resize (src.size ());
        for (int ii = 0; ii < src.size (); ii++)
            dst[ii] = (src[ii] - *bot) / scaleBy;
    }
    
    void compute_median_levelsets () const
    {
        if (m_cached) return;
        sm_filter::Median_levelsets (m_entropies, m_ranks);
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
#if 0
    index_val_t find_flat (const std::deque<double>::iterator& from, const std::deque<double>::iterator& to) const
    {
        
        std::deque<double> adjdiff;
        std::adjacent_difference (from, to, std::ostream_iterator<double>(std::cout, " "));
        std::adjacent_difference (from, to, adjdiff);
        std::deque<double>::iterator mini = std::min_element(adjdiff.begin(), adjdiff.end());
        auto dis = std::distance(from, mini);
        std::cout << dis << " = " << *mini << std::endl;
        
    }
#endif
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
    
};



#endif /* __VF_SIMILARITY_H */





