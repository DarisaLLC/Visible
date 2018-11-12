//
//  CVWrapper.m
//  CVOpenTemplate
//
//  Created by Washe on 02/01/2013.
//  Copyright (c) 2013 foundry. All rights reserved.
//

#import "CVWrapper.h"
#import "UIImage+OpenCV.h"
//#import "UIImage+SVL.h"
#import "cv_functions.h"
#import "UIImage+Rotate.h"
#import "vision/localvariance.h"
#import "vision/gradient.h"
#import "vision/opencv_utils.hpp"

@implementation CVWrapper

+ (CGPoint) cvGetMotionCenter:(UIImage*) inputImage
{
    CGPoint gp = CGPointMake(0,0);
    cv::Mat matImage = [self rotateCorrect:inputImage];
    cv::Mat grey;
    cv::cvtColor(matImage, grey, cv::COLOR_BGR2GRAY);
    
    std::shared_ptr<roiWindow_P8U_t> roiImage = NewRefFromOCV(grey);
    fPair xyp;
    GetMotionCenter(*roiImage.get(), xyp, 5);
    gp.x = xyp.first; /// grey.cols;
    gp.y = xyp.second; // / grey.rows;
    return gp;
}


+ (UIImage*) cvProduceOutline:(UIImage*) inputImage
{
    std::vector<cv::Mat> objpyr;
    cv::Mat matImage = [self rotateCorrect:inputImage];
    // build pyramid
    cv::buildPyramid(matImage, objpyr, 3);
    cv::Mat grey;
    cv::Mat mask;
    cv::cvtColor(objpyr[2], grey, cv::COLOR_RGB2GRAY);
    cv::Size ap (7,7);
    svl::localVAR tv(ap);
    tv.operator () (objpyr[2], objpyr[2], mask);
    UIImage* result =  [UIImage imageWithCVMat:mask];
    return result;
}

+ (CGPoint) cvGetLuminanceCenter:(UIImage*) inputImage
{
    CGPoint gp = CGPointMake(0,0);
    cv::Point2f com;
    cv::Mat matImage = [self rotateCorrect:inputImage];
    cv::Mat grey;
    cv::cvtColor(matImage, grey, cv::COLOR_RGB2GRAY);
    getLuminanceCenterOfMass (grey, com);
    gp.x = com.x;
    gp.y = com.y;
    return gp;
}


+ (cv::Mat) rotateCorrect:(UIImage*) inputImage
{
    cv::Mat matImage;
    
    if ([inputImage isKindOfClass: [UIImage class]])
    {
        /*
         All images taken with the iPhone/iPa cameras are LANDSCAPE LEFT orientation. The  UIImage imageOrientation flag is an instruction to the OS to transform the image during display only. When we feed images into openCV, they need to be the actual orientation that we expect them to be for stitching. So we rotate the actual pixel matrix here if required.
         */
        UIImage* rotatedImage = [inputImage rotateToImageOrientation];
        matImage = [rotatedImage CVMat3];
    }
    
    return matImage;
}



//+ (UIImage*) toUIImage:(CVImageBufferRef)imageBuffer
//{
//    CIImage *ciImage = [CIImage imageWithCVPixelBuffer:imageBuffer];
//    CIContext *temporaryContext = [CIContext contextWithOptions:nil];
//    CGImageRef videoImage = [temporaryContext
//                             createCGImage:ciImage
//                             fromRect:CGRectMake(0, 0,
//                                                 CVPixelBufferGetWidth(imageBuffer),
//                                                 CVPixelBufferGetHeight(imageBuffer))];
//    
//    UIImage *image = [[UIImage alloc] initWithCGImage:videoImage];
//    return image;
//}
@end
