#ifndef __LOCALVARTEST__
#define __LOCALVARTEST__



#include <stdio.h>
#include <stdlib.h>
#include "gtest/gtest.h"
#include <opencv2/imgproc/imgproc.hpp>  // cvtColor
#include "opencv2/core/core_c.h"
#include "vision/localvariance.h"
using namespace svl;


void free_print_image_float (const IplImage* src)
{
    printf (" [  ] ");
    for(int col = 0; col < src->width; col++)
    {
        printf("%5d", col);
        
    }
    printf("\n");
    
    for(int row = 0; row < src->height; row++)
    {
        printf (" [%2d] ", row);
        for(int col = 0; col < src->width; col++)
        {
            printf(" % 4.1f", ((float*)(src->imageData + src->widthStep*row))[col]);
            
        }
        printf("\n");
    }
}

class ut_localvar
{
    
public:
    
    ut_localvar()
    {
        
    }
    void test_method ()
    {
        test_unit ();
        test_tiny ();
        
    }
    
    void test_unit ()
    {
        cv::Mat src1 = cv::Mat::ones(23, 31, CV_8UC1); // 2 rows, 3 columns
        cv::Mat result;
        
        cv::Size ap (3, 3);
        svl::localVAR tv (ap);
        tv.process (src1,  result);
        IplImage ires = result;
        EXPECT_EQ(src1.cols - ap.width + 1 , result.cols);
        EXPECT_EQ(src1.rows - ap.height + 1 , result.rows);
        
        //    free_print_image_float(&ires);
        
        int rc = 15, cc = 11;
        src1.at<uint8_t>(rc, cc) = 255;
        EXPECT_TRUE(tv.process (src1,  result));
        ires = result;
        rc -= ap.width / 2;
        cc -= ap.height / 2;
        
        EXPECT_EQ(result.at<float>(rc-1, cc-1), 7168);
        EXPECT_EQ(result.at<float>(rc-1, cc), 7168);
        EXPECT_EQ(result.at<float>(rc-1, cc+1), 7168);
        EXPECT_EQ(result.at<float>(rc+1, cc-1), 7168);
        EXPECT_EQ(result.at<float>(rc+1, cc), 7168);
        EXPECT_EQ(result.at<float>(rc+1, cc+1), 7168);
        EXPECT_EQ(result.at<float>(rc, cc-1), 7168);
        EXPECT_EQ(result.at<float>(rc, cc), 7168);
        EXPECT_EQ(result.at<float>(rc, cc+1), 7168);
        
        
        
        
    }
    
    
    void test_tiny()
    {
        cv::Mat src1 = cv::Mat(2, 3, CV_8UC1); // 2 rows, 3 columns
        src1.at<uint8_t>(0,0) = 1;
        src1.at<uint8_t>(0,1) = 2;
        src1.at<uint8_t>(0,2) = 3;
        src1.at<uint8_t>(1,0) = 3;
        src1.at<uint8_t>(1,1) = 2;
        src1.at<uint8_t>(1,2) = 1;
        
        cv::Mat result;
        
        // Test with 1,1
        cv::Size ap (1,1);
        svl::localVAR tv (ap);
        EXPECT_FALSE(tv.process (src1,  result));
        
        cv::Size app (3, 3);
        svl::localVAR tv2 (app);
        EXPECT_TRUE(tv2.process (src1,  result));
    }
    
};


#endif // _rcUT_WINDOW_H_
