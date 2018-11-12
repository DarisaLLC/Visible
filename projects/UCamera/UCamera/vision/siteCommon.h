//
//  siteCommon.h
//  Camera
//
//  Created by Arman Garakani on 5/12/17.
//

#ifndef siteCommon_h
#define siteCommon_h
#include <stddef.h>


struct colorStats {
    uint32_t n;
    float means [3];
    float stds [3];
    double time;
};

typedef struct colorStats colorStats_t;

struct site_eval
{
    float score;
    float x, y;
    uint32_t flags;
    bool valid;
    
};
typedef struct site_eval site_eval;


#endif
