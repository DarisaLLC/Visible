//
//  ocvCalibrator.h
//  Camera
//
//  Created by Arman Garakani on 6/28/17.
//  Copyright © 2017 Leo Pharma . All rights reserved.
//
#import <AVFoundation/AVFoundation.h>
#import <UIKit/UIKit.h>
#import "UIImage+Rotate.h"
#import <CoreLocation/CoreLocation.h>
#import <MetalKit/MTKView.h>
#import "ocvCalibrator.h"
#import "UIImage+cvMat.h"
#import "UCamera-Swift.h" 

#include <opencv2/opencv.hpp>


/// Number of snapshots taken for calibration.
static constexpr size_t kCalibrationPoints = 10;
/// Minimal average distance between points in two sets.
static constexpr double kMinDistance = 5.0f;
/// Size of the asymetric circle pattern.
static const cv::Size kPatternSize(4, 11);


/**
 Computes the distance between two sets of image points.
 */
static double distance(
                       const std::vector<cv::Point2f> &xs,
                       const std::vector<cv::Point2f>& ys)
{
    assert(xs.size() == ys.size());
    
    float sum = 0.0f;
    for (size_t i = 0; i < xs.size(); ++i) {
        const auto dist = xs[i] - ys[i];
        sum += std::sqrt(dist.dot(dist));
    }
    return std::sqrt(sum / xs.size());
}


/**
 Computes the minimal distance between a point and any sets of points.
 */
static double distance(
                       const std::vector<std::vector<cv::Point2f>> &xxs,
                       const std::vector<cv::Point2f> &ys)
{
    auto min = std::numeric_limits<double>::max();
    for (const auto &xs: xxs) {
        min = std::min(min, distance(xs, ys));
    }
    
    return min;
}



@implementation ocvCalibrator
{
    // OpenCV images.
    cv::Mat gray;
    cv::Mat bgra;
    cv::Mat rgba;
    
    // Buffer for calibration points.
    std::vector<std::vector<cv::Point2f>> imagePoints;
    std::vector<cv::Point3f> grid;
    
    // Computed parameters.
    cv::Mat cameraMatrix;
    cv::Mat distCoeffs;
    
    // Delegate that handles events.
    id<ocvCalibratorDelegate> delegate;
    
    // Focal distance.
    NSNumber *focus;
}

- (instancetype)initWithDelegate:(id)delegate_
{
    if (!(self = [super init])) {
        return nil;
    }
    
    delegate = delegate_;
    
    // Initialize the intrinsic parameters.
    cameraMatrix = cv::Mat::eye(3, 3, CV_64F);
    distCoeffs = cv::Mat::zeros(4, 1, CV_64F);
    
    // Reserve storage for the point sets.
    imagePoints.reserve(kCalibrationPoints);
    
    // Initialize the coordinates in the grid pattern.
    for (int i = 0; i < kPatternSize.height; i++ ) {
        for (int j = 0; j < kPatternSize.width; j++) {
            grid.emplace_back((2 * j + i % 2) * 4.0f, i * 4.0f, 0.0f);
        }
    }
    
    // No focal point set.
    focus = nil;
    
    return self;
}


- (cv::Mat) rotateCorrect:(UIImage*) inputImage
{
    cv::Mat matImage;
    
    if ([inputImage isKindOfClass: [UIImage class]])
    {
        /*
         All images taken with the iPhone/iPa cameras are LANDSCAPE LEFT orientation. The  UIImage imageOrientation flag is an instruction to the OS to transform the image during display only. When we feed images into openCV, they need to be the actual orientation that we expect them to be for stitching. So we rotate the actual pixel matrix here if required.
         */
        UIImage* rotatedImage = [inputImage rotateToImageOrientation];
        matImage = [rotatedImage cvMat];
    }
    
    return matImage;
}

- (UIImage*)findPattern:(UIImage*)frame
{
    // Focal distance must be set for calibration.
    if (focus == nil) {
        return frame;
    }
    
    // Convert rgb to bgra.
    [frame toCvMat:bgra];
    cv::cvtColor(bgra, rgba, CV_BGRA2RGBA);
    cv::cvtColor(rgba, gray, CV_RGBA2GRAY);
    
    // Find the chessboard.
    std::vector<cv::Point2f> corners;
    auto found = cv::findCirclesGrid(
                                     gray,
                                     kPatternSize,
                                     corners,
                                     cv::CALIB_CB_ASYMMETRIC_GRID | cv::CALIB_CB_CLUSTERING
                                     );
    if (!found) {
        return frame;
    }
    
    // Draw the detected pattern.
    cv::drawChessboardCorners(rgba, kPatternSize, cv::Mat(corners), found);
    
    // If enough points were captured, bail out.
    if (imagePoints.size() >= kCalibrationPoints) {
        std::cout << "have enough " << imagePoints.size() << std::endl;
        return [UIImage imageWithCvMat: rgba];
    }
    
    // If distance between two sets too small, return.
    int mind = distance(imagePoints, corners);
    if (mind < kMinDistance) {
        std::cout << "too close " << mind << std::endl;
        return [UIImage imageWithCvMat: rgba];
    }
    
    // Add the point to the set of calibration points and report progress.
    imagePoints.push_back(corners);
    if ([delegate respondsToSelector: @selector(onProgress:)]) {
        [delegate onProgress:static_cast<float>(imagePoints.size()) / kCalibrationPoints];
    }
    
    // If enough points were recorded, start calibrating in a backgroud thread.
    if (imagePoints.size() == kCalibrationPoints) {
        // Otherwise, calibrate in a background thread.
        dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
            auto rms = static_cast<float>(cv::calibrateCamera(
                                                              std::vector<std::vector<cv::Point3f>>(kCalibrationPoints, grid),
                                                              imagePoints,
                                                              rgba.size(),
                                                              cameraMatrix,
                                                              distCoeffs,
                                                              {},
                                                              {},
                                                              0
                                                              ));
            cameraMatrix.convertTo(cameraMatrix, CV_32F);
            distCoeffs.convertTo(distCoeffs, CV_32F);
            
            if ([delegate respondsToSelector:@selector(onComplete:params:)]) {
                [delegate onComplete:rms params:[[calibParameters alloc]
                                                 initWithFx: cameraMatrix.at<float>(0, 0)
                                                 fy: cameraMatrix.at<float>(1, 1)
                                                 cx: cameraMatrix.at<float>(0, 2)
                                                 cy: cameraMatrix.at<float>(1, 2)
                                                 k1: distCoeffs.at<float>(0, 0)
                                                 k2: distCoeffs.at<float>(1, 0)
                                                 r1: distCoeffs.at<float>(2, 0)
                                                 r2: distCoeffs.at<float>(3, 0)
                                                 f: [focus floatValue]
                                                 ]];
            }
        });
    }
    
    return [UIImage imageWithCvMat: rgba];
}


- (void)focus:(float)f x:(float)x y:(float)y
{
    focus = @(f);
    
    imagePoints.clear();
    
    if ([delegate respondsToSelector:@selector(onProgress:)]) {
        [delegate onProgress: 0.0f];
    }
}

@end
