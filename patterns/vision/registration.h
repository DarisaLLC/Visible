#ifndef __IMGREG__
#define __IMGREG__

#include "roiWindow.h"
#include "pair.hpp"
#include "rectangle.h"
#include "fit.hpp"
#include "ipUtils.h"
#include <vector>
#include <queue>
#include <bitset>
#include <map>

using namespace DS;


enum Side : int
{
    Left = 0,
    Right = 1,
    Top = Left,
    Bottom = Right
};

struct locationPeak
{
    bool validInput;
    iPair integer;
    fPair interpolated;
    double rval;
    std::bitset<2> onVEdge;
    std::bitset<2> onHEdge;
};

struct spaceResult
{
    roiWindow<P32F> space;
    std::vector<iPair> peaks;
    float accept;
};


template <typename T>
static locationPeak parInterpAtLocation(const roiWindow<T> & image, const iPair & loc)
{
    typedef typename PixelType<T>::pixel_ptr_t pixel_ptr_t;
    typedef typename PixelType<T>::pixel_t pixel_t;

    locationPeak lp;
    lp.validInput = image.isBound();
    lp.onHEdge[Side::Left] = loc.first >= 1;
    lp.onHEdge[Side::Right] = loc.first < image.width() - 1;
    lp.onVEdge[Side::Top] = loc.second >= 1;
    lp.onVEdge[Side::Bottom] = loc.second < image.height() - 1;

    lp.integer = loc;
    lp.interpolated = fPair(numeric_limits<float>::infinity(), numeric_limits<float>::infinity());
    if (lp.onVEdge.all())
    {
        auto pval = parabolicFit<pixel_t, float>(image.getPixel(loc.first, loc.second - 1), image.getPixel(loc.first, loc.second),
                                                 image.getPixel(loc.first, loc.second + 1), &lp.interpolated.second);
    }

    if (lp.onHEdge.all())
    {
        auto pval = parabolicFit<pixel_t, float>(image.getPixel(loc.first - 1, loc.second), image.getPixel(loc.first, loc.second),
                                                 image.getPixel(loc.first + 1, loc.second), &lp.interpolated.first);
    }

    return lp;
}


// @design idea: std::map<uint32_t, std::function<std::shared_ptr<Class>()>>


namespace Correlation
{
template <typename P>
void point(const roiWindow<P> & moving, const roiWindow<P> & fixed, CorrelationParts & res);
template <typename P>
bool area_translation(const roiWindow<P> & moving, const roiWindow<P> & fixed, spaceResult &);
}


template <typename P>
class imageTranslation
{
public:
    // @todo:
    // Construct from image pair and region vector
    // Construct from jspon file
    imageTranslation()
        : m_valid(false) {}
    void clear()
    {
        m_regions.clear();
        m_results.clear();
    }

    // Check that each rect is sl clear of root's boundaries
    bool addRegion(const iRect & rr, uint32_t unique_tag);

    void setRoots(const roiWindow<P> & left, const roiWindow<P> & right, const iPair & sl)
    {
        m_left = left;
        m_right = right;
        m_slide = sl;
    }
    bool isValid() const { return m_valid; }
    const std::map<uint32_t, spaceResult> & results() { return m_results; }
    void run();

private:
    std::map<uint32_t, iRect> m_regions;
    std::map<uint32_t, spaceResult> m_results;
    roiWindow<P> m_left, m_right;
    mutable bool m_valid;
    bool run_region();
    iPair m_slide;
};


#endif
