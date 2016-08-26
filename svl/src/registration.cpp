#include "vision/roiWindow.h"
#include "vision/rowfunc.h"
#include "core/pair.hpp"
#include "vision/pixel_traits.h"
#include "vision/ipUtils.h"
#include "vision/registration.h"

using namespace svl;


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


template locationPeak parInterpAtLocation(const roiWindow<P32F> & image, const iPair & loc);

template<typename P, class pixel_t = typename PixelType<P>::pixel_t >
void MaximaDetect(const roiWindow<P>& space, std::vector<iPair>& peaks, pixel_t accept)
{
    // Make sure it is empty
    peaks.resize(0);
    
    const int rowUpdate(space.rowUpdate());
    int height = space.height() - 3;
    int width = space.width() - 3;
    for (int row = 3; row < height; row++)
    {
        pixel_t* row_ptr = space.pelPointer(3, row);
        pixel_t* pel = row_ptr;
        for (int col = 3; col < width; col++, pel++)
        {
            if (*pel >= accept &&
                *pel > *(pel - 1) &&
                *pel > *(pel - rowUpdate - 1) &&
                *pel > *(pel - rowUpdate) &&
                *pel > *(pel - rowUpdate + 1) &&
                *pel > *(pel + 1) &&
                *pel > *(pel + rowUpdate + 1) &&
                *pel > *(pel + rowUpdate) &&
                *pel > *(pel + rowUpdate - 1))
            {
                iPair dp(col, row);
                peaks.push_back(dp);
            }
        }
    }
}

//template void roiPrint(const roiWindow<P8U> & image, ostream & o);
//template void roiPrint(const roiWindow<P16U> & image, ostream & o);
//template void roiPrint(const roiWindow<P32F> & image, ostream & o);


/* +---------------+---------------+ */
/* |               |               | */
/* |               |               | */
/* |               |               | */
/* |      *========|======*========| */
/* |      !-1,-1   |      !1,-1    | */
/* |      !        |      !        | */
/* |      !        |      !        | */
/* +---------------+---------------+ */
/* |      !  (0,0) |      !        | */
/* |      !        |      !        | */
/* |      !        |      !        | */
/* |      *========|======*========| */
/* |      !-1,1    |      !1,1     | */
/* |      !        |      !        | */
/* |      !        |      !        | */
/* +---------------+---------------+ */


template <typename P>
void Correlation::point(const roiWindow<P> & moving, const roiWindow<P> & fixed, CorrelationParts & res)
{
    typedef typename PixelType<P>::pixel_t pixel_t;
    basicCorrRowFunc<pixel_t> corrfunc(moving.rowPointer(0), fixed.rowPointer(0), moving.rowUpdate(), fixed.rowUpdate(), fixed.width(), fixed.height());
    corrfunc.areaFunc();
    corrfunc.epilog(res);
}


template <typename P>
bool Correlation::area_translation(const roiWindow<P> & moving, const roiWindow<P> & fixed, spaceResult& sres)
{
    CorrelationParts cp;

    // Size and create search space
    const iPair searchSpace(moving.width() - fixed.width() + 1,
                            moving.height() - fixed.height() + 1);
    roiWindow<P32F> cspace(searchSpace.x(), searchSpace.y());
    cspace.set(0);

    uint32_t curRow = 0;
    do
    {
        uint32_t curCol = 0;
        do
        {
            // rcCorrelationWindow for the model
            roiWindow<P> movingWin(moving, curCol, curRow, fixed.width(), fixed.height());

            Correlation::point(fixed, movingWin, cp);
            const float r = cp.r();
            cspace.setPixel(curCol, curRow, r);
        } while (++curCol && curCol < searchSpace.x());
        curRow++;
    } while (curRow < searchSpace.y());


  //  cspace.print_pixel();
    sres.space = cspace;
    MaximaDetect(cspace, sres.peaks, sres.accept);

    return ! sres.peaks.empty();
}


template <typename P>
bool imageTranslation<P>::addRegion(const iRect & rr, uint32_t unique_tag)
{
    if (m_regions.find(unique_tag) != m_regions.end()) return false;
    m_regions[unique_tag] = rr;
    return (m_regions.find(unique_tag) != m_regions.end());
}


template <typename P>
void imageTranslation<P>::run()
{
    auto regionItr = m_regions.begin();

    while (regionItr != m_regions.end())
    {
        auto & win = *regionItr;
        roiWindow<P> model(m_right.frameBuf(), win.second);
        bool padcheck(false);
        iRect swin = win.second.trim(-m_slide.first, -m_slide.first, -m_slide.second, -m_slide.second, padcheck);
        if (!padcheck) continue;
        spaceResult result;
        roiWindow<P> search(m_left.frameBuf(), swin);
        Correlation::area_translation(search, model, result);
        uint32_t tag = win.first;
        m_results.insert(std::make_pair(tag, result));
        ++regionItr;
    }
}



template void Correlation::point(const roiWindow<P8U> & moving, const roiWindow<P8U> & fixed, CorrelationParts & res);

template bool Correlation::area_translation(const roiWindow<P8U> & moving, const roiWindow<P8U> & fixed, spaceResult& );

template class imageTranslation<P8U>;


