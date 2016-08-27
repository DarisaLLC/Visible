//
//  cpVision.h
//  cpVision
//

#import <Foundation/Foundation.h>
#import <AVFoundation/AVFoundation.h>

// support for swift compiler
#ifndef NS_ASSUME_NONNULL_BEGIN
# define NS_ASSUME_NONNULL_BEGIN
# define nullable
# define NS_ASSUME_NONNULL_END
#endif

NS_ASSUME_NONNULL_BEGIN

// vision types

typedef NS_ENUM(NSInteger, cpCameraDevice) {
    cpCameraDeviceBack = 0,
    cpCameraDeviceFront
};

typedef NS_ENUM(NSInteger, cpCameraMode) {
    cpCameraModePhoto = 0,
    cpCameraModeVideo
};

typedef NS_ENUM(NSInteger, cpCameraOrientation) {
    cpCameraOrientationPortrait = AVCaptureVideoOrientationPortrait,
    cpCameraOrientationPortraitUpsideDown = AVCaptureVideoOrientationPortraitUpsideDown,
    cpCameraOrientationLandscapeRight = AVCaptureVideoOrientationLandscapeRight,
    cpCameraOrientationLandscapeLeft = AVCaptureVideoOrientationLandscapeLeft,
};

typedef NS_ENUM(NSInteger, cpFocusMode) {
    cpFocusModeLocked = AVCaptureFocusModeLocked,
    cpFocusModeAutoFocus = AVCaptureFocusModeAutoFocus,
    cpFocusModeContinuousAutoFocus = AVCaptureFocusModeContinuousAutoFocus
};

typedef NS_ENUM(NSInteger, cpExposureMode) {
    cpExposureModeLocked = AVCaptureExposureModeLocked,
    cpExposureModeAutoExpose = AVCaptureExposureModeAutoExpose,
    cpExposureModeContinuousAutoExposure = AVCaptureExposureModeContinuousAutoExposure
};

typedef NS_ENUM(NSInteger, cpFlashMode) {
    cpFlashModeOff = AVCaptureFlashModeOff,
    cpFlashModeOn = AVCaptureFlashModeOn,
    cpFlashModeAuto = AVCaptureFlashModeAuto
};

typedef NS_ENUM(NSInteger, cpMirroringMode) {
	cpMirroringAuto = 0,
	cpMirroringOn,
	cpMirroringOff
};

typedef NS_ENUM(NSInteger, cpAuthorizationStatus) {
    cpAuthorizationStatusNotDetermined = 0,
    cpAuthorizationStatusAuthorized,
    cpAuthorizationStatusAudioDenied
};

typedef NS_ENUM(NSInteger, cpOutputFormat) {
    cpOutputFormatPreset = 0,
    cpOutputFormatSquare, // 1:1
    cpOutputFormatWidescreen, // 16:9
    cpOutputFormatStandard // 4:3
};

// cpError

extern NSString * const cpVisionErrorDomain;

typedef NS_ENUM(NSInteger, cpVisionErrorType)
{
    cpVisionErrorUnknown = -1,
    cpVisionErrorCancelled = 100,
    cpVisionErrorSessionFailed = 101,
    cpVisionErrorBadOutputFile = 102,
    cpVisionErrorOutputFileExists = 103,
    cpVisionErrorCaptureFailed = 104,
};

// additional video capture keys

extern NSString * const cpVisionVideoRotation;

// photo dictionary keys

extern NSString * const cpVisionPhotoMetadataKey;
extern NSString * const cpVisionPhotoJPEGKey;
extern NSString * const cpVisionPhotoImageKey;
extern NSString * const cpVisionPhotoThumbnailKey; // 160x120

// video dictionary keys

extern NSString * const cpVisionVideoPathKey;
extern NSString * const cpVisionVideoThumbnailKey;
extern NSString * const cpVisionVideoThumbnailArrayKey;
extern NSString * const cpVisionVideoCapturedDurationKey; // Captured duration in seconds

// suggested videoBitRate constants

static CGFloat const cpVideoBitRate480x360 = 87500 * 8;
static CGFloat const cpVideoBitRate640x480 = 437500 * 8;
static CGFloat const cpVideoBitRate1280x720 = 1312500 * 8;
static CGFloat const cpVideoBitRate1920x1080 = 2975000 * 8;
static CGFloat const cpVideoBitRate960x540 = 3750000 * 8;
static CGFloat const cpVideoBitRate1280x750 = 5000000 * 8;

@class EAGLContext;
@protocol cpVisionDelegate;
@interface cpVision : NSObject

+ (cpVision *)sharedInstance;

@property (nonatomic, weak, nullable) id<cpVisionDelegate> delegate;

// session

@property (nonatomic, readonly, getter=isCaptureSessionActive) BOOL captureSessionActive;

// setup

@property (nonatomic) cpCameraOrientation cameraOrientation;
@property (nonatomic) cpCameraMode cameraMode;
@property (nonatomic) cpCameraDevice cameraDevice;
// Indicates whether the capture session will make use of the app’s shared audio session. Allows you to
// use a previously configured audios session with a category such as AVAudioSessionCategoryAmbient.
@property (nonatomic) BOOL usesApplicationAudioSession;
- (BOOL)isCameraDeviceAvailable:(cpCameraDevice)cameraDevice;

@property (nonatomic) cpFlashMode flashMode; // flash and torch
@property (nonatomic, readonly, getter=isFlashAvailable) BOOL flashAvailable;

@property (nonatomic) cpMirroringMode mirroringMode;

// video output settings

@property (nonatomic, copy) NSDictionary *additionalVideoProperties;
@property (nonatomic, copy) NSString *captureSessionPreset;
@property (nonatomic, copy) NSString *captureDirectory;
@property (nonatomic) cpOutputFormat outputFormat;

// video compression settings

@property (nonatomic) CGFloat videoBitRate;
@property (nonatomic) NSInteger audioBitRate;
@property (nonatomic) NSDictionary *additionalCompressionProperties;

// video frame rate (adjustment may change the capture format (AVCaptureDeviceFormat : FoV, zoom factor, etc)

@property (nonatomic) NSInteger videoFrameRate; // desired fps for active cameraDevice
- (BOOL)supportsVideoFrameRate:(NSInteger)videoFrameRate;

// preview

@property (nonatomic, readonly) AVCaptureVideoPreviewLayer *previewLayer;
@property (nonatomic) BOOL autoUpdatePreviewOrientation;
@property (nonatomic) cpCameraOrientation previewOrientation;
@property (nonatomic) BOOL autoFreezePreviewDuringCapture;

@property (nonatomic, readonly) CGRect cleanAperture;

- (void)startPreview;
- (void)stopPreview;

- (void)freezePreview;
- (void)unfreezePreview;

// focus, exposure, white balance

// note: focus and exposure modes change when adjusting on point
- (BOOL)isFocusPointOfInterestSupported;
- (void)focusExposeAndAdjustWhiteBalanceAtAdjustedPoint:(CGPoint)adjustedPoint;

@property (nonatomic) cpFocusMode focusMode;
@property (nonatomic, readonly, getter=isFocusLockSupported) BOOL focusLockSupported;
- (void)focusAtAdjustedPointOfInterest:(CGPoint)adjustedPoint;
- (BOOL)isAdjustingFocus;

@property (nonatomic) cpExposureMode exposureMode;
@property (nonatomic, readonly, getter=isExposureLockSupported) BOOL exposureLockSupported;
- (void)exposeAtAdjustedPointOfInterest:(CGPoint)adjustedPoint;
- (BOOL)isAdjustingExposure;

// photo

@property (nonatomic, readonly) BOOL canCapturePhoto;
- (void)capturePhoto;

// video
// use pause/resume if a session is in progress, end finalizes that recording session

@property (nonatomic, readonly) BOOL supportsVideoCapture;
@property (nonatomic, readonly) BOOL canCaptureVideo;
@property (nonatomic, readonly, getter=isRecording) BOOL recording;
@property (nonatomic, readonly, getter=isPaused) BOOL paused;

@property (nonatomic, getter=isVideoRenderingEnabled) BOOL videoRenderingEnabled;
@property (nonatomic, getter=isAudioCaptureEnabled) BOOL audioCaptureEnabled;

@property (nonatomic, readonly) EAGLContext *context;
@property (nonatomic) CGRect presentationFrame;

@property (nonatomic) CMTime maximumCaptureDuration; // automatically triggers vision:capturedVideo:error: after exceeding threshold, (kCMTimeInvalid records without threshold)
@property (nonatomic, readonly) Float64 capturedAudioSeconds;
@property (nonatomic, readonly) Float64 capturedVideoSeconds;

- (void)startVideoCapture;
- (void)pauseVideoCapture;
- (void)resumeVideoCapture;
- (void)endVideoCapture;
- (void)cancelVideoCapture;

// thumbnails

@property (nonatomic) BOOL thumbnailEnabled; // thumbnail generation, disabling reduces processing time for a photo or video
@property (nonatomic) BOOL defaultVideoThumbnails; // capture first and last frames of video

- (void)captureCurrentVideoThumbnail;
- (void)captureVideoThumbnailAtFrame:(int64_t)frame;
- (void)captureVideoThumbnailAtTime:(Float64)seconds;

@end

@protocol cpVisionDelegate <NSObject>
@optional

// session

- (void)visionSessionWillStart:(cpVision *)vision;
- (void)visionSessionDidStart:(cpVision *)vision;
- (void)visionSessionDidStop:(cpVision *)vision;

- (void)visionSessionWasInterrupted:(cpVision *)vision;
- (void)visionSessionInterruptionEnded:(cpVision *)vision;

// device / mode / format

- (void)visionCameraDeviceWillChange:(cpVision *)vision;
- (void)visionCameraDeviceDidChange:(cpVision *)vision;

- (void)visionCameraModeWillChange:(cpVision *)vision;
- (void)visionCameraModeDidChange:(cpVision *)vision;

- (void)visionOutputFormatWillChange:(cpVision *)vision;
- (void)visionOutputFormatDidChange:(cpVision *)vision;

- (void)vision:(cpVision *)vision didChangeCleanAperture:(CGRect)cleanAperture;

- (void)visionDidChangeVideoFormatAndFrameRate:(cpVision *)vision;

// focus / exposure

- (void)visionWillStartFocus:(cpVision *)vision;
- (void)visionDidStopFocus:(cpVision *)vision;

- (void)visionWillChangeExposure:(cpVision *)vision;
- (void)visionDidChangeExposure:(cpVision *)vision;

- (void)visionDidChangeFlashMode:(cpVision *)vision; // flash or torch was changed

// authorization / availability

- (void)visionDidChangeAuthorizationStatus:(cpAuthorizationStatus)status;
- (void)visionDidChangeFlashAvailablility:(cpVision *)vision; // flash or torch is available

// preview

- (void)visionSessionDidStartPreview:(cpVision *)vision;
- (void)visionSessionDidStopPreview:(cpVision *)vision;

// photo

- (void)visionWillCapturePhoto:(cpVision *)vision;
- (void)visionDidCapturePhoto:(cpVision *)vision;
- (void)vision:(cpVision *)vision capturedPhoto:(nullable NSDictionary *)photoDict error:(nullable NSError *)error;

// video

- (NSString *)vision:(cpVision *)vision willStartVideoCaptureToFile:(NSString *)fileName;
- (void)visionDidStartVideoCapture:(cpVision *)vision;
- (void)visionDidPauseVideoCapture:(cpVision *)vision; // stopped but not ended
- (void)visionDidResumeVideoCapture:(cpVision *)vision;
- (void)visionDidEndVideoCapture:(cpVision *)vision;
- (void)vision:(cpVision *)vision capturedVideo:(nullable NSDictionary *)videoDict error:(nullable NSError *)error;

// video capture progress

- (void)vision:(cpVision *)vision didCaptureVideoSampleBuffer:(CMSampleBufferRef)sampleBuffer;
- (void)vision:(cpVision *)vision didCaptureAudioSample:(CMSampleBufferRef)sampleBuffer;

NS_ASSUME_NONNULL_END

@end
