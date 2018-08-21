
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
    const int32_t length =  a.size();
    if ( length == 0 )
        return 0.0;
    
    const bool is_odd = length % 2;
    const int array_length = (length / 2) + 1;
    std::vector<T> array (array_length);
    
    
    
    partial_sort_copy(a.begin(), a.end(), array.begin(), array.end());
    
    if (is_odd)
        return double(array [array_length-1]);
    else
        return double( (array [array_length - 2] + array [array_length-1]) / T(2) );
}

}

#endif

