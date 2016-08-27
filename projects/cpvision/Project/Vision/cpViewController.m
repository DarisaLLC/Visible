//
//  cpViewController.m
//  cpVision
//

#import "cpViewController.h"
#import "cpStrobeView.h"
#import "cpFocusView.h"

#import "cpVision.h"
#import "cpVisionUtilities.h"

#import <AssetsLibrary/AssetsLibrary.h>
#import <GLKit/GLKit.h>

@interface ExtendedHitButton : UIButton

+ (instancetype)extendedHitButton;

- (BOOL)pointInside:(CGPoint)point withEvent:(UIEvent *)event;

@end

@implementation ExtendedHitButton

+ (instancetype)extendedHitButton
{
    return (ExtendedHitButton *)[ExtendedHitButton buttonWithType:UIButtonTypeCustom];
}

- (BOOL)pointInside:(CGPoint)point withEvent:(UIEvent *)event
{
    CGRect relativeFrame = self.bounds;
    UIEdgeInsets hitTestEdgeInsets = UIEdgeInsetsMake(-35, -35, -35, -35);
    CGRect hitFrame = UIEdgeInsetsInsetRect(relativeFrame, hitTestEdgeInsets);
    return CGRectContainsPoint(hitFrame, point);
}

@end

@interface cpViewController () <
    UIGestureRecognizerDelegate,
    cpVisionDelegate,
    UIAlertViewDelegate>
{
    cpStrobeView *_strobeView;
    UIButton *_doneButton;
    
    UIButton *_flipButton;
    UIButton *_focusButton;
    UIButton *_frameRateButton;
    UIButton *_overlayButton;
    UIView *_captureDock;

    UIView *_previewView;
    AVCaptureVideoPreviewLayer *_previewLayer;
    cpFocusView *_focusView;
    GLKViewController *_effectsViewController;
    
    UILabel *_instructionLabel;
    UIView *_gestureView;
    UILongPressGestureRecognizer *_longPressGestureRecognizer;
    UITapGestureRecognizer *_focusTapGestureRecognizer;
    UITapGestureRecognizer *_photoTapGestureRecognizer;
    
    BOOL _recording;

    ALAssetsLibrary *_assetLibrary;
    __block NSDictionary *_currentVideo;
    __block NSDictionary *_currentPhoto;
}

@end

@implementation cpViewController

#pragma mark - UIViewController

- (BOOL)prefersStatusBarHidden
{
    return YES;
}

#pragma mark - init

- (void)dealloc
{
    [UIApplication sharedApplication].idleTimerDisabled = NO;
    _longPressGestureRecognizer.delegate = nil;
    _focusTapGestureRecognizer.delegate = nil;
    _photoTapGestureRecognizer.delegate = nil;
}

#pragma mark - view lifecycle

- (void)viewDidLoad
{
    [super viewDidLoad];

    self.view.backgroundColor = [UIColor blackColor];
    self.view.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;

    _assetLibrary = [[ALAssetsLibrary alloc] init];
    
    CGFloat viewWidth = CGRectGetWidth(self.view.frame);

    // elapsed time and red dot
    _strobeView = [[cpStrobeView alloc] initWithFrame:CGRectZero];
    CGRect strobeFrame = _strobeView.frame;
    strobeFrame.origin = CGPointMake(15.0f, 15.0f);
    _strobeView.frame = strobeFrame;
    [self.view addSubview:_strobeView];

    // done button
    _doneButton = [ExtendedHitButton extendedHitButton];
    _doneButton.frame = CGRectMake(viewWidth - 25.0f - 15.0f, 18.0f, 25.0f, 25.0f);
    UIImage *buttonImage = [UIImage imageNamed:@"capture_done"];
    [_doneButton setImage:buttonImage forState:UIControlStateNormal];
    [_doneButton addTarget:self action:@selector(_handleDoneButton:) forControlEvents:UIControlEventTouchUpInside];
    [self.view addSubview:_doneButton];

    // preview and AV layer
    _previewView = [[UIView alloc] initWithFrame:CGRectZero];
    _previewView.backgroundColor = [UIColor blackColor];
    CGRect previewFrame = CGRectMake(0, 60.0f, CGRectGetWidth(self.view.frame), CGRectGetWidth(self.view.frame));
    _previewView.frame = previewFrame;
    _previewLayer = [[cpVision sharedInstance] previewLayer];
    _previewLayer.frame = _previewView.bounds;
    _previewLayer.videoGravity = AVLayerVideoGravityResizeAspectFill;
   [_previewView.layer addSublayer:_previewLayer];
    
    // overlay skin
    _effectsViewController = [[GLKViewController alloc] init];
    _effectsViewController.preferredFramesPerSecond = 60;
    
    GLKView *view = (GLKView *)_effectsViewController.view;
    CGRect viewFrame = _previewView.bounds;
    view.frame = viewFrame;
    view.context = [[cpVision sharedInstance] context];
    view.contentScaleFactor = [[UIScreen mainScreen] scale];
//    view.alpha = 0.0f;
    view.hidden = YES;
    [[cpVision sharedInstance] setPresentationFrame:_previewView.frame];
    [_previewView addSubview:_effectsViewController.view];

    // focus view
    _focusView = [[cpFocusView alloc] initWithFrame:CGRectZero];

    // instruction label
    _instructionLabel = [[UILabel alloc] initWithFrame:self.view.bounds];
    _instructionLabel.textAlignment = NSTextAlignmentCenter;
    _instructionLabel.font = [UIFont fontWithName:@"HelveticaNeue" size:15.0f];
    _instructionLabel.textColor = [UIColor whiteColor];
    _instructionLabel.backgroundColor = [UIColor blackColor];
    _instructionLabel.text = NSLocalizedString(@"Touch & hold to snap", @"Instruction message for capturing video.");
    [_instructionLabel sizeToFit];
    CGPoint labelCenter = _previewView.center;
    labelCenter.y += ((CGRectGetHeight(_previewView.frame) * 0.5f) + 35.0f);
    _instructionLabel.center = labelCenter;
    [self.view addSubview:_instructionLabel];
    
    // touch to record
    _longPressGestureRecognizer = [[UILongPressGestureRecognizer alloc] initWithTarget:self action:@selector(_handleLongPressGestureRecognizer:)];
    _longPressGestureRecognizer.delegate = self;
    _longPressGestureRecognizer.minimumPressDuration = 0.05f;
    _longPressGestureRecognizer.allowableMovement = 10.0f;
    
    // tap to focus
    _focusTapGestureRecognizer = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(_handleFocusTapGesterRecognizer:)];
    _focusTapGestureRecognizer.delegate = self;
    _focusTapGestureRecognizer.numberOfTapsRequired = 1;
    _focusTapGestureRecognizer.enabled = NO;
    [_previewView addGestureRecognizer:_focusTapGestureRecognizer];
    
    // gesture view to record
    _gestureView = [[UIView alloc] initWithFrame:CGRectZero];
    CGRect gestureFrame = self.view.bounds;
    gestureFrame.origin = CGPointMake(0, 60.0f);
    gestureFrame.size.height -= (40.0f + 85.0f);
    _gestureView.frame = gestureFrame;
    [self.view addSubview:_gestureView];
    
    [_gestureView addGestureRecognizer:_longPressGestureRecognizer];

    // bottom dock
    _captureDock = [[UIView alloc] initWithFrame:CGRectMake(0, CGRectGetHeight(self.view.bounds) - 60.0f, CGRectGetWidth(self.view.bounds), 60.0f)];
    _captureDock.backgroundColor = [UIColor clearColor];
    _captureDock.autoresizingMask = UIViewAutoresizingFlexibleTopMargin;
    [self.view addSubview:_captureDock];
    
    // flip button
    _flipButton = [ExtendedHitButton extendedHitButton];
    UIImage *flipImage = [UIImage imageNamed:@"capture_flip"];
    [_flipButton setImage:flipImage forState:UIControlStateNormal];
    CGRect flipFrame = _flipButton.frame;
    flipFrame.origin = CGPointMake(20.0f, 16.0f);
    flipFrame.size = flipImage.size;
    _flipButton.frame = flipFrame;
    [_flipButton addTarget:self action:@selector(_handleFlipButton:) forControlEvents:UIControlEventTouchUpInside];
    [_captureDock addSubview:_flipButton];
    
    // focus mode button
    _focusButton = [ExtendedHitButton extendedHitButton];
    UIImage *focusImage = [UIImage imageNamed:@"capture_focus_button"];
    [_focusButton setImage:focusImage forState:UIControlStateNormal];
    [_focusButton setImage:[UIImage imageNamed:@"capture_focus_button_active"] forState:UIControlStateSelected];
    CGRect focusFrame = _focusButton.frame;
    focusFrame.origin = CGPointMake((CGRectGetWidth(self.view.bounds) * 0.5f) - (focusImage.size.width * 0.5f), 16.0f);
    focusFrame.size = focusImage.size;
    _focusButton.frame = focusFrame;
    [_focusButton addTarget:self action:@selector(_handleFocusButton:) forControlEvents:UIControlEventTouchUpInside];
    [_captureDock addSubview:_focusButton];
    
    if ([[cpVision sharedInstance] supportsVideoFrameRate:120]) {
        // set faster frame rate
    }
    
    // overlay button
    _overlayButton = [ExtendedHitButton extendedHitButton];
    UIImage *overlayImage = [UIImage imageNamed:@"capture_overlay"];
    [_overlayButton setImage:overlayImage forState:UIControlStateNormal];
    [_overlayButton setImage:[UIImage imageNamed:@"capture_overlay_active"] forState:UIControlStateSelected];
    CGRect overlayFrame = _overlayButton.frame;
    overlayFrame.origin = CGPointMake(CGRectGetWidth(self.view.bounds) - overlayImage.size.width - 20.0f, 16.0f);
    overlayFrame.size = overlayImage.size;
    _overlayButton.frame = overlayFrame;
    _overlayButton.imageView.frame = _overlayButton.bounds;
    [_overlayButton addTarget:self action:@selector(_handleoverlaySkinningButton:) forControlEvents:UIControlEventTouchUpInside];
    [_captureDock addSubview:_overlayButton];
    _overlayButton.alpha = (float) (_overlayButton.selected ? 1.0 : 0.5);
}

- (void)viewWillAppear:(BOOL)animated
{
    [super viewWillAppear:animated];

    // iOS 6 support
    [[UIApplication sharedApplication] setStatusBarHidden:YES withAnimation:UIStatusBarAnimationSlide];
    
    [self _resetCapture];
    [[cpVision sharedInstance] startPreview];
}

- (void)viewWillDisappear:(BOOL)animated
{
    [super viewWillDisappear:animated];
    
    [[cpVision sharedInstance] stopPreview];
    
    // iOS 6 support
    [[UIApplication sharedApplication] setStatusBarHidden:NO withAnimation:UIStatusBarAnimationSlide];
}

#pragma mark - private start/stop helper methods

- (void)_startCapture
{
    [UIApplication sharedApplication].idleTimerDisabled = YES;

    [UIView animateWithDuration:0.2f delay:0 options:UIViewAnimationOptionCurveEaseInOut animations:^{
        _instructionLabel.alpha = 0;
        _instructionLabel.transform = CGAffineTransformMakeTranslation(0, 10.0f);
    } completion:^(BOOL finished) {
    }];
    [[cpVision sharedInstance] startVideoCapture];
}

- (void)_pauseCapture
{
    [UIView animateWithDuration:0.2f delay:0 options:UIViewAnimationOptionCurveEaseInOut animations:^{
        _instructionLabel.alpha = 1;
        _instructionLabel.transform = CGAffineTransformIdentity;
    } completion:^(BOOL finished) {
    }];

    [[cpVision sharedInstance] pauseVideoCapture];
    _effectsViewController.view.hidden = !_overlayButton.selected;
    _overlayButton.alpha = (float) (_overlayButton.selected ? 1.0 : 0.5);
}

- (void)_resumeCapture
{
    [UIView animateWithDuration:0.2f delay:0 options:UIViewAnimationOptionCurveEaseInOut animations:^{
        _instructionLabel.alpha = 0;
        _instructionLabel.transform = CGAffineTransformMakeTranslation(0, 10.0f);
    } completion:^(BOOL finished) {
    }];
    
    [[cpVision sharedInstance] resumeVideoCapture];
    _effectsViewController.view.hidden = YES;
}

- (void)_endCapture
{
    [UIApplication sharedApplication].idleTimerDisabled = NO;
    [[cpVision sharedInstance] endVideoCapture];
    _effectsViewController.view.hidden = YES;
}

- (void)_resetCapture
{
    [_strobeView stop];
    _longPressGestureRecognizer.enabled = YES;

    cpVision *vision = [cpVision sharedInstance];
    vision.delegate = self;

    if ([vision isCameraDeviceAvailable:cpCameraDeviceBack]) {
        vision.cameraDevice = cpCameraDeviceBack;
        _flipButton.hidden = NO;
    } else {
        vision.cameraDevice = cpCameraDeviceFront;
        _flipButton.hidden = YES;
    }
    
    vision.cameraMode = cpCameraModeVideo;
    //vision.cameraMode = cpCameraModePhoto; // PHOTO: uncomment to test photo capture
    vision.cameraOrientation = cpCameraOrientationPortrait;
    vision.focusMode = cpFocusModeContinuousAutoFocus;
    vision.outputFormat = cpOutputFormatSquare;
    vision.videoRenderingEnabled = YES;
    vision.additionalCompressionProperties = @{AVVideoProfileLevelKey : AVVideoProfileLevelH264Baseline30}; // AVVideoProfileLevelKey requires specific captureSessionPreset
    
    // specify a maximum duration with the following property
    // vision.maximumCaptureDuration = CMTimeMakeWithSeconds(5, 600); // ~ 5 seconds
}

#pragma mark - UIButton

- (void)_handleFlipButton:(UIButton *)button
{
    cpVision *vision = [cpVision sharedInstance];
    vision.cameraDevice = vision.cameraDevice == cpCameraDeviceBack ? cpCameraDeviceFront : cpCameraDeviceBack;
}

- (void)_handleFocusButton:(UIButton *)button
{
    _focusButton.selected = !_focusButton.selected;
    
    if (_focusButton.selected) {
        _focusTapGestureRecognizer.enabled = YES;
        _gestureView.hidden = YES;

    } else {
        if (_focusView && [_focusView superview]) {
            [_focusView stopAnimation];
        }
        _focusTapGestureRecognizer.enabled = NO;
        _gestureView.hidden = NO;
    }
    
    [UIView animateWithDuration:0.15f delay:0 options:UIViewAnimationOptionCurveEaseInOut animations:^{
        _instructionLabel.alpha = 0;
    } completion:^(BOOL finished) {
        _instructionLabel.text = _focusButton.selected ? NSLocalizedString(@"Touch to auto-expose & focus", @"Touch to focus") :
                                                         NSLocalizedString(@"Touch & hold to snap/record", @"Touch and hold to record");
        [UIView animateWithDuration:0.15f delay:0 options:UIViewAnimationOptionCurveEaseInOut animations:^{
            _instructionLabel.alpha = 1;
        } completion:^(BOOL finished1) {
        }];
    }];
}

- (void)_handleFrameRateChangeButton:(UIButton *)button
{
}

- (void)_handleoverlaySkinningButton:(UIButton *)button
{
    _overlayButton.selected = !_overlayButton.selected;
    
    if (_recording) {
        _effectsViewController.view.hidden = !_overlayButton.selected;
    }
    _overlayButton.alpha = (float)(_overlayButton.selected ? 1.0 : 0.5);
}

- (void)_handleDoneButton:(UIButton *)button
{
    // resets long press
    _longPressGestureRecognizer.enabled = NO;
    _longPressGestureRecognizer.enabled = YES;
    
    [self _endCapture];
}

#pragma mark - UIAlertViewDelegate

- (void)alertView:(UIAlertView *)alertView clickedButtonAtIndex:(NSInteger)buttonIndex
{
    [self _resetCapture];
}

#pragma mark - UIGestureRecognizer

- (void)_handleLongPressGestureRecognizer:(UIGestureRecognizer *)gestureRecognizer
{
    // PHOTO: uncomment to test photo capture
//    if (gestureRecognizer.state == UIGestureRecognizerStateBegan) {
//        [[cpVision sharedInstance] capturePhoto];
//        return;
//    }

    switch (gestureRecognizer.state) {
      case UIGestureRecognizerStateBegan:
        {
            if (!_recording)
                [self _startCapture];
            else
                [self _resumeCapture];
            break;
        }
      case UIGestureRecognizerStateEnded:
      case UIGestureRecognizerStateCancelled:
      case UIGestureRecognizerStateFailed:
        {
            [self _pauseCapture];
            break;
        }
      default:
        break;
    }
}

- (void)_handleFocusTapGesterRecognizer:(UIGestureRecognizer *)gestureRecognizer
{
    CGPoint tapPoint = [gestureRecognizer locationInView:_previewView];

    // auto focus is occuring, display focus view
    CGPoint point = tapPoint;
    
    CGRect focusFrame = _focusView.frame;
#if defined(__LP64__) && __LP64__
    focusFrame.origin.x = rint(point.x - (focusFrame.size.width * 0.5));
    focusFrame.origin.y = rint(point.y - (focusFrame.size.height * 0.5));
#else
    focusFrame.origin.x = rintf(point.x - (focusFrame.size.width * 0.5f));
    focusFrame.origin.y = rintf(point.y - (focusFrame.size.height * 0.5f));
#endif
    [_focusView setFrame:focusFrame];
    
    [_previewView addSubview:_focusView];
    [_focusView startAnimation];

    CGPoint adjustPoint = [cpVisionUtilities convertToPointOfInterestFromViewCoordinates:tapPoint inFrame:_previewView.frame];
    [[cpVision sharedInstance] focusExposeAndAdjustWhiteBalanceAtAdjustedPoint:adjustPoint];
}

#pragma mark - cpVisionDelegate

// session

- (void)visionSessionWillStart:(cpVision *)vision
{
}

- (void)visionSessionDidStart:(cpVision *)vision
{
    if (![_previewView superview]) {
        [self.view addSubview:_previewView];
        [self.view bringSubviewToFront:_gestureView];
    }
}

- (void)visionSessionDidStop:(cpVision *)vision
{
    [_previewView removeFromSuperview];
}

// preview

- (void)visionSessionDidStartPreview:(cpVision *)vision
{
    NSLog(@"Camera preview did start");
    
}

- (void)visionSessionDidStopPreview:(cpVision *)vision
{
    NSLog(@"Camera preview did stop");
}

// device

- (void)visionCameraDeviceWillChange:(cpVision *)vision
{
    NSLog(@"Camera device will change");
}

- (void)visionCameraDeviceDidChange:(cpVision *)vision
{
    NSLog(@"Camera device did change");
}

// mode

- (void)visionCameraModeWillChange:(cpVision *)vision
{
    NSLog(@"Camera mode will change");
}

- (void)visionCameraModeDidChange:(cpVision *)vision
{
    NSLog(@"Camera mode did change");
}

// format

- (void)visionOutputFormatWillChange:(cpVision *)vision
{
    NSLog(@"Output format will change");
}

- (void)visionOutputFormatDidChange:(cpVision *)vision
{
    NSLog(@"Output format did change");
}

- (void)vision:(cpVision *)vision didChangeCleanAperture:(CGRect)cleanAperture
{
}

// focus / exposure

- (void)visionWillStartFocus:(cpVision *)vision
{
}

- (void)visionDidStopFocus:(cpVision *)vision
{
    if (_focusView && [_focusView superview]) {
        [_focusView stopAnimation];
    }
}

- (void)visionWillChangeExposure:(cpVision *)vision
{
}

- (void)visionDidChangeExposure:(cpVision *)vision
{
    if (_focusView && [_focusView superview]) {
        [_focusView stopAnimation];
    }
}

// flash

- (void)visionDidChangeFlashMode:(cpVision *)vision
{
    NSLog(@"Flash mode did change");
}

// photo

- (void)visionWillCapturePhoto:(cpVision *)vision
{
}

- (void)visionDidCapturePhoto:(cpVision *)vision
{
}

- (void)vision:(cpVision *)vision capturedPhoto:(NSDictionary *)photoDict error:(NSError *)error
{
    if (error) {
        // handle error properly
        return;
    }
    _currentPhoto = photoDict;
    
    // save to library
    NSData *photoData = _currentPhoto[cpVisionPhotoJPEGKey];
    NSDictionary *metadata = _currentPhoto[cpVisionPhotoMetadataKey];
   [_assetLibrary writeImageDataToSavedPhotosAlbum:photoData metadata:metadata completionBlock:^(NSURL *assetURL, NSError *error1) {
        if (error1 || !assetURL) {
            // handle error properly
            return;
        }
       
        NSString *albumName = @"cpVision";
        __block BOOL albumFound = NO;
        [_assetLibrary enumerateGroupsWithTypes:ALAssetsGroupAlbum usingBlock:^(ALAssetsGroup *group, BOOL *stop) {
            if ([albumName compare:[group valueForProperty:ALAssetsGroupPropertyName]] == NSOrderedSame) {
                albumFound = YES;
                [_assetLibrary assetForURL:assetURL resultBlock:^(ALAsset *asset) {
                    [group addAsset:asset];
                    UIAlertView *alert = [[UIAlertView alloc] initWithTitle:@"Photo Saved!" message: @"Saved to the camera roll."
                                                       delegate:nil
                                              cancelButtonTitle:nil
                                              otherButtonTitles:@"OK", nil];
                    [alert show];
                } failureBlock:nil];
            }
            if (!group && !albumFound) {
                __weak ALAssetsLibrary *blockSafeLibrary = _assetLibrary;
                [_assetLibrary addAssetsGroupAlbumWithName:albumName resultBlock:^(ALAssetsGroup *group1) {
                    [blockSafeLibrary assetForURL:assetURL resultBlock:^(ALAsset *asset) {
                        [group1 addAsset:asset];
                        UIAlertView *alert = [[UIAlertView alloc] initWithTitle: @"Photo Saved!" message: @"Saved to the camera roll."
                                                       delegate:nil
                                              cancelButtonTitle:nil
                                              otherButtonTitles:@"OK", nil];
                        [alert show];
                    } failureBlock:nil];
                } failureBlock:nil];
            }
        } failureBlock:nil];
    }];
    
    _currentPhoto = nil;
}

// video capture

- (void)visionDidStartVideoCapture:(cpVision *)vision
{
    [_strobeView start];
    _recording = YES;
}

- (void)visionDidPauseVideoCapture:(cpVision *)vision
{
    [_strobeView stop];
}

- (void)visionDidResumeVideoCapture:(cpVision *)vision
{
    [_strobeView start];
}

- (void)vision:(cpVision *)vision capturedVideo:(NSDictionary *)videoDict error:(NSError *)error
{
    _recording = NO;

    if (error && [error.domain isEqual:cpVisionErrorDomain] && error.code == cpVisionErrorCancelled) {
        NSLog(@"recording session cancelled");
        return;
    } else if (error) {
        NSLog(@"encounted an error in video capture (%@)", error);
        return;
    }

    _currentVideo = videoDict;
    
    NSString *videoPath = [_currentVideo  objectForKey:cpVisionVideoPathKey];
    [_assetLibrary writeVideoAtPathToSavedPhotosAlbum:[NSURL URLWithString:videoPath] completionBlock:^(NSURL *assetURL, NSError *error1) {
        UIAlertView *alert = [[UIAlertView alloc] initWithTitle: @"Video Saved!" message: @"Saved to the camera roll."
                                                       delegate:self
                                              cancelButtonTitle:nil
                                              otherButtonTitles:@"OK", nil];
        [alert show];
    }];
}

// progress

- (void)vision:(cpVision *)vision didCaptureVideoSampleBuffer:(CMSampleBufferRef)sampleBuffer
{
    NSLog(@"captured video (%f) seconds", vision.capturedVideoSeconds);
}

- (void)vision:(cpVision *)vision didCaptureAudioSample:(CMSampleBufferRef)sampleBuffer
{
//    NSLog(@"captured audio (%f) seconds", vision.capturedAudioSeconds);
}

@end
