#include <cstdint>

#include <algorithm>
#include <iostream>

#include <opencv2/opencv.hpp>
#include <cinder/cinderMath.h>

#include "cinder/opencv_utils.hpp"

using namespace std;
using namespace cv;
using namespace ci;

namespace svl
{
  void NewFromOCV (const cv::Mat& mat, roiWindow<P8U>& roi)
    {
        roiWindow<P8U> win (mat.cols, mat.rows);
        for (uint32_t row = 0; row < mat.rows; row++)
            std::memmove(win.rowPointer(row), mat.ptr(row), mat.cols);
        roi = win;
    }
    void NewFromSVL (const roiWindow<P8U>& roi, cv::Mat& mat)
    {
        mat = cv::Mat (roi.height(), roi.width(), CV_8U);
        for (uint32_t row = 0; row < mat.rows; row++)
            std::memmove(mat.ptr(row), roi.rowPointer(row), mat.cols);
    }


void DrawHistogram( MatND histograms[], int number_of_histograms, Mat& display_image )
{
    int number_of_bins = histograms[0].size[0];
    double max_value=0, min_value=0;
    double channel_max_value=0, channel_min_value=0;
    for (int channel=0; (channel < number_of_histograms); channel++)
    {
        minMaxLoc(histograms[channel], &channel_min_value, &channel_max_value, 0, 0);
        max_value = ((max_value > channel_max_value) && (channel > 0)) ? max_value : channel_max_value;
        min_value = ((min_value < channel_min_value) && (channel > 0)) ? min_value : channel_min_value;
    }
    float scaling_factor = ((float)256.0)/((float)number_of_bins);
    
    Mat histogram_image((int)(((float)number_of_bins)*scaling_factor),(int)(((float)number_of_bins)*scaling_factor),CV_8UC3,Scalar(255,255,255));
    display_image = histogram_image;
    int highest_point = static_cast<int>(0.9*((float)number_of_bins)*scaling_factor);
    for (int channel=0; (channel < number_of_histograms); channel++)
    {
        int last_height;
        for( int h = 0; h < number_of_bins; h++ )
        {
            float value = histograms[channel].at<float>(h);
            int height = static_cast<int>(value*highest_point/max_value);
            int where = (int)(((float)h)*scaling_factor);
            if (h > 0)
                line(histogram_image,cv::Point((int)(((float)(h-1))*scaling_factor),(int)(((float)number_of_bins)*scaling_factor)-last_height),
                     cv::Point((int)(((float)h)*scaling_factor),(int)(((float)number_of_bins)*scaling_factor)-height),
                     cv::Scalar(channel==0?255:0,channel==1?255:0,channel==2?255:0));
            last_height = height;
        }
    }
}

void compute2DRectCorners(const cv::RotatedRect rect, std::vector<cv::Point2f> & corners)
{
    corners.resize(4);
    float a = (float) sin(toRadians(rect.angle)) * 0.5f;
    float b = (float) cos(toRadians(rect.angle)) * 0.5f;
    corners[0].x = rect.center.x - a * rect.size.height - b * rect.size.width;
    corners[0].y = rect.center.y + b * rect.size.height - a * rect.size.width;
    corners[1].x = rect.center.x + a * rect.size.height - b * rect.size.width;
    corners[1].y = rect.center.y - b * rect.size.height - a * rect.size.width;
    corners[2].x = 2 * rect.center.x - corners[0].x;
    corners[2].y = 2 * rect.center.y - corners[0].y;
    corners[3].x = 2 * rect.center.x - corners[1].x;
    corners[3].y = 2 * rect.center.y - corners[1].y;
}

  
    
    void getLuminanceCenterOfMass (const cv::Mat& gray, cv::Point2f& com)
    {
        CvMoments moms = cv::moments(gray);
        double inv_m00 = moms.inv_sqrt_m00*moms.inv_sqrt_m00;
        com = cv::Point2f ((moms.m10 * inv_m00), moms.m01 * inv_m00);
    }
    
    void computeNormalizedColorHist(const Mat& image, Mat& hist, int N, double minProb)
    {
        const int histSize[] = {N, N, N};
        
        // make sure that the histogram has a proper size and type
        hist.create(3, histSize, CV_32F);
        
        // and clear it
        hist = Scalar(0);
        
        // the loop below assumes that the image
        // is a 8-bit 3-channel. check it.
        CV_Assert(image.type() == CV_8UC3);
        MatConstIterator_<Vec3b> it = image.begin<Vec3b>(),
        it_end = image.end<Vec3b>();
        for( ; it != it_end; ++it )
        {
            const Vec3b& pix = *it;
            hist.at<float>(pix[0]*N/256, pix[1]*N/256, pix[2]*N/256) += 1.f;
        }
        
        minProb *= image.rows*image.cols;
        
        // intialize iterator (the style is different from STL).
        // after initialization the iterator will contain
        // the number of slices or planes the iterator will go through.
        // it simultaneously increments iterators for several matrices
        // supplied as a null terminated list of pointers
        const Mat* arrays[] = {&hist, 0};
        Mat planes[1];
        NAryMatIterator itNAry(arrays, planes, 1);
        double s = 0;
        // iterate through the matrix. on each iteration
        // itNAry.planes[i] (of type Mat) will be set to the current plane
        // of the i-th n-dim matrix passed to the iterator constructor.
        for(int p = 0; p < itNAry.nplanes; p++, ++itNAry)
        {
            threshold(itNAry.planes[0], itNAry.planes[0], minProb, 0, THRESH_TOZERO);
            s += sum(itNAry.planes[0])[0];
        }
        
        s = 1./s;
        itNAry = NAryMatIterator(arrays, planes, 1);
        for(int p = 0; p < itNAry.nplanes; p++, ++itNAry)
            itNAry.planes[0] *= s;
    }
    
    void drawPolygon(
                          Mat &im,
                          const vector<cv::Point> &pts,
                          const Scalar &color,
                          int thickness,
                          int lineType,
                          bool convex
                          )
    {
        int nPts = int(pts.size());
        const cv::Point *startPt = &pts[0];
        if (thickness < 0) {
            if (convex) {
                fillConvexPoly(im, startPt, nPts, color, lineType);
            } else {
                fillPoly(im, &startPt, &nPts, 1, color, lineType);
            }
        } else {
            polylines(im, &startPt, &nPts, 1, true, color, thickness, lineType);
        }
    }
    
    
    Point2f map_point(const Mat_<float> &transMat, Point2f pt)
    {
        pt.x = transMat(0, 0) * pt.x + transMat(0, 1) * pt.y + transMat(0, 2);
        pt.y = transMat(1, 0) * pt.y + transMat(1, 1) * pt.y + transMat(1, 2);
        
        return pt;
    }

#if 0
template <typename ElemT, int cn>
inline float vecNormSqrd(const cv::Vec<ElemT, cn> &v)
{
    float s = 0;
    for (int i = 0; i < cn; i++) {
        s += float(v[i] * v[i]);
    }
    return s;
}

float vecAngle(const Vec2f &a, const Vec2f &b)
{
    float dp = float(a.dot(b));
    float cross = float(b[1] * a[0] - b[0] * a[1]);
    dp /= sqrt(float((vecNormSqrd(a) * vecNormSqrd(b))));
    
    return acos(dp) * float(std::signbit(cross));
}

float orintedAngle (const cv::Point2f &a, const cv::Point2f &b, const cv::Point2f &c) {
    return vecAngle((a - b), (c - b));
}


Mat &svl::getRotatedROI(const Mat &src, const RotatedRect &r, Mat &dest)
{
    Mat transMat;
    xformMat(src, dest, r.angle, 1.0, &transMat);
    cv::Point newPt = svl::warpAffinePt(transMat, r.center);
    dest = dest(cv::Rect(Point2f(float(newPt.x) - r.size.width * 0.5f, float(newPt.y) - r.size.height * 0.5f), r.size));
    return dest;
}




    {
ostream &cv::operator<<(ostream &out, const Scalar s) {
    out << "<Scalar (" << s[0] << ", " << s[1] << ", " << s[2] << ", " << s[3] << ")>";
    return out;
}

ostream &cv::operator<<(ostream &out, const uchar c) {
    out << unsigned(c);
    return out;
}
    }
#endif
    
}
