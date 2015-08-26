
#ifndef _SIMPLEIPL_H_
#define _SIMPLEIPL_H_

// Assumes you have OpenCv includes and lib in the right places.

#include <memory>
#include <iostream>

#if defined(HAVE_OPENCV)

#include <opencv2/opencv.hpp>

struct IPlnoreleaser
{
	void operator()(IplImage * image) const
	{
	}
};

// Not to be used for IplImage* returned from cvQueryFrame
class IplReleaser
{
public:
    void operator() (IplImage* image)
        {
            if (image != 0)
                cvReleaseImage (&image);
        }
};

typedef std::shared_ptr<IplImage> sharedIpl;


// Series of macros to bit stream line image attribute access
// The pel pointer access macros may not work for interleaved image formats
//

#define sIPLpointer(s) (s).get()
#define sIPLwidth(s) (s).get()->width
#define sIPLheight(s) (s).get()->height
/* Pixel depth in bits: IPL_DEPTH_8U, IPL_DEPTH_8S, IPL_DEPTH_16S, IPL_DEPTH_32S, IPL_DEPTH_32F and IPL_DEPTH_64F are supported.  */
#define sIPLdepth(s) (s).get()->depth   
#define sIPLROIptr(s) (s).get()->roi
#define sIPLROIwidth(s) ((s).get()->roi!=0) ? (s).get()->roi->width : sIPLwidth ((s))
#define sIPLROIheight(s) ((s).get()->roi!=0) ? (s).get()->roi->height : sIPLheight ((s))
#define sIPLROI_x(s) ((s).get()->roi ? (s).get()->roi->xOffset : 0)
#define sIPLROI_y(s) ((s).get()->roi ? (s).get()->roi->yOffset : 0)
#define sIPLequalInSize(s,t) (sIPLwidth(s) == sIPLwidth(t) && sIPLheight(s) == sIPLheight(t))
#define sIPLsizePair(s) bfPair_I (sIPLROIwidth((s)), sIPLROIheight((s)))

#define sIPLid(s) (s).get()->ID
#define sIPLnChannels(s) (s).get()->nChannels
#define sIPLrowBytes(s) (s).get()->widthStep
#define sIPLdataOrder(s) (s).get()->dataOrder
#define sIPLimageData(s) (s).get()->imageData
#define sIPLrowPointer(s, r)  (sIPLpointer((s))->imageData + sIPLrowBytes((s))*(r))
#define sIPLpelPointer(s,z,c,r)   (&((z*) (sIPLrowPointer((s),(r))))[(c)])
#define sIPLpelGet(s,z,c,r)   sIPLpelPointer((s),(z),(c),(r))[0]

#define sIPLsetROI(s,x,y,w,h) cvSetImageROI((s).get(), cvRect(x,y,w,h))

#define sIPLcreate(name,ww,hh,tt,cc) sharedIpl name (cvCreateImage(cvSize ((ww), (hh)), (tt), (cc)), IplReleaser ())


struct StaticIPl
{
	void operator()(IplImage * image) const
	{
	}
};

template<typename T, int I>
sharedIpl makeIplCopy(const T* images, int width, int height)
{
    sIPLcreate(ret, width, height, I, 1);
    T* iplpels = (T*) sIPLimageData(ret);
    const T* ipels = images;
    int ipl_rowbytes = sIPLrowBytes(ret) / sizeof(T);

    for (int hh = 0; hh < height; hh++)
    {
        std::memcpy(iplpels, ipels, sizeof(T) * width);
        iplpels += ipl_rowbytes;
        ipels += width;
    }

    return ret;
}


#endif

#endif
