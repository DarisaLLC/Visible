//
//  svTest.h.h
//  svl
//
//  Created by Arman Garakani on 5/23/16.
//  Copyright © 2016 Arman Garakani. All rights reserved.
//

#ifndef svTest_h_h
#define svTest_h_h

#include "vision/histo.h"
#include "vision/drawUtils.hpp"
#include "vision/gradient.h"
#include "vision/ipUtils.h"
#include "vision/edgel.h"
#include "core/pair.hpp"
#include "vision/roiWindow.h"
#include "vision/rowfunc.h"
#include "vision/gauss.hpp"
#include "vision/gmorph.hpp"
#include "vision/sample.hpp"
#include "core/stl_utils.hpp"
#include "vision/labelconnect.hpp"
#include "vision/registration.h"
#include "cinder/cinder_xchg.hpp"

#define EXPECT_EQ(expected, actual) assert((expected) == (actual))
#define EXPECT_NE(expected, actual) assert((expected) != (actual))

class UT_similarity
{
public:
    
    template<typename P>
    void testCorrelation (int32_t width, int32_t height, bool useAltivec )
    {
        
        sharedRoot<P> ptr (new root<P8U>(width, height));
        sharedRoot<P> ptr2 (new root<P8U>( width, height));
        
        roiWindow<P> imgA (ptr, 0, 0, width, height);
        roiWindow<P> imgB (ptr2, 0, 0, width, height);
        
        uint32_t val = 255;
        
        // Set all pels
        imgA.set (val);
        imgB.set (val);
        
        // Set a single Pel
        ptr->setPixel (imgA.width() / 2, imgA.height() / 2, val-1);
        ptr2->setPixel (imgB.width() / 2, imgB.height() / 2, val-1);
        
        double wh = imgA.width() * imgA.height();
        
        //  if (_maskValid)
        //   rfCorrelate(i, m, _mask, _corrParams, res, _maskN);
        CorrelationParts res, res2;
        EXPECT_EQ( res , res2 );
        CorrelationParts::sumproduct_t jr = 666;
        res2.Sim(jr);
        EXPECT_NE(res, res2);
        res2 = res;
        
        Correlation::point(imgA, imgB, res);
        
        
        CorrelationParts::sumproduct_t trueSi = wh * 255 - 1;
        CorrelationParts::sumproduct_t trueSm = wh * 255 - 1;
        CorrelationParts::sumproduct_t trueSii = ((wh - 1) * 255 * 255) +  (255 - 1) * (255 - 1);
        
        CorrelationParts::sumproduct_t  trueSmm = trueSii;
        CorrelationParts::sumproduct_t trueSim = trueSii;
        
        EXPECT_EQ (1.0, res.r() );
        EXPECT_EQ ( int32_t(imgA.width() * imgB.height()), res.n());
        EXPECT_EQ (trueSi, res.Si());
        EXPECT_EQ (trueSm, res.Sm());
        EXPECT_EQ (trueSii, res.Sii() );
        EXPECT_EQ (trueSmm, res.Smm() );
        EXPECT_EQ (trueSim, res.Sim() );
        
    }
};

#endif /* svTest_h_h */
