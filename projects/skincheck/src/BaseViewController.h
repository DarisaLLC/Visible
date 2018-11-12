

#import <UIKit/UIKit.h>


@interface BaseViewController : UINavigationController

- (void)updateFpsLabel:(int)fps;

- (void)updateLensLabel:(float)lensValue;

- (void)lensPositionChanged:(UISlider *)slider;


@end
