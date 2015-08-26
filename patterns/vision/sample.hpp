#include "vision/roiWindow.h"


/*
 * Pixel Expansion with "smart" copy support
 */

template<class P>
void PixelExpand(const roiWindow<P>& src, roiWindow<P>& dest,  int32_t xmag, int32_t ymag);

template<class P>
roiWindow<P> PixelExpand(const roiWindow<P>& src, int32_t xmag, int32_t ymag)
{
  roiWindow<P> dest;
  rfPixelExpand(src, dest, xmag, ymag);
  return dest;
}

template<class P>
void PixelExpand(const roiWindow<P>& src, roiWindow<P>& dest, int32_t xmag, int32_t ymag)
{
  assert(xmag >= 1);
  assert(ymag >= 1);

  typedef typename PixelType<P>::pixel_ptr_t pixel_ptr_t;
  typedef typename PixelType<P>::pixel_t pixel_t;

  if (!src.isBound())
    return;

  iRect dstgcr;
  dstgcr = iRect (iPair (0,0), iPair(src.width() * xmag, src.height() * ymag));

  if (!dest.isBound())
    {
      dest = roiWindow<P> (int32_t (src.width() * xmag),
		       int32_t (src.height() * ymag));
    }

  assert (static_cast<int32_t>(dest.width()) >= dstgcr.width());
  assert (static_cast<int32_t>(dest.height()) >= dstgcr.height());

  // Consider only whole blocks of src pixels
  int32_t srcWidth  = src.width();
  int32_t srcHeight = src.height();

  int32_t srcRowUpdate = src.rowUpdate();
  int32_t dstRowUpdate = dest.rowUpdate();
  int32_t dstMagRowUpdate = dstRowUpdate * ymag;

  // Setup source and destination pointers according to magnification
  const pixel_ptr_t srcPtr = src.pelPointer(0,0);
  pixel_ptr_t dstPtr = dest.pelPointer(0,0);

  for (int32_t y=0; y<srcHeight; y++)
    {
      pixel_ptr_t pDest = dstPtr;
      pixel_ptr_t pSrc = srcPtr;
      for (int32_t x=0; x<srcWidth; x++)
	{
	  // read the val to be expanded
	  pixel_t destVal = (pixel_t) *pSrc;

	  pixel_ptr_t pDestTemp = pDest;

	  // Write one block of xmag by ymag
	  for (int32_t ycopy = ymag; ycopy; --ycopy)
	    {
	      pixel_ptr_t pDestRow = pDestTemp;
	      for (int32_t xcopy = xmag; xcopy; --xcopy)
		{
		  //		  assert (dest.contains (pDestRow));
		  *pDestRow++ = destVal;
		}
	      // Next destination row
	      pDestTemp += dstRowUpdate;
	    }

	  pSrc++;
	  pDest += xmag;
	}

      // Source: next row, Destination: next ymag destination row
      srcPtr += srcRowUpdate;
      dstPtr += dstMagRowUpdate;
    }
}



/*
 * Pixel Sub Sampling
 *
 * Note that if skip is even then
 * center of pixel is shifted by -0.5;
 *
 */
template<class P>
void PixelSample(const roiWindow<P>& src, roiWindow<P>& dest,  int32_t xskip, int32_t yskip);

template<class P>
roiWindow<P> PixelSample(const roiWindow<P>& src, int32_t xskip, int32_t yskip)
{
  roiWindow<P> dest;
  PixelSample(src, dest, xskip, yskip);
  return dest;
}
template<class P>
void PixelSample(const roiWindow<P>& src, roiWindow<P>& dest, int32_t xskip, int32_t yskip)
{
  assert(xskip >= 1);
  assert(yskip >= 1);

  typedef typename PixelType<P>::pixel_ptr_t pixel_ptr_t;
  typedef typename PixelType<P>::pixel_t pixel_t;

  // See the note above for Expand
  int32_t ulx = 0; int32_t uly = 0;
  int32_t lrx = src.width() / xskip;  int32_t lry = src.height()/ yskip;

  if (ulx >= lrx || uly >= lry)
    return;   

  iRect redSrc (iPair(ulx, uly), iPair(lrx, lry));

  if (!dest.isBound())
    {
      dest = roiWindow<P> (redSrc.width(), redSrc.height());
    }

  // Holds coords of the pixel in source image coordinate frame where <0,0> in
  // destination image coordinate frame maps to.
  int32_t xOrigin = (xskip-1) / 2 ;
  int32_t yOrigin = (yskip-1) / 2;

  for (int32_t y = 0; y < dest.height(); y++)
    {
      pixel_ptr_t pDest = dest.pelPointer(0, y);
      pixel_ptr_t pSrc = src.pelPointer(xOrigin, y*yskip + yOrigin);
      int32_t x = dest.width();
      do
	{
	  *pDest++ = pixel_t (*pSrc);
	  pSrc += xskip;
	}
      while (--x);
    }
}


template<class P>
std::pair<roiWindow<P8U>, roiWindow<P8U> > GeneratePyramid(roiWindow<P>& bottom, iRect& roi)
{
    std::pair<roiWindow<P8U>, roiWindow<P8U> > midtop;
    if (bottom.isBound())
    {
        roiWindow<P8U> middle;
        roiWindow<P8U> top;
        {
            roiWindow<P8U> smooth(bottom.width(), bottom.height());
            roiWindow<P8U> soft(bottom.width(), bottom.height());
            Gauss3by3(bottom, soft);
            Gauss3by3(soft, smooth);
            PixelSample(smooth, middle, 2, 2);
        }
        if (middle.isBound())
        {
            roiWindow<P8U> sampled(middle.width(), middle.height());
            roiWindow<P8U> coarse(middle.width(), middle.height());
            Gauss3by3(middle, coarse);
            Gauss3by3(coarse, sampled);
            PixelSample(sampled, top, 2, 2);
        }
        midtop.first = middle;
        midtop.second = top;
    }
}