//
//  VisibleAppDelegateImpl.m
//  Visible
//
//  Created by Arman Garakani on 8/11/18.
//


#import "cinder/app/cocoa/AppImplMac.h"
#import <HockeySDK/HockeySDK.h>


using namespace cinder;
using namespace cinder::app;


@interface VisibleAppImpl :  AppImplMac

@end

@implementation VisibleAppImpl

- (void)applicationDidFinishLaunching:(NSNotification *)notification
{
    mApp->getRenderer()->makeCurrentContext();
    
    // update positions for all windows, their frames might not have been correct when created.
    for( WindowImplBasicCocoa* winIt in mWindows ) {
        [winIt updatePosRelativeToPrimaryDisplay];
    }
    
    mApp->privateSetup__();
    
    //   [Fabric with:@[[Answers class], [Crashlytics class]]];
    [[BITHockeyManager sharedHockeyManager] configureWithIdentifier:@"9614386b13784f67946728d5bc17f056"];
    // Do some additional configuration if needed here
    [[BITHockeyManager sharedHockeyManager] startManager];
    
    // call initial window resize signals
    for( WindowImplBasicCocoa* winIt in mWindows ) {
        [winIt->mCinderView makeCurrentContext];
        [self setActiveWindow:winIt];
        winIt->mWindowRef->emitResize();
    }
    
    // when available, make the first window the active window
    [self setActiveWindow:[mWindows firstObject]];
    [self startAnimationTimer];
}



@end
