/******************************************************************************
 *  Copyright (c) 2003 Reify Corp. All rights reserved.
 *
 *   File Name      rc_filter1d.h
 *   Creation Date  07/17/2003
 *   Author         Peter Roberts
 *
 * Some simple 1D filtering capability
 *
 *****************************************************************************/

#ifndef _RC_FILTER1D_H_
#define _RC_FILTER1D_H_

#include <vector>
#include "core/fit.hpp"
#include "core/pair.hpp"

template <class T>
class rc1dPeak
{
public:
  rc1dPeak() {};
  rc1dPeak(T v,const int32_t l) : mLocation(l), mValue(v) {};

  const int32_t location () const
  {
    return mLocation;
  }

  const T value () const
  {
    return mValue;
  }

  bool operator<(const rc1dPeak& rhs) const
  {return mValue < rhs.mValue;}
  bool operator==(const rc1dPeak& rhs) const
    {return mValue == rhs.mValue;}
  bool operator!=(const rc1dPeak& rhs) const;
  bool operator>(const rc1dPeak& rhs) const;

 private:
  int32_t mLocation;
  T mValue;
};

template<class P>
void rf1DPeakDetect(const vector<P>& signal, vector<uint32_t>& peakBin,
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
      peakLoc.push_back(rfInterpolatePeak(signal, (uint32_t)pPeak, &peakValue));
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
    peakLoc.push_back(rfInterpolatePeak(signal, (uint32_t)pPeak, &peakValue));
    peakVal.push_back(peakValue);
  }
}






/* rcEdgeFilter1DPeak - Encapsulation of peak strength (_value) and
 * location (_location).
 */
class rcEdgeFilter1DPeak
{
 public:
  rcEdgeFilter1DPeak() : _value(0), _location(0) {}

  int32_t _value, _location;
};
  
class rcEdgeFilter1D
{
 public:
  /* ctor - Initialize object with identity filter.
   */
  rcEdgeFilter1D();

  virtual ~rcEdgeFilter1D();

  /* filter - Define filter to be used by subsequent genPeaks() calls.
   */
  void filter(const vector<int32_t>& filter);

  /* genPeaks - Filter img using the current filter and return the
   * peaks found in peak. The locations of the returned peaks will be
   * in monotonically increasing order. If filteredImg is non-NULL, a
   * copy of the filtered image is also returned.
   *
   * For a potential edge location in the filtered image to be
   * considered a peak, its ABSOLUTE value must both be >= absMinPeak
   * and relMinPeak*stdDev(). Setting either absMinPeak or relMinPeak
   * to <= 0 makes them NOOPs.
   *
   * The returned location of any peak is the corresponding index into
   * the filtered image PLUS filterOffset. This allows users to get
   * results back that have already been adjusted to account for the
   * filter being used.
   */
 
  void genPeaks(const vector<float>& signal,
		vector<int32_t>* filteredImg,
		vector<rcEdgeFilter1DPeak>& peak,
		int32_t filterOffset,
		double absMinPeak, double relMinPeak);

    static bool peakDetect1d (const vector<float>& signal, dPair& extendPeaks);
    
  /* Intermediate result accessors. Returns intermediate calculations
   * generated by last call to genPeaks(). min() and max() are the
   * magnitudes of the minimum and maximum values in the filtered
   * image.
   */
  double stdDev() const { return _stdDev; }
 uint32_t min() const { return _min; }
 uint32_t max() const { return _max; }

  /* genFilter - Helper fct that will generate a filter of the form:
   * { -1, -1, . . ., 0, 0, . . ., 1, 1, . . .}
   *
   * where there are oneCnt number of -1's and 1's, and zeroCnt
   * number of 0's.
   *
   * oneCnt must be >= 1.
   */
  static void genFilter(vector<int32_t>& filter, 
			const uint32_t oneCnt, const uint32_t zeroCnt);

 private:
  vector<int32_t> _filter;
  vector<int32_t> _filteredImg;
  double _stdDev;
  uint32_t _min, _max;
};


template<class S, class D, class T>
void rfCtfDiff1d (deque<S>& src, int32_t halfWidth, deque<D>& dst, T& dum, bool direction)
{
  int32_t kernelWidth = halfWidth * 2 + 1; // one bin of leniency
 // if (src.size() < kernelWidth)
  //  rmExceptionMacro (<< "Ctf Size Error");

  dst.resize (src.size() - kernelWidth + 1);

  int32_t i;
  auto size = dst.size();
  const S* front1ptr = &src[0];
  const S* back1ptr = front1ptr;
  const S* front2ptr = front1ptr;
  const S* back2ptr = front1ptr;
  D* outptr = &dst[0];
  T tmp1;
  D dummy(0);

  int32_t rightSize_ (halfWidth);
  int32_t leftSize_ (halfWidth);

  if (rightSize_ && leftSize_)
    {
      if (rightSize_ == 1 && leftSize_ == 1)
	{
	  if (direction)
	    {
	      for (i=0; i<size; i++)
		outptr[i] = rfRound((T)back1ptr[i+1] - (T)back1ptr[i], dummy);
	    } else
	    {
	      for (i=0; i<size; i++)
		outptr[i] = rfRound((T)back1ptr[i] - (T)back1ptr[i+1], dummy);
	    }
	} else
	{
	  tmp1 = 0;
	  if (direction) for (i = 0; i < leftSize_; i++)  tmp1 = tmp1 - (T) *front2ptr++;
	  else for (i = 0; i < leftSize_; i++)  tmp1 = tmp1 + (T) *front2ptr++;

	  front1ptr += leftSize_ + 1;
	  back1ptr = front1ptr;

	  if (direction) for (i = 0; i < rightSize_; i++)  tmp1 = tmp1 + (T) *front1ptr++;
	  else for (i = 0; i < rightSize_; i++)  tmp1 = tmp1 - (T) *front1ptr++;

	  *outptr++ = rfRound(tmp1, dummy);
	  --size;

	  if (direction)
	    {
	      for (i = 0; i < size; i++)
		{
		  tmp1 = tmp1 + (T) front1ptr[i] - (T) back1ptr[i] - (T) front2ptr[i] + (T) back2ptr[i];
		  *outptr++ = rfRound(tmp1, dummy);
		}
	    } else {
	    for (i = 0; i < size; i++)
	      {
		tmp1 = tmp1 - (T) front1ptr[i] + (T) back1ptr[i] + (T) front2ptr[i] - (T) back2ptr[i];
		*outptr++ = rfRound(tmp1, dummy);
	      }
	  }
	}
    }
  else
    {
      tmp1 = 0;
      if (direction)
	{
	  for (i = 0; i < rightSize_; i++)  tmp1 += (T) *front1ptr++;
	  *outptr++ = rfRound(tmp1, dummy);
	  for (i = 0; i < size - 1; i++)
	    {
	      tmp1 = tmp1 + (T) front1ptr[i] - (T) back1ptr[i];
	      outptr[i] = rfRound(tmp1, dummy);
	    }
	} else
	{
	  for (i = 0; i < rightSize_; i++)  tmp1 -= (T) *front1ptr++;
	  *outptr++ = rfRound(tmp1, dummy);
	  for (i = 0; i < size - 1; i++)
	    {
	      tmp1 = tmp1 - (T) front1ptr[i] + (T) back1ptr[i];
	      outptr[i] = rfRound(tmp1, dummy);
	    }
	}
    }
}



#endif
