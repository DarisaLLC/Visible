/*
 *  lif.h
 *
 *  Created by Arman Garakani on 9/3/16.
 *  Copyright © 2016 Arman Garakani. All rights reserved.
 *
 */

#ifndef lif_
#define lif_

#include "cinder/ImageIo.h"


namespace lif
{
    enum ColorType {
        InvalidColorType = -1,
        Monochrome,
        RGB,
        ARGB,
        Indexed
    };
    
 
    
    enum Compression {
        RAW,
        JPEG,
        LZW,
        JPEG2000_LOSSLESS,
        JPEG2000_LOSSY
    };
    
    enum Interpolation {
        NearestNeighbor,
        Linear
    };
    
    struct lif_serie
    {
        lif_serie ()
        {
            x = y = c = t = z = 0;
        }
        
        uint32_t x;
        uint32_t y;
        uint32_t c;
        uint32_t z;
        uint32_t t;
    };
}

#endif