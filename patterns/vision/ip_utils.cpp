#include "roiWindow.h"
#include "rowfunc.h"
#include "pair.hpp"
#include "angle_units.hpp"
#include "ipUtils.h"
#include <tuple>
#include <array>
#include "vision/histo.h"
#include "gauss.hpp"
#include "gmorph.hpp"
#include "sample.hpp"

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


/*
 * Slide the smaller image over the larger one, computer normalized correlation and return the max peak information
 * in the pixel coordinates of the larger image. 
 * Slide has to be within the largest possible slide between the big and small ( big - small + 1 )
 * If the two image are same size, only one correlation is computed and returned. 
 * @todo: partial overlap
 */

iRect computeCompleteOverlap(std::pair<iRect, iRect> & rects, iRect & slide)
{
    bool first_is_bigger = rects.first.contains(rects.second);
    if (!first_is_bigger)
    {
        iRect tmp = rects.first;
        rects.first = rects.second;
        rects.second = tmp;
    }
    bool sucess = false;
    iRect play = rects.first.intersect(rects.second, sucess);
    if (sucess) return play;
    return iRect();
}


std::pair<roiWindow<P8U>, tpair<uint16_t>> invariantStructureSegmentation(const uint16_t * src16, int width, int height, int scan)
{
    tpair<uint16_t> range;
    roiWindow<P8U> src = convertandReport(src16, width, height, range);

    histoStats hh;
    hh.from_image<P8U>(src);
    std::cout << hh;

    // Start at Median point - 5 to + 5
    iPair bounds;
    bounds.first = std::max(0, (hh.max() - hh.min()) / 2 - scan);
    bounds.second = std::min(255, bounds.first + scan + scan);


    // Create lut mapps
    std::vector<uint8_t> backtoback(512, 0);
    for (int ii = 256; ii < 512; ii++)
        backtoback[ii] = 1;

    // Start at last element before 1s and --
    std::vector<uint8_t>::const_iterator pmBegin = backtoback.begin();
    std::advance(pmBegin, bounds.first);
    std::vector<uint8_t>::const_iterator pmEnd = backtoback.begin();
    std::advance(pmEnd, bounds.second);

    roiWindow<P8U> tmp(width, height);
    std::vector<int> i3results;
    int last_corr_val(0);
    roiWindow<P8U> last;
    for (int thr = bounds.first; pmBegin < pmEnd && pmBegin < backtoback.end(); pmBegin++, thr++)
    {
        pixelMap<uint8_t, uint8_t> converted(src.pelPointer(0, 0), tmp.pelPointer(0, 0), width, width, width, height, pmBegin);
        converted.areaFunc();
        basicCorrRowFunc<uint8_t> corrfunc(src.pelPointer(0, 0), tmp.pelPointer(0, 0), width, width, width, height);
        corrfunc.areaFunc();
        CorrelationParts cp;
        corrfunc.epilog(cp);
        i3results.push_back(cp.milR());
        std::cout << "[" << thr << "]: " << cp.milR() << std::endl;
        if (cp.milR() >= last_corr_val)
            last_corr_val = cp.milR();
        else
            break;
    }

    return make_pair(tmp, range);
}


template void Gauss3by3(const roiWindow<P8U> &, roiWindow<P8U> &);
template void Gauss3by3(const roiWindow<P16U> &, roiWindow<P16U> &);

template void PixelMin3by3(const roiWindow<P8U> &, roiWindow<P8U> &);
template void PixelMax3by3(const roiWindow<P16U> &, roiWindow<P16U> &);

template void PixelSample(const roiWindow<P8U> &, roiWindow<P8U> &, int32_t, int32_t);
template void PixelSample(const roiWindow<P16U> &, roiWindow<P16U> &, int32_t, int32_t);
