
#import "AVReader.h"

/*
 * Notes:
 * this AV reader reads all after setup, startWithDelegate is called to request frame copy [assetReaderOutput copyNextSampleBuffer] repeatedly unless 
 * there is an error or we are done.
 * To update for a caching mechanism, we will have to run setup a toc with time ranges and then setup to go to a time point and capture N frames.
 * Additional API needed:
 *                  run_random_access (timpoint start, timepoint end)
 *                  using output reader - (void)resetForReadingTimeRanges:(NSArray<NSValue *> *)timeRanges NS_AVAILABLE(10_10, 8_0);
 *
 */


static inline CGFloat RadiansToDegrees(CGFloat radians) {
    return radians * 180 / M_PI;
};


static inline BOOL orientationFromTransform(CGAffineTransform transform)
{
    BOOL isPortrait = false;
    
    if (transform.a == 0 && transform.b == 1.0 && transform.c == -1.0 && transform.d == 0)
    {
        isPortrait = true;
    } else if (transform.a == 0 && transform.b == -1.0 && transform.c == 1.0 && transform.d == 0) {
        isPortrait = true;
    } else if (transform.a == 1.0 && transform.b == 0 && transform.c == 0 && transform.d == 1.0) {
    } else if (transform.a == -1.0 && transform.b == 0 && transform.c == 0 && transform.d == -1.0) {
    }
    return isPortrait;
}

static inline CGAffineTransform tansformForTrack(AVAssetTrack* assetTrack, CGRect bounds)
{
    
    CGAffineTransform transform = [assetTrack preferredTransform];
    BOOL isPortrait = orientationFromTransform(transform);
    CGAffineTransform updated;
    float scaleToFitRatio = bounds.size.width / assetTrack.naturalSize.width;
    if (isPortrait)
    {
        scaleToFitRatio = bounds.size.width / assetTrack.naturalSize.height;
        CGAffineTransform scaleFactor = CGAffineTransformMakeScale(scaleToFitRatio, scaleToFitRatio);
        updated = CGAffineTransformConcat(assetTrack.preferredTransform, scaleFactor);
    }
    else
    {
        CGAffineTransform scaleFactor = CGAffineTransformMakeScale(scaleToFitRatio, scaleToFitRatio);
        updated = CGAffineTransformConcat(CGAffineTransformConcat(assetTrack.preferredTransform, scaleFactor),
                                          CGAffineTransformMakeTranslation(0, bounds.size.width / 2));
        
        float orient = RadiansToDegrees(atan2(transform.b, transform.a));
        
        if (orient == -90.0)
        {
            CGAffineTransform fixUpsideDown = CGAffineTransformMakeRotation(CGFloat(M_PI));
            float yFix = assetTrack.naturalSize.height + bounds.size.height;
            CGAffineTransform centerFix = CGAffineTransformMakeTranslation(assetTrack.naturalSize.width, yFix);
            updated = CGAffineTransformConcat(CGAffineTransformConcat(fixUpsideDown, centerFix), scaleFactor);
        }
    }
    
    return updated;
}

@protocol AVSampleBufferChannelDelegate;

@interface AVSampleBufferChannel : NSObject
{
@private
	AVAssetReaderOutput		*assetReaderOutput;
	AVAssetWriterInput		*assetWriterInput;
	
	dispatch_block_t		completionHandler;
	dispatch_queue_t		serializationQueue;
	BOOL					finished;  // only accessed on serialization queue
}
- (id)initWithAssetReaderOutput:(AVAssetReaderOutput *)assetReaderOutput;
@property (nonatomic, readonly) NSString *mediaType;
- (void)startWithDelegate:(id <AVSampleBufferChannelDelegate>)delegate completionHandler:(dispatch_block_t)completionHandler;  // delegate is retained until completion handler is called.  Completion handler is guaranteed to be called exactly once, whether reading/writing finishes, fails, or is cancelled.  Delegate may be nil.
- (void)cancel;
@end


@protocol AVSampleBufferChannelDelegate <NSObject>
@required
- (void)sampleBufferChannel:(AVSampleBufferChannel *)sampleBufferChannel didReadSampleBuffer:(CMSampleBufferRef)sampleBuffer;
@end


@interface AVReader () <AVSampleBufferChannelDelegate>
// These three methods are always called on the serialization dispatch queue
- (BOOL)setUpReaderAndWriterReturningError:(NSError **)outError;  // make sure "tracks" key of asset is loaded before calling this
- (BOOL)startReadingAndWritingReturningError:(NSError **)outError;
- (void)readingAndWritingDidFinishSuccessfully:(BOOL)success withError:(NSError *)error;
@end


@implementation AVReader

+ (NSArray *)readableTypes
{
	return [AVURLAsset audiovisualTypes];
}

+ (BOOL)canConcurrentlyReadDocumentsOfType:(NSString *)typeName
{
	return YES;
}

- (id)init
{
    self = [super init];
    
	if (self)
	{
		NSString *serializationQueueDescription = [NSString stringWithFormat:@"%@ serialization queue", self];
		serializationQueue = dispatch_queue_create([serializationQueueDescription UTF8String], NULL);
    }
    
	return self;
}

- (void)dealloc
{


}


- (BOOL)readFromURL:(NSURL *)url ofType:(NSString *)typeName error:(NSError **)outError
{
	NSDictionary *assetOptions = [NSDictionary dictionaryWithObject:[NSNumber numberWithBool:YES] forKey:AVURLAssetPreferPreciseDurationAndTimingKey];
	AVAsset *localAsset = [AVURLAsset URLAssetWithURL:url options:assetOptions];
	[self setAsset:localAsset];
	if (localAsset)
		imageGenerator = [[AVAssetImageGenerator alloc] initWithAsset:localAsset];
	
	return (localAsset != nil);
}


@synthesize asset=asset;
@synthesize timeRange=timeRange;
@synthesize writingSamples=writingSamples;
@synthesize outputURL=outputURL;
@synthesize getNewFrame=_getNewFrame;
@synthesize getProgress=_getProgress;
@synthesize getDone=_getDone;
@synthesize movieInfo=_movieInfo;


- (BOOL)setup:(NSString*) videoURL
{
    
    static NSDictionary* supportedTypes = nil;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        supportedTypes = @{
                         @"mov": @"com.apple.quicktime-movie",
                         @"mp4": @"public.mpeg-4",
                         };
    });
    
	cancelled = NO;
    
    NSString *extension = [videoURL pathExtension];
    NSString* supported = supportedTypes[extension];
    if (! supported) return FALSE;
    NSError *localError = nil;
    NSURL *url = [NSURL URLWithString: videoURL];
    BOOL check = [self readFromURL:url ofType:supported error:&localError];
    if (check)
        [self start];
    
    return check;
}


- (void)start
{

    NSError *localError = nil;
    
    // Set up the AVAssetReader and AVAssetWriter, then begin writing samples or flag an error
    [self setUpReaderAndWriterReturningError:&localError];
}


- (void)run
{
    
    AVAsset *localAsset = [self asset];
    [localAsset loadValuesAsynchronouslyForKeys:[NSArray arrayWithObjects:@"tracks", @"duration", nil] completionHandler:^{
        // Dispatch the setup work to the serialization queue, to ensure this work is serialized with potential cancellation
        dispatch_async(serializationQueue, ^{
            // Since we are doing these things asynchronously, the user may have already cancelled on the main thread.  In that case, simply return from this block
            if (cancelled)
                return;
            
            BOOL success = YES;
            NSError *localError = nil;
            
            success = ([localAsset statusOfValueForKey:@"tracks" error:&localError] == AVKeyValueStatusLoaded);
            if (success)
                success = ([localAsset statusOfValueForKey:@"duration" error:&localError] == AVKeyValueStatusLoaded);
            
            // Set up the AVAssetReader and AVAssetWriter, then begin writing samples or flag an error
            if (success)
                success = [self setUpReaderAndWriterReturningError:&localError];
            if (success)
                success = [self startReadingAndWritingReturningError:&localError];
            if (!success)
                [self readingAndWritingDidFinishSuccessfully:success withError:localError];
        });
    }];
}



- (BOOL)setUpReaderAndWriterReturningError:(NSError **)outError
{
	BOOL success = YES;
	NSError *localError = nil;
	AVAsset *localAsset = [self asset];
	
	// Create asset reader
	assetReader = [[AVAssetReader alloc] initWithAsset:asset error:&localError];
	success = (assetReader != nil);


	// Create asset reader outputs and asset writer inputs for the first audio track and first video track of the asset
	if (success)
	{
		AVAssetTrack *audioTrack = nil, *videoTrack = nil;
		
		// Grab first audio track and first video track, if the asset has them
		NSArray *audioTracks = [localAsset tracksWithMediaType:AVMediaTypeAudio];
		if ([audioTracks count] > 0)
			audioTrack = [audioTracks objectAtIndex:0];
		NSArray *videoTracks = [localAsset tracksWithMediaType:AVMediaTypeVideo];
		if ([videoTracks count] > 0)
			videoTrack = [videoTracks objectAtIndex:0];
		
		if (audioTrack)
		{
			// Decompress to Linear PCM with the asset reader
			NSDictionary *decompressionAudioSettings = [NSDictionary dictionaryWithObjectsAndKeys:
														[NSNumber numberWithUnsignedInt:kAudioFormatLinearPCM], AVFormatIDKey,
														nil];
			AVAssetReaderOutput *output = [AVAssetReaderTrackOutput assetReaderTrackOutputWithTrack:audioTrack outputSettings:decompressionAudioSettings];
			[assetReader addOutput:output];
			
            // No writing of audio for now
			// Create and save an instance of AVSampleBufferChannel, which will coordinate the work of reading and writing sample buffers
            audioSampleBufferChannel = [[AVSampleBufferChannel alloc] initWithAssetReaderOutput:output];
		}
		
		if (videoTrack)
		{
            _movieInfo.embedded = videoTrack.preferredTransform;
            _movieInfo.orientation = RadiansToDegrees(atan2(_movieInfo.embedded.b, _movieInfo.embedded.a));
            _movieInfo.size = CGSizeApplyAffineTransform(videoTrack.naturalSize, _movieInfo.embedded);
            _movieInfo.mFps = videoTrack.nominalFrameRate;
            CGRect r;
            r.origin = CGPointZero;
            r.size = _movieInfo.size;
            _movieInfo.toSize = tansformForTrack(videoTrack, r);
            timeRange = videoTrack.timeRange;

         
            _movieInfo.duration = videoTrack.timeRange.duration.value/videoTrack.timeRange.duration.timescale;
            _movieInfo.count = _movieInfo.getDuration() * _movieInfo.getFramerate();
            
			// Decompress to ARGB with the asset reader
			NSDictionary *decompressionVideoSettings = [NSDictionary dictionaryWithObjectsAndKeys:
														[NSNumber numberWithUnsignedInt:kCVPixelFormatType_32ARGB], (id)kCVPixelBufferPixelFormatTypeKey,
														[NSDictionary dictionary], (id)kCVPixelBufferIOSurfacePropertiesKey,
														nil];
			AVAssetReaderOutput *output = [AVAssetReaderTrackOutput assetReaderTrackOutputWithTrack:videoTrack outputSettings:decompressionVideoSettings];
			[assetReader addOutput:output];
			
					
			// Create and save an instance of AVSampleBufferChannel, which will coordinate the work of reading and writing sample buffers
            videoSampleBufferChannel = [[AVSampleBufferChannel alloc] initWithAssetReaderOutput:output];
		}
	}
	
	if (outError)
		*outError = localError;
	
	return success;
}

- (BOOL)startReadingAndWritingReturningError:(NSError **)outError
{
	BOOL success = YES;
	NSError *localError = nil;

	// Instruct the asset reader and asset writer to get ready to do work
	success = [assetReader startReading];
	if (!success)
		localError = [assetReader error];
		
	if (success)
	{
		dispatch_group_t dispatchGroup = dispatch_group_create();

		// Start a sample-writing session
		//[assetWriter startSessionAtSourceTime:[self timeRange].start];
		
		// Start reading and writing samples
		if (audioSampleBufferChannel)
		{
			// Only set audio delegate for audio-only assets, else let the video channel drive progress
			id <AVSampleBufferChannelDelegate> delegate = nil;
			if (!videoSampleBufferChannel)
				delegate = self;

			dispatch_group_enter(dispatchGroup);
			[audioSampleBufferChannel startWithDelegate:delegate completionHandler:^{
				dispatch_group_leave(dispatchGroup);
			}];
		}
		if (videoSampleBufferChannel)
		{
			dispatch_group_enter(dispatchGroup);
			[videoSampleBufferChannel startWithDelegate:self completionHandler:^{
				dispatch_group_leave(dispatchGroup);
			}];
		}
		
		// Set up a callback for when the sample writing is finished
		dispatch_group_notify(dispatchGroup, serializationQueue, ^{
			BOOL finalSuccess = YES;
			NSError *finalError = nil;
			
			if (cancelled)
			{
				[assetReader cancelReading];
			}
			else
			{
                AVAssetReaderStatus status = [assetReader status];
                switch(status)
                {
                    case AVAssetReaderStatusCompleted:
                    {
                        finalSuccess = YES;
                        finalError = [assetReader error];
                            break;
                    }
                    case AVAssetReaderStatusFailed:
                    case AVAssetReaderStatusCancelled:
                    case AVAssetReaderStatusUnknown:
                    {
                        finalSuccess = NO;
                        finalError = [assetReader error];
                        break;
                    }
                    case AVAssetReaderStatusReading:
                        return;
                }
            }

			[self readingAndWritingDidFinishSuccessfully:finalSuccess withError:finalError];
		});
		
	}
	
	if (outError)
		*outError = localError;
	
	return success;
}

- (void)cancel:(id)sender
{
	// Dispatch cancellation tasks to the serialization queue to avoid races with setup and teardown
	dispatch_async(serializationQueue, ^{
		[audioSampleBufferChannel cancel];
		[videoSampleBufferChannel cancel];
		cancelled = YES;
	});
}

- (void)readingAndWritingDidFinishSuccessfully:(BOOL)success withError:(NSError *)error
{
	if (!success)
	{
		[assetReader cancelReading];
	}
	
	// Tear down ivars
	//[assetReader release];
	assetReader = nil;
	//[assetWriter release];
	assetWriter = nil;
	//[audioSampleBufferChannel release];
	audioSampleBufferChannel = nil;
	//[videoSampleBufferChannel release];
	videoSampleBufferChannel = nil;
	cancelled = NO;
	
    if(_getDone)
        _getDone ();
  
}

- (void)alertDidEnd:(NSAlert *)alert returnCode:(NSInteger)returnCode contextInfo:(void *)contextInfo
{
	// Do nothing
}

static bool progressOfSampleBufferInTimeRange(CMSampleBufferRef sampleBuffer, CMTimeRange timeRange, CMTime* progress)
{
    if (progress)
    {
        *progress = CMSampleBufferGetPresentationTimeStamp(sampleBuffer);
        return YES;
    }
    return NO;

}


/*
 * is run on a new sample. If there is a progress callback, it will be called. If there is a getnewFrame callback, it will be called.
 *
 */

- (void)sampleBufferChannel:(AVSampleBufferChannel *)sampleBufferChannel didReadSampleBuffer:(CMSampleBufferRef)sampleBuffer
{
	CVPixelBufferRef pixelBuffer = NULL;
    CMTime progress = kCMTimeInvalid;
	
	if (_getProgress && progressOfSampleBufferInTimeRange(sampleBuffer, [self timeRange], &progress))
        _getProgress(progress);
    
	
	// Grab the pixel buffer from the sample buffer, if possible
	CVImageBufferRef imageBuffer = CMSampleBufferGetImageBuffer(sampleBuffer);
	if (imageBuffer && (CFGetTypeID(imageBuffer) == CVPixelBufferGetTypeID()))
	{
		pixelBuffer = (CVPixelBufferRef)imageBuffer;
        if (_getNewFrame)
            _getNewFrame(pixelBuffer);
	}
}

@end


@interface AVSampleBufferChannel ()
- (void)callCompletionHandlerIfNecessary;  // always called on the serialization queue
@end

@implementation AVSampleBufferChannel

- (id)initWithAssetReaderOutput:(AVAssetReaderOutput *)localAssetReaderOutput
{
	self = [super init];
	
	if (self)
	{
		assetReaderOutput = localAssetReaderOutput;
		
		finished = NO;
		NSString *serializationQueueDescription = [NSString stringWithFormat:@"%@ serialization queue", self];
		serializationQueue = dispatch_queue_create([serializationQueueDescription UTF8String], NULL);
	}
	
	return self;
}

- (void)dealloc
{
	//if (serializationQueue)
	//	dispatch_release(serializationQueue);
	
	//[super dealloc];
}

- (NSString *)mediaType
{
	return [assetReaderOutput mediaType];
}

- (void)startWithDelegate:(id <AVSampleBufferChannelDelegate>)delegate completionHandler:(dispatch_block_t)localCompletionHandler
{
	completionHandler = [localCompletionHandler copy];  // released in -callCompletionHandlerIfNecessary

    dispatch_async(serializationQueue , ^{
        
		if (finished)
			return;
		
		BOOL completedOrFailed = NO;
		
		// Read samples in a loop as long as the asset writer input is ready
		while (!completedOrFailed)
		{
			CMSampleBufferRef sampleBuffer = [assetReaderOutput copyNextSampleBuffer];
			if (sampleBuffer != NULL)
			{
				if ([delegate respondsToSelector:@selector(sampleBufferChannel:didReadSampleBuffer:)])
					[delegate sampleBufferChannel:self didReadSampleBuffer:sampleBuffer];
				
                completedOrFailed = NO;
			}
			else
			{
				completedOrFailed = YES;
			}
		}
		
		if (completedOrFailed)
			[self callCompletionHandlerIfNecessary];
    });
}

- (void)cancel
{
	dispatch_async(serializationQueue, ^{
		[self callCompletionHandlerIfNecessary];
	});
}

- (void)callCompletionHandlerIfNecessary
{
	// Set state to mark that we no longer need to call the completion handler, grab the completion handler, and clear out the ivar
	BOOL oldFinished = finished;
	finished = YES;

	if (oldFinished == NO)
	{
        // ? Perhaps another callback for ending
        //		[assetWriterInput markAsFinished];  // let the asset writer know that we will not be appending any more samples to this input

        dispatch_block_t localCompletionHandler = completionHandler;

		if (localCompletionHandler)
		{
			localCompletionHandler();
		}
	}
}




@end
