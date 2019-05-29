/******************************************************************************
 *  Copyright (c) 2003 Reify Corp. All rights reserved.
 *
 *   File Name      rc_filter1d.cpp
 *   Creation Date  07/17/2003
 *   Author         Peter Roberts
 *
 * Some simple 1D filtering capability
 *
 *****************************************************************************/

#include "vision/rc_filter1d.h"
#include "core/pair.hpp"

rcEdgeFilter1D::rcEdgeFilter1D() : _filter(1), _stdDev(0.0), _min(0), _max(0)
{
  _filter[0] = 1;
}

rcEdgeFilter1D::~rcEdgeFilter1D()
{
}

void rcEdgeFilter1D::filter(const vector<int32_t>& filter)
{
  assert(filter.size());
  _filter = filter;
}


void rcEdgeFilter1D::genFilter(vector<int32_t>& filter, const uint32_t oneCnt,
			       const uint32_t zeroCnt)
{
  assert(oneCnt);
  filter.resize(oneCnt*2 + zeroCnt);

  uint32_t i = 0;

  while (i < oneCnt)
    filter[i++] = -1;

  if (zeroCnt)
    while (i < oneCnt + zeroCnt)
      filter[i++] = 0;

  while (i < oneCnt*2 + zeroCnt)
    filter[i++] = 1;
}


// Unfortunate code duplication for now

void rcEdgeFilter1D::genPeaks(const vector<float>& signal,
			      vector<int32_t>* filteredImg,
			      vector<rcEdgeFilter1DPeak>& peak,
			      int32_t filterOffset,
			      double absMinPeak, double relMinPeak)
{
  const auto iSz = signal.size();
  const auto fSz = _filter.size();
  assert(iSz >= fSz);
  const auto fiSz = iSz - fSz + 1;
  
  vector<int32_t>& fImg = filteredImg ? *filteredImg : _filteredImg;

  /* Note: Possible performance hack would be to test for "<", but this
   * way the caller knows exactly how large the filtered image is.
   */
  if (fImg.size() != fiSz)
    fImg.resize(fiSz);

  /* Generate the filtered image. Along the way calculate 3 statistics
   * for the filtered image: std dev, min value, max value.
   */
  _min = 0xFFFFFFFF;
  _max = 0;
  double sumY = 0, sumYY = 0;
  
  for (auto fi = 0; fi < fiSz; fi++) {
    int32_t curVal = 0;
    for (auto f = 0; f < fSz; f++)
      curVal += (int32_t)signal[fi+f] * _filter[f];

    fImg[fi] = curVal;
    double absVal = fabs((double)fImg[fi]);
    sumY += absVal;
    sumYY += absVal*absVal;
//    if (absVal < _min)
//      _min = (uint32_t)absVal;
//    if (absVal > _max)
//      _max = (uint32_t)absVal;
  }

  double n = fiSz;
  _stdDev = sqrt((n*sumYY - sumY*sumY)/(n*(n-1)));
  
  /* Now search the filtered image for peaks and store them.
   */
  int32_t rMin = 0;
  if (relMinPeak > 0.0)
    rMin = (int32_t)(relMinPeak*_stdDev + 0.5);

  int32_t aMin = 0;
  if (absMinPeak > 0.0)
    aMin = (int32_t)(absMinPeak + 0.5);

  int32_t minPeakValuePos = rMin > aMin ? rMin : aMin;
  int32_t minPeakValueNeg = -minPeakValuePos;

  peak.resize(0);
  rcEdgeFilter1DPeak result;
  
  for (auto fi = 0; fi < fiSz; fi++) {
    if (fImg[fi] < 0) {
      if (fImg[fi] <= minPeakValueNeg) {
	result._value = fImg[fi];
	result._location = fi + filterOffset;
	peak.push_back(result);
      }
    }
    else if (fImg[fi] >= minPeakValuePos) {
      result._value = fImg[fi];
      result._location = fi + filterOffset;
      peak.push_back(result);
    }
  }
}


bool rcEdgeFilter1D::peakDetect1d (const vector<float>& signal, dPair& extendPeaks)
{
    // assume nothing.
    extendPeaks.x() = 0.0; extendPeaks.y() = 0;
    
    // Take the highest positive and the highest negative.
    rcEdgeFilter1DPeak posMax, negMax;
    posMax._value = 0, posMax._location = -1, negMax._value = 0, negMax._location = -1;
    vector<rcEdgeFilter1DPeak>peaks, posPeaks, negPeaks;
    rcEdgeFilter1D pfinder;
    vector<int32_t> efilter, filtered;
    pfinder.genFilter (efilter, 2, 1);
    pfinder.filter (efilter);
    pfinder.genPeaks (signal, &filtered, peaks, 0, 1.0, 0.5);
    
    for (auto i = 0; i < peaks.size(); i++)
    {
        int32_t left = (i == 0) ? 0 : peaks[i-1]._value;
        int32_t right = (i == (peaks.size() -1) ) ? 0 : peaks[i+1]._value;
        int32_t val = peaks[i]._value;
        
        if (val <= 0)
        {
            if (val <= left && val <= right)
            {
                negPeaks.push_back (peaks[i]);
                
                if (val < negMax._value)
                {
                    negMax._value = val;
                    negMax._location = peaks[i]._location;
                }
            }
        }
        else
        {
            assert (val > 0);
            
            if (val >= left && val >= right)
            {
                posPeaks.push_back (peaks[i]);
                
                if (val > posMax._value)
                {
                    posMax._value = val;
                    posMax._location = peaks[i]._location;
                }
            }
        }
    }
    
    if (posMax._location < 0 || negMax._location < 0) return false;
    // In case we needed space for all the score vector <float> scores (posPeaks.size() * negPeaks.size(), zf);
    // Score every pos/neg pair based on spatial distance and peak strength.
    float dMax (signal.size());
    float pnMax = posMax._value - negMax._value;
    float sdMax (0.0f);
    iPair sMax;
    
    for (auto i = 0; i < negPeaks.size(); i++)
    {
        for (auto j = 0; j < posPeaks.size(); j++)
        {
            float d = std::fabs (posPeaks[j]._location - negPeaks[i]._location) / dMax;
            float s = (posPeaks[j]._value - negPeaks[i]._value) / pnMax;
            s *= d;
            if (s > sdMax)
            {
                sMax = iPair ((int32_t) i, (int32_t) j);
                sdMax = s;
            }
        }
    }
    
    // First things first
    // Given the filtering, peaks will never be the ends but right now be paranoid
    
    dPair extends;
    extends.x() =  negPeaks[sMax.x()]._location;
    extends.y() = posPeaks[sMax.y()]._location;
    extendPeaks.x() = std::min(extends.x(), extends.y());
    extendPeaks.y() = std::max (extends.x(), extends.y());
    
    // Debugging Print
    
#ifdef SHAPEDEBUG
    
    cerr << dec << "+++++++++++++++++++++++++" << endl;
    cerr << signal << endl;
    cerr << filtered << endl;
    cerr << efilter << endl;
    for (uint32 i = 0; i < peaks.size(); i++) cerr << "[" << i << "]" << peaks[i]._location << ":\t" << peaks[i]._value << endl;
    cerr << negPeaks[sMax.x()]._location << " ---- " << posPeaks[sMax.y()]._location  << endl;
    cerr << extendPeaks << endl;
    
    cerr << "+++++++++++++++++++++++++" << endl;
    
#endif
    
    return true;
}
