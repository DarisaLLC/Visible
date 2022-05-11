//
//  correlation1d.hpp
//  Visible
//
//  Created by Arman Garakani on 9/18/19.
//

#ifndef correlation1d_h
#define correlation1d_h

#include <iterator>
#include "core/fit.hpp"
#include "core/stats.hpp"
#include "core/core.hpp"

using namespace std;

namespace svl{
// 1D correlation: Normalized Correlation
//
template <class Iterator>
double f1dNormalizedCorr (Iterator Ib, Iterator Ie, Iterator Mb, Iterator Me)
{
    double sii(0.0), si(0.0), smm(0.0), sm(0.0), sim (0.0);

    double rsq;
    
    auto n = Ie - Ib;
    assert (n);
    assert ((Me - Mb) == n);
    
    Iterator ip = Ib;
    Iterator mp = Mb;
    
    while (ip < Ie)
    {
        double iv (*ip);
        double mv (*mp);
        si += iv;
        sm += mv;
        sii += (iv * iv);
        smm += (mv * mv);
        sim += (iv * mv);
        
        ip++;
        mp++;
    }
    
    double cross = ((n * sim) - (si * sm));
    double energyA = ((n * sii) - (si * si));
    double energyB = ((n * smm) - (sm * sm));
    
    energyA = energyA * energyB;
    rsq = 0.;
    if (energyA != 0.)
        rsq = (cross * cross) / energyA;
    
    return rsq;
}


template <class Iterator>
double f1dCrossCorr (Iterator Ib, Iterator Ie, Iterator Mb, Iterator Me)
{
    double sim (0.0);

    auto n = Ie - Ib;
    assert (n);
    assert ((Me - Mb) == n);
    
    // Get container value type
    typedef typename std::iterator_traits<Iterator>::value_type value_type;
    
    Iterator ip = Ib;
    Iterator mp = Mb;
    
    while (ip < Ie)
    {
        value_type iv (*ip);
        value_type mv (*mp);
        sim += (iv * mv);
        ip++;
        mp++;
    }
    
    return sim;
}


// 1D auto-correlation: Normalized Correlation
//
template <class Iterator>
void f1dAutoCorr (Iterator Ib, Iterator Ie, Iterator acb, bool op = true)
{
    // Get container value type
    typedef typename std::iterator_traits<Iterator>::value_type value_type;
    
    int32_t n = distance (Ib, Ie);
    vector<value_type> ring (n + n);
    Iterator ip = Ib;
    int32_t i;
    for (i = 0; i < n; i++) ring[i] = *ip++;
    ip = Ib;
    for (; i < (int32_t) ring.size(); i++) ring[i] = *ip++;
    
    ip = ring.begin();
    Iterator cp = acb;
    
    if (op)
    {
        for (int32_t i = 0; i < n; i++, ip++)
            *cp++ = f1dNormalizedCorr (Ib, Ie, ip, ip + n);
    }
    else
    {
        for (int32_t i = 0; i < n; i++, ip++)
            *cp++ = f1dCrossCorr (Ib, Ie, ip, ip + n);
    }
}

template <class Iterator>
void f1dAutoCorr (Iterator Ib, Iterator Ie, vector<double>& acb, bool op = true)
{
    // Get container value type
    typedef typename std::iterator_traits<Iterator>::value_type value_type;
    
    auto n = std::distance (Ib, Ie);
    vector<value_type> ring (n + n);
    Iterator ip = Ib;
    uint32_t i;
    for (i = 0; i < (uint32_t) n; i++) ring[i] = *ip++;
    ip = Ib;
    for (; i < ring.size(); i++) ring[i] = *ip++;
    
    ip = ring.begin();
    acb.resize(n);
    vector<double>::iterator cp = acb.begin();
    
    if (op)
    {
        for (uint32_t i = 0; i < (uint32_t) n; i++, ip++)
            *cp++ = f1dNormalizedCorr (Ib, Ie, ip, ip + n);
    }
    else
    {
        for (uint32_t i = 0; i < (uint32_t) n; i++, ip++)
            *cp++ = f1dCrossCorr (Ib, Ie, ip, ip + n);
    }
}

// 1D Signal Registration:
// @function f1dRegister
// @description return best registration point of sliding model represented by Mb/Me on Ib/Ie
// Slide has to be greater or equal to 1. Mb is lined up with (Ib+slide) with 0 passed in for
// slide number of bins on the other end (and reduced count in calculation of correlation). Similarly
// (Mb+slide) is matched with Ib with first slide model bins treated as 0s and reduction of count accordingly
// @todo: constant time implementation

template <class Iterator>
double f1dRegister (Iterator Ib, Iterator Ie, Iterator Mb, Iterator Me, uint32_t slide, double& pose)
{
    
    assert (slide >= 1);
    vector<double> space (slide + slide + 1);
    vector<double>::iterator orgItr = space.begin() + slide;
    vector<double>::iterator slidItr;
    
    // Sliding I to the right
    slidItr = orgItr;
    for (uint32_t i = 0; i < (slide + 1); i++)
    {
        *slidItr-- = f1dNormalizedCorr (Ib + i, Ie, Mb, Me - i);
    }
    
    // Sliding I to the left (orgin already done)
    slidItr = orgItr+1;
    for (uint32_t i = 1; i < (slide + 1); i++)
    {
        *slidItr++ = f1dNormalizedCorr (Ib, Ie - i, Mb + i, Me);
    }
    
    vector<double>::iterator endd = space.end();
    std::advance (endd, -1); // The last guy
    
    vector<double>::iterator maxd = max_element (space.begin(), space.end());
    pose = *maxd;
    double alignment = (double) std::distance  (space.begin(), maxd);
    
    // If the middle is a peak, interpolate around it.
    if (!(space.size() == 1 || maxd == space.begin() || maxd == endd))
    {
        double left (*(maxd-1)), centre (*maxd), right (*(maxd+1));
        if (centre >= left && centre >= right)
        {
            alignment += parabolicFit (left, centre, right, &pose);
        }
    }
    else
    {
        //      cerr << "Edge Condition" << endl;
        //      rfPRINT_STL_ELEMENTS (space);
    }
    
    return alignment;
    
}


template <class Iterator>
double f1dRegister (Iterator Ib, Iterator Ie, Iterator Mb, Iterator Me, uint32_t slide, double& pose, double& acc)
{
    
    assert (slide >= 1);
    vector<double> space (slide + slide + 1);
    vector<double>::iterator orgItr = space.begin() + slide;
    vector<double>::iterator slidItr;
    
    // Sliding I to the right
    slidItr = orgItr;
    for (uint32_t i = 0; i < (slide + 1); i++)
    {
        *slidItr-- = f1dNormalizedCorr (Ib + i, Ie, Mb, Me - i);
    }
    
    // Sliding I to the left (orgin already done)
    slidItr = orgItr+1;
    for (uint32_t i = 1; i < (slide + 1); i++)
    {
        *slidItr++ = f1dNormalizedCorr (Ib, Ie - i, Mb + i, Me);
    }
    
    vector<double>::iterator endd = space.end();
    std::advance (endd, -1); // The last guy
    
    vector<double>::iterator maxd = max_element (space.begin(), space.end());
    pose = *maxd;
    double alignment = (double) std::distance  (space.begin(), maxd);
    svl::stats<double> stat;
    vector<double>::iterator tmpIt = space.begin ();
    for (;tmpIt < space.end (); tmpIt++)
    {
        if (tmpIt != maxd) stat.add (*tmpIt);
    }
    //    cerr << space << stat << endl;
    
    // If the middle is a peak, interpolate around it.
    if (!(space.size() == 1 || maxd == space.begin() || maxd == endd))
    {
        double left (*(maxd-1)), centre (*maxd), right (*(maxd+1));
        acc = stat.mean ();
        if (centre >= left && centre >= right)
        {
            alignment += parabolicFit (left, centre, right, &pose);
        }
    }
    else
    {
        //        cerr << "Edge Condition" << endl;
        //        rfPRINT_STL_ELEMENTS (space);
    }
    
    return alignment;
    
}



template <class Iterator>
double f1dRegister (Iterator Ib, Iterator Ie, Iterator Mb, Iterator Me,
                     uint32_t slideLeft, uint32_t slideRight, double& pose)
{
    uint32_t slide = slideLeft + slideRight;
    assert (slide >= 1);
    vector<double> space (slideLeft + slideRight + 1);
    vector<double>::iterator orgItr = space.begin() + slideRight;
    vector<double>::iterator slidItr;
    
    // Sliding I to the right
    slidItr = orgItr;
    for (auto i = 0; i < (slideRight + 1); i++)
    {
        *slidItr-- = f1dNormalizedCorr (Ib + i, Ie, Mb, Me - i);
    }
    
    // Sliding I to the left (orgin already done)
    slidItr = orgItr+1;
    for (auto i = 1; i < (slideLeft + 1); i++)
    {
        *slidItr++ = f1dNormalizedCorr (Ib, Ie - i, Mb + i, Me);
    }
    
    vector<double>::iterator endd = space.end();
    std::advance (endd, -1); // The last guy
    
    vector<double>::iterator maxd = max_element (space.begin(), space.end());
    pose = *maxd;
    double alignment = (double) std::distance  (space.begin(), maxd);
    
    // If the middle is a peak, interpolate around it.
    if (!(space.size() == 1 || maxd == space.begin() || maxd == endd))
    {
        double left (*(maxd-1)), centre (*maxd), right (*(maxd+1));
        if (centre >= left && centre >= right)
        {
            alignment += parabolicFit (left, centre, right, &pose);
        }
    }
    else
    {
        //      cerr << "Edge Condition" << endl;
        //      rfPRINT_STL_ELEMENTS (space);
    }
    
    return alignment;
    
}



// 1D Histogram Intersection
//
template <class Iterator>
double f1dSignalIntersection (Iterator Ib, Iterator Ie, Iterator Mb, Iterator Me)
{
    double sm(0.0), sim (0.0);
    int32_t n;
    
    n = Ie - Ib;
    assert (n);
    assert ((Me - Mb) == n);
    
    // Get container value type
    typedef typename std::iterator_traits<Iterator>::value_type value_type;
    
    Iterator ip = Ib;
    Iterator mp = Mb;
    
    while (ip < Ie)
    {
        value_type iv (*ip);
        value_type mv (*mp);
        sm += mv;
        sim += rmMin (iv, mv);
        ip++;
        mp++;
    }
    
    double cross = sim / sm;
    return cross;
}


// 1D Histogram Euclidean Distance
//
template <class Iterator>
double f1dSignalNormEuclideanIntersection (Iterator Ib, Iterator Ie, Iterator Mb, Iterator Me)
{
    double  sim (0.0);
    int32_t n;
    
    n = Ie - Ib;
    assert (n);
    assert ((Me - Mb) == n);
    
    // Get container value type
    typedef typename std::iterator_traits<Iterator>::value_type value_type;
    
    Iterator ip = Ib;
    Iterator mp = Mb;
    
    while (ip < Ie)
    {
        value_type iv (*ip);
        value_type mv (*mp);
        sim += squareOf (iv - mv);
        ip++;
        mp++;
    }
    
    return 1.0 - sim;
}

} // namespace svl

#endif /* correlation1d_h */

