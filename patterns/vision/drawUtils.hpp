#ifndef __DRAWUTIL__
#define __DRAWUTIL__

#include "roiWindow.h"

template <class T>
inline void Identity_2x2(T Y[4])
{
    Y[0] = 1;
    Y[1] = 0;
    Y[2] = 0;
    Y[3] = 1;
}

template <class T>
inline void Identity_3x3(T Y[9])
{
    Y[0] = 1;
    Y[1] = 0;
    Y[2] = 0;
    Y[3] = 0;
    Y[4] = 1;
    Y[5] = 0;
    Y[6] = 0;
    Y[7] = 0;
    Y[8] = 1;
}

template <class T>
inline void Identity_4x4(T Y[9])
{
    Y[0] = 1;
    Y[1] = 0;
    Y[2] = 0;
    Y[3] = 0;
    Y[4] = 0;
    Y[5] = 1;
    Y[6] = 0;
    Y[7] = 0;
    Y[8] = 0;
    Y[9] = 0;
    Y[10] = 1;
    Y[11] = 0;
    Y[12] = 0;
    Y[13] = 0;
    Y[14] = 0;
    Y[15] = 1;
}

/*
 * Utility function to draw a shape into a pelbuffer
 * optional scaling
 */
static void DrawShape(roiWindow<P8U>& image, char **shape, int32_t x = 0, int32_t y = 0, int32_t scale = 1)
{
    char **s;
    int32_t width = x;
    int32_t height = y;
    for (s = shape; *s; s++, height += scale)
    {
        int32_t this_width = static_cast<int32_t>(strlen(*s))*scale + x;
        if (this_width > width)
            width = this_width;
    }

    roiWindow<P8U> src(width, height);
    src.set(0);

    for (; *shape; shape++)
    {
        for (char *valPtr = *shape; *valPtr; valPtr++)
        {
            for (int32_t xx = 0; xx < scale; xx++)
            {
                for (int32_t yy = 0; yy < scale; yy++)
                {
                    int rr = y + yy;
                    int cc = static_cast<int>(x + (valPtr - *shape)*scale);
                    if (cc < 0 || cc >(width - 1) || rr < 0 || rr >(height - 1)) continue;

                    uint8_t* pel = src.pelPointer( cc, rr);
                    *pel = (*valPtr - '0');
                }
            }
        }
        y += scale;
    }

    image = src;
}


#endif
