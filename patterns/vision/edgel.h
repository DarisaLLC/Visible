
#ifndef _isocline_H
#define _isocline_H

#include <vector>
#include <iostream>
#include "pair.hpp"
#include "vision/roiWindow.h"
#include <tuple>
#include <map>
#include "lineseg.hpp"


typedef std::tuple<iPair, fVector_2d, uint8_t, uint8_t, int> protuple;

static void make_probe(int x, int y, uint8_t ang, protuple & fillit)
{
    std::get<0>(fillit) = iPair(x, y);
    std::get<1>(fillit) = fVector_2d(x,y);
    std::get<2>(fillit) = ang;
    std::get<3>(fillit) = 0;
    std::get<4>(fillit) = 0;
}

class isocline;
typedef std::vector<isocline> isocline_vector_t;
typedef std::vector<isocline_vector_t> chains_directed_t;



class isocline
{
public:

    static unsigned int chain_count (const chains_directed_t&);
    static unsigned int edge_count (const chains_directed_t&);
    
    
    isocline()
    {
        clear ();
    }
    isocline(const std::vector<protuple>::const_iterator start, const std::vector<protuple>::const_iterator stop)
    {
        clear ();
        for (auto pp = start; pp != stop; pp++) add (*pp);
    }
    
    void anchor(const iRect & rect)
    {
        m_anchor = rect;
        m_enclosing = fRect ();
    }
    
    size_t add(const protuple& tup)
    {
        return add (std::get<0>(tup).x(), std::get<0>(tup).y(), std::get<2>(tup));
    }
    
    size_t add(int x, int y, uint8_t a8)
    {
        
        switch(m_probes.size())
        {
            case 0:
            case 1:
                m_ends[m_probes.size()] = fVector_2d(x,y);
                break;
            default:
                m_ends[1] = fVector_2d(x,y);
                break;
        }
        
        if (m_probes.size() > 1)
        {
            if (m_probes.size() == 2)
                m_enclosing = fRect(fPair(m_ends[0].x(), m_ends[0].y()),
                                    fPair(m_ends[1].x(), m_ends[1].y()));
            else
                m_enclosing.include(m_ends[1].x(), m_ends[1].y());
            m_seg = fLineSegment2d(m_ends[0], m_ends[1]);
        }
        
        protuple newp;
        make_probe(x, y, a8, newp);
        m_sum += fVector_2d(x, y);
        m_a8_sum += a8;
        m_probes.push_back(newp);
        
        
        return m_probes.size();
    }
    
    size_t count () const { return m_probes.size(); }
    fRect enclosing () const { return m_enclosing; }
    
    fVector_2d mid () const
    {
        if (m_probes.empty()) return fVector_2d ();
        return m_sum / m_probes.size();
    }

    const fVector_2d& first () const { return m_ends[0]; }
    const fVector_2d& last () const { return m_ends[1]; }
    
    uint8_t mean_angle () const { return m_a8_sum / m_probes.size(); }
    float mean_angle_norm () const { return mean_angle() / 255.0; }
    
    const std::vector<protuple>& container () const { return m_probes; }
    
    static std::vector<fRect> get_minbb_per_direction (const chains_directed_t& cba)
    {
        std::vector<fRect> bb (256);
        
        for (int dd = 0; dd < 256; dd++)
        {
            if (cba[dd].empty()) continue;
            fRect tmp;
            for (int ff = 0; ff < cba[dd].size(); ff++)
            {
                tmp |= cba[dd][ff].enclosing();
            }
            bb[dd] = tmp;
        }
        return bb;
    }

    // Run length: add probes as long as they overlap
//    static isocline combine (const isocline_vector_t& apart)
 //   {
        
//    }
    
private:
    std::vector<protuple> m_probes;
    fVector_2d m_sum;
    iRect m_anchor;
    fRect m_enclosing;
    uint32_t m_a8_sum;
    fVector_2d m_ends[2];
    fLineSegment2d m_seg;
    
    void clear ()
    {
        m_probes.resize(0);
        m_sum = fVector_2d ();
        m_anchor = iRect ();
        m_enclosing = fRect();
        m_ends[0] = m_ends[1] = fVector_2d ();
        m_a8_sum = 0;
    }
};



void extract(roiWindow<P8U> & mag, roiWindow<P8U> & angle, roiWindow<P8U> & peaks,
             chains_directed_t & output, int low_threshold);


#endif // _FEHF_H
