#include "vision/roiWindow.h"

 

//////////// Min && Max Pixel Routines /////////////
//////// (core of morphology) //////////////

template<class P>
void PixelMin3by3(const roiWindow<P>& srcImg, roiWindow<P>& dstImg)
{
    typedef typename PixelType<P>::pixel_ptr_t pixel_ptr_t;
    typedef typename PixelType<P>::pixel_t pixel_t;

    pixel_ptr_t  srcPtr1, srcPtr2, srcRowBase;
    pixel_ptr_t  dstPtr, dstRowBase;
    pixel_t    currMinVal, pelVal;

    int       i, j, k, l;
    int       rowIter, colIter, srcRUC, dstRUC;

    colIter = srcImg.width() - 2;
    rowIter = srcImg.height() - 2;
    srcRUC = srcImg.rowUpdate();
    dstRUC = dstImg.rowUpdate();


    srcRowBase = srcImg.rowPointer(0);
    dstRowBase = dstImg.pelPointer(1, 1);

    for (i = rowIter;
         i; i--, dstRowBase += dstRUC, srcRowBase += srcRUC)
    {
        dstPtr = dstRowBase;
        srcPtr1 = srcRowBase;
        for (j = colIter; j; j--, dstPtr++, srcPtr1++)
        {
            srcPtr2 = srcPtr1;
            currMinVal = 255;
            for (k = 3; k; k--)
            {
                for (l = 3; l; l--, srcPtr2++)
                {
                    pelVal = *srcPtr2;
                    currMinVal = (pelVal < currMinVal) ? pelVal : currMinVal;
                }
                srcPtr2 += srcRUC - 3;
            }
            *dstPtr = (pixel_t)currMinVal;
        }
    }

    dstImg.setBorder(1);
}




template<class P>
void PixelMax3by3(const roiWindow<P>& srcImg, roiWindow<P>& dstImg)
{
    typedef typename PixelType<P>::pixel_ptr_t pixel_ptr_t;
    typedef typename PixelType<P>::pixel_t pixel_t;

    pixel_ptr_t  srcPtr1, srcPtr2, srcRowBase;
    pixel_ptr_t  dstPtr, dstRowBase;
    pixel_t    currMaxVal, pelVal;

    int       i, j, k, l;
    int       rowIter, colIter, srcRUC, dstRUC;

    colIter = srcImg.width() - 2;
    rowIter = srcImg.height() - 2;
    srcRUC = srcImg.rowUpdate();
    dstRUC = dstImg.rowUpdate();

    srcRowBase = srcImg.rowPointer(0);
    dstRowBase = dstImg.pelPointer(1, 1);

    for (i = rowIter; i;
         i--, dstRowBase += dstRUC, srcRowBase += srcRUC)
    {
        dstPtr = dstRowBase;
        srcPtr1 = srcRowBase;
        for (j = colIter; j; j--, dstPtr++, srcPtr1++)
        {
            srcPtr2 = srcPtr1;
            currMaxVal = 0;
            for (k = 3; k; k--)
            {
                for (l = 3; l; l--, srcPtr2++)
                {
                    pelVal = *srcPtr2;
                    currMaxVal = (pelVal > currMaxVal) ? pelVal : currMaxVal;
                }
                srcPtr2 += srcRUC - 3;
            }
            *dstPtr = currMaxVal;
        }
    }
    dstImg.setBorder(1);
}


