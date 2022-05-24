//
//  vImageRef.h
//  svl
//
//  Created by Arman Garakani on 6/6/16.
//  Copyright © 2016 Arman Garakani. All rights reserved.
//

#ifndef vImageRef_h
#define vImageRef_h

#include <vector>
#include <Accelerate/Accelerate.h>
#include <iostream>
#include <memory>

#include "singleton.hpp"
#include "shared_queue.hpp"

namespace svl

{
typedef std::shared_ptr<vImage_Buffer> vImage_BufferRef;

struct vImageDeleter
{
    void operator()(vImage_Buffer* vi)
    {
        if (vi && vi->data) { free (vi->data); vi->data = NULL; }
        if (vi) delete vi;
    }
};

vImage_BufferRef CreatevImage_BufferRef (uint32_t pixel_width, uint32_t pixel_height, uint32_t bytes_per_pixel);

    /*
     * If desired is NULL, BGRA 8888 is used.
     */
    
vImage_BufferRef vImage_BufferRefFromCVBuffer (CVPixelBufferRef cvPixelBuffer,
                                               vImage_CGImageFormat* desired = NULL,
                                               BOOL DoNotAllocateNewMemory = false);

class vImageQueue : public internal_singleton<vImageQueue>, public shared_queue<vImage_BufferRef>
{
public:
    
    vImageQueue () {}
    
private:
    
};

    
}
#endif /* vImageRef_h */
