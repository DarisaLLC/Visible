

#include "vision/histo.h"
#include <limits>
#include <array>
#include <vector>
//#include "core/stl_utils.hpp"


histoStats::histoStats()
: computedMoments_(0), computedIC_(0), computedMode_(0),computedSumSq_(false), n_(0), mode_(0),
mean_(0), sDev_(0), var_(0), ic_(101), sum_(0), sumsq_(0)
{
    valid_bins_.resize(0);
}

histoStats::histoStats(vector<uint32_t> & histogram)
: computedMoments_(0), computedIC_(0), computedMode_(0),computedSumSq_(false), n_(0), mode_(0),
    mean_(0), sDev_(0), var_(0), ic_(101), sum_(0), sumsq_(0)
{
    valid_bins_.resize(0);
    histogram_ = histogram;
    computeNsamp();
}


histoStats::histoStats(const cv::Mat & histogram)
: computedMoments_(0), computedIC_(0), computedMode_(0),computedSumSq_(false), n_(0), mode_(0),
mean_(0), sDev_(0), var_(0), ic_(101), sum_(0), sumsq_(0)
{
    valid_bins_.resize(0);
    histogram_.resize(256);
    std::transform((float*)histogram.datastart, (float*)histogram.dataend, histogram_.begin(),
                   [](float f) { return (uint32_t) f; });
    
    computeNsamp();
}


const vector<uint32_t> & histoStats::histogram() const
{
    return histogram_;
}

const vector<uint8_t> & histoStats::valids() const
{
    return valid_bins_;
}


/* default copy ctor, assignment, dtor ok */

double histoStats::mean() const
{
    if (computedMoments_) return mean_;
    ((histoStats * const) this)->computeMoments();
    return mean_;
}

int64_t histoStats::sum() const
{
    if (! computedMoments_)
        ((histoStats * const) this)->computeMoments();
    return sum_;
}

int64_t histoStats::sumSquared () const
{
    std::unique_lock <std::mutex> lock(m_mutex, std::try_to_lock);
    
    if (! computedSumSq_ )
        ((histoStats * const) this)->computeSS();
    return sumsq_;
}

int32_t histoStats::mode() const
{
    std::unique_lock <std::mutex> lock(m_mutex, std::try_to_lock);
    
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
    std::unique_lock <std::mutex> lock(m_mutex, std::try_to_lock);
    return inverseCum(50);
}

int32_t histoStats::min(const int32_t discardPels) const
{
    std::unique_lock <std::mutex> lock(m_mutex, std::try_to_lock);
    
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
    std::unique_lock <std::mutex> lock (m_mutex, std::try_to_lock);
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

double histoStats::entropy () const
{
    double Sum (n_);
    
    double Entropy = 0.0;
    
    long tooHiSub = histogram_.size();
    long i;
    for(i=0; i<tooHiSub; i++ )
    {
        const double probability = histogram_[i] / Sum;
        
        if( probability > 1e-16 )
        {
            Entropy += - probability * log( probability ) / log( 2.0 );
        }
    }
    
    return Entropy;
}


long histoStats::computeInverseCum(int p)
{
    // fill in ic_
    std::unique_lock <std::mutex> lock(m_mutex, std::try_to_lock);
    
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
    
    i = 0;
    valid_bins_.clear();
    if (n_)
    {
        while (i < bins())
        {
            if (histogram()[i]) valid_bins_.push_back(i);
            i++;
        }
    }
    
    // validate inverseCum...
    computedIC_ = 1;
    return ic_[p];
}

void histoStats::computeMoments()
{
    std::unique_lock <std::mutex> lock(m_mutex, std::try_to_lock);
    
    if (n_)
    {
        long endI = (long)histogram_.size();
        long i;
        sum_ = 0;
        sumsq_ = 0;
        double temp;
        
        // Accumulate the sum = SUM( i*hist[i] )
        for (i = 0; i < endI; i++)
            sum_ += histogram_[i] * i;
        
        // Calculate mean
        mean_ = (double)sum_ / n_;

        
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

void histoStats::computeSS()
{
    std::unique_lock <std::mutex> lock(m_mutex, std::try_to_lock);
    
    long endI = (long)histogram_.size();
    long i;
    sumsq_ = 0;
    
    for (i = 0; i < endI; i++)
        sumsq_ += histogram_[i] * i * i;
    
    computedSumSq_ = true;
}
void histoStats::computeNsamp()
{
    std::unique_lock <std::mutex> lock(m_mutex, std::try_to_lock);
    
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


void histoStats::clear()
{
    std::unique_lock <std::mutex> lock(m_mutex, std::try_to_lock);
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
    typedef typename PixelType<P>::pixel_ptr_t ptr_t;
    std::array<uint32_t,PixelBinSize<P>::bins> ahist;
    ahist.fill (0);
    
    uint32_t lastRow = src.height() - 1, row = 0;
    const uint32_t opsPerLoop = 8;
    uint32_t unrollCnt = src.width() / opsPerLoop;
    uint32_t unrollRem = src.width() % opsPerLoop;
    
    for (; row <= lastRow; row++)
    {
        ptr_t pixelPtr = src.rowPointer(row);
        
        for (uint32_t touchCount = 0; touchCount < unrollCnt; touchCount++)
        {
            ahist[*pixelPtr++]++;
            ahist[*pixelPtr++]++;
            ahist[*pixelPtr++]++;
            ahist[*pixelPtr++]++;
            ahist[*pixelPtr++]++;
            ahist[*pixelPtr++]++;
            ahist[*pixelPtr++]++;
            ahist[*pixelPtr++]++;
        }
        
        for (uint32_t touchCount = 0; touchCount < unrollRem; touchCount++)
            ahist[*pixelPtr++]++;
    }

    init<P>(ahist);

}

template<typename P>
void  histoStats::init (const std::array<uint32_t, PixelBinSize<P>::bins>& ah)
{
    std::lock_guard <std::mutex> lock(m_mutex);
    clear ();
    valid_bins_.resize(0);
    bins_ = ah.size();
    histogram_.resize(bins_);
    std::copy(ah.begin(), ah.end(), histogram_.begin());
    computeNsamp();
    computeInverseCum(0);
    
}

template <typename P>
double histoStats::mean(const roiWindow<P> & src)
{
    histoStats h;
    h.from_image(src);
    return h.mean();
}


template void histoStats::from_image<P8U>(const roiWindow<P8U> & src);
template void histoStats::init <P8U>(const std::array<uint32_t, PixelBinSize<P8U>::bins>&);
template double histoStats::mean<P8U> (const roiWindow<P8U> & src);

