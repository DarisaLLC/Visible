
#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import "siteCommon.h"
#include <CoreMedia/CMTime.h>


NS_ASSUME_NONNULL_BEGIN



@interface CVSiteTemplate : NSObject

- (instancetype)initWithTitle:(NSString*)title;
- (NSString*)getTitle;
- (void)setTitle:(NSString*)title;

// Free Function
- (colorStats_t)getColorStats:(UIImage*) currentImage : (CGRect*) roi : (CMTime*) currentTimestamp;
    
// Stateful Functions
// x, y, w, h in normalized (0 - 1)
// Trains initial template
- (bool)setNormalizedTrainingRect:(float) x : (float) y : (float) width : (float) height;
- (bool)train:(UIImage*) currentImage : (CMTime*) currentTimestamp;



- (void)reset: (float) width : (float) height;
- (void)setLensPosition:(float) position;
- (float)getLensPosition;
- (void)unsetLensPosition;

- (int32_t)getWidth;
- (int32_t)getHeight;

- (bool) start;
- (CGPoint) step:(UIImage*) currentImage : (CMTime*) currentTimestamp;
- (bool) stop;
- (bool) isRunning;
- (bool) hasStopped;
- (bool) hasStarted;
- (bool) isUninitialized;


@end
NS_ASSUME_NONNULL_END


