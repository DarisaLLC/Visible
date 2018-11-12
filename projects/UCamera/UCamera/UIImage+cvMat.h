
#import <UIKit/UIKit.h>
#include <opencv2/opencv.hpp>

@interface UIImage (cvMat)

+ (CGRect) fromCvRect:(cv::Rect) cvRect;

+ (cv::Rect) fromCGRect:(CGRect) cgRect;
    
// Crop with no Orinetation Fix.
+ (UIImage *)crop:(UIImage*) inputImage :(CGRect*)rect;
    
+ (UIImage *)imageWithCvMat:(const cv::Mat &)mtx;

- (void)toCvMat:(cv::Mat &)mtx;

- (cv::Mat)cvMat;

@end
