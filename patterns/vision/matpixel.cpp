


#include <iostream>
#include <memory>
#include <vector>
#include "pixel_traits.h"
#include "roiRoot.h"
#include "roiWindow.h"
#include <assert.h>


using namespace DS;


template <typename P>
void root<P>::set(pixel_t val)
{
    std::memset((void *)this->alignedPixelData(), val, this->n());
}


template <typename P>
void roiWindow<P>::set(pixel_t val)
{
    for (int32_t j = 0; j < height(); j++)
    {
        pixel_ptr_t row = rowPointer(j);
        std::memset(row, val, width());
    }
}


template <typename P>
void roiWindow<P>::setBorder(int pad, pixel_t val)
{
    const int width(this->width());
    const int height(this->height());
    {
        roiWindow<P> win(*this, 0, 0, width, pad);
        win.set(val);
    }
    {
        roiWindow<P> win(*this, 0, height - pad, width, pad);
        win.set(val);
    }
    {
        roiWindow<P> win(*this, 0, 0, pad, height);
        win.set(val);
    }
    {
        roiWindow<P> win(*this, width - pad, 0, pad, height);
        win.set(val);
    }
}


template <typename P>
roiWindow<P>::roiWindow(const roiWindow<P> & parentWindow, int32_t tl_x, int32_t tl_y, int32_t width, int32_t height)
{
    assert(!(tl_x < 0 || tl_y < 0 || !parentWindow.isBound()));

    int32_t x0 = tl_x + parentWindow.x();
    int32_t y0 = tl_y + parentWindow.y();
    // rect in root buffer of parent
    iRect geom = iRect(x0, y0, width, height);

    // Clip or no clip, our bounds will be this root or a throw
    m_bounds = parentWindow.m_bounds;
    if (parentWindow.bound().contains(geom))
    {
        *this = roiWindow(parentWindow.frameBuf(), x0, y0, width, height);
    }
}


template <typename P>
roiWindow<P> roiWindow<P>::window(int32_t tl_x, int32_t tl_y, int32_t width, int32_t height)
{
    if (!(tl_x < 0 || tl_y < 0 || !isBound()))
    {

        int32_t x0 = tl_x + x();
        int32_t y0 = tl_y + y();
        // rect in root buffer of parent
        iRect geom = iRect(x0, y0, width, height);


        if (frame().contains(geom))
        {
            return roiWindow<P>(frameBuf(), x0, y0, width, height);
        }
    }

    return roiWindow<P>();
}

// Assignment operators
template <typename T>
sharedRoot<T> & sharedRoot<T>::operator=(const sharedRoot<T> & p)
{
    if (mFrameBuf == p)
        return *this;

    mFrameBuf = p.mFrameBuf;
    if (mFrameBuf)
    {
        mFrameBuf->addRef();
    }
    return *this;
}

template <typename T>
sharedRoot<T> & sharedRoot<T>::operator=(root<T> * p)
{
    if (mFrameBuf == p)
        return *this;

    mFrameBuf = p;
    if (mFrameBuf)
    {
        mFrameBuf->addRef();
    }

    return *this;
}


template <typename T>
sharedRoot<T>::~sharedRoot()
{
    if (mFrameBuf)
        mFrameBuf->remRef();
}

namespace DS
{
template class root<P8U>;
template class sharedRoot<P8U>;

template class root<P16U>;
template class sharedRoot<P16U>;

template class root<P32F>;
template class sharedRoot<P32F>;

template class roiWindow<P8U>;
template class roiWindow<P16U>;
template class roiWindow<P32F>;
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
