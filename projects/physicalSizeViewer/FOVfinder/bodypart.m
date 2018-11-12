//
//  bodypart.m
//  physicalSizeViewer
//
//  Created by Arman Garakani on 8/21/17.
//  Copyright © 2017 Darisa LLC. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "bodypart.h"

@implementation BodyPart
@synthesize title;
@synthesize depth;
@synthesize distance;
@synthesize size;

- (id)initWithTitle:(NSString *)name size:(CGSize) area distance:(CGFloat) away depth:(CGFloat) zz {
    if (self = [super init]) {
        self.title = name;
        self.distance = away;
        self.depth = zz;
        self.size = area;
    }
    return self;
}

@end
