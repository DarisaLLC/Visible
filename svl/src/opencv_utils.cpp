#include <cstdint>

#include <algorithm>
#include <iostream>

#include <opencv2/opencv.hpp>
#include "vision/opencv_utils.hpp"
#include "vision/gradient.h"

using namespace std;
using namespace cv;
using namespace ci;

namespace svl
{
    void NewFromOCV (const cv::Mat& mat, roiWindow<P8U>& roi)
    {
        if (! roi.frameBuf() || !roi.isBound() ||
            mat.cols != roi.width() || mat.rows != roi.height())
        {
            roiWindow<P8U> win (mat.cols, mat.rows);
            roi = win;
        }
        for (uint32_t row = 0; row < mat.rows; row++)
            std::memmove(roi.rowPointer(row), mat.ptr(row), mat.cols);
        
    }
    
    void NewFromSVL (const roiWindow<P8U>& roi, cv::Mat& mat)
    {
        mat.create(roi.height(), roi.width(), CV_8U);
        CopyFromSVL(roi, mat);
    }
    
    void CopyFromSVL (const roiWindow<P8U>& roi, cv::Mat& mat)
    {
        assert (!mat.empty() && mat.cols == roi.width() && mat.rows == roi.height());
        for (uint32_t row = 0; row < mat.rows; row++)
            std::memmove(mat.ptr(row), roi.rowPointer(row), mat.cols);
    }
    
    bool matIsEqual(const cv::Mat mat1, const cv::Mat mat2){
        // treat two empty mat as identical as well
        if (mat1.empty() && mat2.empty()) {
            return true;
        }
        // if dimensionality of two mat is not identical, these two mat is not identical
        if (mat1.cols != mat2.cols || mat1.rows != mat2.rows || mat1.dims != mat2.dims) {
            return false;
        }
        cv::Mat diff;
        cv::compare(mat1, mat2, diff, cv::CMP_NE);
        int nz = cv::countNonZero(diff);
        return nz==0;
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
    
    bool direction_moments (cv::Mat image, ellipse_parms& res)
    {
        Moments mu =  moments(image, false);
        Point2d mc(mu.m10/mu.m00, mu.m01/mu.m00);
        double m11 = mu.m11 ;
        double m20 = mu.m20 ;
        double m02 = mu.m02 ;
        Mat M = (Mat_<double>(2,2) << m20, m11, m11, m02);
        
        //Compute and store the eigenvalues and eigenvectors of covariance matrix CvMat* e_vect = cvCreateMat( 2 , 2 , CV_32FC1 );
        Mat e_vects = (Mat_<float>(2,2) << 0.0,0.0,0.0,0.0);
        Mat e_vals = (Mat_<float>(1,2) << 0.0,0.0);

        bool eigenok = cv::eigen(M, e_vals, e_vects);
        int ev = (e_vals.at<float>(0,0) > e_vals.at<float>(0,1)) ? 0 : 1;
        std::cout <<  e_vals.at<float>(0,0) << " " <<  e_vals.at<float>(0,1) << std::endl;
        
        double angle = cv::fastAtan2 (e_vects.at<float>(ev, 0),e_vects.at<float>(ev, 1));

        
        double mm2 = m20 - m02;
        auto tana = mm2 + std::sqrt (4*m11*m11 + mm2*mm2);
        double theta = atan(2*m11 / tana);
        double cos2 = cos(theta)*cos(theta);
        double sin2 = sin(theta)*sin(theta);
        double sin2x = sin(theta)*cos(theta);
        
        double lambda1 = m20*cos2 + m11 * sin2x + m02*sin2;
        double lambda2 = m20*cos2 - m11 * sin2x + m02*sin2;
        
        res.mu = mu;
        res.mc = mc;
        res.a = 2.0 * std::sqrt(lambda1/mu.m00);
        res.b = 2.0 * std::sqrt(lambda2/mu.m00);
        res.theta = theta;
        
        return true;
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
    
    
    //////////  Motion Smear Implementation /////////////////
    /////////////////////////////////////////////////////////
    
    motionSmear::motionSmear () : m_done (false), m_count(0) {}
    void motionSmear::add(const cv::Mat& src) const
    {
        if (m_min.empty() || m_max.empty() || m_sig.empty())
        {
            m_min = src.clone();//m_min = 0.0;
            m_max = src.clone();//m_max = 0.0;
            m_sig = src.clone();m_sig = 0.0;
        }
        cv::min(src, m_min, m_min);
        cv::max(src, m_max, m_max);
        cv::subtract(m_max, m_min, m_sig);
        m_count += 1;
    }
    const cv::Mat& motionSmear::signature (ellipse_parms& ep) const
    {
        direction_moments (m_sig, ep);
        return m_sig;
    }
    
    const uint32_t motionSmear::count () const
    {
        return m_count;
    }
    
    /////////////////////////////////////////////////////////
    
    
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
