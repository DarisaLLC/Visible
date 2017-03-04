
#include "vision/rowfunc.h"

SINGLETON_FCN(sSqrTable<uint8_t>, square_table);
SINGLETON_FCN(sSqrTable<uint16_t>, square_table16);

// Stream output operator
ostream & operator<<(ostream & os, const CorrelationParts & corr)
{
    os << corr.r() << std::endl;
    return os;
}


// public


template <class T>
basicCorrRowFunc<T>::basicCorrRowFunc(const T * baseA, const T * baseB, uint32_t rowPelsA, uint32_t rowPelsB,
                                      uint32_t width, uint32_t height)
    : mRUP(rowPelsA, rowPelsB)
{
    rowFuncTwoSource<T>::mWidth = width;
    rowFuncTwoSource<T>::mHeight = height;
    rowFuncTwoSource<T>::mFirst = baseA;
    rowFuncTwoSource<T>::mSecond = baseB;
}


template <class T>
void basicCorrRowFunc<T>::areaFunc()
{
    uint32_t height = rowFuncTwoSource<T>::mHeight;

    do
    {
        rowFunc();
    } while (--height);
}


template <class T>
void basicCorrRowFunc<T>::epilog(CorrelationParts & res)
{
    auto nsim = mRes.Sim() - mRes.Sii() - mRes.Smm();
    nsim = nsim / 2;
    mRes.Sim(nsim);
    mRes.n (rowFuncTwoSource<T>::mWidth * rowFuncTwoSource<T>::mHeight);
    mRes.compute();
    res = mRes;
}

template <class T>
void basicCorrRowFunc<T>::prolog()
{
}

template <class T>
basicCorrRowFunc<T>::~basicCorrRowFunc() {}


// Pixel Map row functions

template <class T, class U>
pixelMap<T, U>::pixelMap(const T * baseA, const U * baseB, uint32_t rupA, uint32_t rupB, uint32_t width, uint32_t height, const vector<U> & lut)
    : mRUP(rupA, rupB)
{
    rowFuncOneSourceOneDestination<T, U>::mWidth = width;
    rowFuncOneSourceOneDestination<T, U>::mHeight = height;
    rowFuncOneSourceOneDestination<T, U>::mFirst = (T *)baseA;
    rowFuncOneSourceOneDestination<T, U>::mSecond = (U *)baseB;
    mLut = lut.begin();
}


template <class T, class U>
pixelMap<T, U>::pixelMap(const T * baseA, const U * baseB, uint32_t rupA, uint32_t rupB, uint32_t width, uint32_t height, const lutItr & lut)
    : mRUP(rupA, rupB)
{
    rowFuncOneSourceOneDestination<T, U>::mWidth = width;
    rowFuncOneSourceOneDestination<T, U>::mHeight = height;
    rowFuncOneSourceOneDestination<T, U>::mFirst = (T *)baseA;
    rowFuncOneSourceOneDestination<T, U>::mSecond = (U *)baseB;
    mLut = lut;
}


template <class T, class U>
void pixelMap<T, U>::areaFunc()
{
    uint32_t height = rowFuncOneSourceOneDestination<T, U>::mHeight;
    do
    {
        rowFunc();
    } while (--height);
}

template <class T, class U>
pixelMap<T, U>::~pixelMap() {}
template <class T, class U>
void pixelMap<T, U>::prolog() {}


//
// rcBasicCorrRowFunc class implementation
//
template <>
inline void basicCorrRowFunc<uint8_t>::rowFunc()
{
    CorrelationParts::sumproduct_t Si(0), Sm(0), Sim(0), Smm(0), Sii(0);
    const uint8_t * pFirst(mFirst);
    const uint8_t * pSecond(mSecond);
    static bool onetime = false;
    static uint32_t * sqr_lut = square_table().lut_ptr();
    if (! onetime)
    {
        onetime = square_table().validate();
    }
    
    for (const uint8_t * pEnd = mFirst + mWidth; pFirst < pEnd; ++pFirst, ++pSecond)
    {
        const uint32_t i = *pFirst;
        const uint32_t j = *pSecond;

        Si += i;
        Sm += j;
        Sii += sqr_lut[i];
        Smm += sqr_lut[j];
        Sim += sqr_lut[i + j];
    }
    mRes.accumulate(Sim, Sii, Smm, Si, Sm);

    mFirst += mRUP.first;
    mSecond += mRUP.second;
}


template <>
inline void pixelMap<uint8_t, uint8_t>::rowFunc()
{
    const uint8_t * pFirst(mFirst);
    uint8_t * pSecond(mSecond);

    for (const uint8_t * pEnd = mFirst + mWidth; pFirst < pEnd;)
    {
        *pSecond++ = mLut[*pFirst++];
    }
    mFirst += mRUP.first;
    mSecond += mRUP.second;
}


template <>
inline void basicCorrRowFunc<uint16_t>::rowFunc()
{
    CorrelationParts::sumproduct_t Si(0), Sm(0), Sim(0), Smm(0), Sii(0);
    const uint16_t * pFirst(mFirst);
    const uint16_t * pSecond(mSecond);
    static uint32_t * sqr_lut = square_table16().lut_ptr();

    for (const uint16_t * pEnd = mFirst + mWidth; pFirst < pEnd; ++pFirst, ++pSecond)
    {
        const uint16_t i = *pFirst;
        const uint16_t j = *pSecond;

        Si += i;
        Sm += j;
        Sii += sqr_lut[i];
        Smm += sqr_lut[j];
        Sim += sqr_lut[i + j];
    }
    mRes.accumulate(Sim, Sii, Smm, Si, Sm);

    mFirst += mRUP.first;
    mSecond += mRUP.second;
}


template <>
inline void pixelMap<uint16_t, uint8_t>::rowFunc()
{
    const uint16_t * pFirst(mFirst);
    uint8_t * pSecond(mSecond);

    for (const uint16_t * pEnd = mFirst + mWidth; pFirst < pEnd;)
    {
        *pSecond++ = mLut[*pFirst++];
    }
    mFirst += mRUP.first;
    mSecond += mRUP.second;
}


//template <> basicCorrRowFunc<uint8_t>::m_squareTable = sSqrTable < uint8_t > ;

template class sSqrTable<uint8_t>;
template class sSqrTable<uint16_t>;


template class basicCorrRowFunc<uint8_t>;
template class basicCorrRowFunc<uint16_t>;


template class pixelMap<uint8_t, uint8_t>;
template class pixelMap<uint16_t, uint8_t>;



