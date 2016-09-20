/*
 *  lif.h
 *
 *  Created by Arman Garakani on 9/3/16.
 *  Copyright © 2016 Arman Garakani. All rights reserved.
 *
 */

#ifndef lif_
#define lif_

namespace lif
{
    enum ColorType {
        InvalidColorType = -1,
        Monochrome,
        RGB,
        ARGB,
        Indexed
    };
    
    enum DataType {
        InvalidDataType = -1,
        UChar,
        UInt16,
        UInt32,
        Float
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
}

#endif