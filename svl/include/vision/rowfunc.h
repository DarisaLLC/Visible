
#ifndef __rowFunc_H
#define __rowFunc_H

#include <assert.h>
#include <iostream>
#include <sstream>
#include <math.h>
#include <cmath>
#include <stdint.h>
#include <vector>
#include "core/static.hpp"


using namespace std;


// Correlation results. For Sum of Absolute differences. Sim is the sum. r is Average intensity difference normalized

// Note that 16bit and 32bit pixel depth correlations are implemented as multiple 8bit correlations. That is a 16bit image
// is correlated as an 8bit image with twice as many pixels

class CorrelationParts
{
public:
    typedef double sumproduct_t;
    typedef std::pair<sumproduct_t, sumproduct_t> imagesum_t;


    CorrelationParts();
    

    // default copy, assignment, dtor ok

    inline double compute()
    {
        assert(mSi >= 0.0);
        assert(mSm >= 0.0);

        mEi = ((mN * mSii) - (mSi * mSi));
        mEm = ((mN * mSmm) - (mSm * mSm));
        sumproduct_t Eim = mEi * mEm;
        
        // Modified Normalized Correlation. If both image and model have zero standard deviation and equal moments then
        // they are identical. This case produces 0 in regression based correlation because you can not fit a line
        // through a single point. However, from image matching point of view it is a perfect match with a gain.

        mRp = (Eim == 0 ) && (mSmm == mSii) && (mSi == mSm);

        mCosine = ((mN * mSim) - (mSi * mSm));

        // Avoid divide by zero. Singular will indicate 0 standard deviation in both
        if (Eim != 0.0)
            mR = (mCosine * mCosine) / Eim;
        return mR;
    }

    inline double cosine()
    {
        return acos(static_cast<double>(mCosine) / std::sqrt(static_cast<double>(mEi) * static_cast<double>(mEm)));
    }

    inline double relative()
    {
        assert(mSi >= 0.0);
        assert(mSm >= 0.0);
        return std::sqrt(r());
    }

    inline void accumulate(sumproduct_t & sim,
                           sumproduct_t & sii,
                           sumproduct_t & smm,
                           sumproduct_t & si,
                           sumproduct_t & sm)
    {
        mSim += sim;
        mSii += sii;
        mSmm += smm;
        mSi += si;
        mSm += sm;
    }

    inline double iStd()
    {
        double dN(mN);
        return sqrt(((mN * mSii) - (mSi * mSi)) / (dN * (dN - 1.)));
    }

    inline double mStd()
    {
        double dN(mN);
        return sqrt(((mN * mSmm) - (mSm * mSm)) / (dN * (dN - 1.)));
    }


    inline double r() const { return mR; }
    inline int milR() const { return (int)(mR * 1000.0); }

    inline double r(double r) { return mR = r; }
    inline bool singular() const { return mRp; }
    inline int n() const { return mN; }
    inline int n(int n) { return mN = n; }
    inline sumproduct_t Si() const { return mSi; }
    inline sumproduct_t Sm() const { return mSm; }
    inline sumproduct_t Sii() const { return mSii; }
    inline sumproduct_t Smm() const { return mSmm; }
    inline sumproduct_t Sim() const { return mSim; }
    inline void Sim(sumproduct_t & nsim) { mSim = nsim; }

    inline bool operator==(const CorrelationParts & o) const
    {
        return (mR == o.mR && mRp == o.mRp && mN == o.mN && mSi == o.mSi && mSm == o.mSm &&
                mSii == o.mSii && mSmm == o.mSmm && mSim == o.mSim);
    }

    inline bool operator!=(const CorrelationParts & o) const { return !(this->operator==(o)); }

    friend ostream & operator<<(ostream & os, const CorrelationParts & corr);

private:
    double mR;
    sumproduct_t mSi, mSm, mCosine, mEi, mEm;
    sumproduct_t mSii, mSmm, mSim;
    int mN;
    bool mRp;
};


// A class to encapsulate a table of pre-computed squares
template <typename T>
class sSqrTable
{

public:
    // ctor
    sSqrTable()
    {
        mSize = 2 * numeric_limits<T>::max() + 2;
        mTable.resize(mSize);
        for (uint32_t i = 0; i < mSize; i++)
            mTable[i] = i * i;
    };

    // Array access operator
    uint32_t operator[](uint32_t index) const
    {
        assert(index < mSize);
        return mTable[index];
    };
    uint32_t * lut_ptr() { return &mTable[0]; }


    // Array size
    uint32_t size() const
    {
        return mSize;
    }

    bool validate () const
    {
        for(uint32_t i = 0; i < mSize; i++) if (mTable[i] != (i*i)) return false;
        return true;
    }
    
private:
    uint32_t mSize;
    std::vector<uint32_t> mTable;
};


/*
 * Base class for two sources (and a scaler or a non-image
 * result out ).
 *
 * Usage: 
 * drive from the base class
 * implement the row func
 *
 * A processing function is a template function of a derived
 * class of this and if image class is a templatized 
 * 
 */


template <class T>
class rowFuncTwoSource
{

public:
    rowFuncTwoSource() {}
    virtual ~rowFuncTwoSource() {}

    virtual void prolog() = 0;
    virtual void rowFunc() = 0;
    virtual void areaFunc() = 0;

protected:
    const T * mFirst;
    const T * mSecond;
    uint32_t mWidth;
    uint32_t mHeight;
};

template <class T, class U>
class rowFuncOneSourceOneDestination
{

public:
    rowFuncOneSourceOneDestination() {}
    virtual ~rowFuncOneSourceOneDestination() {}

    virtual void prolog() = 0;
    virtual void rowFunc() = 0;
    virtual void areaFunc() = 0;

protected:
    const T * mFirst;
    U * mSecond;
    uint32_t mWidth;
    uint32_t mHeight;
};


//@todo add std::array / container template
template <class T, class U>
class pixelMap : public rowFuncOneSourceOneDestination<T, U>
{
public:
    typedef typename std::vector<U>::const_iterator lutItr;

    pixelMap(const T * baseA, const U * baseB, uint32_t rupA, uint32_t rupB, uint32_t width, uint32_t height, const vector<U> & lut);
    pixelMap(const T * baseA, const U * baseB, uint32_t rupA, uint32_t rupB, uint32_t width, uint32_t height, const lutItr & lut);

    virtual void prolog();
    virtual void rowFunc();
    virtual void areaFunc();

    virtual ~pixelMap();

private:
    lutItr mLut;
    std::pair<uint32_t, uint32_t> mRUP;
};


template <class T>
class basicCorrRowFunc : public rowFuncTwoSource<T>
{
public:
    basicCorrRowFunc(const T * baseA, const T * baseB, uint32_t rupA, uint32_t rupB,
                     uint32_t width, uint32_t height, bool ffMaskOn = false);

    virtual void prolog();
    virtual void rowFunc();
    virtual void areaFunc();
    void epilog(CorrelationParts &);

    virtual ~basicCorrRowFunc();

private:
    CorrelationParts mRes;
    uint32_t mMaskPels;
    bool mffMaskOn;
    std::pair<uint32_t, uint32_t> mRUP;
    sSqrTable<T> m_squareTable;
};


#endif /* __rowFunc_H */
