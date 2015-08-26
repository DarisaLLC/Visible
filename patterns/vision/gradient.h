
#ifndef _FEHF_H
#define _FEHF_H

#include <vector>
#include <iostream>
#include "roiWindow.h"
#include "pair.hpp"
#include <tuple>
#include "edgel.h"

unsigned int SpatialEdge(const roiWindow<P8U> & magImage, const roiWindow<P8U> & angleImage, roiWindow<P8U> & peaks, uint8_t threshold, bool angleLabeled = false);
void Gradient(const roiWindow<P8U> & image, roiWindow<P8U> & magnitudes, roiWindow<P8U> & angles);

class EdgeTables
{
public:
    EdgeTables() { init(); }
    ~EdgeTables() {}

    enum
    {
        eAtanPrecision = 16,
        eAtanRange = (1 << eAtanPrecision),
        eMagPrecision = 8,
        eMagTableSize = (1 << (eMagPrecision + eMagPrecision)),
        eMagRange = (1 << eMagPrecision),
        eMaxMag = (eMagRange - 1)
    };


    const uint8_t * angleTable() const { return &mThetaTable[0]; }


    const uint8_t * magnitudeTable() const { return &mMagTable[0]; }

    uint8_t binAtan(int y, int x) const;

private:
    void init();
    double mMagScale;
    std::vector<uint8_t> mThetaTable;
    std::vector<uint8_t> mMagTable;
};


bool GetMotionCenter(const roiWindow<P8U> & peaks, const roiWindow<P8U> & ang, fPair & center);


class MotionCenter
{
public:
    MotionCenter()
        : mSumUU(0), mSumVV(0), mSumUV(0), mSumUR(0), mSumVR(0), mValid(false) {}

    void add(fPair & pos, fPair & motion)
    {
        mValid = false;
        double u = motion.first;
        double v = motion.second;
        double r = pos.first * u + pos.second * v;
        mSumUU += u * u;
        mSumVV += v * v;
        mSumUV += u * v;
        mSumUR += u * r;
        mSumVR += v * r;
    }

    bool center(fPair & center)
    {
        double d = mSumUU * mSumVV - mSumUV * mSumUV;
        mValid = d > 0;
        if (mValid == true)
        {
            center.first = static_cast<float>((mSumUR * mSumVV - mSumUV * mSumVR) / d);
            center.second = static_cast<float>((mSumVR * mSumUU - mSumUV * mSumUR) / d);
        }
        return mValid;
    }

private:
    double mSumUU;
    double mSumVV;
    double mSumUV;
    double mSumUR;
    double mSumVR;
    bool mValid;
};


class hystheresisThreshold
{
public:
    hystheresisThreshold();

    // Values to label ( will be cast to pixel type )
    int lowVal() const;
    void lowVal(int);
    int highVal() const;
    void highVal(int);

    // Threshold values ( will be cast to pixel type )
    // Overridden by passed arguments
    int lowThreshold() const;
    void lowThreshold(int);
    int highThreshold() const;
    void highThreshold(int);

    void operator()(const roiWindow<P8U> & input, int low, int high);
    void operator()(const roiWindow<P8U> & input);

    bool output_available() const;
    const roiWindow<P8U> & output() const;

    struct hystNode
    {
        uint8_t * destAddr;
        uint8_t * workAddr;
    };
};


#endif // _FEHF_H
