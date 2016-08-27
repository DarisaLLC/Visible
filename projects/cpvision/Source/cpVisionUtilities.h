//
//  cpVisionUtilities.h
//  Vision
//

#import <Foundation/Foundation.h>
#import <AVFoundation/AVFoundation.h>

@interface cpVisionUtilities : NSObject

// coordinate conversion

+ (CGPoint)convertToPointOfInterestFromViewCoordinates:(CGPoint)viewCoordinates inFrame:(CGRect)frame;

// devices and connections

+ (AVCaptureDevice *)captureDeviceForPosition:(AVCaptureDevicePosition)position;
+ (AVCaptureDevice *)audioDevice;
+ (AVCaptureConnection *)connectionWithMediaType:(NSString *)mediaType fromConnections:(NSArray *)connections;

// sample buffers

+ (CMSampleBufferRef)createOffsetSampleBufferWithSampleBuffer:(CMSampleBufferRef)sampleBuffer withTimeOffset:(CMTime)timeOffset;

// orientation

+ (CGFloat)angleOffsetFromPortraitOrientationToOrientation:(AVCaptureVideoOrientation)orientation;

// storage

+ (uint64_t)availableDiskSpaceInBytes;

@end

@interface NSString (cpExtras)

+ (NSString *)cpformattedTimestampStringFromDate:(NSDate *)date;

@end
