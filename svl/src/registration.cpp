#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcomma"
#pragma GCC diagnostic ignored "-Wint-in-bool-context"




#include "vision/roiWindow.h"
#include "vision/rowfunc.h"
#include "core/pair.hpp"
#include "vision/pixel_traits.h"
#include "vision/ipUtils.h"
#include "vision/registration.h"
#include "vision/histo.h"
#include <cmath> // log

using namespace svl;

void MutualInfo::getMI(const roiWindow<P8U> &I, const roiWindow<P8U> &M, Parts8U& out)
{
    // ? Use overlap rect ?
    // if size are different
    out.valid = false;
    if (I.size() != M.size()) return;
    
    out.joint = Eigen::MatrixXd (256,256);
    out.joint.setZero();
    Eigen::MatrixXd iH(1,256), mH(1,256);
    iH.setZero();
    mH.setZero();
    
    size_t width = I.width();
    size_t height = I.height ();

    // Accumulate separate and joint histograms
    uint32_t curRow = 0;
    do
    {
        const uint8_t* iPtr = I.rowPointer(curRow);
        const uint8_t* mPtr = M.rowPointer(curRow);
        
        uint32_t curCol = 0;
        do
        {
            uint8_t iPel = *iPtr++;
            uint8_t mPel = *mPtr++;
            iH(iPel) += 1;
            mH (mPel) += 1;
            out.joint(iPel, mPel) += 1;
        } while (++curCol && curCol < width);
        curRow++;
    } while (curRow < height);
    
    // Normalize all by number of samples
    out.joint=out.joint/I.n();
    iH = iH / I.n();
    mH = mH / M.n();
    
    // Accumulate for separate entropies
    double MI = 0, h_i = 0, h_m = 0;

    // I (c,c') = H(c) + H(c') - H(c,c')

    for(unsigned int t=0;t<256;t++)
    {
        for(unsigned int r=0;r<256;r++)
        {
            if(std::fabs(out.joint(r,t)) > std::numeric_limits<double>::epsilon())
                MI+= out.joint(r,t)*std::log2(out.joint(r,t));
        }
        if(std::fabs(iH(t)) > std::numeric_limits<double>::epsilon())
        {
            auto e = - iH(t)*std::log2(iH(t));
            MI += e;
            h_i += e;
        }
        if(std::fabs(mH(t)) > std::numeric_limits<double>::epsilon())
        {
            auto e = - mH(t)*std::log2(mH(t));
            MI += e;
            h_m += e;
        }
    }
    out.mi = MI;
    out.iH = h_i;
    out.mH = h_m;
    double him = (h_i + h_m) - out.mi;
    out.icv = (2*him - h_i - h_m) / std::log2(I.n());
    
    // Normalized MI = I(X,Y) / (H(X)H(Y))^0.5
    double denom = std::sqrt (out.iH * out.mH);
    out.valid = denom != 0;
    if (out.valid)
    {
        out.nmi = out.icv / denom;
        out.acos_nmi = std::acos(out.nmi);
    }
}

ostream & MutualInfo::operator<<(ostream & o, const MutualInfo::Parts8U & p)
{
    o << " H(I)             : " << p.iH << std::endl;
    o << " H(M)             : " << p.mH << std::endl;
    o << " I(I,M)           : " << p.mi << std::endl;
    o << " VI(I,M)          : " << p.icv << std::endl;
    o << " NMI(I,M)         :" << p.nmi << std::endl;
    o << " acos(NMI(I,M))   :" << p.acos_nmi << std::endl;
    return o;
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

template <typename P>
bool Correlation::autoCorrelation(const roiWindow<P> & fixed, const uint32_t half_size, spaceResult& result)
{
    int delta = 2 * half_size + 1;
    iRect fr (half_size, half_size, fixed.width() - delta, fixed.height() - delta);
    bool contains = fixed.contains (fr);
    if (contains)
    {
        roiWindow<P> ctr (fixed.frameBuf(), fr);
        bool ac = Correlation::area_translation(fixed, ctr, result);
        return ac;
    }
    return contains;
}


template void Correlation::point(const roiWindow<P8U> & moving, const roiWindow<P8U> & fixed, CorrelationParts & res);

template bool Correlation::area_translation(const roiWindow<P8U> & moving, const roiWindow<P8U> & fixed, spaceResult& );

template bool Correlation::autoCorrelation(const roiWindow<P8U> & fixed, const uint32_t half_size, spaceResult& result);

template class imageTranslation<P8U>;


#pragma GCC diagnostic pop
