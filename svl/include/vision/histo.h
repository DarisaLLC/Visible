
#ifndef __HISTOSTAT_H__
#define __HISTOSTAT_H__

#include <limits>
#include <vector>
#include <iostream>
#include <stdint.h>
#include "roiWindow.h"
#include <opencv2/core/core.hpp>


using namespace std;

/*
 number of samples
 mean
 median
 mode
 minimum, maximum
 inverse cumulative
 standard deviation
 variance
 A histogram is represented by a stl vector
 */

class histoStats
{
public:
    histoStats();

    template <typename P>
    static double mean (const roiWindow<P> & image);
    
    template <typename P>
    void from_image(const roiWindow<P> & image);
    /*
    requires an allocated image. Supports 8bit and 16bit images only
    
  */
    histoStats(const cv::Mat& histogram);
    histoStats(vector<uint32_t> & histogram);
    /*
    requires   histogram.size() > 0
    effect     Constructs a histo using the given histogram.  All
               statistics will be computed using the histogram hist.
  */

    /* default copy ctor, assignment, dtor ok */
    /*
    effect     This histo becomes a copy of rhs.
  */

    uint32_t n() const { return n_; }
    /*
    effect     Returns the total number of samples in the histogram supplied
               at construction.
    requires   This histo was not default constructed.
  */

    uint32_t bins() const { return bins_; }
    /*
    effect     Returns the total number of bins in the histogram supplied
               at construction. This is mostly for 16 bit images since almost always
	       the capture image depth is less than 12 bits. 
    requires   This histo was not default constructed.
  */

    double mean() const;
    /*
    effect     Returns the average value in the histogram supplied at
               construction.  Returns zero if nSamp() is zero.
    requires   This histo was not default constructed.
  */

    int32_t mode() const;
    /*
    effect     Returns the most frequent value in the histogram supplied
               at construction.  Returns zero if nSamp() is zero.
    requires   This histo was not default constructed.
  */

    int32_t median() const;
    /*
    effect     Returns the median value in the histogram supplied at
               construction.  Returns zero if nSamp() is zero.
    requires   This histo was not default constructed.
  */

    float interpolatedMedian() const;
    /*
    effect     Returns the interpolated median value in the histogram supplied at
               construction.  Returns zero if nSamp() is zero.
    requires   This histo was not default constructed.
  */

    int32_t min(const int32_t discardPels = 0) const;
    int32_t max(const int32_t discardPels = 0) const;
    /*
    effect     Returns the minimum(maximum) value in the histogram supplied
               at construction.  Returns zero if nSamp() is zero.
    requires   This histo was not default constructed.
  */

    int32_t inverseCum(int percent) const;
    /*
    effect     Returns the value below which a specified percentage of values
               in the histogram fall.  Returns zero if nSamp() is zero.
    requires   This histo was not default constructed.
  */

    double sDev() const;
    /*
    effect     Returns the standard deviation of the values in the histogram
               supplied at construction.  Returns zero if nSamp() is zero.
    requires   This histo was not default constructed.
  */

    double entropy() const;

    /*
    effect     Returns Entropy of this histogram
               Returns zero if nSamp() is zero.
    requires   This histo was not default constructed.
  */

    double var() const;
    /*
    effect     Returns the variance of the values in the histogram supplied
               at construction.  Returns zero if nSamp() is zero.
    requires   This histo was not default constructed.
  */

    const vector<uint32_t> & histogram() const; 
    /*
    returns    Histogram given at construction
  */
    
    const vector<uint8_t> & valids() const;

    /*
     returns    vector of valid bins
     */
    
    friend ostream & operator<<(ostream & o, const histoStats & p);
    
    int64_t sum() const;
    /*
     effect     Returns the sum of the values in the histogram supplied
     at construction.  Returns zero if nSamp() is zero.
     requires   This histo was not default constructed.
     */
    
    int64_t sumSquared() const;
    /*
     effect     Returns the sum of the values in the histogram supplied
     at construction.  Returns zero if nSamp() is zero.
     requires   This histo was not default constructed.
     */

private:
    void clear();
    void computeNsamp();
    long computeInverseCum(int percent); // compute and make valid the ic,
                                         //   return ic_[percent]
    void computeMoments();               // compute mean,sDev,var,energry
    void computeSS();                    // compute sum squared

    template<typename P>
    void init (const std::array<uint32_t, PixelBinSize<P>::bins>&);
    
    vector<uint32_t> histogram_; // histogram given at construction

    /* flags for already computed answers */
    uint8_t computedMoments_;
    bool computedSumSq_;
    uint8_t computedIC_;
    uint8_t computedMode_;

    /* already computed answers */
    uint32_t n_;      // always valid
    long mode_;       // valid if computeMode_
    double mean_;     // valid if computeMoments_
    double sDev_;     // valid if computeMoments_
    double var_;      // valid if computeMoments_
    vector<long> ic_; // valid if computeIC_
    uint32_t bins_;   // always valid
    vector<uint8_t> valid_bins_;
    int64_t sum_;
    int64_t sumsq_;
    
    mutable std::mutex m_mutex;
};

void hysteresisThreshold(const roiWindow<P8U> & magImage,
                         roiWindow<P8U> & dst,
                         uint8_t magLowThreshold, uint8_t magHighThreshold,
                         int32_t & nSurvivors, int32_t nPels);


#endif // __HISTOSTAT_H__
