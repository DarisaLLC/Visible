#pragma once

#include "cinder/audio/MonitorNode.h"
#include "DwtDsp.h"

namespace wavy
{

using DwtNodeRef = std::shared_ptr<class DwtNode>;

class DwtNode : public ci::audio::MonitorNode {
public:
    struct Format : public MonitorNode::Format {
        Format() : mDecompositionLevels(3), mMotherWavelet(dsp::MotherWavelet::Haar) {}

        Format&                 decompositionLevels(std::size_t size)   { mDecompositionLevels = size; return *this; }
        Format&                 motherWavelet(dsp::MotherWavelet wav)   { mMotherWavelet = wav; return *this; }
        Format&		            windowSize(std::size_t size)			{ MonitorNode::Format::windowSize(size); return *this; }

        size_t			        getDecompositionLevels() const			{ return mDecompositionLevels; }
        dsp::MotherWavelet      getMotherWavelet() const			    { return mMotherWavelet; }

    protected:
        std::size_t	            mDecompositionLevels;
        dsp::MotherWavelet      mMotherWavelet;
    };

    DwtNode(const Format &format = Format());
    virtual ~DwtNode() { /* no-op */ }

protected:
    void                            initialize() override;

public:
    //! Returns detail coefficients at lvl specified
    const std::vector<float>&       getCoefficients(int lvl);
    //! Returns detail coefficients at max decomposition lvl
    const std::vector<float>&       getCoefficients();
    //! Gets current format of this node
    const Format&                   getFormat() const { return mCurrentFormat; }
    //! Returns mean of the last level of decomposition
    float                           getMean() const;
    //! Returns standard deviation of the last level of decomposition
    float                           getStdDeviation() const;

private:
    struct Data;
    std::shared_ptr<Data>           mWavelibData;
    ci::audio::BufferT<double>      mSamplesBuffer;
    std::vector<std::vector<float>> mDetailCoefficients;
    const Format                    mCurrentFormat;
    float                           mStdDev, mMean;
};

}
