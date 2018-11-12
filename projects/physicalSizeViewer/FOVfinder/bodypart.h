//
//  bodypart.h
//  physicalSizeViewer
//
//  Created by Arman Garakani on 8/21/17.
//  Copyright © 2017 Darisa LLC. All rights reserved.
//

#ifndef bodypart_h
#define bodypart_h

#import <Foundation/Foundation.h>

@interface BodyPart : NSObject
@property (nonatomic, strong) NSString *title;

@property (assign, nonatomic, readwrite) CGFloat depth;
@property (assign, nonatomic, readwrite) CGFloat distance;
@property (assign, nonatomic, readwrite) CGSize size;
@property (assign, nonatomic, readwrite) int idx;

- (id)initWithTitle:(NSString *)title size:(CGSize) area distance:(CGFloat) away depth:(CGFloat) zz;

@end


#endif /* bodypart_h */
