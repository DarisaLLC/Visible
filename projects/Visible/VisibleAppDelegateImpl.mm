//
//  VisibleAppDelegateImpl.m
//  Visible
//
//  Created by Arman Garakani on 8/11/18.
//

#import <Foundation/Foundation.h>
@interface CBAppDelegateImpl : AppDelegateImpl

@end

@implementation CBAppDelegateImpl

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {
    mApp = cinder::app::AppCocoaTouch::get();
    mAppImpl = mApp->privateGetImpl();
    
    [UIApplication sharedApplication].statusBarHidden = mAppImpl->mStatusBarShouldHide;
    
    for( auto &windowImpl : mAppImpl->mWindows )
        [windowImpl finishLoad];
    
    mApp->privateSetup__();
    mAppImpl->mSetupHasFired = YES;
    
    [mAppImpl startAnimation];
    
    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    NSString *documentsDirectory = [paths objectAtIndex:0];
    NSString *presetsPath = [documentsDirectory stringByAppendingPathComponent:@"presets"];
    
    if(![[NSFileManager defaultManager] fileExistsAtPath:presetsPath]) {
        NSError *error = nil;
        [[NSFileManager defaultManager] createDirectoryAtPath:presetsPath
                                  withIntermediateDirectories:YES
                                                   attributes:nil error:&error];
    }
    
    [Fabric with:@[[Answers class], [Crashlytics class]]];
    
    return YES;
}

@end
