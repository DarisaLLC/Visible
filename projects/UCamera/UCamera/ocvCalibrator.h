//
//  ocvCalibrator.h
//  Camera
//
//  Created by Arman Garakani on 6/28/17.
//  Copyright © 2017 Leo Pharma . All rights reserved.
//
#pragma once

#import <Foundation/Foundation.h>
#import <UIKit/UIImage.h>

@class calibParameters;

@protocol ocvCalibratorDelegate <NSObject>

- (void)onProgress:(float)progress;
- (void)onComplete:(float)rms params:(calibParameters*)params;

@end


/**
 Class that encapsulates calibration logic.
 */
@interface ocvCalibrator : NSObject

/**
 Initializes the calibrator object.
 */
- (instancetype)initWithDelegate:(id)delegate;

/**
 Finds a pattern in a frame and highlights it.
 */
- (UIImage *)findPattern:(UIImage *)frame;

/**
 Resets the calibrator when the focal distance changes.
 */
- (void)focus:(float)f x:(float)x y:(float)y;

@end
