#import "BaseViewController.h"
//#include <IOKit/hid/IOHIDLib.h>
#import "hid/IOHIDEventSystemClient.h"
#import "hid/IOHIDEventSystem.h"
#import <notify.h>
#import <dlfcn.h>

IOHIDEventSystemClientRef IOHIDEventSystemClientCreate(CFAllocatorRef allocator);
int IOHIDEventSystemClientSetMatching(IOHIDEventSystemClientRef client, CFDictionaryRef match);
CFArrayRef IOHIDEventSystemClientCopyServices(IOHIDEventSystemClientRef, int);
typedef struct __IOHIDServiceClient * IOHIDServiceClientRef;
int IOHIDServiceClientSetProperty(IOHIDServiceClientRef, CFStringRef, CFNumberRef);

@interface BaseViewController ()
{
    UILabel *_luxLabel;
    UILabel *_fpsLabel;
    UILabel *_lensLabel;
    UISlider *_lensPositioner;
    CGRect  _draggingBox;
}


@end
@implementation BaseViewController

// note: these synthesizers aren't necessary with Clang 3.0 since it will autogenerate the same thing, but it is added for
// -------------------------------------------------------------------------------------------------
#pragma mark - UIViewController overridden methods

- (void)viewDidLoad
{
	[super viewDidLoad];

    void *uikit = dlopen("/System/Library/Framework/UIKit.framework/UIKit", RTLD_LAZY);
    if (!uikit)
    {
        NSLog(@"AutoBrightness: Failed to open UIKit framework!");
        
    }
    
    UILabel *lux = [[UILabel alloc] initWithFrame:CGRectMake(20, 160, 200, 40)];
    [lux setText:@"Ambient Light (lux): "];
    [self.view addSubview:lux];
    
    UILabel *luxLabel = [[UILabel alloc] initWithFrame:CGRectMake(180, 160, 200, 40)];
    [luxLabel setText:@""];
    [lux setTextColor:[UIColor yellowColor]];
    [self.view addSubview:luxLabel];
    _luxLabel = luxLabel;

    UILabel *fps = [[UILabel alloc] initWithFrame:CGRectMake(20, 180, 200, 40)];
    [fps setText:@"Fps : "];
    [self.view addSubview:fps];
    
    UILabel *fpsLabel = [[UILabel alloc] initWithFrame:CGRectMake(180, 180, 200, 40)];
    [fpsLabel setText:@""];
    [fps setTextColor:[UIColor yellowColor]];
    [self.view addSubview:fpsLabel];
    _fpsLabel = fpsLabel;
    
    UILabel *lens = [[UILabel alloc] initWithFrame:CGRectMake(20, 200, 200, 40)];
    [lens setText:@"lens : "];
    [self.view addSubview:lens];
    
    UILabel *lensLabel = [[UILabel alloc] initWithFrame:CGRectMake(180, 200, 200, 40)];
    [lensLabel setText:@""];
    [lens setTextColor:[UIColor yellowColor]];
    [self.view addSubview:lensLabel];
    _lensLabel = lensLabel;
    
    [self registerForALSEvents];
    
	self.toolbarHidden = NO;
	UIColor *tintColor = [UIColor colorWithRed:0.0f green:0.0f blue:0.0f alpha:1.0f];
	self.navigationBar.tintColor = tintColor;

	self.toolbar.tintColor = tintColor;
    
    UISlider *lensPositioner = [[UISlider alloc] initWithFrame:CGRectMake(20, 240, 200, 40)];
    [lensPositioner setContinuous:YES];
    [self.view addSubview:lensPositioner];
    [lensPositioner setMinimumValue:0.0f];
    [lensPositioner setMaximumValue:1.0f];
    [lensPositioner addTarget:self action:@selector(lensPositionChanged:) forControlEvents:UIControlEventValueChanged];
    _lensPositioner= lensPositioner;
    
    _draggingBox   = CGRectMake(100.0, 200.0, 64.0, 32.0);

}

- (void)updateLuxLabel:(int)luxValue {
    [_luxLabel setTextColor:[UIColor yellowColor]];
    [_luxLabel setText:[NSString stringWithFormat:@"%d", luxValue]];
}

- (void)updateFpsLabel:(int)fpsValue {
  [_luxLabel setTextColor:[UIColor yellowColor]];
    [_fpsLabel setText:[NSString stringWithFormat:@"%d", fpsValue]];
}

- (void)updateLensLabel:(float)lensValue {
  [_luxLabel setTextColor:[UIColor yellowColor]];    
    [_lensLabel setText:[NSString stringWithFormat:@"%1.3f", lensValue]];
}

void handle_event(void* target, void* refcon, IOHIDEventQueueRef queue, IOHIDEventRef event) {
    if (IOHIDEventGetType(event)==kIOHIDEventTypeAmbientLightSensor){ // Ambient Light Sensor Event
        
        int luxValue=IOHIDEventGetIntegerValue(event, (IOHIDEventField)kIOHIDEventFieldAmbientLightSensorLevel); // lux Event Field
        
        if (refcon != NULL) {
                        BaseViewController *rootController = (__bridge BaseViewController *)refcon;
                        [rootController updateLuxLabel:luxValue];
                        refcon = NULL;
        }
    }
}

//- (void)lensPositionChanged:(UISlider *)slider {
//dispatch_async(dispatch_get_main_queue(), ^(void) {
//}
//               

- (void)registerForALSEvents {
    int pv1 = 0xff00;
    int pv2 = 4;
    CFNumberRef mVals[2];
    CFStringRef mKeys[2];
    
    mVals[0] = CFNumberCreate(CFAllocatorGetDefault(), kCFNumberSInt32Type, &pv1);
    mVals[1] = CFNumberCreate(CFAllocatorGetDefault(), kCFNumberSInt32Type, &pv2);
    mKeys[0] = CFStringCreateWithCString(0, "PrimaryUsagePage", 0);
    mKeys[1] = CFStringCreateWithCString(0, "PrimaryUsage", 0);
    
    CFDictionaryRef matchInfo = CFDictionaryCreate(CFAllocatorGetDefault(),(const void**)mKeys,(const void**)mVals, 2, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    
    IOHIDEventSystemClientRef s_hidSysC = IOHIDEventSystemClientCreate(kCFAllocatorDefault);
    IOHIDEventSystemClientSetMatching(s_hidSysC,matchInfo);
    
    CFArrayRef matchingsrvs = IOHIDEventSystemClientCopyServices(s_hidSysC,0);
    
    if (CFArrayGetCount(matchingsrvs) == 0)
    {
        NSLog(@"AutoBrightness: ALS Not found!");
    }
    
    
    IOHIDServiceClientRef alssc = (IOHIDServiceClientRef)CFArrayGetValueAtIndex(matchingsrvs, 0);
    
    int ri = 1 * 1000000;
    CFNumberRef interval = CFNumberCreate(CFAllocatorGetDefault(), kCFNumberIntType, &ri);
    IOHIDServiceClientSetProperty(alssc,CFSTR("ReportInterval"),interval);
    
    IOHIDEventSystemClientScheduleWithRunLoop(s_hidSysC, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
    IOHIDEventSystemClientRegisterEventCallback(s_hidSysC, handle_event, NULL, (void *)CFBridgingRetain(self));
}


// pre iOS 6
- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)toInterfaceOrientation
{
	return YES;
}

// iOS 6+
#if __IPHONE_OS_VERSION_MAX_ALLOWED >= 60000
- (NSUInteger)supportedInterfaceOrientations
{
	return UIInterfaceOrientationMaskAll;
}
#endif

-(void)drawRect:(CGRect)dirtyRect
{
    if ( _draggingBox != CGRectNull)
    {
        [[NSColor grayColor] setStroke];
        [[NSBezierPath bezierPathWithRect:self.draggingBox] stroke];
    }
}

#pragma mark Mouse Events

- (void)mouseDown:(UIEvent *)theEvent
{
    CGPoint pointInView = [self convertPoint:[theEvent locationInWindow] fromView:nil];
    self.draggingBox = NSMakeRect(pointInView.x, pointInView.y, 0, 0);
    [self setNeedsDisplay:YES];
}

- (void)mouseDragged:(UIEvent *)theEvent
{
    CGPoint pointInView = [self convertPoint:[theEvent locationInWindow] fromView:nil];
    _draggingBox.size.width = pointInView.x - (self.draggingBox.origin.x);
    _draggingBox.size.height = pointInView.y - (self.draggingBox.origin.y);
    [self setNeedsDisplay:YES];
}

- (void)mouseUp:(UIEvent *)theEvent
{
    self.draggingBox = NSZeroRect;
    [self setNeedsDisplay:YES];
}

@end
