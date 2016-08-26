#include "vision/roiWindow.h"



/////////////// Gaussian Convolution  ///////////////////////////////

//
// The client passes in a bounded roiWindow of the same sizs as source.
// Upon return dest is a window in to the image client passed with the
// correct size

/*
	* Run Gauss around the edges
	*/

template <class P>
void gaussEdge(const roiWindow<P> & src, roiWindow<P> & dest)
{
    typedef typename PixelType<P>::pixel_ptr_t pixel_ptr_t;
    typedef typename PixelType<P>::pixel_t pixel_t;

    const uint32_t srcWidth(src.width());
    const uint32_t srcHeight(src.height());
    const uint32_t srcUpdate(src.rowPixelUpdate());
    const uint32_t destUpdate(dest.rowPixelUpdate());
    pixel_ptr_t  src0 = src.rowPointer(0);
    pixel_ptr_t  dest0 = dest.rowPointer(0);

    // TL and TR pixels
    *dest0 = *src0;
    dest0[srcWidth - 1] = src0[srcWidth - 1];
    ++dest0;

    // Top
    uint32_t left(uint32_t(3 * *src0 + *(src0 + srcUpdate)));
    ++src0;
    uint32_t centre(uint32_t(3 * *src0 + *(src0 + srcUpdate)));
    ++src0;
    uint32_t width;
    for (width = srcWidth - 2; width--;)
    {
        uint32_t right(uint32_t(3 * *src0 + *(src0 + srcUpdate)));
        *dest0++ = pixel_t ((left + centre + centre + right + 8) / 16);
        left = centre;
        centre = right;
        ++src0;
    }

    // BL and BR pixels
    dest0 = dest.rowPointer(srcHeight - 1);
    src0 = src.rowPointer(srcHeight - 1);
    *dest0 = *src0;
    dest0[srcWidth - 1] = src0[srcWidth - 1];
    ++dest0;

    // Bottom
    left = uint32_t(3 * *src0 + *(src0 - srcUpdate));
    ++src0;
    centre = uint32_t(3 * *src0 + *(src0 - srcUpdate));
    ++src0;
    for (width = srcWidth - 2; width--;)
    {
        uint32_t right(uint32_t(3 * *src0 + *(src0 - srcUpdate)));

        *dest0++ = pixel_t ((left + centre + centre + right + 8) / 16);
        left = centre;
        centre = right;
        ++src0;
    }

    // Left
    src0 = src.rowPointer(0);
    dest0 = dest.rowPointer(1);

    uint32_t top(uint32_t(3 * *src0 + *(src0 + 1)));
    src0 += srcUpdate;
    uint32_t middle(uint32_t(3 * *src0 + *(src0 + 1)));
    src0 += srcUpdate;

    uint32_t height;
    for (height = srcHeight - 2; height--;)
    {
        uint32_t bottom(uint32_t(3 * *src0 + *(src0 + 1)));
        *dest0 = pixel_t ((top + middle + middle + bottom + 8) / 16);
        src0 += srcUpdate;
        dest0 += destUpdate;
        top = middle;
        middle = bottom;
    }

    // Right
    src0 = src.rowPointer(0) + srcWidth - 1;
    dest0 = dest.rowPointer(1) + srcWidth - 1;
    top = uint32_t(3 * *src0 + *(src0 - 1));
    src0 += srcUpdate;
    middle = uint32_t(3 * *src0 + *(src0 - 1));
    src0 += srcUpdate;

    for (height = srcHeight - 2; height--;)
    {
        uint32_t bottom(uint32_t(3 * *src0 + *(src0 - 1)));
        *dest0 = pixel_t ((top + middle + middle + bottom + 8) / 16);
        src0 += srcUpdate;
        dest0 += destUpdate;
        top = middle;
        middle = bottom;
    }
}


template <typename T, typename A = uint32_t>
inline void gaussCompute(T *& top,
                         T *& mid,
                         T *& bot,
                         T *& dst,
                         A & left, A & cent, A & right)
{
    A new_pel;
    right = A(*top++);
    T curr_pel = *mid++;
    right += A(curr_pel + curr_pel);
    right += A(*bot++);
    new_pel = left + cent + cent + right;
    *dst++ = (T)((new_pel + 8) >> 4);
}


template <typename P>
void Gauss3by3(const roiWindow<P> & src, roiWindow<P> & destination)
{
    typedef typename PixelType<P>::pixel_ptr_t pixel_ptr_t;
    typedef typename PixelType<P>::pixel_t pixel_t;

    roiWindow<P> dest = destination.window (1, 1, destination.width() - 2, destination.height() - 2);

    assert(src.width() == dest.width() + 2);
    assert(src.height() == dest.height() + 2);

    uint32_t ht(dest.height());
    uint32_t wd(dest.width());
    uint32_t x, y, sum1, sum2, sum3;
    uint32_t kernel_wds, k;
    uint32_t srcUpdate(src.rowPixelUpdate());
    uint32_t destUpdate(dest.rowPixelUpdate());

    pixel_ptr_t base_dst = dest.rowPointer(0);
    pixel_ptr_t dst;

    pixel_ptr_t top, mid, bot, save;
    pixel_t curr_pel;

    //  Initialize pointers to rows
    top = src.rowPointer(0);
    mid = top + srcUpdate;
    bot = mid + srcUpdate;
    save = mid;

    //  Compute number of kernel multiples contained in image width
    kernel_wds = wd / 3;
    k = wd % 3;

    for (y = ht; y; y--, base_dst += destUpdate)
    {
        //    SETUP FOR FIRST KERNEL POSITION:
        dst = base_dst;
        sum1 = uint32_t(*top++);
        curr_pel = *mid++;
        sum1 += uint32_t(curr_pel + curr_pel);
        sum1 += uint32_t(*bot++);

        sum2 = uint32_t(*top++);
        curr_pel = *mid++;
        sum2 += uint32_t(curr_pel + curr_pel);
        sum2 += uint32_t(*bot++);

        // Fill up the pipeline

        for (x = kernel_wds; x; x--)
        {
            gaussCompute(top, mid, bot, dst, sum1, sum2, sum3);

            gaussCompute(top, mid, bot, dst, sum2, sum3, sum1);

            gaussCompute(top, mid, bot, dst, sum3, sum1, sum2);
        }

        //    Compute any leftover pixels on the row
        for (x = k; x; x--)
        {
            gaussCompute(top, mid, bot, dst, sum1, sum2, sum3);
            sum1 = sum2;
            sum2 = sum3;
        }

        //    Update row pointers for next row iteration
        top = save;
        mid = save + srcUpdate;
        bot = mid + srcUpdate;
        save = mid;
    }

    // Run on the edges
    gaussEdge(src, destination);
}


