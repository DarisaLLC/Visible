
#include "core/angle_units.h"
#include "vision/gradient.h"
#include "vision/edgel.h"
#include "core/pair.hpp"
#include "vision/rowfunc.h"
#include <assert.h>

using namespace std;


static EdgeTables sEdgeTables;

/*
 *  Does not respect the IplROI
 *  sobel 3x3 the outer row and col are undefined
 */


static void sobelEdgeProcess_b(const roiWindow<P8U> & image, roiWindow<P8U> & magnitudes, roiWindow<P8U> & angles)
{
    // I/O images the same size. First sobel out put is at halfK where kernel is 3x3
    assert(image.width() == magnitudes.width());
    assert(image.width() == angles.width());
    assert(image.height() == magnitudes.height());
    assert(image.height() == angles.height());
    
    int magUpdate = magnitudes.rowUpdate(); // rowPixelUpdate();
    int angleUpdate = angles.rowUpdate();   // rowPixelUpdate();
    int srcUpdate = image.rowUpdate();      // rowPixelUpdate();
    const uint8_t * src0 = image.pelPointer(0, 0);
    
    iPair kernel_3(3, 3);
    iPair halfK = kernel_3 / 2;
    
    uint8_t * mag = magnitudes.pelPointer(halfK.first, halfK.second);
    uint8_t * angle = angles.pelPointer(halfK.first, halfK.second);
    
    int h(angles.height() - 2);
    int width(angles.width() - 2);
    
    // For 8 bit images, we divide by 8 (4+4). For 16 bit images, we further divide by 256
    // This could be parameterized as pixel depth is often 12 or 14 bits and not full 16 bits
    
    const int normBits = 3;
    
    const uint8_t * magTable = sEdgeTables.magnitudeTable();
    
    // Sobel kernels are symertic. We use a L C R roll over algorithm
    
    do
    {
        const uint8_t * src = src0;
        uint8_t * m = mag;
        uint8_t * a = angle;
        
        int left(src[0] + 2 * src[srcUpdate] + src[2 * srcUpdate]);
        int yKernelLeft(-src[0] + src[2 * srcUpdate]);
        src++;
        int center(src[0] + 2 * src[srcUpdate] + src[2 * srcUpdate]);
        int yKernelCenter(-src[0] + src[2 * srcUpdate]);
        src++;
        int w(width);
        
        do
        {
            int right(src[0] + 2 * src[srcUpdate] + src[2 * srcUpdate]);
            int yKernelRight(-src[0] + src[2 * srcUpdate]);
            
            // Calculate x and y magnitudes
            left = right - left;
            int product = yKernelLeft + 2 * yKernelCenter + yKernelRight;
            uint8_t x = (uint8_t)(std::abs(left) >> normBits);
            uint8_t y = (uint8_t)(std::abs(product) >> normBits);
            int index = (x << EdgeTables::eMagPrecision) | y;
            
            if (index >= EdgeTables::eMagTableSize)
            {
                std::cerr << index << (int)x << "," << (int)y << "," << w << "," << h << "," << right << "," << left << endl;
            }
            assert(index < EdgeTables::eMagTableSize);
            
            *m = magTable[index];
            m++;
            
            // Angle Table
            // if both x and y gradient are less than 5 (1 gray level edge)
            // then set the angle to zero.
            
            // @note add angle threshold
            //      if (rmABS(product) < 3 && rmABS(left) < 3)
            //	*a++ = dummy;
            //      else
            *a++ = sEdgeTables.binAtan(product, left);
            
            left = center;
            center = right;
            yKernelLeft = yKernelCenter;
            yKernelCenter = yKernelRight;
            src++;
        } while (--w);
        mag += magUpdate;
        angle += angleUpdate;
        src0 += srcUpdate;
    } while (--h);
}

/*
 * TruePeak - This is a 3 pixel operator that selects the middle pixel
 * if it is a peak. This pixel is deemed a peak if either
 * a) it is greater than both its neighbours
 * b) it is greater than a neighbour on the "black" side of the image
 *    and equal to the neighbour on the other side
 *
 * This is fairly inefficient implementation.
 */


// Max Magnitude from one of the kernels is ([1 2 1] * 255) / 4 or 255
// We are using 7bits for magnitude and 16 bits for precision for angle.
// @note really wasteful. Have to design later.


void EdgeTables::init()
{
    mMagScale = 255 / 256.;
    int x, y, mapped;
    double realVal;
    mMagTable.resize(eMagTableSize);
    mThetaTable.resize(eAtanRange + 1);
    
    // Magnitude Table: Linear Scale.
    uint8_t * atIndex = &mMagTable[0];
    for (x = 0; x <= eMaxMag; x++)
        for (y = 0; y <= eMaxMag; y++)
        {
            realVal = sqrt(double(y * y + x * x));
            mapped = (int)(std::min(realVal * mMagScale, 255.0) + 0.5);
            *atIndex++ = uint8_t(mapped);
        }
    
    for (int i = 0; i <= eAtanRange; ++i)
    {
        uRadian rd(atan((double)i / eAtanRange));
        uAngle8 r8(rd);
        
        mThetaTable[i] = r8.basic();
    }
}


inline uint8_t EdgeTables::binAtan(int y, int x) const
{
    static uint8_t mPIover2(1 << ((sizeof(uint8_t) * 8) - 2)), mPI(1 << ((sizeof(uint8_t) * 8) - 1)), m3PIover2(mPIover2 + mPI);
    static int precision(eAtanPrecision);
    
    const uint8_t * ThetaTable = angleTable();
    
    if (y < 0)
    {
        y = -y;
        if (x >= 0)
        {
            if (y <= x)
                return -ThetaTable[(y << precision) / x];
            else
                return (m3PIover2 + ThetaTable[(x << precision) / y]);
        }
        else
        {
            x = -x;
            if (y <= x)
                return (mPI + ThetaTable[(y << precision) / x]);
            else
                return (m3PIover2 - ThetaTable[(x << precision) / y]);
        }
    }
    
    else
    {
        if (x >= 0)
        {
            if (x == 0 && y == 0) return 0;
            if (y <= x)
                return ThetaTable[(y << precision) / x];
            else
                return (mPIover2 - ThetaTable[(x << precision) / y]);
        }
        else
        {
            x = -x;
            if (y <= x)
                return (mPI - ThetaTable[(y << precision) / x]);
            else
                return (mPIover2 + ThetaTable[(x << precision) / y]);
        }
    }
}


// Computes the 8-connected direction from the 8 bit angle
// @todo support 16bit and true peak
static inline int _getAxis(uint8_t val)
{
    return ((val + (1 << (8 - 4))) >> (8 - 3)) % 8;
}


unsigned int SpatialEdge(const roiWindow<P8U> & magImage, const roiWindow<P8U> & angleImage, std::vector<feature>& features, uint8_t threshold)
{
    assert(magImage.width() > 0);
    assert(angleImage.height() > 0);
    assert(magImage.width() == angleImage.width());
    assert(magImage.height() == angleImage.height());
    
    iPair kernel_5(5, 5);
    iPair halfK = kernel_5 / 2;
    int magUpdate(magImage.rowUpdate());
    
    for (int y = halfK.y(); y < magImage.height() - halfK.y(); y++)
    {
        const uint8_t * mag = magImage.pelPointer(halfK.x(), y);
        const uint8_t * ang = angleImage.pelPointer(halfK.x(), y);
        
        for (int x = halfK.x(); x < magImage.width() - halfK.x(); x++, mag++, ang++)
        {
            uint8_t ctr = *mag;
            if (ctr < threshold)
                continue;
            
            uint8_t m1, m2;
            int angle = _getAxis(*ang);
            
            switch (angle)
            {
                case 0:
                case 4:
                    m1 = *(mag - 1);
                    m2 = *(mag + 1);
                    break;
                    
                case 1:
                case 5:
                    m1 = *(mag - 1 - magUpdate);
                    m2 = *(mag + 1 + magUpdate);
                    break;
                    
                case 2:
                case 6:
                    m1 = *(mag - magUpdate);
                    m2 = *(mag + magUpdate);
                    break;
                    
                case 3:
                case 7:
                    m1 = *(mag + 1 - magUpdate);
                    m2 = *(mag - 1 + magUpdate);
                    break;
                    
                default:
                    m1 = m2 = 0;
                    assert(false);
            }
            
//            feature (const int& x, const int& y, uint8_t magnitude, uAngle8 angle,
//                     uint8_t posNeighborMag, uint8_t negNeighborMag,
//                     bool doInterpSubPixel = false,
//                     double weight = 1,
//                     bool isMod180 = false) :
            
            if ((ctr > m1 && ctr >= m2) || (ctr >= m1 && ctr > m2))
                features.emplace_back(x, y, ctr, uAngle8(*ang), angle, m2, m1, false, 1, false);

        }
    }
    return static_cast<int>(features.size());

}


/*
 * mag / ang are |pad| ==> peaks are |pad|pad|
 * Sobel is 3x3. Peak Detection is also 3x3 ==> 5x5 operation
 * Peak is same size and translation as mag and ang which are the same as input image
 */


unsigned int SpatialEdge(const roiWindow<P8U> & magImage, const roiWindow<P8U> & angleImage, roiWindow<P8U> & peaks_, uint8_t threshold, bool angleLabeled)
{
    assert(magImage.width() > 0);
    assert(angleImage.height() > 0);
    assert(magImage.width() == peaks_.width());
    assert(magImage.width() == angleImage.width());
    assert(magImage.height() == peaks_.height());
    assert(magImage.height() == angleImage.height());
    
    unsigned int numPeaks = 0;
    iPair kernel_5(5, 5);
    iPair halfK = kernel_5 / 2;
    int magUpdate(magImage.rowUpdate());
    
    for (int y = halfK.y(); y < peaks_.height() - halfK.y(); y++)
    {
        const uint8_t * mag = magImage.pelPointer(halfK.x(), y);
        const uint8_t * ang = angleImage.pelPointer(halfK.x(), y);
        uint8_t * dest = peaks_.pelPointer(halfK.x(), y);
        
        for (int x = halfK.x(); x < peaks_.width() - halfK.x(); x++, mag++, ang++, dest++)
        {
            if (*mag < threshold)
            {
                *dest = 0;
                continue;
            }
            
            uint8_t m1, m2;
            int angle = _getAxis(*ang);
            
            switch (angle)
            {
                case 0:
                case 4:
                    m1 = *(mag - 1);
                    m2 = *(mag + 1);
                    break;
                    
                case 1:
                case 5:
                    m1 = *(mag - 1 - magUpdate);
                    m2 = *(mag + 1 + magUpdate);
                    break;
                    
                case 2:
                case 6:
                    m1 = *(mag - magUpdate);
                    m2 = *(mag + magUpdate);
                    break;
                    
                case 3:
                case 7:
                    m1 = *(mag + 1 - magUpdate);
                    m2 = *(mag - 1 + magUpdate);
                    break;
                    
                default:
                    m1 = m2 = 0;
                    assert(false);
            }
            
            if ((*mag > m1 && *mag >= m2) || (*mag >= m1 && *mag > m2))
            {
                numPeaks++;
                *dest = (angleLabeled) ? (128 + ((*ang) >> 1)) : *mag;
            }
            else
                *dest = 0;
        }
    }
    
    peaks_.setBorder(halfK.x());
    return numPeaks;
}


void Gradient(const roiWindow<P8U> & image, roiWindow<P8U> & magnitudes, roiWindow<P8U> & angles)
{
    iPair kernel_3(3, 3);
    iPair halfK = kernel_3 / 2;
    sobelEdgeProcess_b(image, magnitudes, angles);
    // Clear the half kernel at all sides
    magnitudes.setBorder(halfK.x());
    angles.setBorder(halfK.x());
}


bool GetMotionCenter(const roiWindow<P8U> & peaks, const roiWindow<P8U> & ang, fPair & center)
{
    MotionCenter mc;
    int width = peaks.width();
    int height = peaks.height();
    float x = peaks.bound().ul().first;
    float y = peaks.bound().ul().second;
    
    for (float j = 0; j < height; j++)
    {
        uint8_t * pptr = peaks.rowPointer(j);
        uint8_t * aptr = ang.rowPointer(j);
        for (float i = 0; i < width; i++)
        {
            uint8_t p = *pptr++;
            uint8_t a = (*aptr++ + 64) % 256;
            if (p)
            {
                uAngle8 a8(a);
                fPair uv, pos;
                uv.first = static_cast<float>(sin(a8));
                uv.second = static_cast<float>(cos(a8));
                pos.first = x + i;
                pos.second = y + j;
                mc.add(pos, uv);
            }
        }
    }
    return mc.center(center);
}

bool GetMotionCenter(const roiWindow<P8U> & image, fPair & center, uint8_t threshold)
{
    MotionCenter mc;
    roiWindow<P8U> peaks (image.width(), image.height());
    roiWindow<P8U> ang (image.width(), image.height());
    roiWindow<P8U> mag (image.width(), image.height());
    Gradient(image, mag, ang);
    SpatialEdge(mag, ang, peaks, threshold);
    return GetMotionCenter(peaks, ang, center);
}

bool GetMotionCenter(const std::vector<svl::feature>& features, fPair & center){
    
    MotionCenter mc;
    for (const auto feature : features){
        fVector_2d uv = feature.uv();
        const fVector_2d& pos = feature.position();
        mc.add(pos, uv);
    }
    return mc.center(center);
}


void hystheresisThreshold::U8 (const roiWindow<P8U> & magImage,
                               roiWindow<P8U> & dst,
                               uint8_t magLowThreshold, uint8_t magHighThreshold,
                               int32_t & nSurvivors, int32_t nPels)
{
    const uint8_t noEdgeLabel = 0;
    const uint8_t edgeLabel = 128;
    uint8_t oscEdgeLabel = 255;
    int width = magImage.width();
    int height = magImage.height();
    
    if (!dst.isBound())
    {
        roiWindow<P8U> dstroot(width + 2, height + 2);
        dstroot.setBorder(1, noEdgeLabel);
        dst = roiWindow<P8U>(dstroot, 1, 1, width, height);
    }
    
    dst.set(noEdgeLabel);
    roiWindow<P8U> work(width + 2, height + 2);
    
    
    // Create the identity map for this set of thresholds
    vector<uint8_t> map(256);
    int32_t i(0);
    for (; i < magLowThreshold; i++)
        map[i] = noEdgeLabel;
    for (i = 0; i < (magHighThreshold - magLowThreshold); i++)
        map[magLowThreshold + i] = oscEdgeLabel;
    for (i = 0; i < (256 - magHighThreshold); i++)
        map[magHighThreshold + i] = edgeLabel;
    
    uint8_t * mag = magImage.pelPointer(0, 0);
    uint8_t * wptr = work.pelPointer(1, 1);
    
    pixelMap<uint8_t, uint8_t> convert(mag, wptr, magImage.rowUpdate(), work.rowUpdate(), width, height, map);
    convert.areaFunc();
    work.setBorder(1, noEdgeLabel);
    
    
    const int32_t maxPeaks(nPels <= 0 ? width * height : nPels);
    
    vector<hystheresisThreshold::hystNode> stack(maxPeaks);
    
    int32_t nPeaks(0);
    i = 0;
    const int32_t workUpdate(work.rowUpdate());
    const int32_t dstUpdate(dst.rowUpdate());
    
    // Note:
    // -- and ++ are push and pop operations for the stack
    for (int32_t y = 0; y < height; y++)
    {
        uint8_t * p = work.pelPointer(1, y);
        uint8_t * dest = dst.pelPointer(1, y);
        
        for (int32_t x = width; x--; p++, dest++)
        {
            if (*p == edgeLabel)
            {
                stack[i].workAddr = p;
                stack[(i++)].destAddr = dest;
                
                while (i)
                {
                    uint8_t * pp = stack[(--i)].workAddr;
                    uint8_t * dd = stack[i].destAddr;
                    
                    // If it is not marked, mark it an edge in dest
                    // and mark it a no edge in mag
                    if (!*dd)
                    {
                        *dd = edgeLabel;
                        nPeaks++;
                    }
                    *pp = noEdgeLabel;
                    
                    // If any of our neighbors is a possible edge push their address in
                    
                    if (*(pp - workUpdate - 1) == oscEdgeLabel)
                    {
                        stack[i].workAddr = pp - workUpdate - 1;
                        stack[(i++)].destAddr = dd - dstUpdate - 1;
                    }
                    if (*(pp - workUpdate) == oscEdgeLabel)
                    {
                        stack[i].workAddr = pp - workUpdate;
                        stack[(i++)].destAddr = dd - dstUpdate;
                    }
                    if (*(pp - workUpdate + 1) == oscEdgeLabel)
                    {
                        stack[i].workAddr = pp - workUpdate + 1;
                        stack[(i++)].destAddr = dd - dstUpdate + 1;
                    }
                    if (*(pp + 1) == oscEdgeLabel)
                    {
                        stack[i].workAddr = pp + 1;
                        stack[(i++)].destAddr = dd + 1;
                    }
                    if (*(pp + workUpdate + 1) == oscEdgeLabel)
                    {
                        stack[i].workAddr = pp + workUpdate + 1;
                        stack[(i++)].destAddr = dd + dstUpdate + 1;
                    }
                    if (*(pp + workUpdate) == oscEdgeLabel)
                    {
                        stack[i].workAddr = pp + workUpdate;
                        stack[(i++)].destAddr = dd + dstUpdate;
                    }
                    if (*(pp + workUpdate - 1) == oscEdgeLabel)
                    {
                        stack[i].workAddr = pp + workUpdate - 1;
                        stack[(i++)].destAddr = dd + dstUpdate - 1;
                    }
                    if (*(pp - 1) == oscEdgeLabel)
                    {
                        stack[i].workAddr = pp - 1;
                        stack[(i++)].destAddr = dd - 1;
                    }
                }
            }
        }
    }
    
    nSurvivors = nPeaks;
    return;
}

void DirectionSignal (const roiWindow<P8U>& ang, const roiWindow<P8U>& mask, vector<double>& signal)
{
    
    
    static const double zd (0.0);
    vector<double> histogram (1 << ang.bits(), zd);
    
    uint32_t lastRow = ang.height() - 1, row = 0;
    const uint32_t opsPerLoop = 8;
    uint32_t unrollCnt = ang.width() / opsPerLoop;
    uint32_t unrollRem = ang.width() % opsPerLoop;
    
    for ( ; row <= lastRow; row++)
    {
        const uint8_t* pixelPtr = ang.rowPointer(row);
        const uint8_t* maskPtr = mask.rowPointer(row);
        
        for (uint32_t touchCount = 0; touchCount < unrollCnt; touchCount++)
        {
            histogram[*pixelPtr++]+= (*maskPtr++) ? 1 : 0;
            histogram[*pixelPtr++]+= (*maskPtr++) ? 1 : 0;
            histogram[*pixelPtr++]+= (*maskPtr++) ? 1 : 0;
            histogram[*pixelPtr++]+= (*maskPtr++) ? 1 : 0;
            histogram[*pixelPtr++]+= (*maskPtr++) ? 1 : 0;
            histogram[*pixelPtr++]+= (*maskPtr++) ? 1 : 0;
            histogram[*pixelPtr++]+= (*maskPtr++) ? 1 : 0;
            histogram[*pixelPtr++]+= (*maskPtr++) ? 1 : 0;
        }
        
        for (uint32_t touchCount = 0; touchCount < unrollRem; touchCount++)
            histogram[*pixelPtr++]+= (*maskPtr++) ? 1 : 0;
    }
    
    signal.clear ();
    swap (signal, histogram);
}

