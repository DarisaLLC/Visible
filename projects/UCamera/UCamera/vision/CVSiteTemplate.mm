//
//  CVWrapper.m
//  CVOpenTemplate
//
//  Created by Washe on 02/01/2013.
//  Copyright (c) 2013 foundry. All rights reserved.
//

#import "CVSiteTemplate.h"
#import "UIImage+cvMat.h"
#include "siteTemplate.h"
#include "siteCommon.h"
#import "UIImage+Rotate.h"
#import "vision/localvariance.h"
#import "vision/gradient.h"
#import "vision/opencv_utils.hpp"


@interface CVSiteTemplate()
@property siteTemplate *cppItem;
@end

@implementation CVSiteTemplate


- (instancetype)initWithTitle:(NSString*)title
{
    if (self = [super init]) {
        self.cppItem = siteTemplate::instance();
        self.cppItem->setTitle(std::string([title cStringUsingEncoding:NSUTF8StringEncoding]));        
    }
    
    return self;
}
- (NSString*)getTitle
{
    return [NSString stringWithUTF8String:self.cppItem->getTitle().c_str()];
}
- (void)setTitle:(NSString*)title
{
    self.cppItem->setTitle(std::string([title cStringUsingEncoding:NSUTF8StringEncoding]));
}

- (float)getLensPosition
{
    return self.cppItem->getLensPosition();
}

- (void)unsetLensPosition
{
    return self.cppItem->unsetLensPosition();
}

- (void)setLensPosition:(float) lp
{
    return self.cppItem->setLensPosition(lp);
}

- (void) reset: (float) width : (float) height

{
    self.cppItem->resetImageSize(width,height);
}

// x, y, w, h in normalized (0 - 1)
// Trains initial template
- (bool)setNormalizedTrainingRect:(float) x : (float) y : (float) width : (float) height
{
    if (self.cppItem->setNormalizedTrainingRect(x, y, width, height))
    {
        self.cppItem->start(siteTemplate::match::cbp);
        return true;
    }
    return false;
}

- (colorStats_t)getColorStats:(UIImage*) currentImage : (CGRect*) roi : (CMTime*) currentTimestamp
{
     colorStats_t ct;
    ct.n = 0;
    // Crop without orientation fix
    UIImage* cropped = [UIImage crop:currentImage :roi];
    if (cropped == nil)
        return ct;
    
    cv::Mat matImage = [CVSiteTemplate rotateCorrect: cropped];
    double ts = CMTimeGetSeconds(*currentTimestamp);
    if (self.cppItem->colorStatistics(matImage, ct))
    {
        ct.time = ts;
        return ct;
    }
    return ct;
    
    
}

    
- (bool)train:(UIImage*) currentImage : (CMTime*) currentTimestamp
{
    cv::Mat matImage = [CVSiteTemplate rotateCorrect: currentImage];
    double ts = CMTimeGetSeconds(*currentTimestamp);
    uint64_t count = self.cppItem->count() + 1;
    timedMat_t timed = std::make_tuple(count, ts, matImage );
    self.cppItem->trainCbp(timed);
    return true;
}

- (CGPoint) step:(UIImage*) currentImage : (CMTime*) currentTimestamp
{
    cv::Mat matImage = [CVSiteTemplate rotateCorrect: currentImage];
    double ts = CMTimeGetSeconds(*currentTimestamp);
    uint64_t count = self.cppItem->count() + 1;
    timedMat_t timed = std::make_tuple(count, ts, matImage );
    
    auto tr = self.cppItem->step(timed);
    CGPoint gp = CGPointZero;
    gp.x = gp.y = -1.0;
    if (std::get<0>(tr))
    {
        fVector_2d pos = std::get<2>(tr);
        gp.x = pos.x();
        gp.y = pos.y();
        std::cout << pos << std::endl;
    }
    return gp;
    
}

- (bool) stop
{
    return self.cppItem->stop();
}

- (bool) isRunning
{
    return self.cppItem->isRunning();
}
- (bool) hasStopped
{
    return self.cppItem->hasStopped();
}
- (bool) hasStarted
{
    return self.cppItem->hasStarted();
}
- (bool) isUninitialized
{
    return self.cppItem->isUninitialized();
}

//
//
//- (bool) start
//{
//    
//}
//

//- (bool) stop
//{
//    
//}
//

- (int32_t)getWidth
{
    return self.cppItem->getWidth();
}
- (int32_t)getHeight
{
    return self.cppItem->getHeight();
}


+ (CGRect) cgRectFromCVRect:(cv::Rect) cvRect{
    return CGRectMake(cvRect.x, cvRect.y, cvRect.width, cvRect.height);
}

+ (cv::Mat) rotateCorrect:(UIImage*) inputImage
{
    cv::Mat matImage;
    
    if ([inputImage isKindOfClass: [UIImage class]])
    {
        /*
         All images taken with the iPhone/iPa cameras are LANDSCAPE LEFT orientation. The  UIImage imageOrientation flag is an instruction to the OS to transform the image during display only. When we feed images into openCV, they need to be the actual orientation that we expect. So we rotate the actual pixel matrix here if required.
         */
        UIImage* rotatedImage = [inputImage rotateToImageOrientation];
        matImage = [rotatedImage cvMat];
    }
    
    return matImage;
}

@end
