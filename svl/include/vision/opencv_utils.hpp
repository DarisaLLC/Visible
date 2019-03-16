#ifndef __OPENCV_UTILS__
#define __OPENCV_UTILS__


#include <unordered_map>
#include <deque>
#include "opencv2/opencv.hpp"
#include "vision/roiWindow.h"
#include "vision/roiVariant.hpp"
#include <stdio.h>
#include <iostream>

using namespace cv;
using namespace std;
using namespace svl;

/*! Functions related to affine transformations. */
namespace svl
{
    // Use view roiWindow memory as cv::Mat.
    // Requires cvType to be appropriate
    // Use within scope. As roiWindow is ref counted. 
    #define cvMatOfroiWindow(a,b,cvType) cv::Mat b ((a).height(),(a).width(), cvType,(a).pelPointer(0,0), size_t((a).rowUpdate()))
    
    double correlation_ocv(const roiWindow<P8U>& i, const roiWindow<P8U>& m);
    void cumani_opencv (const cv::Mat& input_bgr, cv::Mat& gradAbs, cv::Mat& orientation, float& maxVal);
    void PeakDetect(const cv::Mat& space, std::vector<Point2f>& peaks, uint8_t accept);
    void PeakMapDetect(const cv::Mat& space, std::unordered_map<uint8_t,std::vector<Point2f>>& peaks, uint8_t accept);
    
    double getPSNR(const cv::Mat& I1, const cv::Mat& I2);
    
#define cv_drawCross( img, center, color, d )\
cv::line( img, cv::Point( center.x - d, center.y), cv::Point( center.x + d, center.y), color, 1, CV_AA, 0); \
cv::line( img, cv::Point( center.x, center.y - d ), cv::Point( center.x , center.y + d ), color, 1, CV_AA, 0 )
    
    template <typename ElemT, int cn>
    inline float vecNormSqrd(const cv::Vec<ElemT, cn> &v);
    
    float vecAngle(const cv::Vec2f &a, const cv::Vec2f &b);
    
    float orintedAngle (const cv::Point2f &a, const cv::Point2f &b, const cv::Point2f &c);
    
    void skinMaskYCrCb (const cv::Mat &src, cv::Mat &dst);
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
    
   // bool direction_moments (cv::Mat image, momento& res);
    
    float Median1dF (const cv::Mat& fmat, size_t& loc);
    
    void horizontal_vertical_projections (const cv::Mat& mat, cv::Mat& hz, cv::Mat& vt);
  
    std::pair<size_t, size_t> medianPoint (const cv::Mat& mat);
    
    
    void computeNormalizedColorHist(const cv::Mat& image, cv::Mat& hist, int N, double minProb);
    
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
 
    struct ssBlob
    {
        cv::Point loc;
        float radius;
        float ss_val;
    };
    
    void CreateGaussSpace(const cv::Mat& img, std::vector<cv::Mat>& blured, float tmin, float tmax, float tdelta);
    
    void CreateScaleSpace(const cv::Mat& img, std::vector<cv::Mat>& blured, float tmin, float tmax, float tdelta,
                          std::vector<cv::Mat>& scaled,
                          std::vector<ssBlob>& blobs);
    
    cv::Mat gaussianTemplate(const std::pair<uint32_t,uint32_t>& dims, const vec2& sigma = vec2(1.0, 1.0), const vec2& center = vec2(0.5,0.5));
    
    
    float crossMatch (const cv::Mat& img, const cv::Mat& model, cv::Mat& space, bool squareit = true);
    cv::Mat selfMatch (const cv::Mat& img, int32_t dia = 5, bool squareit = true);
    cv::Mat1b acf (const cv::Mat& img, int32_t dia = 5, uint32_t precision = 3);
    size_t writeSpaceToStl (const cv::Mat& space,  std::vector<std::vector<float>> out);
    
    // Computes the 1D histogram.
    cv::Mat getHistogram(const cv::Mat1b &image, const cv::Mat1b& mask = cv::Mat1b () );
   
    // Return -1 or 0-255 for left_tail
    int leftTailPost (const cv::Mat1b& image, float left_tail_fraction);
    void toHistVector (const cv::Mat& hist, std::vector<float>& vh);
    void generate_mask (const cv::Mat& rg, cv::Mat& mask, float left_tail_post, bool debug_output = false);
   
    void sobel_opencv (const cv::Mat& input_gray, cv::Mat& mag, cv::Mat& ang);
    void getRangeC (const cv::Mat& onec, vec2& range);
    double median( cv::Mat channel );
    
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
    
    void output(cv::Mat mat, int prec = 0, char be = '{', char en = '}');
    
    void outputU8 (const cv::Mat& mat, char be = '{', char en = '}');
    
    //Function used to perform the complex DFT of a grayscale image
    //Input:  image
    //Output: DFT (complex numbers)
    void Complex_Gray_DFT(const cv::Mat &src, cv::Mat &dst);
  
    
    //Function used to obtain a magnitude image from a complex DFT
    //Input:  Complex DFT
    //Output: Magnitude Image
    void DFT_Magnitude(const cv::Mat &dft_src, cv::Mat &dft_magnitude);
  
    
    //Function used to re-centre the Magnitude of the DFT, such that the centre shows the low frequencies
    //Input/Output: Magnitude image
    void Recentre(cv::Mat &dft_magnitude);
  
    
    //Function to perform the inverse DFT of a Gray Scale image and obtain an image.
    //Note that the values will be real, not complex
    //Input:  DFT of an image
    //Output: Image in spatial domain
    void Real_Gray_IDFT(const cv::Mat &src, cv::Mat &dst);
   
    
    //Function used to perform bilateral Filtering
    //Input: src image, neighbourhood size, standard dev for colour and space
    //Output: filtered image
    void Bilateral_Filtering(const cv::Mat&  src, cv::Mat& dst, int neighbourhood, double sigma_colour, double sigma_space);
   
    
    //Function used to perform Wiener Denoising. Note that this function does not reverse the blur
    //Input: src image, constant to multiply with (inverse of PSNR)
    //Output: filtered image
    void Wiener_Denoising(const cv::Mat &src, cv::Mat &dst, double PSNR_inverse);
  
    
    //Function used to perform anisotropic diffusion
    //Input: src image, lambda (used for converance), number of iterations that should run,
    //         k values to define what is an edge and what is not (x2: one for north/south, one for east/west)
    //Output: filtered image
    void Anisotrpic_Diffusion(const cv::Mat& src, cv::Mat& dst, double lambda, int iterations, float k_ns, float k_we);
 
    
    //Function used to perform Gamma correction
    //Input:  NORMALISED src image, gamma
    //Output: Modified image
    void Gamma_Correction(const cv::Mat&  src, cv::Mat& dst, double gamma);
   
}



#endif


