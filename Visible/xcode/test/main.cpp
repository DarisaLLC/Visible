//
//  main.cpp
//  VisibleGtest
//
//  Created by Arman Garakani on 5/16/16.
//
//

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wdelete-non-virtual-dtor"
#pragma GCC diagnostic ignored "-Wunused-private-field"
#pragma GCC diagnostic ignored "-Wshorten-64-to-32"
#pragma GCC diagnostic ignored "-Wchar-subscripts"


#include "opencv2/imgproc.hpp"
#include "opencv2/highgui.hpp"
#include "vision/opencv_utils.hpp"
#include "vision/ellipse.hpp"
#include "core/angle_units.h"
#include "vision/ellipse_fit.hpp"
#include "core/lineseg.hpp"


#define precision 0.0001

using namespace cv;
#include <iostream>
#include <cmath>
#include <vector>

using namespace std;
using namespace svl;


std::string toString (const cv::RotatedRect& rr){
    std::ostringstream ss;
    ss << "[ + " << rr.center << " / " << rr.angle << " | " << rr.size.height << " - " << rr.size.width << "]";
    return ss.str();
}

cv::RotatedRect getBoundingRectPCA( cv::Mat& binaryImg ) {
    cv::RotatedRect result;
    
    //1. convert to matrix that contains point coordinates as column vectors
    int count = cv::countNonZero(binaryImg);
    if (count == 0) {
        std::cout << "Utilities::getBoundingRectPCA() encountered 0 pixels in binary image!" << std::endl;
        return cv::RotatedRect();
    }
    
    cv::Mat data(2, count, CV_32FC1);
    int dataColumnIndex = 0;
    for (int row = 0; row < binaryImg.rows; row++) {
        for (int col = 0; col < binaryImg.cols; col++) {
            if (binaryImg.at<unsigned char>(row, col) != 0) {
                data.at<float>(0, dataColumnIndex) = (float) col; //x coordinate
                data.at<float>(1, dataColumnIndex) = (float) (binaryImg.rows - row); //y coordinate, such that y axis goes up
                ++dataColumnIndex;
            }
        }
    }
    
    //2. perform PCA
    const int maxComponents = 1;
    cv::PCA pca(data, cv::Mat() /*mean*/, CV_PCA_DATA_AS_COL, maxComponents);
    //result is contained in pca.eigenvectors (as row vectors)
    //std::cout << pca.eigenvectors << std::endl;
    
    //3. get angle of principal axis
    float dx = pca.eigenvectors.at<float>(0, 0);
    float dy = pca.eigenvectors.at<float>(0, 1);
    float angle = atan2f(dy, dx)  / (float)CV_PI*180.0f;
    
    //find the bounding rectangle with the given angle, by rotating the contour around the mean so that it is up-right
    //easily finding the bounding box then
    cv::Point2f center(pca.mean.at<float>(0,0), binaryImg.rows - pca.mean.at<float>(1,0));
    cv::Mat rotationMatrix = cv::getRotationMatrix2D(center, -angle, 1);
    cv::Mat rotationMatrixInverse = cv::getRotationMatrix2D(center, angle, 1);
    
    std::vector<std::vector<cv::Point> > contours;
    cv::findContours(binaryImg, contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);
    if (contours.size() != 1) {
        std::cout << "Warning: found " << contours.size() << " contours in binaryImg (expected one)" << std::endl;
        return result;
    }
    
    //turn vector of points into matrix (with points as column vectors, with a 3rd row full of 1's, i.e. points are converted to extended coords)
    cv::Mat contourMat(3, contours[0].size(), CV_64FC1);
    double* row0 = contourMat.ptr<double>(0);
    double* row1 = contourMat.ptr<double>(1);
    double* row2 = contourMat.ptr<double>(2);
    for (int i = 0; i < (int) contours[0].size(); i++) {
        row0[i] = (double) (contours[0])[i].x;
        row1[i] = (double) (contours[0])[i].y;
        row2[i] = 1;
    }
    
    cv::Mat uprightContour = rotationMatrix*contourMat;
    
    //get min/max in order to determine width and height
    double minX, minY, maxX, maxY;
    cv::minMaxLoc(cv::Mat(uprightContour, cv::Rect(0, 0, contours[0].size(), 1)), &minX, &maxX); //get minimum/maximum of first row
    cv::minMaxLoc(cv::Mat(uprightContour, cv::Rect(0, 1, contours[0].size(), 1)), &minY, &maxY); //get minimum/maximum of second row
    
    int minXi = cvFloor(minX);
    int minYi = cvFloor(minY);
    int maxXi = cvCeil(maxX);
    int maxYi = cvCeil(maxY);
    
    //fill result
    result.angle = angle;
    result.size.width = (float) (maxXi - minXi);
    result.size.height = (float) (maxYi - minYi);
    
    //Find the correct center:
    cv::Mat correctCenterUpright(3, 1, CV_64FC1);
    correctCenterUpright.at<double>(0, 0) = maxX - result.size.width/2;
    correctCenterUpright.at<double>(1,0) = maxY - result.size.height/2;
    correctCenterUpright.at<double>(2,0) = 1;
    cv::Mat correctCenterMat = rotationMatrixInverse*correctCenterUpright;
    cv::Point correctCenter = cv::Point(cvRound(correctCenterMat.at<double>(0,0)), cvRound(correctCenterMat.at<double>(1,0)));
    
    result.center = correctCenter;
    
    return result;
    
}


void pointsToRotatedRect (std::vector<cv::Point2f>& imagePoints, cv::RotatedRect& rotated_rect) {
    // get the left most
    std::sort (imagePoints.begin(), imagePoints.end(),[](const cv::Point2f&a, const cv::Point2f&b){
        return a.x > b.x;
    });
    
    std::sort (imagePoints.begin(), imagePoints.end(),[](const cv::Point2f&a, const cv::Point2f&b){
        return a.y < b.y;
    });
    
    fVector_2d tl (imagePoints[0].x, imagePoints[0].y);
    fVector_2d bl (imagePoints[1].x, imagePoints[1].y);
    fVector_2d tr (imagePoints[2].x, imagePoints[2].y);
    fVector_2d br (imagePoints[3].x, imagePoints[3].y);
    fLineSegment2d sideOne (tl, tr);
    fLineSegment2d sideTwo (bl, br);
    
    
    cv::Point2f ctr(0,0);
    for (auto const& p : imagePoints){
        ctr.x += p.x;
        ctr.y += p.y;
    }
    ctr.x /= 4;
    ctr.y /= 4;
    rotated_rect.center = ctr;
    float dLR = sideOne.length();
    float dTB = sideTwo.length();
    rotated_rect.size.width = dLR;
    rotated_rect.size.height = dTB;
    std::cout << toDegrees(sideOne.angle().Double()) << " Side One " << std::endl;
    std::cout << toDegrees(sideTwo.angle().Double()) << " Side Two " << std::endl;
    
    uDegree angle = sideTwo.angle();
    if (angle.Double() < -45.0){
        angle = angle + uDegree::piOverTwo();
        std::swap(rotated_rect.size.width,rotated_rect.size.height);
    }
    rotated_rect.angle = angle.Double();
}

    
#if 0
    float X[]={185,182.854,181.225,180.086,178.973,177.868,176.755,175.622,174.296,173.13,171.969,170.798,169.279,168.079,166.893,164.909,163.696,162.138,160.956,159.762,158.209,157.017,155.826,154.192,152.988,150.933,148.857,147.078,144.993,142.906,140.81,139.01,136.911,135.068,132.962,130.85,128.978,127.072,124.962,123.033,120.929,118.985,117.026,114.933,112.97,110.997,109.017,107.03,104.968,102.983,100.992,98.997,96.9998,95.0016,93.0035,91.007,89.0142,86.9663,84.9676,82.9798,81.0034,79.0373,76.9419,74.9862,73.043,70.9357,69.0133,66.9003,64.9952,62.8779,60.9968,58.8842,57.0397,54.9433,52.8446,51.0558,48.9744,46.8964,44.8226,43.1216,41.9258,39.8931,38.2492,37.0425,35.8484,34.2504,33.055,31.8722,30.6703,29.1848,28.0179,26.8576,25.6873,24.2945,23.136,21.9922,20.8515,19.7037,18.3912,17.2477};
    float Y[]={61.198,61.5732,62.3475,63.1181,63.9648,64.8266,65.6698,66.469,67.4408,68.2048,68.9483,69.6458,70.5159,71.1517,71.7898,72.8201,73.4095,74.264,74.9162,75.5511,76.3976,77.0326,77.6426,78.4249,78.9721,79.8223,80.5938,81.2261,81.9788,82.7216,83.4149,84.0314,84.6993,85.2384,85.8604,86.4056,86.9039,87.3347,87.8113,88.1805,88.5699,88.8985,89.1848,89.4768,89.7476,89.9753,90.1773,90.3597,90.5298,90.663,90.7535,90.804,90.82,90.8109,90.7843,90.7399,90.6681,90.5499,90.3878,90.1938,89.9716,89.7166,89.3934,89.0862,88.7556,88.3275,87.9361,87.4582,87.0212,86.503,86.0123,85.4124,84.8682,84.1757,83.4614,82.8385,82.0713,81.272,80.4356,79.7146,79.1696,78.2374,77.4562,76.9088,76.3161,75.5034,74.8949,74.2351,73.5835,72.6877,71.9707,71.2272,70.4893,69.5496,68.7953,68.0116,67.2182,66.4283,65.4503,64.6586};
    
    Eigen::Map<Eigen::VectorXf> xVec(X,4);
    Eigen::Map<Eigen::VectorXf> yVec(Y,4);
    DirectEllipseFit ellipFit(xVec, yVec);
    Ellipse ellip = ellipFit.doEllipseFit();
    std::cout << ellip << std::endl;
    return ellip.phi;
#endif
    
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <math.h>

cv::Mat mkKernel(int ks, double sig, double th, double lm, double ps)
{
    int hks = (ks-1)/2;
    double theta = th*CV_PI/180;
    double psi = ps*CV_PI/180;
    double del = 2.0/(ks-1);
    double lmbd = lm;
    double sigma = sig/ks;
    double x_theta;
    double y_theta;
    cv::Mat kernel(ks,ks, CV_32F);
    for (int y=-hks; y<=hks; y++)
    {
        for (int x=-hks; x<=hks; x++)
        {
            x_theta = x*del*cos(theta)+y*del*sin(theta);
            y_theta = -x*del*sin(theta)+y*del*cos(theta);
            kernel.at<float>(hks+y,hks+x) = (float)exp(-0.5*(pow(x_theta,2)+pow(y_theta,2))/pow(sigma,2))* cos(2*CV_PI*x_theta/lmbd + psi);
        }
    }

 
    return kernel;
}

int kernel_size=21;
int pos_sigma= 5;
int pos_lm = 50;
int pos_th = 0;
int pos_psi = 90;
cv::Mat src_f;
cv::Mat dest;

void Process(int , void *)
{
    double sig = pos_sigma;
    double lm = 0.5+pos_lm/100.0;
    double th = pos_th;
    double ps = pos_psi;
    cv::Mat kernel = mkKernel(kernel_size, sig, th, lm, ps);
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


int main_gabor ()
{
    cv::Mat image = cv::imread("/Volumes/medvedev/Users/arman/tmp/c2_set/c2/image000.png",1);
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
    Process(0,0);
    cv::waitKey(0);
    return 0;
}




int main()
{
   // main_gabor();
    
  
    
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
        cv::Mat kernel = mkKernel(ks, sig, th, lm, ps);
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


#if 0

#include <iostream>
#include <fstream>
#include <chrono>
#include "gtest/gtest.h"
#include <memory>
#include <thread>


#include <boost/foreach.hpp>
#include "boost/algorithm/string.hpp"
#include <boost/range/irange.hpp>
#include "boost/filesystem.hpp"
#include "boost/algorithm/string.hpp"

#include "vision/voxel_frequency.h"

#include "opencv2/highgui.hpp"
#include "vision/roiWindow.h"
#include "vision/self_similarity.h"
#include "seq_frame_container.hpp"
#include "cinder/ImageIo.h"
#include "cinder_cv/cinder_xchg.hpp"
#include "ut_sm.hpp"
#include "AVReader.hpp"
#include "cm_time.hpp"
#include "core/gtest_env_utils.hpp"
#include "vision/colorhistogram.hpp"
#include "cinder_opencv.h"
#include "ut_util.hpp"
#include "algo_cardiac.hpp"
#include "core/csv.hpp"
#include "core/kmeans1d.hpp"
#include "core/stl_utils.hpp"
#include "core/core.hpp"
#include "contraction.hpp"
#include "sm_producer.h"
#include "sg_filter.h"
#include "vision/drawUtils.hpp"
#include "ut_localvar.hpp"
#include "vision/labelBlob.hpp"
#include "result_serialization.h"
#include <cereal/cereal.hpp>
#include <cereal/types/memory.hpp>
#include <cereal/types/utility.hpp>
#include <cereal/types/polymorphic.hpp>
#include <cereal/archives/binary.hpp>
#include "vision/opencv_utils.hpp"
#include "permutation_entropy.h"
#include "result_serialization.h"
#include "cvplot/cvplot.h"
#include "time_series/persistence1d.hpp"
#include "async_tracks.h"
#include "CinderImGui.h"
#include "logger/logger.hpp"
#include "vision/opencv_utils.hpp"
#include "vision/gauss.hpp"
#include "vision/dense_motion.hpp"
#include "vision/gradient.h"
#include "algo_Lif.hpp"
#include "cinder/PolyLine.h"

// @FIXME Logger has to come before these
#include "ut_units.hpp"
#include "ut_cardio.hpp"

#define cimg_plugin1 "plugins/cvMat.h"
#define cimg_display 0
#include "CImg.h"
using namespace cimg_library;


//#define INTERACTIVE

using namespace boost;

using namespace ci;
using namespace ci::ip;
using namespace spiritcsv;
using namespace kmeans1D;
using namespace stl_utils;
using namespace std;
using namespace svl;

namespace fs = boost::filesystem;

static const auto CVCOLOR_RED = cv::Vec3b(0, 0, 255);
static const auto CVCOLOR_YELLOW = cv::Vec3b(0, 255, 255);
static const auto CVCOLOR_GREEN = cv::Vec3b(0, 255, 0);
static const auto CVCOLOR_BLUE = cv::Vec3b(255, 0, 0);
static const auto CVCOLOR_GRAY = cv::Vec3b(127, 127, 127);
static const auto CVCOLOR_BLACK = cv::Vec3b(0, 0, 0);
static const auto CVCOLOR_WHITE = cv::Vec3b(255, 255, 255);
static const auto CVCOLOR_VIOLET = cv::Vec3b(127, 0, 255);


std::vector<double> oneD_example = {39.1747, 39.2197, 39.126, 39.0549, 39.0818, 39.0655, 39.0342,
    38.8791, 38.8527, 39.0099, 38.8608, 38.9188, 38.8499, 38.6693,
    38.2513, 37.9095, 37.3313, 36.765, 36.3621, 35.7261, 35.0656,
    34.2602, 33.2523, 32.3183, 31.6464, 31.0073, 29.8166, 29.3423,
    28.5223, 27.5152, 26.8191, 26.3114, 25.8164, 25.0818, 24.7631,
    24.6277, 24.8184, 25.443, 26.2479, 27.8759, 29.2094, 30.7956,
    32.3586, 33.6268, 35.1586, 35.9315, 36.808, 37.3002, 37.67, 37.9986,
    38.2788, 38.465, 38.5513, 38.6818, 38.8076, 38.9388, 38.9592,
    39.058, 39.1322, 39.0803, 39.1779, 39.1531, 39.1375, 39.1978,
    39.0379, 39.1231, 39.202, 39.1581, 39.1777, 39.2971, 39.2366,
    39.1555, 39.2822, 39.243, 39.1807, 39.1488, 39.2491, 39.265, 39.198,
    39.2855, 39.2595, 39.4274, 39.3258, 39.3162, 39.4143, 39.3034,
    39.2099, 39.2775, 39.5042, 39.1446, 39.188, 39.2006, 39.1799,
    39.4077, 39.2694, 39.1967, 39.2828, 39.2438, 39.2093, 39.2167,
    39.2749, 39.4703, 39.2846};


// Support Functions Implementation at the bottom of the file
bool setup_loggers (const std::string& log_path,  std::string id_name);
std::shared_ptr<std::ofstream> make_shared_ofstream(std::ofstream * ifstream_ptr);
std::shared_ptr<std::ofstream> make_shared_ofstream(std::string filename);
void output_array (const std::vector<std::vector<float>>& data, const std::string& output_file);
void finalize_segmentation (cv::Mat& space);
cv::Mat generateVoxelSelfSimilarities (std::vector<std::vector<roiWindow<P8U>>>& voxels,
                                       std::vector<std::vector<float>>& ss);
bool setup_text_loggers (const fs::path app_support_dir, std::string id_name);
SurfaceRef get_surface(const std::string & path);
void norm_scale (const std::vector<double>& src, std::vector<double>& dst);

std::shared_ptr<test_utils::genv> dgenv_ptr;

typedef std::weak_ptr<Surface8u>	Surface8uWeakRef;
typedef std::weak_ptr<Surface8u>	SurfaceWeakRef;
typedef std::weak_ptr<Surface16u>	Surface16uWeakRef;
typedef std::weak_ptr<Surface32f>	Surface32fWeakRef;


std::vector<Point2f> ellipse_test = {
    {666.895422,372.895287},
    {683.128254,374.338401},
    {698.197455,379.324060},
    {735.650898,381.410000},
    {726.754550,377.262089},
    {778.094505,381.039470},
    {778.129881,381.776216},
    {839.415543,384.804510}};


TEST (ut_rotated_rect, basic){
    std::vector<vec2> points = {{5.5f,3.5f},{7.5f,2.5f},{9.5f,3.5f},{5.5f,6.5f}};
    Point2f center_gold (11.5,2.5);
    std::vector<Point2f> cvpoints = {{5.5f,2.5f},{7.5f,2.5f},{9.5f,2.5f},{5.5f,6.5f}};
    std::vector<Point2f> cvpoints2 = {{5.5f,6.5f},{7.5f,6.5f},{9.5f,6.5f},{5.5f,3.5f}};
    std::vector<Point2f> cvpoints3 = {{5.5f,6.5f},{7.5f,6.5f},{7.5f,8.5f},{7.5f,12.5f}};
    Point2f cvcenter_gold (11.5,2.5);
    
    ci::PolyLine2 pl(points);
    bool is_colinear = true;
    bool isCCW = pl.isCounterclockwise(&is_colinear);
    EXPECT_TRUE(!is_colinear);
    EXPECT_TRUE(isCCW);
    auto com = pl.calcCentroid();
    auto area = pl.calcArea();
    std::cout << com << "  ,   " << area << std::endl;
    
    {
        cv::RotatedRect rr;
        pointsToRotatedRect(cvpoints, rr);
        std::cout << rr.angle << std::endl;
        std::cout << rr.center << std::endl;
        std::cout << rr.size << std::endl;
    }
    
    {
        cv::RotatedRect rr;
        pointsToRotatedRect(cvpoints2, rr);
        std::cout << rr.angle << std::endl;
        std::cout << rr.center << std::endl;
        std::cout << rr.size << std::endl;
    }
    
    
    {
        cv::RotatedRect rr;
        pointsToRotatedRect(cvpoints3, rr);
        std::cout << rr.angle << std::endl;
        std::cout << rr.center << std::endl;
        std::cout << rr.size << std::endl;
    }
    
}

#if 0

#include "ut_lif.hpp"

TEST(serialization, eigen){
    Eigen::MatrixXd test = Eigen::MatrixXd::Random(10, 3);
    
    {
        std::ofstream out("eigen.cereal", std::ios::binary);
        cereal::BinaryOutputArchive archive_o(out);
        archive_o(test);
    }
    
    std::cout << "test:" << std::endl << test << std::endl;
    
    Eigen::MatrixXd test_loaded;
    
    {
        std::ifstream in("eigen.cereal", std::ios::binary);
        cereal::BinaryInputArchive archive_i(in);
        archive_i(test_loaded);
    }
    
    std::cout << "test loaded:" << std::endl << test_loaded << std::endl;
}

TEST (ut_algo_lif, segment){
    
    auto res = dgenv_ptr->asset_path("voxel_ss_.png");
    EXPECT_TRUE(res.second);
    EXPECT_TRUE(boost::filesystem::exists(res.first));
    cv::Mat image = cv::imread(res.first.c_str(), CV_LOAD_IMAGE_GRAYSCALE);
    finalize_segmentation (image);
    
}

TEST(ut_voxel_freq, basic){
    EXPECT_TRUE(fft1D::test());
}
TEST(ut_permutation_entropy, n_2){
    std::vector<double> times_series = {4/12.0,7/12.0,9/12.0,10/12.0,6/12.0,11/12.0,3/12.0};
    {
        auto pe_array_stats = permutation_entropy::permutation_entropy_array_stats(times_series, 2);
        auto pe_dict_stats = permutation_entropy::permutation_entropy_dictionary_stats(times_series, 2);
        std::cout << pe_array_stats << std::endl;
        std::cout << pe_dict_stats << std::endl;
    }
    {
        auto pe_array_stats = permutation_entropy::permutation_entropy_array_stats(times_series, 3);
        auto pe_dict_stats = permutation_entropy::permutation_entropy_dictionary_stats(times_series, 3);
        std::cout << pe_array_stats << std::endl;
        std::cout << pe_dict_stats << std::endl;
    }
}

TEST(ut_serialization, ssResultContainer){
    uint32_t cols = 21;
    uint32_t rows = 21;
    deque<deque<double>> sm;
    for (auto j = 0; j < rows; j++){
        deque<double> row;
        for (auto i = 0; i < cols; i++)
            row.push_back(1.0 / (i + j + 1));
        sm.push_back(row);
    }
    
    // Create sin signals
    auto sinvecD = [](float step, uint32_t size, std::deque<double>& out){
        out.clear ();
        for (auto i : irange(0u, size)) {
            out.push_back(sin(i * 3.14159 *  step));
        }
    };
    
    deque<double> entropy;
    sinvecD(1.0, rows, entropy);
    ssResultContainer ssr;
    ssr.load(entropy, sm);
    auto tempFilePath = boost::filesystem::temp_directory_path() / boost::filesystem::unique_path();
    bool ok = ssResultContainer::store(tempFilePath, entropy, sm);
    EXPECT_TRUE(ok);
    
    auto ssr_new_ref = ssResultContainer::create(tempFilePath.string());
    EXPECT_TRUE(ssr.is_same(*ssr_new_ref));
    
}

TEST (ut_dm, basic){
    roiWindow<P8U> p0 (320, 240);
    roiWindow<P8U> pw(p0, 80, 60, 160, 120);
    p0.set(123.0);
    pw.set(233.0);
    auto p1 = p0.clone();
    p1.set(123.0);
    roiWindow<P8U> pw1(p1, 81, 61, 160, 120);
    pw1.set(233.0);
    auto p2 = p1.clone();
    Gauss3by3 (p2, p1);
    
    cv::Mat im0 (p0.height(), p0.width(), CV_8UC(1), p0.pelPointer(0,0), size_t(p0.rowUpdate()));
    cv::Mat im1 (p1.height(), p1.width(), CV_8UC(1), p1.pelPointer(0,0), size_t(p1.rowUpdate()));
    
    iPair frame_size (p0.width(),p0.height());
    iPair fixed_half_size(16,16);
    iPair moving_half_size(24,24);
    denseMotion ff(frame_size, fixed_half_size, moving_half_size);
    iPair fixed = ff.fixed_size();
    iPair moving = ff.moving_size();
    
    EXPECT_TRUE(fixed == iPair(2*fixed_half_size.first+1,2*fixed_half_size.second+1));
    EXPECT_TRUE(moving == iPair(2*moving_half_size.first+1,2*moving_half_size.second+1));
    
    CorrelationParts cp;
    ff.update(im0);
    ff.update(im0);
    iPair fixed_tl (65, 45);
    auto r = ff.point_ncc_match(fixed_tl, fixed_tl, cp);
    EXPECT_TRUE(svl::equal(r,1.0));
    ff.update(im1);
    std::vector<std::vector<int>> space;
    for (int j = -1; j < 4; j++){
        std::vector<int> rs;
        for (int i = -1; i < 4; i++){
            iPair tmp(fixed_tl.first+i, fixed_tl.second+j);
            auto r = ff.point_ncc_match(fixed_tl, tmp,cp);
            int score = int(r*1000);
            rs.push_back(score);
        }
        space.push_back(rs);
    }
    
    EXPECT_TRUE(space[1][1] == 908);
    EXPECT_TRUE(space[1][3] == 905);
    EXPECT_TRUE(space[3][1] == 905);
    EXPECT_TRUE(space[3][3] == 905);
    
    EXPECT_TRUE(space[2][1] == 942);
    EXPECT_TRUE(space[2][2] == 980);
    EXPECT_TRUE(space[2][3] == 942);
    EXPECT_TRUE(space[1][2] == 942);
    EXPECT_TRUE(space[3][2] == 942);
    
    // Cheap Parabolic Interpolation
    auto x = ((space[2][3]-space[2][1])/2.0) / (space[2][3]+space[2][1]-space[2][2]);
    auto y = ((space[1][2]-space[3][2])/2.0) / (space[1][2]+space[3][2]-space[2][2]);
    cv::Point loc(x,y);
    
    std::cout << std::endl;
    for(std::vector<int>& row : space){
        for (const int& score : row){
            std::cout << setw(4) << score << '\t';
        }
        std::cout << std::endl;
    }
    
    //    {
    //        auto name = "Motion";
    //        namedWindow(name, CV_WINDOW_AUTOSIZE | WINDOW_OPENGL);
    //        cv::Point p0 (fixed_tl.first,fixed_tl.second);
    //        cv::Point p1 (p0.x+fixed.first, p0.y+fixed.second);
    //        rectangle(im1,p0,p1, CV_RGB(20,150,20));
    //        circle(im1,loc+p0, 5, CV_RGB(255,255,255));
    //        imshow( "im0", im0);
    //        cv::waitKey(-1);
    //        imshow( "im1", im1);
    //        cv::waitKey(-1);
    //    }
    
    
    
}

TEST (ut_dm, block){
    iPair dims (9,9);
    iPair fsize(32,64);
    
    float sigma = 1.5f;
    cv::Mat gm = gaussianTemplate(dims,vec2(sigma,sigma));
    
    
    cv::Mat fimage (fsize.second, fsize.first,  CV_8UC(1));
    fimage = 0;
    iPair fixed (11,21);
    CvRect froi(fixed.first,fixed.second,dims.first,dims.second);
    cv::Mat fwindow = fimage(froi);
    cv::add (gm, fwindow, fwindow);
    
    cv::Mat mimage (fsize.second, fsize.first,  CV_8UC(1));
    mimage = 0;
    iPair gold (12,22);
    CvRect mroi(gold.first,gold.second, dims.first,dims.second);
    cv::Mat mwindow = mimage(mroi);
    cv::add (gm, mwindow, mwindow);
    
    iPair scan (6,6);
    denseMotion ff(fsize, dims / 2, (dims + scan) / 2);
    auto moving = fixed - scan / 2;
    denseMotion::match res;
    ff.update(fimage);
    ff.update(mimage);
    auto check = ff.block_match(fixed, moving, res);
    std::cout << std::boolalpha << check << std::endl;
    
    
#ifdef INTERACTIVE
    cv::imshow("block", mimage);
    cv::waitKey();
#endif
}

TEST (ut_fit_ellipse, local_maxima){
    
    auto same_point = [] (const Point2f& a, const Point2f& b, float eps){return svl::equal(a.x, b.x, eps) && svl::equal(a.y, b.y, eps);};
    Point2f ellipse_gold (841.376, 386.055);
    RotatedRect box0 = fitEllipse(ellipse_test);
    EXPECT_TRUE(same_point(box0.center, ellipse_gold, 0.05));
    
    std::vector<Point2f> gold = {{5.5f,2.5f},{7.5f,2.5f},{9.5f,2.5f},{12.5f,4.5f},{13.5f,6.5f}};
    Point2f center_gold (11.5,2.5);
    
    const char * frame[] =
    {
        "00000000000000000000",
        "00000000000000000000",
        "00001020300000000000",
        "00000000000000000000",
        "00000000012300000000",
        "00000000000000000000",
        "00000000000054100000",
        "00000000000000000000",
        "00000000000000000000",
        0};
    
    roiWindow<P8U> pels(20, 5);
    DrawShape(pels, frame);
    cv::Mat im (pels.height(), pels.width(), CV_8UC(1), pels.pelPointer(0,0), size_t(pels.rowUpdate()));
    
    std::vector<Point2f> peaks;
    PeakDetect(im, peaks, 0);
    EXPECT_EQ(peaks.size(),gold.size());
    for (auto cc = 0; cc < peaks.size(); cc++)
        EXPECT_TRUE(same_point(peaks[cc], gold[cc],0.05));
    
    
    RotatedRect box = fitEllipse(peaks);
    EXPECT_TRUE(same_point(box.center, center_gold, 0.05));
    EXPECT_TRUE(svl::equal(box.angle, 60.85f, 1.0f));
    
}



TEST(ut_median, basic){
    std::vector<double> dst;
    bool ok = rolling_median_3(oneD_example.begin(), oneD_example.end(), dst);
    EXPECT_TRUE(ok);
    
}

void done_callback (void)
{
    std::cout << "Done"  << std::endl;
}

TEST (ut_opencvutils, anistropic_diffusion){
    
    double endtime;
    std::clock_t start;
    
    
    auto res = dgenv_ptr->asset_path("image230.png");
    EXPECT_TRUE(res.second);
    EXPECT_TRUE(boost::filesystem::exists(res.first));
    cv::Mat image = cv::imread(res.first.c_str(), CV_LOAD_IMAGE_GRAYSCALE);
    EXPECT_EQ(image.channels() , 1);
    EXPECT_EQ(image.cols , 512);
    EXPECT_EQ(image.rows , 128);
    cv::Mat image_anisotropic;
    start = std::clock();
    Anisotrpic_Diffusion(image, image_anisotropic, 0.25, 10, 45, 45);
    endtime = (std::clock() - start) / ((double)CLOCKS_PER_SEC);
    std::cout << " Anisotrpic Diffusion " << endtime  << " Seconds " << std::endl;
    
#ifdef INTERACTIVE
    std::string file_path = "/Users/arman/tmp/simple_aniso.png";
    cv::imwrite(file_path, image_anisotropic);
    cv::imshow("Anistorpic", image_anisotropic);
    cv::waitKey();
#endif
}

cv::Mat show_cv_angle (const cv::Mat& src, const std::string& name){
    cv::Mat mag, ang;
    svl::sobel_opencv(src, mag, ang, 7);
    
#ifdef INTERACTIVE
    namedWindow(name.c_str(), CV_WINDOW_AUTOSIZE | WINDOW_OPENGL);
    cv::imshow(name.c_str(), ang);
    cv::waitKey();
#endif
    return ang;
    
}


void show_gradient (const cv::Mat& src, const std::string& name){
    cv::Mat disp3c(src.size(), CV_8UC3, Scalar(255,255,255));
    
    roiWindow<P8U> r8(src.cols, src.rows);
    cpCvMatToRoiWindow8U (src, r8);
    roiWindow<P8U> mag = r8.clone();
    roiWindow<P8U> ang = r8.clone();
    roiWindow<P8U> peaks = r8.clone();
    std::vector<svl::feature> features;
    Gradient(r8,mag, ang);
    auto pks = SpatialEdge(mag, ang, features, 2);
    
    cvMatRefroiP8U(peaks, mim, CV_8UC1);
    cvMatRefroiP8U(ang, aim, CV_8UC1);
    
    
    auto rt = rightTailPost(mim, 0.005f);
    std::cout << pks << "    " << rt << std::endl;
    
    auto vec2point = [] (const fVector_2d& v, cv::Point& p) {p.x = v.x(); p.y = v.y();};
    
    for (auto fea : features){
        cv::Point pt;
        vec2point(fea.position(),pt);
        svl::drawCross(disp3c, pt,cv::Scalar(CVCOLOR_RED), 3, 1);
    }
    
#ifdef INTERACTIVE
    namedWindow(name.c_str(), CV_WINDOW_AUTOSIZE | WINDOW_OPENGL);
    cv::imshow(name.c_str(), disp3c);
    cv::waitKey();
#endif
    
}

TEST (ut_opencvutils, difference){
    auto res = dgenv_ptr->asset_path("image230.png");
    EXPECT_TRUE(res.second);
    EXPECT_TRUE(boost::filesystem::exists(res.first));
    cv::Mat fixed = cv::imread(res.first.c_str(), CV_LOAD_IMAGE_GRAYSCALE);
    EXPECT_EQ(fixed.channels() , 1);
    EXPECT_EQ(fixed.cols , 512);
    EXPECT_EQ(fixed.rows , 128);
    
    auto res2 = dgenv_ptr->asset_path("image262.png");
    EXPECT_TRUE(res2.second);
    EXPECT_TRUE(boost::filesystem::exists(res2.first));
    cv::Mat moving = cv::imread(res2.first.c_str(), CV_LOAD_IMAGE_GRAYSCALE);
    EXPECT_EQ(moving.channels() , 1);
    EXPECT_EQ(moving.cols , 512);
    EXPECT_EQ(moving.rows , 128);
    
    CImg<unsigned char> cFixed (fixed);
    CImg<unsigned char> cFixedPast(fixed.clone());
    CImg<unsigned char> cMoving (moving);
    CImg<unsigned char> cMovingPast(moving.clone());
    
    //    const float sharpness=0.33f;
    //    const float anisotropy=0.6f;
    //    const float alpha=30.0f;
    //    const float sigma=1.1f;
    //    const float dl=0.8f;
    //    const float da=3;
    auto mImg = cMoving.blur_anisotropic( 10, 0.8, 3);
    auto mout = mImg.get_MAT();
    auto fImg = cFixed.blur_anisotropic( 10, 0.8, 3);
    auto fout = fImg.get_MAT();
    
    //  show_gradient(fout, " Fixed ");
    //  show_gradient(mout, " Moving ");
    
    auto fang = show_cv_angle(fout, " Fixed ");
    auto mang = show_cv_angle(mout, " Moving ");
    
}



TEST (ut_3d_per_element, standard_dev)
{
    
    roiWindow<P8U> pels0(5, 4); pels0.set(0);
    roiWindow<P8U> pels1(5, 4); pels1.set(1);
    roiWindow<P8U> pels2(5, 4); pels2.set(2);
    
    
    vector<roiWindow<P8U>> frames {pels0, pels1, pels2};
    
    auto test_allpels = [] (cv::Mat& img, float val, bool show_failure){
        for(int i=0; i<img.rows; i++)
            for(int j=0; j<img.cols; j++)
                if (! svl::equal(img.at<float>(i,j), val, 0.0001f )){
                    if(show_failure)
                        std::cout << " rejected value: " << img.at<float>(i,j) << std::endl;
                    return false;
                }
        return true;
    };
    
    cv::Mat m_sum, m_sqsum;
    int image_count = 0;
    std::vector<std::thread> threads(1);
    threads[0] = std::thread(SequenceAccumulator(),std::ref(frames),
                             std::ref(m_sum), std::ref(m_sqsum), std::ref(image_count));
    
    std::for_each(threads.begin(), threads.end(), std::mem_fn(&std::thread::join));
    EXPECT_EQ(3, image_count);
    EXPECT_TRUE(test_allpels(m_sum, 3.0f, true));
    EXPECT_TRUE(test_allpels(m_sqsum, 5.0f, true));
    
    
    cv::Mat m_var_image;
    SequenceAccumulator::computeVariance(m_sum, m_sqsum, image_count, m_var_image);
    EXPECT_TRUE(test_allpels(m_var_image, 0.666667f, true));
}


TEST (ut_ss_voxel, basic){
    
    // Create N X M 1 dimentional roiWindows sized 1 x 64. Containing sin s
    
    // Create sin signals
    auto sinvec8 = [](float step, uint32_t size){
        std::vector<uint8_t> base(size);
        
        for (auto i : irange(0u, size)) {
            float v = (sin(i * 3.14159 *  step) + 1.0)*127.0f;
            base[i] = static_cast<uint8_t>(v);
        }
        
        return base;
    };
    
    // Create random signals
    //    auto randvec8 = []( uint32_t size){
    //        std::vector<uint8_t> base(size);
    //
    //        for (auto i : irange(0u, size)) {
    //            // base[i] = ((i % 256) / 256.0f - 0.5f) * 0.8;
    //            base[i] = rand() % 255;
    //        }
    //
    //        return base;
    //    };
    
    double endtime;
    std::clock_t start;
    
    std::vector<std::vector<roiWindow<P8U>>> voxels;
    
    int rows = 32;
    int cols = 32;
    cv::Point2f ctr (cols/2.0f, rows/2.0f);
    
    //    auto cvDrawPlot = [] (std::vector<float>& tmp){
    //
    //        std::string name = svl::toString(std::clock());
    //        cvplot::setWindowTitle(name, svl::toString(tmp.size()));
    //        cvplot::moveWindow(name, 0, 256);
    //        cvplot::resizeWindow(name, 512, 256);
    //        cvplot::figure(name).series(name).addValue(tmp).type(cvplot::Line).color(cvplot::Red);
    //        cvplot::figure(name).show();
    //    };
    
    start = std::clock();
    for (auto row = 0; row < rows; row++){
        std::vector<roiWindow<P8U>> rrs;
        for (auto col = 0; col < cols; col++){
            float r = (row+col)/2.0;
            r = std::max(1.0f,r);
            
            std::vector<uint8_t> tmp = sinvec8(1.0/r, 64);
            
            rrs.emplace_back(tmp);
            //            cvDrawPlot(signal);
        }
        voxels.push_back(rrs);
    }
    
    endtime = (std::clock() - start) / ((double)CLOCKS_PER_SEC);
    std::cout << " Generating Synthetic Data " << endtime  << " Seconds " << std::endl;
    
    
    std::vector<std::vector<float>> results;
    
#ifdef INTERACTIVE
    std::string filename = svl::toString(std::clock()) + ".csv";
    std::string imagename = svl::toString(std::clock()) + ".png";
    std::string file_path = "/Users/arman/tmp/" + filename;
    std::string image_path = "/Users/arman/tmp/" + imagename;
#endif
    
    start = std::clock();
    auto cm = generateVoxelSelfSimilarities(voxels, results);
#ifdef INTERACTIVE
    endtime = (std::clock() - start) / ((double)CLOCKS_PER_SEC);
    cv::imwrite(image_path, cm);
    std::cout << " Running Synthetic Data " << endtime  << " Seconds " << std::endl;
    output_array(results, file_path);
    
    
    /// Show in a window
    namedWindow( " ss voxel ", CV_WINDOW_KEEPRATIO  | WINDOW_OPENGL);
    imshow( " ss voxel ", cm);
    cv::waitKey(-1);
#endif
    
}


TEST (UT_AVReaderBasic, run)
{
    static bool check_done_val = false;
    static int frame_count = 0;
    const auto check_done = [] (){
        check_done_val = true;
    };
    
    // Note current implementation uses default callbacks for building timestamps
    // and images. User supplied callbacks for replace the default ones.
    const auto progress_report = [] (CMTime time) {
        cm_time t(time); std::cout << frame_count << "  " << t << std::endl; frame_count++;
    };
    
    boost::filesystem::path test_filepath;
    
    // vf does not support QuickTime natively. The ut expectes and checks for failure
    static std::string qmov_name ("box-move.m4v");
    
    auto res = dgenv_ptr->asset_path(qmov_name);
    EXPECT_TRUE(res.second);
    EXPECT_TRUE(boost::filesystem::exists(res.first));
    
    if (res.second)
        test_filepath = res.first;
    
    {
        check_done_val = false;
        avcc::avReader rr (test_filepath.string(), false);
        rr.setUserDoneCallBack(check_done);
        rr.setUserProgressCallBack(progress_report);
        rr.run ();
        
        EXPECT_TRUE(rr.info().count == 30);
        EXPECT_FALSE(rr.isEmpty());
        tiny_media_info mif (rr.info());
        mif.printout();
        
        std::this_thread::sleep_for(std::chrono::duration<double, std::milli> (2000));
        EXPECT_TRUE(check_done_val);
        EXPECT_TRUE(rr.isValid());
        EXPECT_TRUE(rr.size().first == 57);
        EXPECT_TRUE(rr.size().second == 57);
        EXPECT_FALSE(rr.isEmpty());
        
        
    }
    
}

TEST(SimpleGUITest, basic)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    
    // Build atlas
    unsigned char* tex_pixels = nullptr;
    int tex_w, tex_h;
    io.Fonts->GetTexDataAsRGBA32(&tex_pixels, &tex_w, &tex_h);
    
    for (int n = 0; n < 5000; n++) {
        io.DisplaySize = ImVec2(1920, 1080);
        io.DeltaTime = 1.0f / 60.0f;
        ImGui::NewFrame();
        
        static float f = 0.0f;
        ImGui::Text("Hello, world!");
        ImGui::SliderFloat("float", &f, 0.0f, 1.0f);
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
        
        ImGui::Render();
    }
    
    ImGui::DestroyContext();
}

TEST(tracks, basic){
    
    // Generate test data
    namedTrack_t ntrack;
    timedVecOfVals_t& data = ntrack.second;
    data.resize(oneD_example.size());
    for (int tt = 0; tt < oneD_example.size(); tt++){
        data[tt].second = oneD_example[tt];
        data[tt].first.first = tt;
        data[tt].first.second = time_spec_t(tt / 1000.0);
    }
    
    std::vector<float> X, Y;
    domainFromPairedTracks_D (ntrack, X, Y);
    
    EXPECT_EQ(X.size(), oneD_example.size());
    EXPECT_EQ(Y.size(), oneD_example.size());
    for(auto ii = 0; ii < oneD_example.size(); ii++){
        EXPECT_TRUE(svl::RealEq(Y[ii],(float)oneD_example[ii]));
    }
}



void savgol (const vector<double>& signal, vector<double>& dst)
{
    // for scalar data:
    int order = 4;
    int winlen = 17 ;
    SGF::real sample_time = 0; // this is simply a float or double, you can change it in the header sg_filter.h if yo u want
    SGF::ScalarSavitzkyGolayFilter filter(order, winlen, sample_time);
    dst.resize(signal.size());
    SGF::real output;
    for (auto ii = 0; ii < signal.size(); ii++)
    {
        filter.AddData(signal[ii]);
        if (! filter.IsInitialized()) continue;
        dst[ii] = 0;
        int ret_code;
        ret_code = filter.GetOutput(0, output);
        dst[ii] = output;
    }
    
}

TEST(units,basic)
{
    units_ut::run();
    eigen_ut::run();
    cardio_ut::run(dgenv_ptr);
}

TEST(cardiac_ut, load_sm)
{
    auto res = dgenv_ptr->asset_path("sm.csv");
    EXPECT_TRUE(res.second);
    EXPECT_TRUE(boost::filesystem::exists(res.first));
    
    vector<vector<double>> array;
    load_sm(res.first.string(), array, false);
    EXPECT_EQ(size_t(500), array.size());
    for (auto row = 0; row<500; row++){
        EXPECT_EQ(size_t(500), array[row].size());
        EXPECT_EQ(1.0,array[row][row]);
    }
    
    
}
TEST(cardiac_ut, interpolated_length)
{
    auto res = dgenv_ptr->asset_path("avg_baseline25_28_length_length_pct_short_pct.csv");
    EXPECT_TRUE(res.second);
    EXPECT_TRUE(boost::filesystem::exists(res.first));
    
    vector<vector<double>> array;
    load_sm(res.first.string(), array, false);
    EXPECT_EQ(size_t(500), array.size());
    for (auto row = 0; row<500; row++){
        EXPECT_EQ(size_t(5), array[row].size());
    }
    
    double            MicronPerPixel = 291.19 / 512.0;
    double            Length_max   = 118.555 * MicronPerPixel; // 67.42584072;
    double            Lenght_min   = 106.551 * MicronPerPixel; // 60.59880018;
                                                               // double            shortening   = Length_max - Lenght_min;
    double            MSI_max  =  0.37240525;
    double            MSI_min   = 0.1277325;
    // double            shortening_um   = 6.827040547;
    
    data_t dst;
    flip(array,dst);
    auto check = std::minmax_element(dst[1].begin(), dst[1].end() );
    EXPECT_TRUE(svl::equal(*check.first, MSI_min, 1.e-05));
    EXPECT_TRUE(svl::equal(*check.second, MSI_max, 1.e-05));
    
    auto car = contraction_analyzer::create();
    car->load(dst[1]);
    EXPECT_TRUE(car->isPreProcessed());
    EXPECT_TRUE(car->isValid());
    EXPECT_TRUE(car->leveled().size() == dst[1].size());
    car->find_best();
    const std::pair<double,double>& minmax = car->leveled_min_max ();
    EXPECT_TRUE(svl::equal(*check.first, minmax.first, 1.e-05));
    EXPECT_TRUE(svl::equal(*check.second, minmax.second, 1.e-05));
    
    const vector<double>& signal = car->leveled();
    auto start = std::clock();
    car->compute_interpolated_geometries(Lenght_min, Length_max);
    auto endtime = (std::clock() - start) / ((double)CLOCKS_PER_SEC);
    std::cout << endtime << " interpolated length " << std::endl;
    start = std::clock();
    car->compute_force(signal.begin(),signal.end(),Lenght_min, Length_max );
    endtime = (std::clock() - start) / ((double)CLOCKS_PER_SEC);
    std::cout << endtime << " FORCE " << std::endl;
    const vector<double>& lengths = car->interpolated_length();
    EXPECT_EQ(dst[2].size(), lengths.size());
    std::vector<double> diffs;
    for(auto row = 0; row < dst[2].size(); row++)
    {
        auto dd = dst[2][row] - lengths[row];
        diffs.push_back(dd);
    }
    
    auto dcheck = std::minmax_element(diffs.begin(), diffs.end() );
    EXPECT_TRUE(svl::equal(*dcheck.first, 0.0, 1.e-05));
    EXPECT_TRUE(svl::equal(*dcheck.second, 0.0, 1.e-05));
    
    {
        auto name = "force";
        cvplot::setWindowTitle(name, " Force ");
        cvplot::moveWindow(name, 0, 256);
        cvplot::resizeWindow(name, 512, 256);
        cvplot::figure(name).series("Force").addValue(car->total_reactive_force()).type(cvplot::Line).color(cvplot::Red);
        cvplot::figure(name).show();
    }
    {
        auto name = "interpolated length";
        cvplot::setWindowTitle(name, " Length ");
        cvplot::moveWindow(name,0,0);
        cvplot::resizeWindow(name, 512, 256);
        cvplot::figure(name).series("Length").addValue(car->interpolated_length()).type(cvplot::Line).color(cvplot::Blue);
        cvplot::figure(name).show();
    }
    
    
    
    
}

TEST(cardiac_ut, locate_contractions)
{
    auto res = dgenv_ptr->asset_path("avg_baseline25_28_length_length_pct_short_pct.csv");
    EXPECT_TRUE(res.second);
    EXPECT_TRUE(boost::filesystem::exists(res.first));
    
    vector<vector<double>> array;
    load_sm(res.first.string(), array, false);
    EXPECT_EQ(size_t(500), array.size());
    for (auto row = 0; row<500; row++){
        EXPECT_EQ(size_t(5), array[row].size());
    }
    
    data_t dst;
    flip(array,dst);
    
    // Persistence of Extremas
    persistenace1d<double> p;
    vector<double> dst_1;
    dst_1.insert(dst_1.end(),dst[1].begin(), dst[1].end());
    
    // Exponential Smoothing
    eMAvg<double> emm(0.333,0.0);
    vector<double> filtered;
    for (double d : dst_1){
        filtered.push_back(emm.update(d));
    }
    
    auto median_filtered_val = Median(filtered);
    p.RunPersistence(filtered);
    std::vector<int> tmins, lmins, tmaxs, lmaxs;
    p.GetExtremaIndices(tmins,tmaxs);
    std::cout << " Median val" << median_filtered_val << std::endl;
    //   Out(tmins);
    //   Out(tmaxs);
    
    for (auto tmi : tmins){
        if (dst_1[tmi] > median_filtered_val) continue;
        lmins.push_back(tmi);
    }
    
    for (auto tma : tmaxs){
        if (dst_1[tma] < median_filtered_val) continue;
        lmaxs.push_back(tma);
    }
    
    std::sort(lmins.begin(),lmins.end());
    std::sort(lmaxs.begin(),lmaxs.end());
    //    Out(lmins);
    //    Out(lmaxs);
    std::vector<std::pair<float, float>> contractions;
    for (auto lmi : lmins){
        contractions.emplace_back(lmi,dst_1[lmi]);
    }
    
    {
        auto name = "Contraction Localization";
        cvplot::setWindowTitle(name, "Contraction");
        cvplot::moveWindow(name, 300, 100);
        cvplot::resizeWindow(name, 1024, 512);
        cvplot::figure(name).series("Raw").addValue(dst_1);
        cvplot::figure(name).series("filtered").addValue(filtered);
        cvplot::figure(name).series("contractions").set(contractions).type(cvplot::Dots).color(cvplot::Red);
        cvplot::figure(name).show();
    }
    
}

TEST(UT_contraction_profiler, basic)
{
    contraction_analyzer::contraction_t ctr;
    typedef vector<double>::iterator dItr_t;
    
    std::vector<double> fder, fder2;
    fder.resize (oneD_example.size());
    fder2.resize (oneD_example.size());
    
    // Get contraction peak ( valley ) first
    auto min_iter = std::min_element(oneD_example.begin(),oneD_example.end());
    ctr.contraction_peak.first = std::distance(oneD_example.begin(),min_iter);
    
    // Computer First Difference,
    adjacent_difference(oneD_example.begin(),oneD_example.end(), fder.begin());
    std::rotate(fder.begin(), fder.begin()+1, fder.end());
    fder.pop_back();
    auto medianD = stl_utils::median1D<double>(7);
    fder = medianD.filter(fder);
    std::transform(fder.begin(), fder.end(), fder2.begin(), [](double f)->double { return f * f; });
    // find first element greater than 0.1
    auto pos = find_if (fder2.begin(), fder2.end(),    // range
                        std::bind2nd(greater<double>(),0.1));  // criterion
    
    ctr.contraction_start.first = std::distance(fder2.begin(),pos);
    auto max_accel = std::min_element(fder.begin()+ ctr.contraction_start.first ,fder.begin()+ctr.contraction_peak.first);
    ctr.contraction_max_acceleration.first = std::distance(fder.begin()+ ctr.contraction_start.first, max_accel);
    ctr.contraction_max_acceleration.first += ctr.contraction_start.first;
    auto max_relax = std::max_element(fder.begin()+ ctr.contraction_peak.first ,fder.end());
    ctr.relaxation_max_acceleration.first = std::distance(fder.begin()+ ctr.contraction_peak.first, max_relax);
    ctr.relaxation_max_acceleration.first += ctr.contraction_peak.first;
    
    // Initialize rpos to point to the element following the last occurance of a value greater than 0.1
    // If there is no such value, initialize rpos = to begin
    // If the last occurance is the last element, initialize this it to end
    dItr_t rpos = find_if (fder2.rbegin(), fder2.rend(),    // range
                           std::bind2nd(greater<double>(),0.1)).base();  // criterion
    ctr.relaxation_end.first = std::distance (fder2.begin(), rpos);
    
    EXPECT_EQ(ctr.contraction_start.first,16);
    EXPECT_EQ(ctr.contraction_peak.first,35);
    EXPECT_EQ(ctr.contraction_max_acceleration.first,27);
    EXPECT_EQ(ctr.relaxation_max_acceleration.first,43);
    EXPECT_EQ(ctr.relaxation_end.first,52);
    
    
    //   @todo reconcile with recent implementation
    //    contraction_profile_analyzer ca;
    //    ca.run(oneD_example);
    //        bool test = contraction_analyzer::contraction_t::equal(ca.contraction(), ctr);
    //       EXPECT_TRUE(test);
    //    {
    //        cvplot::figure("myplot").series("myline").addValue(ca.first_derivative_filtered());
    //        cvplot::figure("myplot").show();
    //    }
    
}
TEST(timing8, corr)
{
    std::shared_ptr<uint8_t> img1 = test_utils::create_trig(1920, 1080);
    std::shared_ptr<uint8_t> img2 = test_utils::create_trig(1920, 1080);
    double endtime;
    std::clock_t start;
    int l;
    start = std::clock();
    int num_loops = 10;
    // Time setting and resetting
    for (l = 0; l < num_loops; ++l)
    {
        basicCorrRowFunc<uint8_t> corrfunc(img1.get(), img2.get(), 1920, 1920, 1920, 1080);
        corrfunc.areaFunc();
        CorrelationParts cp;
        corrfunc.epilog(cp);
    }
    endtime = (std::clock() - start) / ((double)CLOCKS_PER_SEC);
    double scale = 1000.0 / (num_loops);
    std::cout << " Correlation: 1920 * 1080 * 8 bit " << endtime * scale << " millieseconds per " << std::endl;
}

TEST(timing8, corr_ocv)
{
    std::shared_ptr<uint8_t> img1 = test_utils::create_trig(1920, 1080);
    std::shared_ptr<uint8_t> img2 = test_utils::create_trig(1920, 1080);
    cv::Mat mat1(1080, 1920, CV_8UC(1), img1.get(), 1920);
    cv::Mat mat2(1080, 1920, CV_8UC(1), img2.get(), 1920);
    //    auto binfo = cv::getBuildInformation();
    //    auto numth = cv::getNumThreads();
    //    std::cout << binfo << std::endl << numth << std::endl;
    
    double endtime;
    std::clock_t start;
    int l;
    
    int num_loops = 10;
    Mat space(cv::Size(1,1), CV_32F);
    // Time setting and resetting
    start = std::clock();
    for (l = 0; l < num_loops; ++l)
    {
        cv::matchTemplate (mat1, mat2, space, CV_TM_CCOEFF_NORMED);
    }
    
    endtime = (std::clock() - start) / ((double)CLOCKS_PER_SEC);
    double scale = 1000.0 / (num_loops);
    std::cout << " OCV_Correlation: 1920 * 1080 * 8 bit " << endtime * scale << " millieseconds per " << std::endl;
}



TEST(ut_serialization, double_cv_mat){
    uint32_t cols = 210;
    uint32_t rows = 210;
    cv::Mat data (rows, cols, CV_64F);
    for (auto j = 0; j < rows; j++)
        for (auto i = 0; i < cols; i++){
            data.at<double>(j,i) = 1.0 / (i + j - 1);
        }
    
    auto tempFilePath = boost::filesystem::temp_directory_path() / boost::filesystem::unique_path();
    {
        std::ofstream file(tempFilePath.c_str(), std::ios::binary);
        cereal::BinaryOutputArchive ar(file);
        ar(data);
    }
    // Load the data from the disk again:
    {
        cv::Mat loaded_data;
        {
            std::ifstream file(tempFilePath.c_str(), std::ios::binary);
            cereal::BinaryInputArchive ar(file);
            ar(loaded_data);
            
            EXPECT_EQ(rows, data.rows);
            EXPECT_EQ(cols, data.cols);
            
            EXPECT_EQ(data.rows, loaded_data.rows);
            EXPECT_EQ(data.cols, loaded_data.cols);
            for (auto j = 0; j < rows; j++)
                for (auto i = 0; i < cols; i++){
                    EXPECT_EQ(data.at<double>(j,i), loaded_data.at<double>(j,i));
                }
        }
    }
}

TEST(ut_localvar, basic)
{
    auto res = dgenv_ptr->asset_path("out0.png");
    EXPECT_TRUE(res.second);
    EXPECT_TRUE(boost::filesystem::exists(res.first));
    cv::Mat out0 = cv::imread(res.first.c_str(), CV_LOAD_IMAGE_GRAYSCALE);
    std::cout << out0.channels() << std::endl;
    EXPECT_EQ(out0.channels() , 1);
    EXPECT_EQ(out0.cols , 512);
    EXPECT_EQ(out0.rows , 128);
    
    // create local variance filter size runner
    svl::localVAR tv (cv::Size(7,7));
    cv::Mat var0;
    cv::Mat std0U8;
    tv.process(out0, var0);
    EXPECT_EQ(tv.min_variance() , 0);
    EXPECT_EQ(tv.max_variance() , 11712);
}


TEST(ut_labelBlob, basic)
{
    using blob = svl::labelBlob::blob;
    
    static bool s_results_ready = false;
    static bool s_graphics_ready = false;
    static int64_t cid = 0;
    
    auto res = dgenv_ptr->asset_path("out0.png");
    EXPECT_TRUE(res.second);
    EXPECT_TRUE(boost::filesystem::exists(res.first));
    cv::Mat out0 = cv::imread(res.first.c_str(), CV_LOAD_IMAGE_GRAYSCALE);
    EXPECT_EQ(out0.channels() , 1);
    EXPECT_EQ(out0.cols , 512);
    EXPECT_EQ(out0.rows , 128);
    
    // Histogram -> threshold at 5 percent from the right ( 95 from the left :) )
    int threshold = leftTailPost (out0, 95.0 / 100.0);
    EXPECT_EQ(threshold, 40);
    
    cv::Mat threshold_input, threshold_output;
    
    /// Detect regions using Threshold
    out0.convertTo(threshold_input, CV_8U);
    cv::threshold(threshold_input
                  , threshold_output, threshold, 255, THRESH_BINARY );
    
    labelBlob::ref lbr = labelBlob::create(out0, threshold_output, 6, 666);
    EXPECT_EQ(lbr == nullptr , false);
    std::function<labelBlob::results_ready_cb> res_ready_lambda = [](int64_t& cbi){ s_results_ready = ! s_results_ready; cid = cbi;};
    std::function<labelBlob::graphics_ready_cb> graphics_ready_lambda = [](){ s_graphics_ready = ! s_graphics_ready;};
    boost::signals2::connection results_ready_ = lbr->registerCallback(res_ready_lambda);
    boost::signals2::connection graphics_ready_ = lbr->registerCallback(graphics_ready_lambda);
    EXPECT_EQ(false, s_results_ready);
    EXPECT_EQ(true, cid == 0);
    EXPECT_EQ(true, lbr->client_id() == 666);
    lbr->run_async();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    EXPECT_EQ(true, s_results_ready);
    EXPECT_EQ(true, lbr->client_id() == 666);
    EXPECT_EQ(true, cid == 666);
    EXPECT_EQ(true, lbr->hasResults());
    const std::vector<blob> blobs = lbr->results();
    EXPECT_EQ(44, blobs.size());
    
#ifdef INTERACTIVE
    lbr->drawOutput();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    EXPECT_EQ(true, s_graphics_ready);
    /// Show in a window
    namedWindow( "LabelBlob ", CV_WINDOW_AUTOSIZE | WINDOW_OPENGL);
    imshow( "LabelBlob", lbr->graphicOutput());
    cv::waitKey();
#endif
    
}

TEST(ut_stl_utils, accOverTuple)
{
    float val = tuple_accumulate(std::make_tuple(5, 3.2, 7U, 6.4f), 0L, functor());
    auto diff = std::fabs(val - 21.6);
    EXPECT_TRUE(diff < 0.000001);
    
    typedef std::tuple<int64_t,int64_t, uint32_t> partial_t;
    std::vector<partial_t> boo;
    for (int ii = 0; ii < 9; ii++)
        boo.emplace_back(12345679, -12345679, 1);
    
    
    auto res = std::accumulate(boo.begin(), boo.end(), std::make_tuple(int64_t(0),int64_t(0), uint32_t(0)), tuple_sum<int64_t,uint32_t>());
    bool check = res == make_tuple(111111111, -111111111, 9); //(int64_t(111111111),int64_t(-111111111), uint32_t(9));
    EXPECT_TRUE(check);
    
    
}

TEST(UT_smfilter, basic)
{
    vector<int> ranks;
    vector<double> norms;
    norm_scale(oneD_example,norms);
    //    stl_utils::Out(norms);
    
    vector<double> output;
    savgol(norms, output);
    //  stl_utils::Out(norms);
    //  stl_utils::Out(output);
    
    auto median_value = contraction_analyzer::Median_levelsets(norms,ranks);
    
    std::cout << median_value << std::endl;
    for (auto ii = 0; ii < norms.size(); ii++)
    {
        //        std::cout << "[" << ii << "] : " << norms[ii] << "     "  << std::abs(norms[ii] - median_value) << "     "  << ranks[ii] << "     " << norms[ranks[ii]] << std::endl;
        //        std::cout << "[" << ii << "] : " << norms[ii] << "     " << output[ii] << std::endl;
    }
    
    
}

TEST(basicU8, histo)
{
    
    const char * frame[] =
    {
        "00000000000000000000",
        "00001020300000000000",
        "00000000000000000000",
        "00000000012300000000",
        "00000000000000000000",
        0};
    
    roiWindow<P8U> pels(20, 5);
    DrawShape(pels, frame);
    
    histoStats hh;
    hh.from_image<P8U>(pels);
    EXPECT_EQ(hh.max(0), 3);
    EXPECT_EQ(hh.min(0), 0);
    EXPECT_EQ(hh.sum(), 12);
    EXPECT_EQ(hh.sumSquared(), 28); // 2 * 9 + 2 * 4 + 2 * 1
}



TEST (UT_make_function, make_function)
{
    make_function_test::run();
}


TEST (UT_algo, AVReader)
{
    boost::filesystem::path test_filepath;
    
    // vf does not support QuickTime natively. The ut expectes and checks for failure
    
    auto res = dgenv_ptr->asset_path("box-move.m4v");
    EXPECT_TRUE(res.second);
    EXPECT_TRUE(boost::filesystem::exists(res.first));
    
    if (res.second)
        test_filepath = res.first;
    
    avcc::avReaderRef rref = std::make_shared< avcc::avReader> (test_filepath.string(), false);
    rref->setUserDoneCallBack(done_callback);
    
    rref->run ();
    std::shared_ptr<seqFrameContainer> sm = seqFrameContainer::create(rref);
    
    EXPECT_TRUE(rref->isValid());
    EXPECT_TRUE(sm->count() == 57);
    
}




TEST(ut_similarity, short_term)
{
    
    self_similarity_producer<P8U> sm(3,0);
    
    EXPECT_EQ(sm.depth() , D_8U);
    EXPECT_EQ(sm.matrixSz() , 3);
    EXPECT_EQ(sm.cacheSz() , 0);
    EXPECT_EQ(sm.aborted() , false);
    
    vector<roiWindow<P8U>> fill_images(3);
    
    
    
    //     cv::Mat gaussianTemplate(const std::pair<uint32_t,uint32_t>& dims, const vec2& sigma = vec2(1.0, 1.0), const vec2& center = vec2(0.5,0.5));
    std::pair<uint32_t,uint32_t> dims (32,32);
    
    for (uint32_t i = 0; i < fill_images.size(); i++) {
        float sigma = 0.5 + i * 0.5;
        cv::Mat gm = gaussianTemplate(dims,vec2(sigma,sigma));
        cpCvMatToRoiWindow8U(gm,fill_images[i]);
    }
    roiWindow<P8U> tmp (dims.first, dims.second);
    tmp.randomFill(1066);
    
    /*
     Expected Update Output
     
     Fill     9.86492e-06            gaussian sigma 0.5
     6.21764e-06            gaussian sigma 1.0
     1.1948e-05             gaussian sigma 1.5
     Update   0.367755           update with random filled
     0.367793
     0.994818
     Update   0.367801           update with sigma 0.5
     0.994391
     0.367548
     Update   0.994314           update with sigma 1.0
     0.367543
     0.367757
     Update   9.86492e-06        update with sigma 1.5
     6.21764e-06
     1.1948e-05
     
     */
    deque<double> ent;
    bool fRet = sm.fill(fill_images);
    EXPECT_EQ(fRet, true);
    bool eRet = sm.entropies(ent);
    EXPECT_EQ(eRet, true);
    EXPECT_EQ(ent.size() , fill_images.size());
    EXPECT_EQ(std::distance(ent.begin(), std::max_element(ent.begin(), ent.end())), 2);
    
    // Now feed the set of one random followed by 3 gaussins few times
    {
        bool u1 = sm.update(tmp);
        EXPECT_EQ(u1, true);
        
        bool eRet = sm.entropies(ent);
        EXPECT_EQ(eRet, true);
        EXPECT_EQ(ent.size() , fill_images.size());
        EXPECT_EQ(svl::equal(ent[0], ent[1] , 1.e-3), true);
        EXPECT_EQ(std::distance(ent.begin(), std::max_element(ent.begin(), ent.end())), 2);
        
    }
    
    {
        bool u1 = sm.update(fill_images[0]);
        EXPECT_EQ(u1, true);
        
        bool eRet = sm.entropies(ent);
        EXPECT_EQ(eRet, true);
        EXPECT_EQ(ent.size() , fill_images.size());
        EXPECT_EQ(svl::equal(ent[0], ent[2] , 1.e-3), true);
        EXPECT_EQ(std::distance(ent.begin(), std::max_element(ent.begin(), ent.end())), 1);
    }
    
    {
        bool u1 = sm.update(fill_images[1]);
        EXPECT_EQ(u1, true);
        
        bool eRet = sm.entropies(ent);
        EXPECT_EQ(eRet, true);
        EXPECT_EQ(ent.size() , fill_images.size());
        EXPECT_EQ(svl::equal(ent[1], ent[2] , 1.e-3), true);
        EXPECT_EQ(std::distance(ent.begin(), std::max_element(ent.begin(), ent.end())), 0);
    }
    
    {
        bool u1 = sm.update(fill_images[2]);
        EXPECT_EQ(u1, true);
        
        bool eRet = sm.entropies(ent);
        EXPECT_EQ(eRet, true);
        EXPECT_EQ(ent.size() , fill_images.size());
        EXPECT_EQ(std::distance(ent.begin(), std::max_element(ent.begin(), ent.end())), 2);
    }
    
    // Test RefSimilarator
    self_similarity_producerRef simi (new self_similarity_producer<P8U> (7, 0));
    EXPECT_EQ (simi.use_count() , 1);
    self_similarity_producerRef simi2 (simi);
    EXPECT_EQ (simi.use_count() , 2);
    
}



TEST(ut_similarity, long_term)
{
    vector<roiWindow<P8U>> images(4);
    
    self_similarity_producer<P8U> sm((uint32_t) images.size(),0);
    
    EXPECT_EQ(sm.depth() , D_8U);
    EXPECT_EQ(sm.matrixSz() , images.size());
    EXPECT_EQ(sm.cacheSz() , 0);
    EXPECT_EQ(sm.aborted() , false);
    
    roiWindow<P8U> tmp (640, 480);
    tmp.randomFill(1066);
    
    for (uint32_t i = 0; i < images.size(); i++) {
        images[i] = tmp;
    }
    
    deque<double> ent;
    
    bool fRet = sm.fill(images);
    EXPECT_EQ(fRet, true);
    
    bool eRet = sm.entropies(ent);
    EXPECT_EQ(eRet, true);
    
    EXPECT_EQ(ent.size() , images.size());
    
    for (uint32_t i = 0; i < ent.size(); i++)
        EXPECT_EQ(svl::equal(ent[i], 0.0, 1.e-9), true);
    
    
    for (uint32_t i = 0; i < images.size(); i++) {
        roiWindow<P8U> tmp(640, 480);
        tmp.randomFill(i);
        images[i] = tmp;
    }
    
    
    
    fRet = sm.fill(images);
    EXPECT_EQ(fRet, true);
    
    eRet = sm.entropies(ent);
    EXPECT_EQ(eRet, true);
    
    EXPECT_EQ(ent.size() , images.size());
    EXPECT_EQ(fRet, true);
    
    
    deque<deque<double> > matrix;
    sm.selfSimilarityMatrix(matrix);
    //  rfDumpMatrix (matrix);
    
    // Test RefSimilarator
    self_similarity_producerRef simi (new self_similarity_producer<P8U> (7, 0));
    EXPECT_EQ (simi.use_count() , 1);
    self_similarity_producerRef simi2 (simi);
    EXPECT_EQ (simi.use_count() , 2);
    
    // Test Base Filer
    // vector<double> signal (32);
    // simi->filter (signal);
}



TEST (UT_cm_timer, run)
{
    cm_time c0;
    EXPECT_TRUE(c0.isValid());
    EXPECT_TRUE(c0.isZero());
    EXPECT_FALSE(c0.isNegativeInfinity());
    EXPECT_FALSE(c0.isPositiveInfinity());
    EXPECT_FALSE(c0.isIndefinite());
    EXPECT_TRUE(c0.isNumeric());
    
    cm_time c1 (0.033);
    EXPECT_EQ(c1.getScale(), 60000);
    EXPECT_EQ(c1.getValue(), 60000 * 0.033);
    c1.show();
    std::cout << c1 << std::endl;
    
    EXPECT_TRUE(c1.isValid());
    EXPECT_FALSE(c1.isZero());
    EXPECT_FALSE(c1.isNegativeInfinity());
    EXPECT_FALSE(c1.isPositiveInfinity());
    EXPECT_FALSE(c1.isIndefinite());
    EXPECT_TRUE(c1.isNumeric());
    
}


TEST (UT_QtimeCache, run)
{
    boost::filesystem::path test_filepath;
    
    // vf does not support QuickTime natively. The ut expectes and checks for failure
    static std::string qmov_name ("box-move.m4v");
    
    auto res = dgenv_ptr->asset_path(qmov_name);
    EXPECT_TRUE(res.second);
    EXPECT_TRUE(boost::filesystem::exists(res.first));
    
    if (res.second)
        test_filepath = res.first;
    
    ci::qtime::MovieSurfaceRef m_movie;
    m_movie = qtime::MovieSurface::create( test_filepath.string() );
    EXPECT_TRUE(m_movie->isPlayable ());
    
    std::shared_ptr<seqFrameContainer> sm = seqFrameContainer::create(m_movie);
    
    Surface8uRef s8 = ci::Surface8u::create(123, 321, false);
    time_spec_t t0 = 0.0f;
    time_spec_t t1 = 0.1f;
    time_spec_t t2 = 0.2f;
    
    EXPECT_TRUE(sm->loadFrame(s8, t0)); // loaded the first time
    EXPECT_FALSE(sm->loadFrame(s8, t0)); // second time at same stamp, uses cache return true
    EXPECT_TRUE(sm->loadFrame(s8, t1)); // second time at same stamp, uses cache return true
    EXPECT_TRUE(sm->loadFrame(s8, t2)); // second time at same stamp, uses cache return true
    EXPECT_FALSE(sm->loadFrame(s8, t0)); // second time at same stamp, uses cache return true
    
}



#endif




/***
 **  Use --gtest-filter=*NULL*.*shared" to select https://code.google.com/p/googletest/wiki/V1_6_AdvancedGuide#Running_Test_Programs%3a_Advanced_Options
 */


#define GTESTLOG "GLog"
#define GTESTLOG_INFO(...) spdlog::get("GLog")->info(__VA_ARGS__)
#define GTESTLOG_TRACE(...) spdlog::get("GLog")->trace(__VA_ARGS__)
#define GTESTLOG_ERROR(...) spdlog::get("GLog")->error(__VA_ARGS__)
#define GTESTLOG_WARNING(...) spdlog::get("GLog")->warn(__VA_ARGS__)
#define GTESTLOG_NOTICE(...) spdlog::get("GLog")->notice(__VA_ARGS__)
#define GTESTLOG_SEPARATOR() GTESTLOG_INFO("-----------------------------")


int main(int argc, char ** argv)
{
    static std::string id("VGtest");
    std::shared_ptr<test_utils::genv>  shared_env (new test_utils::genv(argv[0]));
    shared_env->setUpFromArgs(argc, argv);
    dgenv_ptr = shared_env;
    fs::path pp(argv[0]);
    setup_loggers(pp.parent_path().string(), id);
    testing::InitGoogleTest(&argc, argv);
    
    
    auto ret = RUN_ALL_TESTS();
    std::cerr << ret << std::endl;
    
}


/***
 **  Implementation of Support files
 */


bool setup_loggers (const std::string& log_path,  std::string id_name){
    
    try{
        // get a temporary file name
        std::string logname =  logging::reserve_unique_file_name(log_path,
                                                                 logging::create_timestamped_template(id_name));
        
        // Setup APP LOG
        auto daily_file_sink = std::make_shared<logging::sinks::daily_file_sink_mt>(logname, 23, 59);
        auto console_sink = std::make_shared<spdlog::sinks::stdout_sink_mt>();
        console_sink->set_level(spdlog::level::warn);
        console_sink->set_pattern("G[%H:%M:%S:%e:%f %z] [%n] [%^---%L---%$] [thread %t] %v");
        std::vector<spdlog::sink_ptr> sinks;
        sinks.push_back(daily_file_sink);
        sinks.push_back(console_sink);
        
        auto combined_logger = std::make_shared<spdlog::logger>("GLog", sinks.begin(),sinks.end());
        combined_logger->info("Daily Log File: " + logname);
        //register it if you need to access it globally
        spdlog::register_logger(combined_logger);
        
        {
            auto combined_logger = std::make_shared<spdlog::logger>("VLog", sinks.begin(),sinks.end());
            combined_logger->info("Daily Log File: " + logname);
            //register it if you need to access it globally
            spdlog::register_logger(combined_logger);
        }
    }
    catch (const spdlog::spdlog_ex& ex)
    {
        std::cout << "Log initialization failed: " << ex.what() << std::endl;
        return false;
    }
    return true;
}



std::shared_ptr<std::ofstream> make_shared_ofstream(std::ofstream * ifstream_ptr)
{
    return std::shared_ptr<std::ofstream>(ifstream_ptr, ofStreamDeleter());
}

std::shared_ptr<std::ofstream> make_shared_ofstream(std::string filename)
{
    return make_shared_ofstream(new std::ofstream(filename, std::ofstream::out));
}



void output_array (const std::vector<std::vector<float>>& data, const std::string& output_file){
    std::string delim (",");
    fs::path opath (output_file);
    auto papa = opath.parent_path ();
    if (fs::exists(papa))
    {
        std::shared_ptr<std::ofstream> myfile = make_shared_ofstream(output_file);
        for (const vector<float>& row : data)
        {
            auto cnt = 0;
            auto cols = row.size() - 1;
            for (const float & col : row)
            {
                *myfile << col;
                if (cnt++ < cols)
                    *myfile << delim;
            }
            *myfile << std::endl;
        }
    }
}

SurfaceRef get_surface(const std::string & path){
    static std::map<std::string, SurfaceWeakRef> cache;
    static std::mutex m;
    
    std::lock_guard<std::mutex> hold(m);
    SurfaceRef sp = cache[path].lock();
    if(!sp)
    {
        auto ir = loadImage(path);
        cache[path] = sp = Surface8u::create(ir);
    }
    return sp;
}


void norm_scale (const std::vector<double>& src, std::vector<double>& dst)
{
    vector<double>::const_iterator bot = std::min_element (src.begin (), src.end() );
    vector<double>::const_iterator top = std::max_element (src.begin (), src.end() );
    
    if (svl::equal(*top, *bot)) return;
    double scaleBy = *top - *bot;
    dst.resize (src.size ());
    for (int ii = 0; ii < src.size (); ii++)
        dst[ii] = (src[ii] - *bot) / scaleBy;
}


void finalize_segmentation (cv::Mat& space){
    
    using blob=svl::labelBlob::blob;
    static bool s_results_ready = false;
    static bool s_graphics_ready = false;
    static int64_t cid = 0;
    
    
    cv::Point replicated_pad (5,5);
    cv::Mat mono, bi_level;
    copyMakeBorder(space,mono, replicated_pad.x,replicated_pad.y,
                   replicated_pad.x,replicated_pad.y, BORDER_REPLICATE, 0);
    
    threshold(mono, bi_level, 126, 255, THRESH_BINARY | THRESH_OTSU);
    //Show source image
#ifdef INTERACTIVE
    imshow("Monochrome Image",mono);
    imshow("Binary Image", bi_level);
#endif
    
    labelBlob::ref lbr = labelBlob::create(mono, bi_level, 10, 666);
    EXPECT_EQ(lbr == nullptr , false);
    std::function<labelBlob::results_ready_cb> res_ready_lambda = [](int64_t& cbi){ s_results_ready = ! s_results_ready; cid = cbi;};
    std::function<labelBlob::graphics_ready_cb> graphics_ready_lambda = [](){ s_graphics_ready = ! s_graphics_ready;};
    boost::signals2::connection results_ready_ = lbr->registerCallback(res_ready_lambda);
    boost::signals2::connection graphics_ready_ = lbr->registerCallback(graphics_ready_lambda);
    EXPECT_EQ(false, s_results_ready);
    EXPECT_EQ(true, cid == 0);
    EXPECT_EQ(true, lbr->client_id() == 666);
    lbr->run_async();
    while (! s_results_ready) std::this_thread::sleep_for(std::chrono::milliseconds(5));
    
    EXPECT_EQ(true, s_results_ready);
    EXPECT_EQ(true, lbr->client_id() == 666);
    EXPECT_EQ(true, cid == 666);
    EXPECT_EQ(true, lbr->hasResults());
    const std::vector<blob> blobs = lbr->results();
    
    
    lbr->drawOutput();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    EXPECT_EQ(true, s_graphics_ready);
#ifdef INTERACTIVE
    /// Show in a window
    namedWindow( "LabelBlob ", CV_WINDOW_AUTOSIZE | WINDOW_OPENGL);
    //    std::vector<cv::KeyPoint> one;
    //    one.push_back(lbr->keyPoints()[1]);
    cv::drawKeypoints(mono, lbr->keyPoints(),bi_level, cv::Scalar(0,255,0),cv::DrawMatchesFlags::DRAW_RICH_KEYPOINTS );
    imshow( "LabelBlob", bi_level);
    cv::waitKey();
#endif
    
    
    
    
}


cv::Mat generateVoxelSelfSimilarities (std::vector<std::vector<roiWindow<P8U>>>& voxels,
                                       std::vector<std::vector<float>>& ss){
    
    int height = static_cast<int>(voxels.size());
    int width = static_cast<int>(voxels[0].size());
    
    
    // Create a single vector of all roi windows
    std::vector<roiWindow<P8U>> all;
    // semilarity producer
    auto sp = std::shared_ptr<sm_producer> ( new sm_producer () );
    for(std::vector<roiWindow<P8U>>& row: voxels){
        for(roiWindow<P8U>& voxel : row){
            all.emplace_back(voxel.frameBuf(), voxel.bound());
        }
    }
    
    sp->load_images (all);
    std::packaged_task<bool()> task([sp](){ return sp->operator()(0);}); // wrap the function
    std::future<bool>  future_ss = task.get_future();  // get a future
    std::thread(std::move(task)).join(); // Finish on a thread
    if (future_ss.get())
    {
        cv::Mat m_temporal_ss (height,width, CV_32FC1);
        m_temporal_ss.setTo(0.0f);
        
        const deque<double>& entropies = sp->shannonProjection ();
        vector<float> m_entropies;
        ss.resize(0);
        m_entropies.insert(m_entropies.end(), entropies.begin(), entropies.end());
        vector<float>::const_iterator start = m_entropies.begin();
        for (auto row =0; row < height; row++){
            vector<float> rowv;
            auto end = start + width; // point to one after
            rowv.insert(rowv.end(), start, end);
            ss.push_back(rowv);
            start = end;
        }
        
        auto getMat = [] (std::vector< std::vector<float> > &inVec){
            int rows = static_cast<int>(inVec.size());
            int cols = static_cast<int>(inVec[0].size());
            
            cv::Mat_<float> resmat(rows, cols);
            for (int i = 0; i < rows; i++)
            {
                resmat.row(i) = cv::Mat(inVec[i]).t();
            }
            return resmat;
        };
        
        m_temporal_ss = getMat(ss);
        return m_temporal_ss;
    }
    return cv::Mat ();
    
}


#endif


#pragma GCC diagnostic pop
