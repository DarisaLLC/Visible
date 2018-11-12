//
//  StructDefs.metal
//  Camera2
//
//  Created by Arman Garakani on 6/4/17.
//  Copyright © 2017 apple. All rights reserved.
//

#include <metal_stdlib>
using namespace metal;

constant float eps [[ function_constant(4) ]];
constant int minNeighbors [[ function_constant(5) ]];

// the current detection window, using only within the shader
struct DetectionWindow {
    uint x;
    uint y;
    uint width;
    uint height;
};
