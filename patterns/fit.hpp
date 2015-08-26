#ifndef __FIT_FPP__
#define __FIT_FPP__
#include "core.hpp"
#include "stats.hpp"

template<typename T, typename F = T>
F parabolicFit ( const T left, const T center, const T right, T *maxp = NULL)
{
    const bool cl (center == left);
    const bool cr (center == right);
	
    if ((!(center > left || cl) && !(center > right || cr)) &&
        (!(center < left || cl) && !(center < right || cr)))
        return std::numeric_limits<F>::infinity();

	
    const F s = left + right;
    const F d = right - left;
    const F c = center + center;
	
    // case: center is at max
    if (c == s)
    {
        if (maxp) *maxp = center;
        return 0;
    }
	
    // ymax for the quadratic
    if (maxp) *maxp = (4 * c * c - 4 * c * s + d * d) / (8 * (c - s));
	
    // position of the max
    return d / (2 * (c - s));
	
}

// @tbd this is inefficient

template <class Iterator>
double poleFinder (Iterator Ib, Iterator Ie, double& pose, basicStats& bs)
{
    unsigned int size = Ie - Ib;   
    Iterator maxd = max_element (Ib, Ie);
    double score = *maxd;
    double alignment = (double) distance  (Ib, maxd);

		if (!(size == 1 || maxd == Ib || maxd == Ie))
    {
        double left (*(maxd-1)), centre (*maxd), right (*(maxd+1));
        if (centre >= left && centre >= right)
        {
            alignment += parabolicFit (left, centre, right, &score);
        }
    }
    Iterator ip = Ib;
    bs.reset ();
    while (ip++ != Ie)
       bs.add ((double) *ip);
    
    pose = alignment;
    return score;    
}


// Parabolic fit around minima in a valley.

template <class Iterator, typename V>
V valleyFinder (Iterator Ib, Iterator Ie, V& pose)
{
	unsigned int size = Ie - Ib;   
	Iterator mind = min_element (Ib, Ie);
	V score = - (V) *mind;
	V alignment = (V) distance  (Ib, mind);
	
	if (!(size == 1 || mind == Ib || mind == Ie))
    {
			V left ((V) *(mind-1)), right ((V) *(mind+1));
			if (score <= left && score <= right)
        {
					alignment += parabolicFit (-left, score, -right, &score);
        }
    }
	
	pose = alignment;
	return - score;    
}

// 1D correlation: Normalized Correlation
//
template <class Iterator>
double normalized1DCorr (Iterator Ib, Iterator Ie, Iterator Mb)
{
  double sii(0.0), si(0.0), smm(0.0), sm(0.0), sim (0.0);
  int n;
  double rsq;

  n = Ie - Ib;
  assert (n);

  // Get container value type
  typedef typename std::iterator_traits<Iterator>::value_type value_type;

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


template<> 
inline float parabolicFit (const float left, const float center, const float right, float *maxp)
{
	const bool cl (core::RealEq (center - left, (float) (1.e-10)));
	const bool cr (core::RealEq (center - right, (float) (1.e-10)));
	
	if ((!(center > left || cl ) && !(center > right || cr )) &&
			(!(center < left || cl ) && !(center < right || cr )))
            return std::numeric_limits<float>::infinity();
	
	
	const float s = left + right;
	const float d = right - left;
	const float c = center + center;
	
	// case: center is at max
	if (c == s)
    {
			if (maxp) *maxp = center;
			return 0;
    }
	
	// ymax for the quadratic
	if (maxp) *maxp = (4 * c * c - 4 * c * s + d * d) / (8 * (c - s));
	
	// position of the max
	return d / (2 * (c - s));
	
}



#endif
