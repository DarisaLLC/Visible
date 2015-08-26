
#ifndef __STATS__
#define __STATS__


#include <stdint.h>
#include <limits>

template <class T>
class stats
{
    public:
    stats()
    : m_min(numeric_limits<T>::max_value()),
        m_max(numeric_limits<T>::min_value()),
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
    private:
    T m_min, m_max, m_total, m_squared_total;
    uintmax_t m_count;
};


#endif

