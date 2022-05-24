
#ifndef __STATS__
#define __STATS__


#include <stdint.h>
#include <limits>
#include <iostream>
#include <cmath>

using namespace std;

namespace svl
{
template <class T>
class stats
{
    public:
    stats()
    : m_min(std::numeric_limits<T>::max()),
        m_max(std::numeric_limits<T>::min() ),
        m_total(0),
        m_squared_total(0),
        m_count(0)
    {}
    
    stats(const T& total, const T& total_squared, uint32_t count, const T& minv, const T& maxv):
    m_min(minv),
    m_max(maxv),
    m_total(total),
    m_squared_total(total_squared),
    m_count(count)
    
    {}
    
    void add(const T& val)
    {
        if (val < m_min)
            m_min = val;
        if (val > m_max)
            m_max = val;
        m_total += val;
        ++m_count;
        m_squared_total += val*val;
    }
    T minimum()const { return m_min; }
    T maximum()const { return m_max; }
    T total()const { return m_total; }
    T mean()const { return m_total / static_cast<T>(m_count); }
    uintmax_t count()const { return m_count; }
    T variance()const
    {
        T t = m_squared_total - m_total * m_total / m_count;
        t /= (m_count - 1);
        return t;
    }
    T rms()const
    {
        return sqrt(m_squared_total / static_cast<T>(m_count));
    }
    stats& operator+=(const stats& s)
    {
        if (s.m_min < m_min)
            m_min = s.m_min;
        if (s.m_max > m_max)
            m_max = s.m_max;
        m_total += s.m_total;
        m_squared_total += s.m_squared_total;
        m_count += s.m_count;
        return *this;
    }
    
    static void PrintTo(const stats<T>& stinst, std::ostream* stream_ptr)
    {
        if(! stream_ptr) return;
        *stream_ptr  << "Count: " << stinst.count ()              << std::endl;
        *stream_ptr << "Min  : " << stinst.minimum ()             << std::endl;
        *stream_ptr << "Max  : " << stinst.maximum ()             << std::endl;
        *stream_ptr << "Mean : " << stinst.mean ()                << std::endl;
        *stream_ptr << "Std  : " << std::sqrt(stinst.variance ()) << std::endl;
        *stream_ptr << "RMS  : " << stinst.rms ()                 << std::endl;
    }
    
    private:
    T m_min, m_max, m_total, m_squared_total;
    uintmax_t m_count;
};


/*! \fn
 \brief compute median of a list. If length is odd it returns the center element in the sorted list, otherwise (i.e.
 * even length) it returns the average of the two center elements in the list
 \note the definition is identical to Mathematica
 * Partial_sort_copy copies the smallest Nelements from the range [first, last) to the range [result_first, result_first + N)
 * where Nis the smaller of last - first and result_last - result_first .
 * The elements in [result_first, result_first + N) will be in ascending order.
 */

template <class T>
double Median (const std::vector<T>& a )
{
    auto length =  a.size();
    if ( length == 0 )
        return 0.0;
    
    const bool is_odd = length % 2;
    auto array_length = (length / 2) + 1;
    std::vector<T> array (array_length);
    
    
    
    partial_sort_copy(a.begin(), a.end(), array.begin(), array.end());
    
    if (is_odd)
        return double(array [array_length-1]);
    else
        return double( (array [array_length - 2] + array [array_length-1]) / T(2) );
}

    

    
    //-----------------------------------------------------------------------------------------------------------
    //----------------------------------- Computational Support Routines ----------------------------------------
    //-----------------------------------------------------------------------------------------------------------
    //Simple 'building block' routines.
    template <class C> typename C::value_type Min(C in);
    template <class C> typename C::value_type Max(C in);
    template <class C> typename C::value_type Sum(C in);
    template <class C> typename C::value_type Sum_Squares(C in);
    template <class C> typename C::value_type Percentile(C in, double frac);
    template <class C> typename C::value_type Median(C in);
    template <class C> typename C::value_type Mean(C in);
    template <class C> typename C::value_type Unbiased_Var_Est(C in);
    
    //double Sum(std::list<double> in);
    //double Sum_Squares(std::list<double> in);
    //double Median(std::list<double> in);
    //double Mean(std::list<double> in);
    //double Unbiased_Var_Est(std::list<double> in);
    
    //-----------------------------------------------------------------------------------------------------------
    //----------------------------------- Running Accumulators and Tallies --------------------------------------
    //-----------------------------------------------------------------------------------------------------------
    template <class C>
    class Running_MinMax {
    private:
        C PresentMin;
        C PresentMax;
        
    public:
        Running_MinMax();
        
        void Digest(C in);
        
        C Current_Min(void) const;
        C Current_Max(void) const;
    };
    
    
    //-----------------------------------------------------------------------------------------------------------
    //------------------------------------ Statistical Support Routines -----------------------------------------
    //-----------------------------------------------------------------------------------------------------------
    
    //These routines are applicable to any Student's t-test.
    double P_From_StudT_1Tail(double tval, double dof);
    double P_From_StudT_2Tail(double tval, double dof);
    
    //These routines compute a P-value from any z-score.
    double P_From_Zscore_2Tail(double zscore);
    double P_From_Zscore_Upper_Tail(double zscore);
    double P_From_Zscore_Lower_Tail(double zscore);
    
    
    //-----------------------------------------------------------------------------------------------------------
    //--------------------------------------- Paired Difference Tests -------------------------------------------
    //-----------------------------------------------------------------------------------------------------------
    //From Wikipedia (http://en.wikipedia.org/wiki/Paired_difference_test):
    //  "The most familiar example of a paired difference test occurs when subjects are measured before and
    //  after a treatment. Such a "repeated measures" test compares these measurements within subjects, rather
    //  than across subjects, and will generally have greater power than an unpaired test."
    //
    // These tests are useful for comparing two distributions in which datum measure the same thing.
    
    //Paired t-test for comparing the mean difference between paired datum.
    double P_From_Paired_Ttest_2Tail(const std::vector<std::array<double,2>> &paired_datum,
                                     double dof_override = -1.0);
    
    //Paired Wilcoxon signed-rank test for assessing whether population mean ranks differ between the two
    // samples. At the moment, P-values are only possible if there are ten or more unpruned points!
    //
    // NOTE: This is not the same as the "Wilcoxon rank-sum test."
    //
    double P_From_Paired_Wilcoxon_Signed_Rank_Test_2Tail(const std::vector<std::array<double,2>> &paired_datum);
    
    
    
    //"Two-sample unpooled t-test for unequal variances" or "Welch's t-test" for comparing the means
    // of two populations where the variance is not necessarily the same.
    //
    // NOTE: This test examines each distribution separately, instead of the differences between datum.
    double P_From_StudT_Diff_Means_From_Uneq_Vars(double M1, double V1, double N1,
                                                  double M2, double V2, double N2);
    
    //-----------------------------------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------------------------------
    
    //P-value for Pearson's r (aka linear correlation coefficient). This is an alternative to the t-test
    // which is thought to be more precise.
    double P_From_Pearsons_Linear_Correlation_Coeff_1Tail(double corr_coeff, double total_num_of_datum);
    double P_From_Pearsons_Linear_Correlation_Coeff_2Tail(double corr_coeff, double total_num_of_datum);
    
    //-----------------------------------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------------------------------
    
    double Q_From_ChiSq_Fit(double chi_square, double dof);
    
    
    //-----------------------------------------------------------------------------------------------------------
    //--------------------------------------- Z-test for observed mean ------------------------------------------
    //-----------------------------------------------------------------------------------------------------------
    double Z_From_Observed_Mean(double sample_mean, //Or observed mean.
                                double population_mean, //Or true mean.
                                double population_stddev, //Pop. std. dev. of the distribution (NOT of mean!)
                                double sample_size); //Number of datum used to compute sample_mean.
    
    double Z_From_Observed_Mean(double sample_mean, //Or observed mean.
                                double population_mean, //Or true mean.
                                double stddev_of_pop_mean); //Std. dev. of population's mean == std. err..
    

}

#endif

