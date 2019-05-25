//
//  ut_rotated_rect.hpp
//  VisibleGtest
//
//  Created by Arman Garakani on 5/25/19.
//

#ifndef ut_rotated_rect_h
#define ut_rotated_rect_h


#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wdelete-non-virtual-dtor"
#pragma GCC diagnostic ignored "-Wunused-private-field"
#pragma GCC diagnostic ignored "-Wshorten-64-to-32"
#pragma GCC diagnostic ignored "-Wchar-subscripts"




#include <iostream>
#include <cmath>
#include <vector>

#include "opencv2/imgproc.hpp"
#include "opencv2/highgui.hpp"
#include "vision/opencv_utils.hpp"
#include "vision/ellipse.hpp"
#include "core/angle_units.h"
#include "vision/ellipse_fit.hpp"
#include "core/lineseg.hpp"
#include <boost/range/irange.hpp>


using namespace cv;

using namespace std;
using namespace svl;



#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <math.h>

/*
 *  Gabor Play ( not tested )
 */

int kernel_size=21;
int pos_sigma= 5;
int pos_lm = 50;
int pos_th = 0;
int pos_psi = 90;
cv::Mat src_f;
cv::Mat dest;

void Process()
{
    double sig = pos_sigma;
    double lm = 0.5+pos_lm/100.0;
    double th = pos_th;
    double ps = pos_psi;
    cv::Mat kernel = svl::gaborKernel(kernel_size, sig, th, lm, ps);
    cv::filter2D(src_f, dest, CV_32F, kernel);
    cv::imshow("Process window", dest);
    cv::Mat Lkernel(kernel_size*20, kernel_size*20, CV_32F);
    cv::resize(kernel, Lkernel, Lkernel.size());
    Lkernel /= 2.;
    Lkernel += 0.5;
    cv::imshow("Kernel", Lkernel);
    cv::Mat mag;
    cv::pow(dest, 2.0, mag);
    cv::imshow("Mag", mag);
}


int main_gabor (cv::Mat& image)
{
    cv::imshow("Src", image);
    cv::Mat src;
    cv::cvtColor(image, src, CV_BGR2GRAY);
    src.convertTo(src_f, CV_32F, 1.0/255, 0);
    if (!kernel_size%2)
    {
        kernel_size+=1;
    }
    cv::namedWindow("Process window", 1);
    cv::createTrackbar("Sigma", "Process window", &pos_sigma, kernel_size, Process);
    cv::createTrackbar("Lambda", "Process window", &pos_lm, 100, Process);
    cv::createTrackbar("Theta", "Process window", &pos_th, 180, Process);
    cv::createTrackbar("Psi", "Process window", &pos_psi, 360, Process);
    Process();
    cv::waitKey(0);
    return 0;
}


/*
 *  built_rotated_rect_montage ( not tested )
 */


void built_rotated_rect_montage ()
{
    
    Mat image(1000, 2000, CV_8UC3, Scalar(196,196,196,0.5));
    Mat display(1000, 2000, CV_8UC3, Scalar(196,196,196,0.5));
    vector<cv::Mat> bgr_planes;
    split(image, bgr_planes );
    
    
    static cv::Point2f offset(50,50);
    auto inspect = [=](cv::Mat& image, cv::Mat& display, cv::Size& size, cv::Point2f& ctr, float degrees, bool correct = true){
        
        
        
        auto moved = ctr + offset;
        RotatedRect rRect = RotatedRect(moved, size, degrees);
        std::vector<Point2f> points(4);
        rRect.points(points.data());
        RotatedRect ri;
        pointsToRotatedRect(points, ri);
        
        svl::ellipseShape er(ri);
        er.get(ri);
        int maxd = std::sqrt(rRect.size.width*rRect.size.width+rRect.size.height*rRect.size.height)+1;
        maxd = (maxd % 2)==0 ? maxd+1 : maxd;
        cv::Rect bb (rRect.center.x - maxd / 2, rRect.center.y- maxd / 2, maxd, maxd);
        
        double ks=maxd;
        double sig = 32;
        double lm = 0.75;
        double th = ri.angle;
        double ps = 270.0;
        cv::Mat kernel = svl::gaborKernel(ks, sig, th, lm, ps);
        cv::normalize(kernel,kernel,0.0,255.0,cv::NORM_MINMAX);
        cv::Mat kernel2(maxd,maxd ,CV_8U);
        kernel.convertTo(kernel2, CV_8U);
        std::vector<cv::Mat> kcs = {kernel2,kernel2,kernel2};
        cv::Mat pat;
        cv::merge(kcs,pat);
        cv::Mat window (image, bb);
        pat.copyTo(window);
        
        
        
        
        rRect.angle = ri.angle;
        drawRotatedRect(image, ri, Scalar(0,255,0,0.5),Scalar(255,0,255,0.5), Scalar(0,255,255,0.5), true, false);
        
        auto matAffineCrop = [] (Mat input, const RotatedRect& box){
            double angle = box.angle;
            auto size = box.size;
            
            
            
            
            //Rotate the text according to the angle
            auto transform = getRotationMatrix2D(box.center, angle, 1.0);
            Mat rotated;
            warpAffine(input, rotated, transform, input.size(), INTER_NEAREST);
            
            //Crop the result
            Mat cropped;
            size.width -= 3;
            size.height -= 3;
            
            getRectSubPix(rotated, size, box.center, cropped);
            //            cv::Mat big;
            //            cv::resize(cropped, big,cv::Size(),10,10,cv::INTER_NEAREST);
            //            imshow("cropped", big);
            //            waitKey(0);
            return cropped;
            
        };
        
        auto md = matAffineCrop(image, rRect);
        
        
        
        
        moved.y += 100;
        rRect.center.y += 100;
        offset.x += (1.67 * std::max(size.width,size.height));
        
    };
    
    
    cv::Size sz(65,65);
    cv::Point2f cc(64,64);
    float dt = -22.5;
    int count = 1 + 360/22.5;
    for (auto ii = 0; ii < count; ii++)
        inspect(image, display, sz, cc , -90+(ii*dt));
        
        offset.x = 50;
        offset.y = 300;
        
        sz.width = 32;
        for (auto ii = 0; ii < count; ii++)
            inspect(image, display, sz, cc , -90+(ii*dt));
            
            
            offset.x = 50;
            offset.y = 600;
            
            sz.width = 65;;
    sz.height = 32;
    for (auto ii = 0; ii < count; ii++)
        inspect(image, display, sz, cc , -90+(ii*dt));
        
        
        imshow("rectangles", image);
        waitKey(0);
        
        
        return 0;
}

#endif /* ut_rotated_rect_h */
