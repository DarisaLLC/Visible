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
	const bool cl (svl::RealEq (center - left, (float) (1.e-10)));
	const bool cr (svl::RealEq (center - right, (float) (1.e-10)));
	
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


template<class P>
float interpolatePeak(const vector<P>& signal, uint32_t index, P* ampP = NULL)
{
	float delta, damp(0.0f), *dampPtr (NULL);
	if (ampP) dampPtr = &damp;
	
	if (index == (signal.size()-1))  { // Peak is pegged at end
		delta = parabolicFit((float) signal[index-1],
							 (float) signal[index], 0.0f, dampPtr);
	}
	else if (index == 0) { // Peak is pegged at start
		delta = parabolicFit(0.0f, (float) signal[0], (float) signal[1], dampPtr);
	}
	else { // Interpolate peak location
		delta = parabolicFit((float) signal[ index-1], (float) signal[index],
							 (float) signal[index+1], dampPtr);
	}
	
	if (ampP)
		{
		P amp;
		amp = damp;
		* ((P *) ampP) = amp;
		}
	
	return index + delta;
}



template<class P>
void oneDpeakDetect(const vector<P>& signal, vector<uint32_t>& peakBin,
					vector<P>& peakLoc, vector<P>& peakVal,
					const P hysteresisPct, bool DEBUG_PD = false)
{
	peakBin.clear();
	peakLoc.clear();
	peakVal.clear();
	
	if (signal.empty())
		return;
	
	/* Calculate peaks using hysteresisPct to threshold for true valleys.
	 */
	int32_t pPeak = 0, pValley = 0, min = 0, max = 0;
	P peakThresh = signal[pPeak]*(1 - hysteresisPct);
	P valleyThresh = signal[pValley]*(1 + hysteresisPct);
	
	if (DEBUG_PD) printf("signal[%d] = %f peakThresh %f valleyThresh %f\n",
						 0, signal[0], peakThresh, valleyThresh);
	
	for (int32_t i = 1; i < (int32_t)signal.size(); i++) {
		if (DEBUG_PD) printf("signal[%d] = %f", i, signal[i]);
		
		if ((pPeak >= 0) && signal[i] < peakThresh) {
				// Found peak
			peakBin.push_back((uint32_t)pPeak);
			P peakValue;
			peakLoc.push_back(interpolatePeak(signal, (uint32_t)pPeak, &peakValue));
			peakVal.push_back(peakValue);
			if (DEBUG_PD) printf(" Peak (%f) @ %d.", signal[pPeak], pPeak);
			pPeak = -1;
			pValley = i;
			min = max = i;
			valleyThresh = signal[pValley]*(1 + hysteresisPct);
			if (DEBUG_PD)
				printf(" New pPeak %d pValley %d min %d max %d valleyThresh %f.\n",
					   pPeak, pValley, min, max, valleyThresh);
		}
		else if (signal[i] < signal[min]) {
			if (pValley >= 0) {
				pValley = i;
				valleyThresh = signal[pValley]*(1 + hysteresisPct);
				if (DEBUG_PD) printf(" New pValley %d valleyThresh %f.",
									 pValley, valleyThresh);
			}
			min = i;
			if (DEBUG_PD) printf(" New min %d.\n", min);
		}
		
		if ((pValley >= 0) && signal[i] > valleyThresh) {
				// Found valley
			if (DEBUG_PD) printf(" Valley (%f) @ %d.", signal[pValley], pValley);
			pPeak = i;
			pValley = -1;
			min = max = i;
			peakThresh = signal[pPeak]*(1 - hysteresisPct);
			if (DEBUG_PD)
				printf(" New pPeak %d pValley %d min %d max %d peakThresh %f.\n",
					   pPeak, pValley, min, max, peakThresh);
		}
		else if (signal[i] > signal[max]) {
			if (pPeak >= 0) {
				pPeak = i;
				peakThresh = signal[pPeak]*(1 - hysteresisPct);
				if (DEBUG_PD) printf(" New pPeak %d peakThresh %f.",
									 pPeak, peakThresh);
			}
			max = i;
			if (DEBUG_PD) printf(" New max %d.\n", max);
		}
		
	} // End of: for (int32 i = 1; i < signal.size(); i++) {
	
	if (pPeak >= 0) {
		if (DEBUG_PD) printf("Last Peak (%f) @ %d.\n", signal[pPeak], pPeak);
		peakBin.push_back((uint32_t)pPeak);
		P peakValue;
		peakLoc.push_back(interpolatePeak(signal, (uint32_t)pPeak, &peakValue));
		peakVal.push_back(peakValue);
	}
}

#endif
