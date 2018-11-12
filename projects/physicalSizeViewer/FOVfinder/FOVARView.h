//
//  FOVARView.h
//  FOVfinder
//
//  Created by Tim Gleue on 21.10.13.
//  Copyright (c) 2015 Tim Gleue ( http://gleue-interactive.com )
//
//  Permission is hereby granted, free of charge, to any person obtaining a copy
//  of this software and associated documentation files (the "Software"), to deal
//  in the Software without restriction, including without limitation the rights
//  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//  copies of the Software, and to permit persons to whom the Software is
//  furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included in
//  all copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//  THE SOFTWARE.

#import <UIKit/UIKit.h>
#import <GLKit/GLKit.h>

#import <AVFoundation/AVFoundation.h>

@class FOVARView;

@protocol FOVARViewDelegate <NSObject>

@optional

- (void)arView:(FOVARView *)view didChangeFocusMode:(AVCaptureFocusMode)mode;
- (void)arView:(FOVARView *)view didChangeLensPosition:(CGFloat)position;

- (void)arViewDidStartAdjustingFocus:(FOVARView *)view;
- (void)arViewDidStopAdjustingFocus:(FOVARView *)view;

@end

@interface FOVARView : UIView  <GLKViewDelegate>

@property (weak, nonatomic) id <FOVARViewDelegate> delegate;

@property (strong, nonatomic) NSArray *shapes;
@property (strong, nonatomic) NSString *videoPreset;
@property (strong, nonatomic) NSString *videoGravity;
@property (assign, nonatomic) UIInterfaceOrientation interfaceOrienation;

@property (assign, nonatomic) CGFloat fovScalePortrait;
@property (assign, nonatomic) CGFloat fovScaleLandscape;

@property (readonly, nonatomic) CGFloat fieldOfViewPortrait;
@property (readonly, nonatomic) CGFloat fieldOfViewLandscape;

- (EAGLContext *)renderContext;

- (void)start;
- (void)stop;

- (NSString *)currentVideoPreset;

- (CGFloat)maxVideoZoom;
- (CGFloat)currentVideoZoom;
- (void)setCurrentVideoZoom:(CGFloat)zoom;

- (CGFloat)effectiveFieldOfViewPortrait;
- (CGFloat)effectiveFieldOfViewLandscape;

@end
