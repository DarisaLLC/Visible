
#include "genHistogram.h"
#include "histo.h"
#include <limits>
#include <math.h>

histoStats::histoStats(vector<uint32_t> & histogram, bool xown)
    : computedMoments_(0), computedIC_(0), computedMode_(0), n_(0), mode_(0), mean_(0), sDev_(0), var_(0), ic_(101)
{
    if (xown) // take ownership
    {
        histogram_.swap(histogram);
        histogram.resize(0);
    }
    else // make copy
        histogram_ = histogram;

    computeNsamp();
}

/* default copy ctor, assignment, dtor ok */

double histoStats::mean() const
{
    if (computedMoments_) return mean_;
    ((histoStats * const) this)->computeMoments();
    return mean_;
}

int32_t histoStats::mode() const
{
    if (computedMode_) return mode_;

    // compute mode, make valid, return mode_

    long startIndex;
    long lastIndex;
    if (computedIC_)
    {
        startIndex = ic_[0];
        lastIndex = ic_[100];
    }
    else
    {
        lastIndex = (long)histogram_.size() - 1;
    }

    long i, indexOfMax;
    uint32_t maxCountSoFar;
    vector<uint32_t>::const_iterator pBinI;
    for (i = 1,
        indexOfMax = 0,
        pBinI = histogram_.begin(),
        maxCountSoFar = *pBinI;
         i <= lastIndex;
         i++)
        if (*++pBinI > maxCountSoFar) maxCountSoFar = *pBinI, indexOfMax = i;

    ((histoStats * const) this)->computedMode_ = 1;
    return ((histoStats * const) this)->mode_ = indexOfMax;
}

int32_t histoStats::median() const
{
    return inverseCum(50);
}

int32_t histoStats::min(const int32_t discardPels) const
{
    if (!discardPels)
        return inverseCum(0);
    int32_t sum(0);
    long i = 0;
    long endI = (long)histogram_.size();

    if (n_)
        while (i < endI && sum < discardPels)
        {
            sum += histogram_[i++];
        }
    return i;
}

int32_t histoStats::max(const int32_t discardPels) const
{
    if (!discardPels)
        return inverseCum(100);
    int32_t sum(0);
    long i = (long)histogram_.size() - 1;
    if (n_)
        while (i >= 0 && sum < discardPels)
        {
            sum += histogram_[i--];
        }
    return i;
}

double histoStats::sDev() const
{
    if (computedMoments_) return sDev_;
    ((histoStats * const) this)->computeMoments();
    return sDev_;
}

double histoStats::var() const
{
    if (computedMoments_) return var_;
    ((histoStats * const) this)->computeMoments();
    return var_;
}


long histoStats::computeInverseCum(int p)
{
    // fill in ic_

    double one_percent = (double)n_ / 100.;
    double percent;
    uint32_t sum = 0;
    long nextIndex = 0;

    long i = 0;
    long endI = (long)histogram_.size();
    long j;

    if (n_) // if no samples, leave ic_ all zeros
        while (i < endI)
        {
            if (histogram_[i])
            {
                ic_[100] = i;
                sum += histogram_[i];
                percent = (double)sum / one_percent;

                if (percent > nextIndex)
                {
                    for (j = nextIndex; j < percent; j++)
                        ic_[j] = i;
                    nextIndex = j;
                }
            }
            i++;
        }

    // validate inverseCum...
    computedIC_ = 1;
    return ic_[p];
}

void histoStats::computeMoments()
{
    if (n_)
    {
        long endI = (long)histogram_.size();
        long i;
        uint32_t sum = 0;
        double temp;

        // Accumulate the sum = SUM( i*hist[i] )
        for (i = 0; i < endI; i++)
            sum += histogram_[i] * i;

        // Calculate mean
        mean_ = (double)sum / n_;

        // Calculate variance
        if (n_ > 1)
        {
            for (i = 0; i < endI; i++)
                if (histogram_[i])
                {
                    temp = i - mean_;
                    var_ += temp * temp * histogram_[i];
                }
            var_ /= (double)(n_ - 1);
        }
        else
            var_ = 0.0;

        // Calculate sDev
        sDev_ = sqrt(var_);
    }
    else
        mean_ = var_ = sDev_ = 0.0;
    computedMoments_ = 1;
}


void histoStats::computeNsamp()
{
    // compute nSamp
    auto tooHiSub = histogram_.size();
    long i;
    for (i = 0; i < (long)tooHiSub; i++)
        n_ += histogram_[i];

    for (i = 0; i < 101; i++)
        ic_[i] = 0;
}

ostream & operator<<(ostream & o, const histoStats & p)
{
    o << "{" << endl << "Mean: " << p.mean() << ",Median  " << p.median() << "Min:  " << p.min() << "Max: " << p.max() << "Bins " << p.bins() << endl;

    uint32_t i = 0;
    if (p.n())
    {
        while (i < p.bins())
        {
            uint32_t pd(p.histogram()[i]);
            if (pd)
            {
                cerr << "[" << i << "]: " << pd << "," << (float)pd / (float)p.n() << endl;
            }
            i++;
        }
    }
    return o;
}


int32_t histoStats::inverseCum(int percent) const
{
    if (computedIC_)
        return ic_[percent];
    else
        return ((histoStats * const) this)->computeInverseCum(percent);
}

histoStats::histoStats()
    : computedMoments_(0), computedIC_(0), computedMode_(0), n_(0), mode_(0), mean_(0), sDev_(0), var_(0), ic_(101), bins_()
{
}

void histoStats::clear()
{
    computedMoments_ = 0;
    computedIC_ = 0;
    computedMode_ = 0;
    n_ = 0;
    mode_ = 0;
    mean_ = 0;
    sDev_ = 0;
    var_ = 0;
    ic_.resize(101);
    bins_ = 0;
}

template <typename P>
void histoStats::from_image(const roiWindow<P> & src)
{
    clear();
    bins_ = P::maximum();
    getHistogram<P> gen;
    gen.process(src, histogram_);
    computeNsamp();
}


template void histoStats::from_image<P8U>(const roiWindow<P8U> & src);
