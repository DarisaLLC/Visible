/*
    Copyright (C) 2016 Apple Inc. All Rights Reserved.
    See LICENSE.txt for this sample’s licensing information
    
    Abstract:
    Main class used to demonstrate reading/writing of assets.
 */

#ifndef __AVReader__
#define __AVReader__

#import <AppKit/AppKit.h>
#import <CoreMedia/CoreMedia.h>
#import <AVFoundation/AVFoundation.h>
#import "mediaInfo.h"

@class AVSampleBufferChannel;

typedef void (^image_callback)(CVPixelBufferRef pixels);
typedef void (^progress_callback)(CMTime progress);
typedef void (^done_callback)();


@interface AVReader : NSObject
{
  
@private

	AVAsset						*asset;
	AVAssetImageGenerator		*imageGenerator;
	CMTimeRange					timeRange;
	NSInteger					filterTag;
	dispatch_queue_t			serializationQueue;
	
	// Only accessed on the main thread
	NSURL						*outputURL;
	BOOL						writingSamples;

	// All of these are createed, accessed, and torn down exclusively on the serializaton queue
	AVAssetReader				*assetReader;
	AVAssetWriter				*assetWriter;
	AVSampleBufferChannel		*audioSampleBufferChannel;
	AVSampleBufferChannel		*videoSampleBufferChannel;
	BOOL						cancelled;
}

- (BOOL)                       setup:(NSString*) filepath;
- (void)                       start;
- (void)                       run;

@property (nonatomic, retain) AVAsset *asset;
@property (nonatomic) CMTimeRange timeRange;
@property (nonatomic, copy) NSURL *outputURL;
@property (nonatomic) tiny_media_info movieInfo;

@property (nonatomic, copy) image_callback  getNewFrame;
@property (nonatomic, copy) progress_callback getProgress;
@property (nonatomic, copy) done_callback getDone;

@property (nonatomic, getter=isWritingSamples) BOOL writingSamples;


@end


#endif


