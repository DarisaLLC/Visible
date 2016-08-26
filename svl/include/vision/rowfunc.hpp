
#ifndef __rowFunc_H
#define __rowFunc_H

#include <assert.h>
#include <iostream>
#include <sstream>
#include <math.h>
#include <stdint.h>
#include <vector>
#include <static.hpp>


using namespace std;

// Correlation results. For Sum of Absolute differences. Sim is the sum. r is Average intensity difference normalized

// Note that 16bit and 32bit pixel depth correlations are implemented as multiple 8bit correlations. That is a 16bit image
// is correlated as an 8bit image with twice as many pixels

class CorrelationParts
{
public:
    typedef int64_t sumproduct_t;
    
    CorrelationParts () : mR(0.0), mSi(0), mSm (0), mSii (0), mSmm (0), mSim (0), mN(0), mRp(0), mEim (0), mCosine (0) {}
    
    // default copy, assignment, dtor ok
    
    inline double compute ()
    {
        assert( mSi >= 0.0 );
        assert( mSm >= 0.0 );
        
        const double ei = ((mN * mSii) - (mSi * mSi));
        const double em = ((mN * mSmm) - (mSm * mSm));
        const double mEim = ei * em;
        
        // Modified Normalized Correlation. If both image and model have zero standard deviation and equal moments then
        // they are identical. This case produces 0 in regression based correlation because you can not fit a line
        // through a single point. However, from image matching point of view it is a perfect match.
        
        mRp = (mEim == 0.) && (mSmm == mSii) && (mSi == mSm);
        
        mCosine = ((mN * mSim) - (mSi * mSm));
        mR = (mCosine * mCosine ) / mEim;
        return mR;
    }
    
    inline double cosine ()
    {
        return mCosine / std::sqrt (mEim);
    }
    
    inline double fisher ()
    {
        return atanh(cosine());
    }
    
    inline double relative ()
    {
        assert( mSi >= 0.0 );
        assert( mSm >= 0.0 );
        return std::sqrt (r () );
    }
    
    inline void accumulate (sumproduct_t& sim,
                            sumproduct_t& sii,
                            sumproduct_t& smm,
                            sumproduct_t& si,
                            sumproduct_t& sm)
    {
        mSim += sim;
        mSii += sii;
        mSmm += smm;
        mSi += si;
        mSm += sm;
    }
    
    inline double iStd ()
    {
        double dN (mN);
        return sqrt (((mN * mSii) - (mSi * mSi)) / (dN * (dN - 1.)));
    }
    
    inline double mStd ()
    {
        double dN (mN);
        return sqrt (((mN * mSmm) - (mSm * mSm)) / (dN * (dN - 1.)));
    }
    
    
    inline   double r () const { return mR; }
    inline   double r (double r)  {return mR = r;}
    inline   bool singular () const { return mRp; }
    inline   int n () const { return mN; }
    inline   int n (int n) {return mN = n;}
    inline   sumproduct_t Si () const { return mSi; }
    inline   sumproduct_t Sm () const { return mSm; }
    inline   sumproduct_t Sii () const { return mSii; }
    inline   sumproduct_t Smm () const { return mSmm; }
    inline   sumproduct_t Sim () const { return mSim; }
    inline   void Sim(sumproduct_t& nsim) { mSim = nsim;  }
    
    inline bool operator ==(const CorrelationParts& o) const
    {
        return (mR == o.mR && mRp == o.mRp && mN == o.mN && mSi == o.mSi && mSm == o.mSm &&
                mSii == o.mSii && mSmm == o.mSmm && mSim == o.mSim);
    }
    
    inline bool operator !=(const CorrelationParts& o) const { return !(this->operator==(o)); }
    
private:
    double mR, mCosine, mEim;
    sumproduct_t mSi, mSm;
    sumproduct_t mSii, mSmm, mSim;
    int mN;
    bool mRp;
};

// Stream output operator
ostream& operator << (ostream& os, const CorrelationParts& corr)
{
    os << corr.r() << std::endl;
    return os;
}




// A class to encapsulate a table of pre-computed squares
template<typename T>
class sSqrTable
{
    
public:
    // ctor
    sSqrTable()
    {
        mSize = 2 * numeric_limits<T>::max() - 1;
        mTable.resize(mSize);
        for (uint32_t i = 0; i < mSize; i++)
            mTable[i] = i * i;
    };
    
    // Array access operator
    uint32_t operator [] (int index) const { assert ( index < mSize ); return mTable[index]; };
    uint32_t* lut_ptr () { return &mTable[0]; }
    
    
    // Array size
    uint32_t size() const
    {
        return mSize;
    }
    
private:
    uint32_t mSize;
    std::vector<uint32_t> mTable;
};

SINGLETON_FCN(sSqrTable<uint8_t>, square_table);
SINGLETON_FCN(sSqrTable<uint16_t>, square_table16);


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
    rowFuncTwoSource () {}
    virtual ~rowFuncTwoSource () {}
    
    virtual void prolog () = 0;
    virtual void rowFunc () = 0;
    virtual void areaFunc () = 0;
    
protected:
    const T* mFirst;
    const T* mSecond;
    uint32_t mWidth;
    uint32_t mHeight;
};

template <class T, class U>
class rowFuncOneSourceOneDestination
{
    
public:
    rowFuncOneSourceOneDestination () {}
    virtual ~rowFuncOneSourceOneDestination () {}
    
    virtual void prolog () = 0;
    virtual void rowFunc () = 0;
    virtual void areaFunc () = 0;
    
protected:
    const T* mFirst;
    U* mSecond;
    uint32_t mWidth;
    uint32_t mHeight;
};


template<class T, class U>
class pixelMap : public rowFuncOneSourceOneDestination<T,U>
{
public:
    typedef typename std::vector<U>::const_iterator lutItr;
    
    pixelMap (const T* baseA, const U* baseB, uint32_t rupA, uint32_t rupB, uint32_t width, uint32_t height,  const vector<U>& lut);
    
    virtual void prolog ();
    virtual void rowFunc ();
    virtual void areaFunc ();
    
    virtual ~pixelMap ();
    
private:
    lutItr mLut;
    std::pair<uint32_t,uint32_t> mRUP;
};


template <class T>
class basicCorrRowFunc : public rowFuncTwoSource<T>
{
public:
    basicCorrRowFunc (const T* baseA, const T* baseB, uint32_t rupA, uint32_t rupB,
                      uint32_t width, uint32_t height);
    
    virtual void prolog ();
    virtual void rowFunc ();
    virtual void areaFunc ();
    void epilog (CorrelationParts&);
    
    virtual ~basicCorrRowFunc ();
    
private:
    CorrelationParts   mRes;
    std::pair<uint32_t,uint32_t> mRUP;
    static sSqrTable <T> m_squareTable;
};



// public


template <class T>
basicCorrRowFunc<T>::basicCorrRowFunc (const T* baseA, const T* baseB, uint32_t rowPelsA, uint32_t rowPelsB,
                                       uint32_t width, uint32_t height) :
mRUP( rowPelsA , rowPelsB  )
{
    rowFuncTwoSource<T>::mWidth = width;
    rowFuncTwoSource<T>::mHeight = height;
    rowFuncTwoSource<T>::mFirst = baseA;
    rowFuncTwoSource<T>::mSecond = baseB;
}


template <class T>
void basicCorrRowFunc<T>::areaFunc ()
{
    uint32_t height = rowFuncTwoSource<T>::mHeight;
    
    do { rowFunc(); }
    while (--height);
}


template <class T>
void basicCorrRowFunc<T>::epilog (CorrelationParts& res)
{
    mRes.n (rowFuncTwoSource<T>::mWidth * rowFuncTwoSource<T>::mHeight);
    mRes.compute ();
    res = mRes;
}

template <class T>
void basicCorrRowFunc<T>::prolog ()
{
}

template <class T>
basicCorrRowFunc<T>::~basicCorrRowFunc () {}





// Pixel Map row functions

template <class T, class U>
pixelMap<T,U>::pixelMap (const T* baseA, const U* baseB, uint32_t rupA, uint32_t rupB, uint32_t width, uint32_t height,  const vector<U>& lut)
: mRUP(rupA, rupB)
{
    rowFuncOneSourceOneDestination<T,U>::mWidth = width;
    rowFuncOneSourceOneDestination<T,U>::mHeight = height;
    rowFuncOneSourceOneDestination<T,U>::mFirst = (T *) baseA;
    rowFuncOneSourceOneDestination<T,U>::mSecond = (U *) baseB;
    mLut = lut.begin();
}


template <class T, class U>
void pixelMap<T,U>::areaFunc ()
{
    uint32_t height = rowFuncOneSourceOneDestination<T,U>::mHeight;
    do { rowFunc(); } while (--height);
}

template <class T, class U>
pixelMap<T,U>::~pixelMap () {}
template <class T, class U>
void pixelMap<T,U>::prolog () {}




//
// rcBasicCorrRowFunc class implementation
//
template <>
inline void basicCorrRowFunc<uint8_t>::rowFunc()
{
    CorrelationParts::sumproduct_t Si (0), Sm (0), Sim (0), Smm (0), Sii (0);
    const uint8_t *pFirst (mFirst);
    const uint8_t *pSecond (mSecond);
    static uint32_t* sqr_lut = square_table().lut_ptr();
    
    for (const uint8_t *pEnd = mFirst + mWidth; pFirst < pEnd; ++pFirst, ++pSecond)
    {
        const uint8_t i = *pFirst;
        const uint8_t j = *pSecond;
        
        Si  += i;
        Sm  += j;
        Sii += sqr_lut[i];
        Smm += sqr_lut[j];
        Sim += sqr_lut[i + j];
    }
    mRes.accumulate (Sim, Sii, Smm, Si, Sm);
    
    mFirst += mRUP.first;
    mSecond += mRUP.second;
}



// uint8 version uses the Square Table technique
// Correlate two sources. We can not cache sums since we do not have a model representation
// Use Square table technique [reference Moravoc paper in the 80s]
template <>
inline void basicCorrRowFunc<uint8_t>::epilog (CorrelationParts& res)
{
    auto nsim = mRes.Sim() - mRes.Sii() - mRes.Smm();
    nsim = nsim / 2;
    mRes.Sim(nsim);
    mRes.n (mWidth * mHeight);
    mRes.compute ();
    res = mRes;
}


template <>
inline void pixelMap<uint8_t,uint8_t>::rowFunc ()
{
    const uint8_t *pFirst (mFirst);
    uint8_t *pSecond (mSecond);
    
    for (const uint8_t *pEnd = mFirst + mWidth; pFirst < pEnd; )
    {
        *pSecond++ = mLut[*pFirst++];
    }
    mFirst += mRUP.first;
    mSecond += mRUP.second;
}


template <>
inline void basicCorrRowFunc<uint16_t>::rowFunc()
{
    CorrelationParts::sumproduct_t Si (0), Sm (0), Sim (0), Smm (0), Sii (0);
    const uint16_t *pFirst (mFirst);
    const uint16_t *pSecond (mSecond);
    static uint32_t* sqr_lut = square_table16().lut_ptr();
    
    for (const uint16_t *pEnd = mFirst + mWidth; pFirst < pEnd; ++pFirst, ++pSecond)
    {
        const uint16_t i = *pFirst;
        const uint16_t j = *pSecond;
        
        Si  += i;
        Sm  += j;
        Sii += sqr_lut[i];
        Smm += sqr_lut[j];
        Sim += sqr_lut[i + j];
    }
    mRes.accumulate (Sim, Sii, Smm, Si, Sm);
    
    mFirst += mRUP.first;
    mSecond += mRUP.second;
}



// uint8 version uses the Square Table technique
// Correlate two sources. We can not cache sums since we do not have a model representation
// Use Square table technique [reference Moravoc paper in the 80s]
template <>
inline void basicCorrRowFunc<uint16_t>::epilog (CorrelationParts& res)
{
    auto nsim = mRes.Sim() - mRes.Sii() - mRes.Smm();
    nsim = nsim / 2;
    mRes.Sim(nsim);
    mRes.n (mWidth * mHeight);
    mRes.compute ();
    res = mRes;
}


template <>
inline void pixelMap<uint16_t, uint8_t>::rowFunc ()
{
    const uint16_t *pFirst (mFirst);
    uint8_t *pSecond (mSecond);
    
    for (const uint16_t *pEnd = mFirst + mWidth; pFirst < pEnd; )
    {
        *pSecond++ = mLut[*pFirst++];
    }
    mFirst += mRUP.first;
    mSecond += mRUP.second;
}


template class basicCorrRowFunc < uint8_t > ;
template class basicCorrRowFunc < uint16_t > ;


template class pixelMap<uint8_t, uint8_t>;
template class pixelMap<uint16_t, uint8_t>;


#if 0
/////////////// Gaussian Convolution  ///////////////////////////////

//
// The client passes in a bounded rcWindow of the same sizs as source.
// Upon return dest is a window in to the image client passed with the
// correct size

/*
	* Run Gauss around the edges
	*/
template<class P>
void gaussEdge (P* pixelPtr, const rcWindow& src, rcWindow& dest)
{
    const uint32 srcWidth(src.width());
    const uint32 srcHeight(src.height());
    const uint32 srcUpdate(src.rowPixelUpdate());
    const uint32 destUpdate(dest.rowPixelUpdate());
    const P *src0 = pixelPtr;
    P* dest0 = (P *) dest.rowPointer(0);
    
    // TL and TR pixels
    *dest0 = *src0;
    dest0[srcWidth - 1] = src0[srcWidth - 1];
    dest0++;
    
    // Top
    uint32 left(uint32 (3* *src0 + *(src0 + srcUpdate)));
    src0++;
    uint32 centre(uint32 (3* *src0 + *(src0 + srcUpdate)));
    src0++;
    uint32 width;
    for(width = srcWidth - 2; width--;)
    {
        uint32 right(uint32 (3* *src0 + *(src0 + srcUpdate)));
        *dest0++ = P ((left + centre + centre + right + 8 )/ 16);
        left = centre;
        centre = right;
        src0++;
    }
    
    // BL and BR pixels
    dest0 = (P *) dest.rowPointer (srcHeight - 1);
    src0 = (P *) src.rowPointer (srcHeight - 1);
    *dest0 = *src0;
    dest0[srcWidth - 1] = src0[srcWidth - 1];
    dest0++;
    
    // Bottom
    left = uint32 (3* *src0 + *(src0 - srcUpdate));
    src0++;
    centre = uint32 (3* *src0 + *(src0 - srcUpdate));
    src0++;
    for(width = srcWidth - 2; width--;)
    {
        uint32 right(uint32 (3* *src0 + *(src0 - srcUpdate)));
        
        *dest0++ = P ((left + centre + centre + right + 8 )/ 16);
        left = centre;
        centre = right;
        src0++;
    }
    
    // Left
    src0 = (P *) src.rowPointer (0);
    dest0 = (P *) dest.rowPointer (1);
    
    uint32 top(uint32 (3* *src0 + *(src0+1)));
    src0 += srcUpdate;
    uint32 middle(uint32 (3* *src0 + *(src0+1)));
    src0 += srcUpdate;
    
    uint32 height;
    for(height = srcHeight - 2; height--;)
    {
        uint32 bottom(uint32 (3* *src0 + *(src0 + 1)));
        *dest0 = P ((top + middle + middle + bottom + 8)/16);
        src0 += srcUpdate;
        dest0 += destUpdate;
        top = middle;
        middle = bottom;
    }
    
    // Right
    src0 = (P *) src.rowPointer (0) + srcWidth - 1;
    dest0 = (P *) dest.rowPointer (1) + srcWidth - 1;
    top = uint32 (3* *src0 + *(src0 - 1));
    src0 += srcUpdate;
    middle = uint32 (3* *src0 + *(src0 - 1));
    src0 += srcUpdate;
    
    for(height = srcHeight - 2; height--;)
    {
        uint32 bottom(uint32 (3* *src0 + *(src0 - 1)));
        *dest0 = P ((top + middle + middle + bottom + 8)/16);
        src0 += srcUpdate;
        dest0 += destUpdate;
        top = middle;
        middle = bottom;
    }
}
#endif


#endif /* __rowFunc_H */
