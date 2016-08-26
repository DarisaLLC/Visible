#include "vision/roiWindow.h"
#include "vision/rowfunc.h"
#include "core/pair.hpp"
#include "core/angle_units.h"
#include "vision/ipUtils.h"
#include <tuple>
#include <array>
#include "vision/histo.h"
#include "vision/gauss.hpp"
#include "vision/gmorph.hpp"
#include "vision/sample.hpp"

// A class to encapsulate a table of pre-computed shifts

template <int N>
class mapUpLUT
{

public:
    // ctor
    mapUpLUT()
    {
        for (uint32_t i = 0; i < mTable.size(); i++)
            mTable[i] = i << N;
    };

    // Array access operator
    uint32_t operator[](uint32_t index) const
    {
        assert(index < mTable.size());
        return mTable[index];
    };
    uint32_t * lut_ptr() { return &mTable[0]; }


    // Array size
    uint32_t size() const
    {
        return mTable.size();
    }

private:
    std::array<uint16_t, 256> mTable;
};

static mapUpLUT<2> uint8_to_10bit_lut;

uRadian uRadian::norm() const
{
    uRadian r;
    r.val = fmod(val, 2 * uPI);
    if (r.val < 0.) r.val += 2 * uPI;
    return r;
}

uDegree uDegree::norm() const
{
    uDegree r;
    r.val = fmod(val, 360.);
    if (r.val < 0.) r.val += 360.;
    return r;
}

/*
 * Signed unit normalization routines:
 *
 * The standard normalization routines force the result into the
 * In the range of [-N/2 - N/2] return the sign that produces
 * the minimum magnitude result. 
 */
uRadian uRadian::normSigned() const
{
    uRadian r;
    r = norm();
    if (r.val < 2 * uPI - r.val)
        return r;
    r.val = r.val - 2 * uPI;
    return r;
}

uDegree uDegree::normSigned() const
{
    uDegree r;
    r = norm();
    if (r.val < 360. - r.val)
        return r;
    r.val = r.val - 360.;
    return r;
}


//
//   |                      /----------------
//  O|                     / |
//  u|                    /  |
//  t|                   /   |
//  p|------------------/    |
//  u|                  |    |
//  t|<---------------->|<-->|
//       |  "offset"         "spread"
//   |
// --+---Input--------------------------------
//   |


roiWindow<P8U> convertandReport(const uint16_t * pels, uint32_t width, uint32_t height, tpair<uint16_t> & range)
{
    uint32_t num = width * height;
    range = GetRange<const uint16_t, uint16_t>(pels, num);
    fPair frange((float)range.first, (float)range.second);

    float spread = frange.second - frange.first + 1;
    float factor = std::max(spread / 256.0f, 1.0f);

    std::vector<uint8_t> lut(1 << 16);
    std::vector<uint8_t>::iterator itr = lut.begin();
    std::advance(itr, range.first);

    for (uint16_t val16 = range.first; val16 <= range.second; val16++, itr++)
    {
        uint16_t val = val16 - range.first;
        val = (uint16_t)(val / factor);
        *itr = (uint8_t)(val);
    }

    roiWindow<P8U> src8(width, height);
    pixelMap<uint16_t, uint8_t> converted(pels, src8.pelPointer(0, 0), width, width, width, height, lut);
    converted.areaFunc();
    return src8;
}


roiWindow<P8U> convert(const uint16_t * pels, uint32_t width, uint32_t height)
{
    tpair<uint16_t> range;
    return convertandReport(pels, width, height, range);
}

void upBiValueMap(roiWindow<P8U> & src, tpair<uint16_t> & range, std::shared_ptr<uint16_t> & src16)
{
    uint16_t * pels16ptr = src16.get();
    for (int hh = 0; hh < src.height(); hh++)
    {
        uint8_t * row_ptr = src.rowPointer(hh);
        uint8_t * pel_ptr = row_ptr;
        for (int ww = 0; ww < src.width(); ww++)
            *pels16ptr++ = *pel_ptr++ == 0 ? range.first : range.second;
    }
}



std::pair<uint8_t,std::vector<std::pair<uint8_t,int> > > segmentationByInvaraintStructure(roiWindow<P8U>& src)
{

    histoStats hh;
    hh.from_image<P8U>(src);
     std::cout << hh;
    std::vector<uint8_t> valid_bins;
    valid_bins.resize(0);
    int two_pct = hh.n() / 50;
    for (unsigned binh = 0; binh < hh.histogram().size(); binh++)
    {
        if (hh.histogram()[binh] > two_pct ) valid_bins.push_back((uint8_t) binh);
    }
    
    auto width = src.width();
    auto height = src.height();

    // Create lut mapps
    std::vector<uint8_t> backtoback(512, 0);
    for (int ii = 256; ii < 512; ii++)
        backtoback[ii] = 1;

    // Start at mid point between 0000 and 1111
    std::vector<uint8_t>::const_iterator pmMid = backtoback.begin();
    std::advance(pmMid, 256);


    roiWindow<P8U> tmp(src.width(), src.height());
    std::vector<std::pair<uint8_t, int> > i3results;
    int last_corr_val(0);
    uint8_t peak = 255;
    roiWindow<P8U> last;

    for (unsigned tt = 0; tt < valid_bins.size(); tt++)
    {
        std::vector<uint8_t>::const_iterator pmBegin = pmMid;
        std::advance(pmBegin, - valid_bins[tt]);
        
        pixelMap<uint8_t, uint8_t> converted(src.pelPointer(0, 0), tmp.pelPointer(0, 0), width, width, width, height, pmBegin);
        converted.areaFunc();
        basicCorrRowFunc<uint8_t> corrfunc(src.pelPointer(0, 0), tmp.pelPointer(0, 0), width, width, width, height);
        corrfunc.areaFunc();
        CorrelationParts cp;
        corrfunc.epilog(cp);
        int32_t weight = (1000*hh.histogram()[valid_bins[tt]]) / hh.n();
        i3results.push_back(make_pair(valid_bins[tt],weight));
        std::cout << "[" << (int) valid_bins[tt] << "]: " << cp.milR() << "   " << weight << std::endl;
        if (cp.milR() >= last_corr_val)
        {
            last_corr_val = cp.milR();
            peak = valid_bins[tt];
        }
    }

    return make_pair(peak, i3results);
}

roiWindow<P8U> extractAtLevel (roiWindow<P8U>& src, uint8_t thr)
{
    std::vector<uint8_t> map (256, 0);
    map[thr] = thr;
    auto width = src.width();
    auto height = src.height();
    roiWindow<P8U> dst (width, height);
    
    pixelMap<uint8_t, uint8_t> converted(src.pelPointer(0, 0), dst.pelPointer(0, 0), width, width, width, height, map);
    converted.areaFunc();
    return dst;
}


template void Gauss3by3(const roiWindow<P8U> &, roiWindow<P8U> &);
template void Gauss3by3(const roiWindow<P16U> &, roiWindow<P16U> &);

template void PixelMin3by3(const roiWindow<P8U> &, roiWindow<P8U> &);
template void PixelMax3by3(const roiWindow<P16U> &, roiWindow<P16U> &);

template void PixelSample(const roiWindow<P8U> &, roiWindow<P8U> &, int32_t, int32_t);
template void PixelSample(const roiWindow<P16U> &, roiWindow<P16U> &, int32_t, int32_t);
