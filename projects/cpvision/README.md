![cpVision](https://raw.githubusercontent.com/piemonte/cpVision/master/pbj.gif)

## cpVision

`cpVision` is an iOS camera engine library that allows easy integration of special capture features and camera customization in your iOS app.

[![Build Status](https://api.travis-ci.org/piemonte/cpVision.svg?branch=master)](https://travis-ci.org/piemonte/cpVision)
[![Pod Version](https://img.shields.io/cocoapods/v/cpVision.svg?style=flat)](http://cocoadocs.org/docsets/cpVision/)

### Features
- [x] touch-to-record video capture
- [x] slow motion capture (120 fps on [supported hardware](https://www.apple.com/iphone/compare/))
- [x] photo capture
- [x] customizable UI and user interactions
- [x] ghosting (onion skinning) of last recorded segment
- [x] flash/torch support
- [x] white balance, focus, and exposure adjustment support
- [x] mirroring support

Capture is possible without having to use the touch-to-record gesture interaction as the sample project provides.

If you need a video player, check out [PBJVideoPlayer (obj-c)](https://github.com/piemonte/PBJVideoPlayer) and [Player (Swift)](https://github.com/piemonte/player).

Contributions are welcome!

### About

This library was originally created at [DIY](https://diy.org/) as a fun means for young people to author video and share their [skills](https://diy.org//skills). The touch-to-record interaction was originally pioneered by [Vine](https://vine.co/) and [Instagram](https://instagram.com/).

Thanks to everyone who has contributed and helped make this a fun project and community.

## Installation

### CocoaPods

`cpVision` is available and recommended for installation using the Cocoa dependency manager [CocoaPods](https://cocoapods.org/). 

To integrate, just add the following line to your `Podfile`:

```ruby
pod 'cpVision'
```

## Usage

Import the header.

```objective-c
#import "cpVision.h"
```

Setup the camera preview using `[[cpVision sharedInstance] previewLayer]`.

```objective-c
    // preview and AV layer
    _previewView = [[UIView alloc] initWithFrame:CGRectZero];
    _previewView.backgroundColor = [UIColor blackColor];
    CGRect previewFrame = CGRectMake(0, 60.0f, CGRectGetWidth(self.view.frame), CGRectGetWidth(self.view.frame));
    _previewView.frame = previewFrame;
    _previewLayer = [[cpVision sharedInstance] previewLayer];
    _previewLayer.frame = _previewView.bounds;
    _previewLayer.videoGravity = AVLayerVideoGravityResizeAspectFill;
    [_previewView.layer addSublayer:_previewLayer];
```

Setup and configure the `cpVision` controller, then start the camera preview.

```objective-c
- (void)_setup
{
    _longPressGestureRecognizer.enabled = YES;

    cpVision *vision = [cpVision sharedInstance];
    vision.delegate = self;
    vision.cameraMode = PBJCameraModeVideo;
    vision.cameraOrientation = PBJCameraOrientationPortrait;
    vision.focusMode = PBJFocusModeContinuousAutoFocus;
    vision.outputFormat = PBJOutputFormatSquare;

    [vision startPreview];
}
```

Start/pause/resume recording.

```objective-c
- (void)_handleLongPressGestureRecognizer:(UIGestureRecognizer *)gestureRecognizer
{
    switch (gestureRecognizer.state) {
      case UIGestureRecognizerStateBegan:
        {
            if (!_recording)
                [[cpVision sharedInstance] startVideoCapture];
            else
                [[cpVision sharedInstance] resumeVideoCapture];
            break;
        }
      case UIGestureRecognizerStateEnded:
      case UIGestureRecognizerStateCancelled:
      case UIGestureRecognizerStateFailed:
        {
            [[cpVision sharedInstance] pauseVideoCapture];
            break;
        }
      default:
        break;
    }
}
```

End recording.

```objective-c
    [[cpVision sharedInstance] endVideoCapture];
```

Handle the final video output or error accordingly.

```objective-c
- (void)vision:(cpVision *)vision capturedVideo:(NSDictionary *)videoDict error:(NSError *)error
{   
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
```

To specify an automatic end capture maximum duration, set the following property on the 'cpVision' controller.

```objective-c
    [[cpVision sharedInstance] setMaximumCaptureDuration:CMTimeMakeWithSeconds(5, 600)]; // ~ 5 seconds
```

To adjust the video quality and compression bit rate, modify the following properties on the `cpVision` controller.

```objective-c
    @property (nonatomic, copy) NSString *captureSessionPreset;

    @property (nonatomic) CGFloat videoBitRate;
    @property (nonatomic) NSInteger audioBitRate;
    @property (nonatomic) NSDictionary *additionalCompressionProperties;
```

## Community

- Need help? Use [Stack Overflow](http://stackoverflow.com/questions/tagged/pbjvision) with the tag 'pbjvision'.
- Questions? Use [Stack Overflow](http://stackoverflow.com/questions/tagged/pbjvision) with the tag 'pbjvision'.
- Found a bug? Open an [issue](https://github.com/piemonte/cpVision/issues).
- Feature idea? Open an [issue](https://github.com/piemonte/cpVision/issues).
- Want to contribute? Submit a [pull request](https://github.com/piemonte/cpVision/blob/master/CONTRIBUTING.md).

## Resources

* [AV Foundation Programming Guide](https://developer.apple.com/library/ios/documentation/AudioVideo/Conceptual/AVFoundationPG/Articles/00_Introduction.html)
* [AV Foundation Framework Reference](https://developer.apple.com/library/ios/documentation/AVFoundation/Reference/AVFoundationFramework/)
* [objc.io Camera and Photos](https://www.objc.io/issues/21-camera-and-photos/)
* [objc.io Video] (https://www.objc.io/issues/23-video/)
* [PBJVideoPlayer, a simple iOS video player in Objective-C](https://github.com/piemonte/PBJVideoPlayer)
* [Player, a simple iOS video player in Swift](https://github.com/piemonte/player)

## License

cpVision is available under the MIT license, see the [LICENSE](https://github.com/piemonte/cpVision/blob/master/LICENSE) file for more information.
