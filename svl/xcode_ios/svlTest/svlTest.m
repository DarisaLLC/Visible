//
//  svlTest.m
//  svlTest
//
//  Created by Arman Garakani on 5/23/16.
//  Copyright © 2016 Arman Garakani. All rights reserved.
//

#include <iostream>
#include "svTest.h"



using namespace svl;
using namespace ci;


using namespace svl;
using namespace ci;


template svl::roiWindow<P8U> svl::image_io_read(const boost::filesystem::path & pp);






const char * pi341[] =
{
    "0000000000000000",
    "0000000000000000",
    "0000000000000000",
    "0000000000000000",
    "0000011110000000",
    "0000001111000000",
    "0000000111100000",
    "0000000011110000",
    "0000000000000000"
    "0000000000000000",
    "0000000000000000",
    "0000000000000000", 0};

const char * pi341234[] =
{
    "0000000000000000",
    "0000000000000000",
    "0000000000000000",
    "0000000000000000",
    "0000012340000000",
    "0000001234000000",
    "0000000123400000",
    "0000000012340000",
    "0000000000000000"
    "0000000000000000",
    "0000000000000000",
    "0000000000000000", 0};



//TEST(basicTheta, angle_units)
//{
//	for (int aa = 0; aa <= 255; aa++)
//	{
//		uAngle8 a8(aa);
//		uRadian r(a8);
//		std::cout << (int)a8.basic() << " ~~~ " << r.Double() << std::endl;
//	}
//}
//

#if 0
TEST (ut_similarity, run)
{
    UT_similarity tester;
    tester.run();
}
#endif


#import <XCTest/XCTest.h>

@interface svlTest : XCTestCase

@end

@implementation svlTest

- (void)setUp {
    [super setUp];
    // Put setup code here. This method is called before the invocation of each test method in the class.
}

- (void)tearDown {
    // Put teardown code here. This method is called after the invocation of each test method in the class.
    [super tearDown];
}

- (void)testExample {

    
    
    // This is an example of a functional test case.
    // Use XCTAssert and related functions to verify your tests produce the correct results.
}

- (void)testPerformanceExample {
    // This is an example of a performance test case.
    [self measureBlock:^{
        // Put the code you want to measure the time of here.
    }];
}

@end
