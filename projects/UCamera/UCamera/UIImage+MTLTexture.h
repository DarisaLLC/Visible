
#import <UIKit/UIKit.h>
#import <MetalKit/MetalKit.h>

@interface UIImage (MTLTexture)

-(void) toMTLTexture:(id<MTLTexture>)texture;

@end
