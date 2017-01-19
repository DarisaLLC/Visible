#ifndef __OPENCV_UTILS__
#define __OPENCV_UTILS__


#include <cstdint>

#include <algorithm>
#include <iostream>
#include <typeinfo>
#include "vision/roiWindow.h"

#include <opencv2/opencv.hpp>

using namespace cv;

/*! Functions related to affine transformations. */
namespace svl
{

    
    struct ellipse_parms
    {
        double a;
        double b;
        double theta;
        double eigen_angle;
        Point2d mc;
        Moments mu;
        bool eigen_ok;
    };

    void NewFromOCV (const cv::Mat& mat, roiWindow<P8U>& roi);
    void NewFromSVL (const roiWindow<P8U>& roi, cv::Mat& mat);
    void CopyFromSVL (const roiWindow<P8U>& roi, cv::Mat& mat);
    
    
#define cv_drawCross( img, center, color, d )\
cv::line( img, cv::Point( center.x - d, center.y), cv::Point( center.x + d, center.y), color, 1, CV_AA, 0); \
cv::line( img, cv::Point( center.x, center.y - d ), cv::Point( center.x , center.y + d ), color, 1, CV_AA, 0 )
    
    template <typename ElemT, int cn>
    inline float vecNormSqrd(const cv::Vec<ElemT, cn> &v);
    
    float vecAngle(const cv::Vec2f &a, const cv::Vec2f &b);
    
    float orintedAngle (const cv::Point2f &a, const cv::Point2f &b, const cv::Point2f &c);
    
    
    
    void drawPolygon( cv::Mat &im,const std::vector<cv::Point> &pts,const cv::Scalar &color=cv::Scalar(255, 255, 255),
                     int thickness=1,
                     int lineType=8,
                     bool convex=true
                     );
    
    cv::Mat &getRotatedROI(const cv::Mat &src, const cv::RotatedRect &r, cv::Mat &dest);
    void xformMat(const cv::Mat &src, cv::Mat &dest, float angle, float scale=1.0, cv::Mat *transMat=NULL);
    
    cv::Size2f rotatedSize(cv::Size2f s, float angle);
    cv::Point2f warpAffinePt(const cv::Mat_<float> &transMat, cv::Point2f pt);
    
    
    void compute2DRectCorners(const cv::RotatedRect rect, std::vector<cv::Point2f> & corners);
    
    bool direction_moments (cv::Mat image, ellipse_parms& res);
    
    void getLuminanceCenterOfMass (const cv::Mat& gray, cv::Point2f& com);
    
    void computeNormalizedColorHist(const Mat& image, Mat& hist, int N, double minProb);
    
    bool matIsEqual(const cv::Mat mat1, const cv::Mat mat2);
       
    template <typename T, typename U>
    Size_<T> operator*(const Size_<T> &s, U a) {
        return Size_<T>(s.width * a, s.height * a);
    }
    
    template <typename T, typename U>
    Size_<T> operator*(U a, const Size_<T> &s) {
        return Size_<T>(s.width * a, s.height * a);
    }
    
    template <typename T, typename U>
    Size2f operator/(const Size2f &s, float a) {
        return Size_<T>(s.width / a, s.height / a);
    }
    
    template <typename T, typename U>
    bool operator==(const Size_<T> &a, const Size_<U> &b) {
        return (a.width == b.width and a.height == b.height);
    }
    
    template <typename T, typename U>
    bool operator!=(const Size_<T> &a, const Size_<U> &b) {
        return not (a == b);
    }
    
    
    template <typename T>
    std::ostream &operator<<(std::ostream &out, Rect_<T> r) {
        return out << r.x << " y=" << r.y << " width=" << r.width << " height=" << r.height << ">";
    }
 
    class motionSmear
    {
        public:
            motionSmear ();
            void add (const cv::Mat&) const;
            const cv::Mat& signature (ellipse_parms&) const;
            const uint32_t count () const;
        
        
        private:
        mutable cv::Mat m_min;
        mutable cv::Mat m_max;
        mutable cv::Mat m_sig;
        mutable uint32_t m_count;
        mutable bool m_done;

          
    };
    
    
    std::ostream &operator<<(std::ostream &out, const Scalar s);
    
    /*! This must be overloaded, otherwise it prints it as a `char` instead of an
     * `unsigned`, which is almost always what is wanted.
     */
    std::ostream &operator<<(std::ostream &out, const uchar c);
    
    template <typename T, int cn>
    std::ostream &operator<<(std::ostream &out, const Vec<T, cn> v) {
        out << "<Vec cn=" << cn << " data=" << "(";
        int i;
        for (i = 0; i < cn - 1; i++) {
            out << v[i] << ", ";
        }
        out << v[i] << ")>";
        
        return out;
    }
    
}

#endif


