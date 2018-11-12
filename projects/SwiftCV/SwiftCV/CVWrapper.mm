//
//  CVWrapper.m
//  CVOpenTemplate
//
//  Created by Washe on 02/01/2013.
//  Copyright (c) 2013 foundry. All rights reserved.
//

#import "CVWrapper.h"
#import "UIImage+OpenCV.h"
#import "cv_functions.h"
#import "UIImage+Rotate.h"
#import "localvariance.h"

@implementation CVWrapper

+ (UIImage*) cvProduceOutline:(UIImage*) inputImage
{
    if ([inputImage isKindOfClass: [UIImage class]]) {
        /*
         All images taken with the iPhone/iPa cameras are LANDSCAPE LEFT orientation. The  UIImage imageOrientation flag is an instruction to the OS to transform the image during display only. When we feed images into openCV, they need to be the actual orientation that we expect them to be for stitching. So we rotate the actual pixel matrix here if required.
         */
        std::vector<cv::Mat> objpyr;
        cv::Mat matImage = [inputImage CVMat3];
        // build pyramid
        cv::buildPyramid(matImage, objpyr, 3);
        
    
        cv::Mat grey;
        cv::Mat mask;
        cv::cvtColor(objpyr[2], grey, cv::COLOR_RGB2GRAY);
//        UIImage* rotatedImage = [inputImage rotateToImageOrientation];
        cv::Size ap (7,7);
        svl::localVAR tv(ap);
        tv.operator () (objpyr[2], objpyr[2], mask);
//        cv::Canny(grey, mask, 30, 20 * 5, 7);
//        cv::normalize(mask,mask, 0.0, 255.0, cv::NORM_MINMAX);
        UIImage* result =  [UIImage imageWithCVMat:mask];
        return result;
    }
    return 0;
}

+ (UIImage*) stitchImageWithOpenCV: (UIImage*) inputImage
{
    NSArray* imageArray = [NSArray arrayWithObject:inputImage];
    UIImage* result = [[self class] stitchWithArray:imageArray];
    return result;
}

+ (UIImage*) stitchWithOpenCVImage1:(UIImage*)inputImage1 image2:(UIImage*)inputImage2;
{
    NSArray* imageArray = [NSArray arrayWithObjects:inputImage1,inputImage2,nil];
    UIImage* result = [[self class] stitchWithArray:imageArray];
    return result;
}

+ (UIImage*) stitchWithArray:(NSArray*)imageArray
{
    if ([imageArray count]==0){
        NSLog (@"imageArray is empty");
        return 0;
        }
    std::vector<cv::Mat> matImages;

    for (id image in imageArray) {
        if ([image isKindOfClass: [UIImage class]]) {
            /*
             All images taken with the iPhone/iPa cameras are LANDSCAPE LEFT orientation. The  UIImage imageOrientation flag is an instruction to the OS to transform the image during display only. When we feed images into openCV, they need to be the actual orientation that we expect them to be for stitching. So we rotate the actual pixel matrix here if required.
             */
            UIImage* rotatedImage = [image rotateToImageOrientation];
            cv::Mat matImage = [rotatedImage CVMat3];
            NSLog (@"matImage: %@",image);
            matImages.push_back(matImage);
        }
    }
    NSLog (@"stitching...");
    cv::Mat stitchedMat = stitch (matImages);
    UIImage* result =  [UIImage imageWithCVMat:stitchedMat];
    return result;
}


@end
