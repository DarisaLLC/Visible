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
    
    enum DataType {
        InvalidDataType = ci::ImageIo::DataType::DATA_UNKNOWN,
        UChar = ci::ImageIo::DataType::UINT8,
        UInt16 = ci::ImageIo::DataType::UINT16,
        Float = ci::ImageIo::DataType::FLOAT32
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