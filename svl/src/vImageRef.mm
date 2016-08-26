
#include <stdio.h>
#include "vImageRef.h"
#include "GLMmath.h"
#include <assert.h>

using namespace svl;

static vImage_CGImageFormat m_BGRA_format = {8, 32, NULL, (CGBitmapInfo)kCGImageAlphaLast, 0, NULL, kCGRenderingIntentDefault};



vImage_BufferRef CreatevImage_BufferRef (uint32_t pixel_width, uint32_t pixel_height, uint32_t bytes_per_pixel)
{
    vImage_BufferRef ret;
    uint32_t mem_size = pixel_height * pixel_width * bytes_per_pixel;
    
    if (mem_size == 0) return ret;
    
    assert(bytes_per_pixel <= 256); // prevent the insane or
    
    vImage_BufferRef dst (new vImage_Buffer, vImageDeleter ());
    dst->width = pixel_width;
    dst->height = pixel_height;
    dst->rowBytes = pixel_width * bytes_per_pixel;
    dst->data = malloc(mem_size);
    
    return dst;
}

/*
 * kvImageDoNotAllocate        Under normal operation, new memory is allocated to hold the image pixels and its address is written

 */

vImage_BufferRef vImage_BufferRefFromCVBuffer (CVPixelBufferRef cvPixelBuffer,
                                               vImage_CGImageFormat* desired,
                                               BOOL DoNotAllocateNewMemory)
{
    static CGFloat fl = 0.5;
    vImage_Error error = kvImageNoError;
    vImage_CGImageFormat* local = desired ? desired : & m_BGRA_format;
    
    vImage_BufferRef dst (new vImage_Buffer, vImageDeleter ());
    if (kvImageNoError == (error = vImageBuffer_InitWithCVPixelBuffer(dst.get(), local, cvPixelBuffer, NULL, &fl, DoNotAllocateNewMemory ? kvImageNoAllocate : kvImageNoFlags)))
        return dst;
    
    vImage_BufferRef ret;
    return ret;
}

