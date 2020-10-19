#include <cstdint>

#include <algorithm>
#include "vision/opencv_utils.hpp"
#include "vision/gradient.h"
#include <cstdio>
#include <iostream>


using namespace std;
using namespace cv;
using namespace svl;

namespace svl
{
    
    cv::Mat getPadded (const cv::Mat& src, uiPair pad, double pad_value){
        cv::Mat padded (src.rows+2*pad.second, src.cols+2*pad.first, src.type());
        padded.setTo(pad_value);
        cv::Mat win (padded, cv::Rect(pad.first, pad.second, src.cols, src.rows));
        src.copyTo(win);
        return padded;
    }
    
    static const char *cv_depth_names[] = {
        "CV_8U",
        "CV_8S",
        "CV_16U",
        "CV_16S",
        "CV_32S",
        "CV_32F",
        "CV_64F"
    };
    
    string matInfo(const Mat &m) {
        char buf[100];
        snprintf(buf, sizeof(buf), "%sC%d(%dx%d)", cv_depth_names[m.depth()], m.channels(), m.rows, m.cols);
        return string(buf);
    }
    
    Mat matRotateSize(Size sizeIn, Point2f center, float angle, float &minx, float &maxx, float &miny, float &maxy, float scale) {
        Mat transform = getRotationMatrix2D( center, angle, scale );
        
        transform.convertTo(transform, CV_32F);
        
        Matx<float,3,4> pts(
                            0, sizeIn.width-1.0f, sizeIn.width-1.0f, 0,
                            0, 0, sizeIn.height-1.0f, sizeIn.height-1.0f,
                            1.0f, 1.0f, 1.0f, 1.0f);
        Mat mpts(pts);
        Mat newPts = transform * mpts;
        minx = newPts.at<float>(0,0);
        maxx = newPts.at<float>(0,0);
        miny = newPts.at<float>(1,0);
        maxy = newPts.at<float>(1,0);
        for (int c=1; c<4; c++) {
            float x = newPts.at<float>(0,c);
            minx = min(minx, x);
            maxx = max(maxx, x);
            float y = newPts.at<float>(1,c);
            miny = min(miny, y);
            maxy = max(maxy, y);
        }

        return transform;
    }
    
    void matWarpAffine(const Mat &image, Mat &result, Point2f center, float angle, float scale,
                       Point2f offset, Size size, int borderMode, Scalar borderValue, Point2f reflect, int flags)
    {
        float minx;
        float maxx;
        float miny;
        float maxy;
        Mat transform = matRotateSize(Size(image.cols,image.rows), center, angle, minx, maxx, miny, maxy, scale);
        
        transform.at<float>(0,2) += offset.x;
        transform.at<float>(1,2) += offset.y;
        
        Size resultSize(size);
        if (resultSize.width <= 0) {
            resultSize.width = (int)(maxx - minx + 1.5);
            transform.at<float>(0,2) += (resultSize.width-1)/2.0f - center.x;
        }
        if (resultSize.height <= 0) {
            resultSize.height = (int)(maxy - miny + 1.5);
            transform.at<float>(1,2) += (resultSize.height-1)/2.0f - center.y;
        }
        
        Mat resultLocal;
        warpAffine( image, resultLocal, transform, resultSize, flags, borderMode, borderValue );
        
        double normReflect = norm(reflect);
//        LOGTRACE3("matWarpAffine() reflect:(%g,%g) norm:%g", reflect.x, reflect.y, normReflect);
        if (normReflect != 0) {
            Mat mReflect = Mat::eye(3,3,CV_32F);
            mReflect.at<float>(0,0) = (reflect.x*reflect.x-reflect.y*reflect.y)/normReflect;
            mReflect.at<float>(1,1) = (reflect.y*reflect.y-reflect.x*reflect.x)/normReflect;
            mReflect.at<float>(0,1) = mReflect.at<float>(1,0) = 2*reflect.x*reflect.y/normReflect;
            // warpAffine does not work properly with reflection. maybe one day it will
            if (reflect.x == 0) {
                flip(resultLocal, resultLocal, 1);    // reflect in y-axis
            } else if (reflect.y == 0) {
                flip(resultLocal, resultLocal, 0);    // reflect in x-axis
            } else if (reflect.x == reflect.y) {
                flip(resultLocal, resultLocal, -1);    // reflect in x- and y-axes
            }
        }
        
        result = resultLocal;
    }
    
    void findPeaks(const Mat& src, std::vector<cv::Point>& peaks) {
        peaks.resize(0);
        //finds local maximas
        for (int i = 1; i < src.rows-1; i++)
        {
            for (int j = 1; j < src.cols - 1; j++)
            {
                if (src.at<float>(i, j) > src.at<float>(i, j - 1) && src.at<float>(i, j) > src.at<float>(i, j + 1)){
                    peaks.emplace_back(i, j);
                }
            }
        }
   
    }
    
    
    
    cv::Mat gaborKernel(int ks, double sig, double th, double lm, double ps)
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
    
    cv::Mat filterWithGaborKernel (cv::Mat& image, cv::Mat& kernel){
        cv::Mat src_f, dest;
        cv::Mat src = image.clone();
        if(image.channels() > 1)
            cv::cvtColor(image, src, COLOR_BGR2GRAY);
        src.convertTo(src_f, CV_32F, 1.0/255, 0);
        cv::filter2D(src_f, dest, CV_32F, kernel);
        return dest;
    }

    std::string to_string (const cv::RotatedRect& rr){
        std::ostringstream ss;
        ss << "[ + " << rr.center << " / " << rr.angle << " | " << rr.size.height << " - " << rr.size.width << "]";
        return ss.str();
    }

    
    double integralMatSum(const Mat &integralMat, Rect roi) {
        switch (integralMat.type()) {
            case CV_32SC1:
                return integralMat.at<int32_t>(roi.br())
                - integralMat.at<int32_t>(roi.y + roi.height, roi.x)
                - integralMat.at<int32_t>(roi.y, roi.x + roi.width)
                + integralMat.at<int32_t>(roi.tl());
            case CV_32FC1:
                return integralMat.at<float>(roi.br())
                - integralMat.at<float>(roi.y + roi.height, roi.x)
                - integralMat.at<float>(roi.y, roi.x + roi.width)
                + integralMat.at<float>(roi.tl());
            case CV_64FC1:
                return integralMat.at<double>(roi.br())
                - integralMat.at<double>(roi.y + roi.height, roi.x)
                - integralMat.at<double>(roi.y, roi.x + roi.width)
                + integralMat.at<double>(roi.tl());
            default:
                throw invalid_argument("unsupported type of integralMat");
        }
    }
    

    
    Size2f rotateSize(Size2f s, float angle) {
        float w = s.width, h = s.height;
        angle = float(fabs(angle / 180 * svl::constants::pi));
        float diagAngle = atan(h / w);
        float diagLen = sqrt(w * w + h * h);
        return Size2f(
                      std::roundf(fabs(diagLen * cos(diagAngle - angle))),
                      std::roundf(fabs(diagLen * sin(diagAngle + angle)))
                      );
    }

    void get_mid_points(const cv::RotatedRect& rotrect, std::vector<cv::Point2f>& mids) {
        //Draws a rectangle
        cv::Point2f rect_points[4];
        rotrect.points( &rect_points[0] );
        for( int j = 0; j < 4; j++ ) {
            mids.emplace_back ((rect_points[j].x + rect_points[(j + 1) % 4].x)/2.0f,
                        (rect_points[j].y + rect_points[(j + 1) % 4].y)/2.0f);
        }
    }
    


    cv::Point2f rotate2d(const cv::Point2f& pt_in, const double& angle_rad) {
        cv::Point2f pt_out;
        //CW rotation
        pt_out.x = std::cos(angle_rad) * pt_in.x - std::sin(angle_rad) * pt_in.y;
        pt_out.y = std::sin(angle_rad) * pt_in.x + std::cos(angle_rad) * pt_in.y;
        return pt_out;
    }
    
    cv::Point2f rotatePoint(const cv::Point2f& pt_in, const cv::Point2f& center, const double& angle_rad) {
        return rotate2d(pt_in - center, angle_rad) + center;
    }

    // original from https://stackoverflow.com/a/46635372
    // Computes the quadrant for a and b (0-3):
    //     ^
    //   1 | 0
    //  ---+-->
    //   2 | 3
    //  (a.x > center.x) ? ((a.y > center.y) ? 0 : 3) : ((a.y > center.y) ? 1 : 2)); */
    // In comparing if quadrants are the same return (b.x - center.x) * (a.y - center.y) < (b.y - center.y) * (a.x - center.x);
    //        std::sort(corners.begin(), corners.end(), [=](const Point2f& a, const Point2f&b) {
    //            auto dqa = ccwQ(ctr, a); auto dqb = ccwQ(ctr, b);
    //            if (dqa == dqb) {
    //                return (b.x - ctr.x) * (a.y - ctr.y) < (b.y - ctr.y) * (a.x - ctr.x);
    //            }
    //            return dqa < dqb;
    //        });
    
    
    int ccwQ(const Point2f &center, const Point2f &a){
        const int dax = ((a.x - center.x) > 0) ? 1 : 0;
        const int day = ((a.y - center.y) > 0) ? 1 : 0;
        return  (1 - dax) + (1 - day) + ((dax & (1 - day)) << 1);
    }
    
    RotatedRect RotatedRectOutOf4 (std::vector<cv::Point2f>& src){
        assert(src.size() == 4);
        std::vector<int> idx = {0,1,2,3};
        
        auto pcheck = [idx](std::vector<Point2f>& cand){
            Vec2f vecs[2];
            vecs[0] = Vec2f(cand[idx[0]] - cand[idx[1]]);
            vecs[1] = Vec2f(cand[idx[1]] - cand[idx[2]]);
            // check that given sides are perpendicular
            return ( abs(vecs[0].dot(vecs[1])) / (norm(vecs[0]) * norm(vecs[1])) <= FLT_EPSILON );
        };
        
        do {
            if(pcheck(src)){
                return RotatedRect(src[idx[0]], src[idx[1]], src[idx[2]]);
            }
        } while(std::next_permutation(idx.begin(),idx.end()));
        
        return RotatedRect ();
        
    }
    
    void cpCvMatToRoiWindow8U (const cv::Mat& m, roiWindow<P8U>& r){
        assert(m.type() == CV_8U);
        auto rowPointer = [] (void* data, size_t step, int32_t row ) { return reinterpret_cast<void*>( reinterpret_cast<uint8_t*>(data) + row * step ); };
        unsigned cols = m.cols;
        unsigned rows = m.rows;
        roiWindow<P8U> rw(cols,rows);
        for (auto row = 0; row < rows; row++) {
            std::memcpy(rw.rowPointer(row), rowPointer(m.data, m.step, row), cols);
        }
        r = rw;
    }
    
    void cpCvMatToRoiWindow16U (const cv::Mat& m, roiWindow<P16U>& r){
        assert(m.type() == CV_16U);
        auto rowPointer = [] (void* data, size_t step, int32_t row ) { return reinterpret_cast<void*>( reinterpret_cast<uint16_t*>(data) + row * step ); };
        unsigned cols = m.cols;
        unsigned rows = m.rows;
        roiWindow<P16U> rw(cols,rows);
        for (auto row = 0; row < rows; row++) {
            std::memcpy(rw.rowPointer(row), rowPointer(m.data, m.step, row), cols);
        }
        r = rw;
    }
    
    
    //Function used to perform the complex DFT of a grayscale image
    //Input:  image
    //Output: DFT (complex numbers)
    void Complex_Gray_DFT(const cv::Mat &src, cv::Mat &dst)
    {
        //Convert the original grayscale image into a normalised floating point image with only 1 channel
        cv::Mat src_float;
        src.convertTo(src_float, CV_32FC1);
        cv::normalize(src_float, src_float, 0, 1, cv::NORM_MINMAX);
        
        //Run the DFT
        cv::dft(src_float, dst, cv::DFT_COMPLEX_OUTPUT);
    }
    
    //Function used to obtain a magnitude image from a complex DFT
    //Input:  Complex DFT
    //Output: Magnitude Image
    void DFT_Magnitude(const cv::Mat &dft_src, cv::Mat &dft_magnitude)
    {
        //The dft_src mat is split into 2 separate channels, one for the real and the other for the imaginary
        cv::Mat channels[2] = { cv::Mat::zeros(dft_src.size(),CV_32F), cv::Mat::zeros(dft_src.size(), CV_32F) };
        cv::split(dft_src, channels);
        
        //Compute the Magnitude
        cv::magnitude(channels[0], channels[1], dft_magnitude);
        
        //Obtain the log and normalise it between 0 and 1. +1 since log(0) is undefined.
        cv::log(dft_magnitude + cv::Scalar::all(1), dft_magnitude);
        cv::normalize(dft_magnitude, dft_magnitude, 0, 1, cv::NORM_MINMAX);
    }
    
    //Function used to re-centre the Magnitude of the DFT, such that the centre shows the low frequencies
    //Input/Output: Magnitude image
    void Recentre(cv::Mat &dft_magnitude)
    {
        //define the centre points. These will be used when selecting the quadrants
        int centre_x = dft_magnitude.cols / 2;
        int centre_y = dft_magnitude.rows / 2;
        
        //create the quadrant objects and a temporary object that will be used in the swapping process
        cv::Mat temp;
        //NB: The way the quadrants are created means that they all reference the same matrix. (shallow copy!)
        //    A change in the quadrants results in a change in the source
        cv::Mat quad_1(dft_magnitude, cv::Rect(0, 0, centre_x, centre_y));
        cv::Mat quad_2(dft_magnitude, cv::Rect(centre_x, 0, centre_x, centre_y));
        cv::Mat quad_3(dft_magnitude, cv::Rect(0, centre_y, centre_x, centre_y));
        cv::Mat quad_4(dft_magnitude, cv::Rect(centre_x, centre_y, centre_x, centre_y));
        
        //swapping process
        quad_1.copyTo(temp);
        quad_4.copyTo(quad_1);
        temp.copyTo(quad_4);
        
        quad_2.copyTo(temp);
        quad_3.copyTo(quad_2);
        temp.copyTo(quad_3);
    }
    
    //Function to perform the inverse DFT of a Gray Scale image and obtain an image.
    //Note that the values will be real, not complex
    //Input:  DFT of an image
    //Output: Image in spatial domain
    void Real_Gray_IDFT(const cv::Mat &src, cv::Mat &dst)
    {
        //Perform the inverse dft. Note that, by the flags passed, it will return a
        //real output (since an image is made out of real components) and will also
        //scale the values so that information can be visualised.
        cv::idft(src, dst, cv::DFT_REAL_OUTPUT | cv::DFT_SCALE);
        
        //Note that for consistency purpose, it is recomended to convert the dst to the original image's type and normalise it
        //Here, it is assumed that the image is 8 bits and unsigned
        dst.convertTo(dst, CV_8U, 255);
        cv::normalize(dst, dst, 0, 255, cv::NORM_MINMAX);
    }
    
    //Function used to perform bilateral Filtering
    //Input: src image, neighbourhood size, standard dev for colour and space
    //Output: filtered image
    void Bilateral_Filtering(const cv::Mat&  src, cv::Mat& dst, int neighbourhood, double sigma_colour, double sigma_space)
    {
        cv::bilateralFilter(src, dst, neighbourhood, sigma_colour, sigma_space);
    }
    
   //Function used to perform anisotropic diffusion
   //Input: src image, lambda (used for converance), number of iterations that should run,
   //         k values to define what is an edge and what is not (x2: one for north/south, one for east/west)
   //Output: filtered image
   void Anisotrpic_Diffusion(const cv::Mat& src, cv::Mat& dst, double lambda, int iterations, float k_ns, float k_we)
   {
       //copy the src to dst as initilisation and convert dst to a 32 bit float mat, for accuracy when processing
       dst = src.clone();
       dst.convertTo(dst, CV_32F);
       
       //Create the kernels to be used to calculate edges in North, South, East and West directions
       //These are similar to Laplacian kernels
       cv::Mat_<int> kernel_n = cv::Mat::zeros(3, 3, CV_8U);
       kernel_n(1, 1) = -1;
       cv::Mat_<int> kernel_s, kernel_w, kernel_e;
       kernel_s = kernel_n.clone();
       kernel_w = kernel_n.clone();
       kernel_e = kernel_n.clone();
       
       kernel_n(0, 1) = 1;
       kernel_s(2, 1) = 1;
       kernel_w(1, 0) = 1;
       kernel_e(1, 2) = 1;
       
       //Create the parameters that will be used in the loop:
       
       // grad_x = gradient obtained by using the directional edge detection kernel defined above
       
       // param_x = mat which containes the parameter to be fed to the exp function, used when calculating the conduction coefficient value (c_x)
       
       // c_x = conduction coefficient values which help to control the amount of diffusion
       
       // k_ns and k_we are gradient magnitude parameters - they serve as thresholds to define what is a gradient and what is noise.
       // therefore, they are used in the function to define the c_x values (one for north and south directions, the other fo west and east)
       
       cv::Mat_<float> grad_n, grad_s, grad_w, grad_e;
       cv::Mat_<float> param_n, param_s, param_w, param_e;
       cv::Mat_<float> c_n, c_s, c_w, c_e;
       
       //perform the number of iterations requested
       for (int time = 0; time < iterations; time++)
       {
           //perform the filtering
           cv::filter2D(dst, grad_n, -1, kernel_n);
           cv::filter2D(dst, grad_s, -1, kernel_s);
           cv::filter2D(dst, grad_w, -1, kernel_w);
           cv::filter2D(dst, grad_e, -1, kernel_e);
           
           //obtain the iteration values as per the conduction function used
           //in this case, the conduction function is found in [52], eq 11
           param_n = (std::sqrt(5) / k_ns) * grad_n;
           cv::pow(param_n, 2, param_n);
           cv::exp(-(param_n), c_n);
           
           param_s = (std::sqrt(5) / k_ns) * grad_s;
           cv::pow(param_s, 2, param_s);
           cv::exp(-(param_s), c_s);
           
           param_w = (std::sqrt(5) / k_we) * grad_w;
           cv::pow(param_w, 2, param_w);
           cv::exp(-(param_w), c_w);
           
           param_e = (std::sqrt(5) / k_we) * grad_e;
           cv::pow(param_e, 2, param_e);
           cv::exp(-(param_e), c_e);
           
           //update the image
           dst = dst + lambda*(c_n.mul(grad_n) + c_s.mul(grad_s) + c_w.mul(grad_w) + c_e.mul(grad_e));
       }
       //convert the output to 8 bits
       dst.convertTo(dst, CV_8U);
   }
   
    //Function used to perform Gamma correction
    //Input:  NORMALISED src image, gamma
    //Output: Modified image
    void Gamma_Correction(const cv::Mat&  src, cv::Mat& dst, double gamma)
    {
        //Obtain the inverse of the gamma provided
        double inverse_gamma = 1.0 / gamma;
        
        //Create a lookup table, to reduce processing time
        cv::Mat Lookup(1, 256, CV_8U);            //Lookup table having values from 0 to 255
        uchar* ptr = Lookup.ptr<uchar>(0);        //pointer access is fast in opencv, since it does not perform a range check for each call
                                                  //uchar since 1 byte unsigned integer, in the range 0 - 255
        for (int i = 0; i < 256; i++)
        {
            ptr[i] = (int)(std::pow((double)i / 255.0, inverse_gamma) * 255.0);
            
        }
        //Apply the lookup table to the source image
        cv::LUT(src, Lookup, dst);
        
    }


    void printMat(cv::Mat& A)
    {
        for(int r=0; r<A.rows; r++)
        {
            for(int c=0; c<A.cols; c++)
            {
                cout<<A.at<double>(r,c)<<'\t';
            }
            cout<<endl;
        }
    }

    void PeakDetect(const cv::Mat& space, std::vector<Point>& peaks){
        // Make sure it is empty
        peaks.resize(0);
        
        int height = space.rows - 2;
        int width = space.cols - 2;
        auto rowUpdate = space.ptr(1) - space.ptr(0);
        
        for (int row = 1; row < height; row++)
        {
            const uint8_t* row_ptr = space.ptr(row);
            const uint8_t* pel = row_ptr;
            for (int col = 1; col < width; col++, pel++)
            {
                if (*pel > *(pel - 1) &&
                    *pel > *(pel - rowUpdate - 1) &&
                    *pel > *(pel - rowUpdate) &&
                    *pel > *(pel - rowUpdate + 1) &&
                    *pel > *(pel + 1) &&
                    *pel > *(pel + rowUpdate + 1) &&
                    *pel > *(pel + rowUpdate) &&
                    *pel > *(pel + rowUpdate - 1))
                {
                    peaks.emplace_back(col,row);
                }
            }
        }
    }
    
    void PeakMapDetect(const cv::Mat& space, std::unordered_map<uint8_t,std::vector<Point2f>>& peak_map, uint8_t accept){
        
        peak_map.clear();
        for (uint8_t ii = 0; ii <= 255; ii++){
            peak_map[ii].clear();
        }
        
        int height = space.rows - 2;
        int width = space.cols - 2;
        auto rowUpdate = space.ptr(1) - space.ptr(0);
        
        for (int row = 1; row < height; row++)
        {
            const uint8_t* row_ptr = space.ptr(row);
            const uint8_t* pel = row_ptr;
            for (int col = 1; col < width; col++, pel++)
            {
                uint8_t pval = *pel;
                int s = 0;
                s += (pval > *(pel - 1)) ? 1 : 0;
                s += (pval > *(pel - rowUpdate - 1) ) ? 1 : 0;
                s += (pval > *(pel - rowUpdate) ) ? 1 : 0;
                s += (pval > *(pel - rowUpdate + 1) ) ? 1 : 0;
                s += (pval > *(pel + 1) ) ? 1 : 0;
                s += (pval > *(pel + rowUpdate + 1) ) ? 1 : 0;
                s += (pval > *(pel + rowUpdate) ) ? 1 : 0;
                s += (pval > *(pel + rowUpdate - 1) ) ? 1 : 0;
                if (pval > accept && s > 5)
                {
                    peak_map[pval].emplace_back(col+0.5, row+0.5);
                }
            }
        }
        
        
    }
    
    double getPSNR(const Mat& I1, const Mat& I2)
    {
        Mat s1;
        absdiff(I1, I2, s1);       // |I1 - I2|
        s1.convertTo(s1, CV_32F);  // cannot make a square on 8 bits
        s1 = s1.mul(s1);           // |I1 - I2|^2
        
        Scalar s = sum(s1);         // sum elements per channel
        
        double sse = s.val[0] + s.val[1] + s.val[2]; // sum channels
        
        if( sse <= 1e-10) // for small values return zero
            return 0;
        else
        {
            double  mse =sse /(double)(I1.channels() * I1.total());
            double psnr = 10.0*log10((255*255)/mse);
            return psnr;
        }
    }
    

    double correlation (cv::Mat &image_1, cv::Mat &image_2)
    {
        // convert data-type to "float"
        cv::Mat im_float_1;
        image_1.convertTo(im_float_1, CV_32F);
        cv::Mat im_float_2;
        image_2.convertTo(im_float_2, CV_32F);
        
        int n_pixels = im_float_1.rows * im_float_1.cols;
        
        // Compute mean and standard deviation of both images
        cv::Scalar im1_Mean, im1_Std, im2_Mean, im2_Std;
        meanStdDev(im_float_1, im1_Mean, im1_Std);
        meanStdDev(im_float_2, im2_Mean, im2_Std);
        
        // Compute covariance and correlation coefficient
         double covar = (im_float_1 - im1_Mean).dot(im_float_2 - im2_Mean) / n_pixels;
        double correl = covar / (im1_Std[0] * im2_Std[0]);
        
        return correl;
    }
    
    double correlation_ocv(const roiWindow<P8U>& i, const roiWindow<P8U>& m)
    {
        cv::Mat im (i.height(), i.width(), CV_8UC(1), i.pelPointer(0,0), size_t(i.rowUpdate()));
        cv::Mat mm (m.height(), m.width(), CV_8UC(1), m.pelPointer(0,0), size_t(m.rowUpdate()));
        double r = correlation (im, mm);
        return r * r;
    }
    
    float crossMatch (const cv::Mat& img, const cv::Mat& model, cv::Mat& space, bool squareit)
    {
        int result_cols =  img.cols - model.cols + 1;
        int result_rows = img.rows - model.rows + 1;
        space.create(result_rows, result_cols, CV_32FC1);
        cv::matchTemplate ( img, model, space, TM_CCOEFF_NORMED);
        if (squareit)
            space.mul(space);
        return space.at<float>(0,0);
    }
    
    cv::Mat selfMatch (const cv::Mat& img, int32_t dia, bool squareit)
    {
        auto width = img.cols - 2 * dia;
        auto height = img.rows - 2 * dia;
        cv::Rect inner (dia, dia, width, height);
        cv::Mat model (img, inner);
        cv::Mat space (height, width, CV_32FC1);
        crossMatch (img, model, space, squareit);
        return space;
    }
    
    cv::Mat1b prepare_acf (const cv::Mat& space, uint32_t precision)
    {
        float mult = pow(10.0f, precision);
        cv::Mat1b acf (space.rows, space.cols);
        
        for (auto row = 0; row < space.rows; row++)
        {
            const float* floatPtr = reinterpret_cast<const float*>(space.ptr(row));
            uint8_t* uint8Ptr = acf.ptr(row);
            for (auto col = 0; col < space.cols; col++, floatPtr++, uint8Ptr++)
            {
                int32_t vap = std::floor((1.0f - *floatPtr) * mult);
                *uint8Ptr = (uint8_t) (vap == 0 ? 1 : 0);
            }
        }
        return acf;
    }
    
   size_t writeSpaceToStl (const cv::Mat& space,  std::vector<std::vector<float>> out)
    {
        out.resize(space.rows);
        size_t total = 0;
        for (auto row = 0; row < space.rows; row++)
        {
            const float* floatPtr = reinterpret_cast<const float*>(space.ptr(row));
            std::vector<float>& row_vector = out[row];
            row_vector.resize (space.cols);
            for (auto col = 0; col < space.cols; col++, floatPtr++)
            {
                row_vector[col] = *floatPtr;
            }
            total += row_vector.size();
        }
        return total;
    }

    
    cv::Mat1b acf (const cv::Mat& img, int32_t dia, uint32_t precision )
    {
        cv::Mat space = selfMatch(img, dia);
        return prepare_acf(space,precision);
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
            int last_height = 0;
            for( int h = 0; h < number_of_bins; h++ )
            {
                float value = histograms[channel].at<float>(h);
                int height = static_cast<int>(value*highest_point/max_value);
//                int where = (int)(((float)h)*scaling_factor);
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
    
    void getRangeC (const cv::Mat& onec, vec2& range)
    {
        double lmin, lmax;
        cv::minMaxLoc(onec, &lmin, &lmax);
        range.x = lmin;
        range.y = lmax;
    }
    

/***************
                             #define CV_8U   0
                             #define CV_8S   1
                             #define CV_16U  2
                             #define CV_16S  3
                             #define CV_32S  4       stats
                             #define CV_32F  5
                             #define CV_64F  6       centroids

 ****************/
 

    
    void generate_mask (const cv::Mat& rg, cv::Mat& mask, float left_tail_post, bool debug_output)
    {
        cv::Mat1b msk = rg.clone();
        msk = 0;
        
        int rows, cols;
        cv::Size s = rg.size();
        rows = s.height;
        cols = s.width;
        
        int tail_post = svl::leftTailPost (rg, left_tail_post);
        tail_post = svl::Clamp(tail_post, 2, 10);
        if (tail_post < 0 || tail_post > 255)
        {
            if (debug_output) std::cout << " tail_post " << tail_post << std::endl;
            return;
        }
        
        for (auto row = 0; row < rows; row++)
        {
            for (auto col = 0; col < cols; col++)
            {
                uint8_t rgpel = rg.at<uint8_t>(row, col);
                if (rgpel <= tail_post)
                    msk.at<uint8_t>(row, col) = 255;
            }
        }
        mask = msk;
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
    
    
    ////////// Gaussian Template  /////////////////
    /////////////////////////////////////////////////////////
    
    
    
    cv::Mat gaussianTemplate(const std::pair<uint32_t,uint32_t>& dims, const vec2& sigma, const vec2& ctr)
    {
        int w = dims.first, h = dims.second;
        cv::Mat ret ( h, w, CV_32F);
        cv::Mat ret8 (h, w, CV_8U);
        std::pair<float,float> maxVar (std::numeric_limits<float>::max (), std::numeric_limits<float>::min ());
        vec2 center (ctr.x * w, ctr.y * h);
        
        float fac = 0.5f / (M_1_PI * sigma.x * sigma.y);
        float vx = -0.5f / (sigma.x * sigma.x);
        float vy = -0.5f / (sigma.y * sigma.y);
        for (int jj = 0; jj < h; jj ++)
        {
            float vydy2 = float(jj - center.y)/h; vydy2 *= vydy2 * vy;
            float* pelPtr = (float*) ret.ptr(jj);
            for (int ii = 0; ii < w; ii ++, pelPtr++)
            {
                float dx2 = float(ii - center.x)/w; dx2 *= dx2;
                dx2 = (fac * expf(vx * dx2 + vydy2));
                *pelPtr = dx2;
                maxVar.first = (dx2 < maxVar.first) ? dx2 : maxVar.first;
                maxVar.second = (dx2 > maxVar.second) ? dx2 : maxVar.second;
            }
        }
        
        float range = maxVar.second - maxVar.first;
        for (int jj = 0; jj < h; jj ++)
        {
            uint8_t* pel8Ptr = ret8.ptr(jj);
            float* pelPtr = (float*) ret.ptr(jj);
            for (int ii = 0; ii < w; ii ++, pelPtr++, pel8Ptr++)
            {
                float dx2 = *pelPtr;
                dx2 = (dx2 - maxVar.first) / range;
                *pel8Ptr = (uchar) (dx2*255);
            }
        }
        return ret8;
    }
    
    
     // Silly Implementation: calculates the median value of a single channel
    // based on https://github.com/arnaudgelas/OpenCVExamples/blob/master/cvMat/Statistics/Median/Median.cpp
    double median( cv::Mat channel )
    {
        double m = (channel.rows*channel.cols) / 2;
        int bin = 0;
        double med = -1.0;
        
        int histSize = 256;
        float range[] = { 0, 256 };
        const float* histRange = { range };
        bool uniform = true;
        bool accumulate = false;
        cv::Mat hist;
        cv::calcHist( &channel, 1, 0, cv::Mat(), hist, 1, &histSize, &histRange, uniform, accumulate );
        
        for ( int i = 0; i < histSize && med < 0.0; ++i )
        {
            bin += cvRound( hist.at< float >( i ) );
            if ( bin > m && med < 0.0 )
                med = i;
        }
        
        return med;
    }

    float Median1dF (const cv::Mat& fmat, size_t& loc)
    {
        assert((1 == fmat.rows) || (1 == fmat.cols));
        
        std::vector<float> array;
        auto sum = std::accumulate(fmat.begin<float>(), fmat.end<float>(), 0.0f);
        auto med_target = sum / 2;
        auto sofar = 0;
        for (auto mItr = fmat.begin<float>(); mItr < fmat.end<float>(); mItr++)
        {
            sofar += *mItr;
            array.push_back ((sofar - med_target) * (sofar - med_target));
        }
        
        
        // Find where it was closest
        auto closest_to_median = std::min_element(array.begin(), array.end());
        loc = std::distance (array.begin(), closest_to_median);
        return med_target;
    }
    

    void horizontal_vertical_projections (const cv::Mat& mat, cv::Mat& hz, cv::Mat& vt)
    {
//        @param dim dimension index along which the matrix is reduced. 0 means that the matrix is reduced to
//        a single row. 1 means that the matrix is reduced to a single column.
        reduce(mat,vt,1,REDUCE_SUM,CV_32F);
        reduce(mat,hz,0,REDUCE_SUM,CV_32F);
    }
    
    std::pair<size_t, size_t> medianPoint (const cv::Mat& mat)
    {
        Mat vertMat, horzMat;
        horizontal_vertical_projections(mat, horzMat, vertMat);
        std::pair<size_t,size_t> med;
        Median1dF(vertMat, med.second);
        Median1dF(horzMat, med.first);
        return med;
    }
    
    
    void sobel_opencv (const cv::Mat& input_gray, cv::Mat& mag, cv::Mat& ang, uint32_t square_size)
    {
        
        // compute the gradients on both directions x and y
        Mat fgray, grad_x, grad_y;
        cv::Mat sobel_mag;
        cv::Mat sobel_ang;
        
        int ddepth = CV_32F; // use 16 bits unsigned to avoid overflow
        input_gray.convertTo(fgray, CV_32F);
        Sobel( fgray, grad_x, ddepth, 1, 0, square_size); //kx, scale, delta, BORDER_DEFAULT );
        Sobel( fgray, grad_y, ddepth, 0, 1, square_size); // , ky, scale, delta, BORDER_DEFAULT );
        cv::cartToPolar(grad_x, grad_y, sobel_mag, sobel_ang, false); // return 0 - 2pi
        mag = input_gray.clone();
        ang = input_gray.clone();
        sobel_mag.convertTo(mag, CV_8UC1, 255.0 );
        sobel_ang.convertTo(ang, CV_8UC1, 255.0 / svl::constants::two_pi);
     }
    
    // Computes the 1D histogram.
    cv::Mat getHistogram(const cv::Mat1b &image, const cv::Mat1b &mask)
    {
        cv::Mat hist;
        
        int histSize[1];         // number of bins in histogram
        float hranges[2];        // range of values
        const float* ranges[1];  // pointer to the different value ranges
        int channels[1];         // channel number to be examined
        histSize[0]= 256;   // 256 bins
        hranges[0]= 0.0;    // from 0 (inclusive)
        hranges[1]= 256.0;  // to 256 (exclusive)
        ranges[0]= hranges;
        channels[0]= 0;     // we look at channel 0
        
        // Compute histogram
        cv::calcHist(&image,
                     1,			// histogram of 1 image only
                     0,	// the channel used
                     mask,	// no mask is used
                     hist,		// the resulting histogram
                     1,			// it is a 1D histogram
                     histSize,	// number of bins
                     ranges		// pixel value range
                     );
        
        return hist;
    }
    
    
    // Return -1 or 0-255 for left_tail
    // @note: add accumulative hist
    int leftTailPost (const cv::Mat1b& image, float left_tail_fraction)
    {
        auto hist = getHistogram (image);
        float left_fraction_n = left_tail_fraction * image.rows * image.cols;
        float ncount = 0;
        int left_tail_value = 0;
        for (int i=0; i<256 ;i++)
        {
            ncount += hist.at<float>(i);
            if (ncount > left_fraction_n)
            {
                left_tail_value = i;
                break;
            }
        }
        return std::max(1,left_tail_value);
    }
    
    int rightTailPost (const cv::Mat1b& image, float right_tail_fraction)
    {
        auto hist = getHistogram (image);
        float right_fraction_n = right_tail_fraction * image.rows * image.cols;
        float ncount = 0;
        int right_tail_value = 0;
        for (int i=255; i >= 0 ;i--)
        {
            ncount += hist.at<float>(i);
            if (ncount > right_fraction_n)
            {
                right_tail_value = i;
                break;
            }
        }
        return std::min(255,right_tail_value);
    }
    
    
    void output(Mat mat, int prec, char be, char en)
    {
        cout << be;
        for(int i=0; i<mat.size().height; i++)
        {
            cout << be;
            for(int j=0; j<mat.size().width; j++)
            {
                cout << setprecision(prec) << static_cast<int32_t>(prec * mat.at<float>(i,j));
                if(j != mat.size().width-1)
                    cout << ", ";
                else
                    cout << en << endl;
            }
            if(i != mat.size().height-1)
                cout << ", ";
            else
                cout << en << endl;
        }
    }
    

    void outputU8 (const Mat& mat, char be, char en)
    {
        cout << be;
        for(int i=0; i<mat.size().height; i++)
        {
            cout << be;
            for(int j=0; j<mat.size().width; j++)
            {
                cout << static_cast<int32_t>(mat.at<uint8_t>(i,j));
                if(j != mat.size().width-1)
                    cout << ", ";
                else
                    cout << en << endl;
            }
            if(i != mat.size().height-1)
                cout << ", ";
            else
                cout << en << endl;
        }
    }
    
}

