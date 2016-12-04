/*
 *
 *$Header $
 *$Id $
 *$Log: $
 *
 * Copyright (c) 2002 Reify Corp. All rights reserved.
 */
#ifndef __PEAKER_H
#define __PEAKER_H



template <class Iterator>
vector<float> rf1dPeaker (Iterator Ib, Iterator Ie)
{

  // Get container value type
  typedef typename std::iterator_traits<Iterator>::value_type value_type;
  vector<value_type> d1 ();

  // get the first diff. element i of d1 is [i] - [i-1] of the data
  adjacent_difference (Ib, Ie, d1.begin ());
  vector<uint32> indmax;

  // Find peak indices. If the diff > 0 and diff[i] * diff[i+1] < 0
  for (uint32 i = 1; i < d1.size() - 1; i++)
    {
      dd = d1[i];
      if (dd > 0 && ((dd * d1[i+1]) < 0))
	indmax.push_back (i);
    }

  rcIn32 length (0);
  distance (Ib, Ie, length);
  const float *data = (const float *) (*Ib);
  for (uint32 i = 0; i < indmax.size(); i++)
    {
      uint32 ind = indmax[i];
      rmAssert (ind < length);
      PeakSpotter peaker;
      uint32 mB = rmMax (0, ind-4);
      uint32 mE = rmMin (ind+4, length-1);
      peakLocation = peaker.detectPeaks (data, mB, mE);
    }
}



#endif /* __PEAKER_H */
