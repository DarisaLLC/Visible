#include "DwtNode.h"
#include "wavelib.h"

#include "cinder/CinderMath.h"
#include <numeric>

namespace {

float calculateMean(const std::vector<float>& samples)
{
    return std::accumulate(samples.cbegin(), samples.cend(), 0.0f) / samples.size();
}

float calculateStdDev(const std::vector<float>& samples, float mean)
{
    std::vector<float> diff(samples.size());
    std::transform(samples.cbegin(), samples.cend(), diff.begin(), [mean](float x) { return x - mean; });
    float sq_sum = std::inner_product(diff.cbegin(), diff.cend(), diff.cbegin(), 0.0f);
    return std::sqrt(sq_sum / samples.size());
}

}

namespace {

class ScopedWaveObject
{
public:
    ScopedWaveObject(dsp::MotherWavelet w)
        : mWaveObj(::wave_init(const_cast<char*>(dsp::WavelettoString(w).c_str()))) {}

    ~ScopedWaveObject() { ::wave_free(mWaveObj); }
    wave_object         getWaveObject() const { return mWaveObj; }

private:
    wave_object mWaveObj;
};

class ScopedWtObject
{
public:
    ScopedWtObject()                    = delete;
    explicit ScopedWtObject(dsp::MotherWavelet wavelet, int num_samples, int decomp_levels)
        : mWaveObject(wavelet), mNumSamples(num_samples), mDecompLevels(decomp_levels)
    {
        mWtObject = ::wt_init(mWaveObject.getWaveObject(), "dwt", mNumSamples, mDecompLevels);
        ::setDWTExtension(getWtObject(), "sym");
        ::setWTConv(getWtObject(), "direct");
    }

    ~ScopedWtObject()   { wt_free(mWtObject); }
    wt_object           getWtObject() const { return mWtObject; }
    wave_object         getWaveObject() const { return mWaveObject.getWaveObject(); }

private:
    ScopedWaveObject    mWaveObject;
    wt_object           mWtObject;
    int                 mNumSamples;
    int                 mDecompLevels;
};

}

namespace wavy
{

struct DwtNode::Data
{
    Data()              = delete;
    Data(const Data&)   = delete;
    explicit Data(const DwtNode::Format& fmt)
        : mWtObject(
            fmt.getMotherWavelet(),
            fmt.getWindowSize(),
            fmt.getDecompositionLevels())
    { /* no-op */ }

    ScopedWtObject      mWtObject;
};

DwtNode::DwtNode(const Format &format /*= Format()*/)
    : mCurrentFormat(format)
    , mWavelibData(std::make_unique<Data>(format))
    , mStdDev(0), mMean(0)
{}

void DwtNode::initialize()
{
    MonitorNode::initialize();

    mSamplesBuffer = ci::audio::BufferT<double>(mCurrentFormat.getWindowSize());
    mSamplesBuffer.zero();

    // we perform the first pass to obtain length of Detail coefficients
    ::dwt(mWavelibData->mWtObject.getWtObject(), mSamplesBuffer.getData());

    mDetailCoefficients.resize(mCurrentFormat.getDecompositionLevels());

    // According to docs, we exclude the first and last element of lenlength (approx and input length)
    if (mCurrentFormat.getDecompositionLevels() != mWavelibData->mWtObject.getWtObject()->lenlength - 2)
        throw std::runtime_error("Invalid decomposition level found.");

    for (int detail = 1; detail < mWavelibData->mWtObject.getWtObject()->lenlength - 1; ++detail)
        mDetailCoefficients[detail - 1].resize(mWavelibData->mWtObject.getWtObject()->length[detail]);
}

const std::vector<float>& DwtNode::getCoefficients(int lvl)
{
    fillCopiedBuffer();
    mSamplesBuffer.zero();

    double scale = 1.0f / getNumChannels();
    for (std::size_t ch = 0; ch < getNumChannels(); ch++) {
        for (std::size_t i = 0; i < mWindowSize; i++)
            mSamplesBuffer[i] += mCopiedBuffer.getChannel(ch)[i] * scale;
    }

    ::dwt(mWavelibData->mWtObject.getWtObject(), mSamplesBuffer.getData());

    for (int detail_level = mCurrentFormat.getDecompositionLevels(); detail_level > 0; --detail_level)
    {
        int offset = 0;
        for (int lvl = mCurrentFormat.getDecompositionLevels(); lvl >= detail_level; --lvl)
            offset += mDetailCoefficients[lvl - 1].size();

        for (std::size_t coeff_index = 0; coeff_index < mDetailCoefficients[detail_level - 1].size(); ++coeff_index)
        {
            int output_index = mWavelibData->mWtObject.getWtObject()->outlength - offset + coeff_index;
            mDetailCoefficients[detail_level - 1][coeff_index] = static_cast<float>(mWavelibData->mWtObject.getWtObject()->output[output_index]);
        }
    }

    mMean = calculateMean(mDetailCoefficients.back());
    mStdDev = calculateStdDev(mDetailCoefficients.back(), getMean());

    return mDetailCoefficients[ci::math<int>::clamp(lvl, 1, mCurrentFormat.getDecompositionLevels()) - 1];
}

const std::vector<float>& DwtNode::getCoefficients()
{
    return getCoefficients(mCurrentFormat.getDecompositionLevels());
}

float DwtNode::getMean() const
{
    return mMean;
}

float DwtNode::getStdDeviation() const
{
    return mStdDev;
}

}
