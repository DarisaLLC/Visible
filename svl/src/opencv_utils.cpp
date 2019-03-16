#include <cstdint>

#include <algorithm>
#include <iostream>

#include "vision/opencv_utils.hpp"
#include "vision/gradient.h"
#include "boost/variant.hpp"
#include <cstdio>
#include <iostream>


using namespace std;
using namespace cv;
using namespace ci;
using namespace svl;

namespace svl
{
    
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
        cv::normalize(dft_magnitude, dft_magnitude, 0, 1, CV_MINMAX);
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
        cv::normalize(dst, dst, 0, 255, CV_MINMAX);
    }
    
    //Function used to perform bilateral Filtering
    //Input: src image, neighbourhood size, standard dev for colour and space
    //Output: filtered image
    void Bilateral_Filtering(const cv::Mat&  src, cv::Mat& dst, int neighbourhood, double sigma_colour, double sigma_space)
    {
        cv::bilateralFilter(src, dst, neighbourhood, sigma_colour, sigma_space);
    }
    
    //Function used to perform Wiener Denoising. Note that this function does not reverse the blur
    //Input: src image, constant to multiply with (inverse of PSNR)
    //Output: filtered image
    void Wiener_Denoising(const cv::Mat &src, cv::Mat &dst, double PSNR_inverse)
    {
        cv::Mat src_dft;
        
        //Obtain the DFT of the src image
        Complex_Gray_DFT(src, src_dft);
        
        //Multiply it by the constant
        cv::Mat output_dft;
        output_dft = (PSNR_inverse)*src_dft;
        
        //Perform the inverse PSNR to get the filtered output
        Real_Gray_IDFT(output_dft, dst);
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


    void PeakDetect(const cv::Mat& space, std::vector<Point2f>& peaks, uint8_t accept)
    {
        
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
                if (*pel >= accept &&
                    *pel > *(pel - 1) &&
                    *pel > *(pel - rowUpdate - 1) &&
                    *pel > *(pel - rowUpdate) &&
                    *pel > *(pel - rowUpdate + 1) &&
                    *pel > *(pel + 1) &&
                    *pel > *(pel + rowUpdate + 1) &&
                    *pel > *(pel + rowUpdate) &&
                    *pel > *(pel + rowUpdate - 1))
                {
                    Point2f dp(col+0.5, row+0.5);
                    peaks.push_back(dp);
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
    
    void skinMaskYCrCb (const cv::Mat &src, cv::Mat &dst)
    {
        Mat m;
        cvtColor(src, m, CV_BGR2YCrCb);
        std::vector<Mat> mv;
        split(m, mv);
        Mat mask = Mat(m.rows, m.cols, CV_8UC1);
        
        for (int i=0; i<m.rows; i++) {
            for (int j=0; j<m.cols; j++) {
                int Cr= mv[1].at<uint8_t>(i,j);
                int Cb =mv[2].at<uint8_t>(i,j);
                mask.at<uint8_t>(i, j) = (Cr>130 && Cr<170) && (Cb>70 && Cb<125) ? 255 : 0;
            }
        }
        dst = mask;
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
        cv::matchTemplate ( img, model, space, CV_TM_CCOEFF_NORMED);
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
    
    void cumani_opencv (const cv::Mat& input_bgr, cv::Mat& gradAbs, cv::Mat& orientation, float& maxVal)
    {
        
        // compute the gradients on both directions x and y
        Mat grad_x, grad_y;
        Mat abs_grad_x, abs_grad_y;
        int ddepth = CV_32F; // use 16 bits unsigned to avoid overflow
        std::vector<Mat> mv;
        std::vector<Mat> gradXs(3);
        std::vector<Mat> gradYs(3);
        split(input_bgr, mv);
        gradAbs = Mat::zeros(input_bgr.rows,input_bgr.cols, CV_32F);
        orientation = Mat::zeros(input_bgr.rows,input_bgr.cols, CV_32F);

        for (auto cc = 0; cc < 3; cc++)
        {
            gradXs[cc] = Mat::zeros(input_bgr.rows,input_bgr.cols, CV_32F);
            gradYs[cc] = Mat::zeros(input_bgr.rows,input_bgr.cols, CV_32F);
            Sobel( mv[cc], gradXs[cc], ddepth, 1, 0); //kx, scale, delta, BORDER_DEFAULT );
            Sobel( mv[cc], gradYs[cc], ddepth, 0, 1); // , ky, scale, delta, BORDER_DEFAULT );
        }
        Mat dfdx = Mat::zeros(1, 3, CV_32FC1);
        Mat dfdy = Mat::zeros(1, 3, CV_32FC1);
        float E,F,G,EpG,EmG,a;
        
        for (auto y = 0; y < input_bgr.rows; y++)
            for (auto x = 0; x < input_bgr.cols; x++)
            {
                dfdx.at<float>(0,0) = gradXs[0].at<float>(y,x);
                dfdx.at<float>(0,1) = gradXs[1].at<float>(y,x);
                dfdx.at<float>(0,2) = gradXs[2].at<float>(y,x);
                
                dfdy.at<float>(0,0) = gradYs[0].at<float>(y,x);
                dfdy.at<float>(0,1) = gradYs[1].at<float>(y,x);
                dfdy.at<float>(0,2) = gradYs[2].at<float>(y,x);

                E = dfdx.dot(dfdx);
                F = dfdx.dot(dfdy);
                G = dfdy.dot(dfdy);
                
                // the vector of maximal contrast is given by the eigenvalues of
                // the tensor matrix lambda = (E+G) +/- sqrt((E-G)^2+4F^2)/2;
                EpG=E+G;
                if (EpG<=std::numeric_limits<float>::epsilon()) {
                    gradAbs.at<float>(y,x)=0.0f;
                    orientation.at<float>(y,x)=0.0f;
                } else {
                    EmG=E-G;
                    a = sqrt(EmG*EmG+4.0f*F*F);
                    // maximum directional derivative -> lambda_max
                    maxVal=max((gradAbs.at<float>(y,x)=sqrt((EpG + a)*0.5f)),maxVal);
                    a = 0.5f*atan2(2.0f*F,EmG);
                    orientation.at<float>(y,x)=a;
                }
            }
        
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
 
#if 0
    void drawBlob (const blobRecordRef& blob, InputOutputArray image, const Scalar& color, int thickness, int lineType)
    {
        cv::drawContours( image, blob->poly, blob->id, color, 1, 8, vector<Vec4i>(), 0, cv::Point() );
    }
    
    size_t detectContourBlobs (const cv::Mat& grayImage, const cv::Mat&threshold_output,  std::vector<blobRecordRef>& blobs, cv::Mat& graphics)
    {
        RNG rng(12345);
        
        vector<vector<cv::Point> > contours;
        vector<Vec4i> hierarchy;
        
        /// Find contours
        findContours( threshold_output, contours, hierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE, cv::Point(0, 0) );
        
        blobs.clear();
        
        for (size_t contourIdx = 0; contourIdx < contours.size(); contourIdx++)
        {
            blobRecordRef blob = std::make_shared<blob_record_t>();
            blob->id = contourIdx;
            blob->moms = moments(Mat(contours[contourIdx]));
            blob->area = blob->moms.m00;
            blob->perimeter = arcLength(Mat(contours[contourIdx]), true);
            blob->circularity = 4 * CV_PI * blob->area / (blob->perimeter * blob->perimeter);
            double denominator = std::sqrt(std::pow(2 * blob->moms.mu11, 2) + std::pow(blob->moms.mu20 - blob->moms.mu02, 2));
            const double eps = 1e-2;
            if (denominator > eps)
            {
                double cosmin = (blob->moms.mu20 - blob->moms.mu02) / denominator;
                double sinmin = 2 * blob->moms.mu11 / denominator;
                double cosmax = -cosmin;
                double sinmax = -sinmin;
                
                double imin = 0.5 * (blob->moms.mu20 + blob->moms.mu02) - 0.5 * (blob->moms.mu20 - blob->moms.mu02) * cosmin - blob->moms.mu11 * sinmin;
                double imax = 0.5 * (blob->moms.mu20 + blob->moms.mu02) - 0.5 * (blob->moms.mu20 - blob->moms.mu02) * cosmax - blob->moms.mu11 * sinmax;
                blob->inertia = imin / imax;
            }
            else
            {
                blob->inertia = 1;
            }
            std::vector < Point > hull;
            convexHull(Mat(contours[contourIdx]), hull);
            blob->contourArea = contourArea(Mat(contours[contourIdx]));
            blob->hullArea = contourArea(Mat(hull));
            
            approxPolyDP( Mat(contours[contourIdx]), blob->poly, 3, true );
            blob->bounding = boundingRect( Mat(blob->poly) );
            
            if(blob->moms.m00 > 0.0)
                blob->location = Point2d(blob->moms.m10 / blob->moms.m00, blob->moms.m01 / blob->moms.m00);
            
            //compute blob radius
            std::vector<double> dists;
            for (size_t pointIdx = 0; pointIdx < contours[contourIdx].size(); pointIdx++)
            {
                Point2d pt = contours[contourIdx][pointIdx];
                dists.push_back(norm(blob->location - pt));
            }
            std::sort(dists.begin(), dists.end());
            blob->radius = (dists[(dists.size() - 1) / 2] + dists[dists.size() / 2]) / 2.;
            blobs.emplace_back(blob);
        }

        for( int i = 0; i< blobs.size(); i++ )
        {
            Scalar color = Scalar( rng.uniform(0, 255), rng.uniform(0,255), rng.uniform(0,255) );
            drawBlob(blobs[i],graphics, color);

        }
        return blobs.size();
        
    }
#endif
    
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
        reduce(mat,vt,1,CV_REDUCE_SUM,CV_32F);
        reduce(mat,hz,0,CV_REDUCE_SUM,CV_32F);
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
    
    
    void sobel_opencv (const cv::Mat& input_gray, cv::Mat& mag, cv::Mat& ang)
    {
        
        // compute the gradients on both directions x and y
        Mat grad_x, grad_y;
        cv::Mat sobel_mag;
        cv::Mat sobel_ang;
        
        int ddepth = CV_32F; // use 16 bits unsigned to avoid overflow
        
        Sobel( input_gray, grad_x, ddepth, 1, 0, 7); //kx, scale, delta, BORDER_DEFAULT );
        Sobel( input_gray, grad_y, ddepth, 0, 1, 7); // , ky, scale, delta, BORDER_DEFAULT );
   
        cv::cartToPolar(grad_x, grad_y, sobel_mag, sobel_ang, true);
        mag = sobel_mag;
        ang = sobel_ang;
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
    void toHistVector (const cv::Mat& hist, std::vector<float>& vh)
    {
        vh.clear ();
        if (hist.rows != 256) return;
        
        for (int i=0; i<256 ;i++)
            vh.push_back(hist.at<float>(i));
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
    
    void CreateGaussSpace(const cv::Mat& img, std::vector<cv::Mat>& blured, float tmin, float tmax, float tdelta)
    {
        int L = (int)((tmax - tmin) / tdelta) + 1;
        std::string displayName ("Gauss Space");
        
        cv::namedWindow(displayName, CV_WINDOW_KEEPRATIO | CV_GUI_EXPANDED);
        
        blured.resize(0);
        
        // create level-1 image
        double tI = tmin;
        float sigmaI = exp(tI);
        
        cv::Mat tmp = img.clone ();
        cv::GaussianBlur(img, tmp, cv::Size(0,0), sigmaI);
        
        blured.push_back(tmp);
        tI += tdelta;
        
        for(int i = 1; i < L; i++)
        {
            float sigmaI_tmp = exp(tI);
            float sigmaI_new = sqrt(sigmaI_tmp*sigmaI_tmp - sigmaI*sigmaI);
            cv::Mat next = img.clone ();
            std::cout << sigmaI_new << std::endl;
            
            cv::GaussianBlur(img, next, cv::Size(0,0), sigmaI_new);
            blured.push_back(next);
            sigmaI = sigmaI_tmp;
            tI += tdelta;
        }
    }
    
    void CreateScaleSpace(const cv::Mat& img, std::vector<cv::Mat>& blured, float tmin, float tmax, float tdelta,
                          std::vector<cv::Mat>& scaled,
                          std::vector<ssBlob>& blobs)
    {
        
        CreateGaussSpace(img,blured,tmin,tmax,tdelta);
        
        scaled.resize (0);
        // apply laplacian filter
        for(int i = 0; i < blured.size(); i++)
        {
            cv::Mat laplace;
            cv::Laplacian(blured[i],laplace, CV_32F);
            scaled.push_back(laplace);
        }
        
        // find maxima - detect blobs
        blobs.resize (0);
        
        // traverse the scale space
        for(int s = 0; s < blured.size(); s++)
        {
            const cv::Mat& imgS = blured[s];
            for(int j = 4; j < imgS.cols-4; j++)
            {
                for(int i = 4; i < imgS.rows-4; i++)
                {
                    float lum = imgS.at<float>(i,j);
                    
                    if (isnan(lum))
                        continue;
                    
                    if(lum < 0.75f) continue;
                    
                    int w = 5;
                    bool max = true;
                    for(int k = 0; k < w; k++)
                    {
                        for(int l=0; l < w; l++)
                        {
                            if(k - w/2 == 0 && l - w/2 == 0) continue;
                            if(lum <= imgS.at<float>(i + k - w/2, j + l - w/2))
                            {
                                max = false;
                                break;
                            }
                        }
                        if(!max) break;
                    }
                    
                    if(max)
                    {
                        if(s != 0)
                        {
                            if(lum <= scaled[s-1].at<float>(i,j) ||
                               lum <= scaled[s-1].at<float>(i-1,j) ||
                               lum <= scaled[s-1].at<float>(i+1,j) ||
                               lum <= scaled[s-1].at<float>(i,j-1) ||
                               lum <= scaled[s-1].at<float>(i,j+1) ||
                               lum <= scaled[s-1].at<float>(i-1,j-1) ||
                               lum <= scaled[s-1].at<float>(i+1,j+1) ||
                               lum <= scaled[s-1].at<float>(i-1,j+1) ||
                               lum <= scaled[s-1].at<float>(i+1,j-1))
                            {
                                max = false;
                                continue;
                            }
                        }
                        
                        if(s != scaled.size()-1)
                        {
                            if(lum <= scaled[s+1].at<float>(i,j) ||
                               lum <= scaled[s+1].at<float>(i-1,j) ||
                               lum <= scaled[s+1].at<float>(i+1,j) ||
                               lum <= scaled[s+1].at<float>(i,j-1) ||
                               lum <= scaled[s+1].at<float>(i,j+1) ||
                               lum <= scaled[s+1].at<float>(i-1,j-1) ||
                               lum <= scaled[s+1].at<float>(i+1,j+1) ||
                               lum <= scaled[s+1].at<float>(i-1,j+1) ||
                               lum <= scaled[s+1].at<float>(i+1,j-1))
                            {
                                max = false;
                                continue;
                            }
                        }
                        
                        ssBlob sb;
                        sb.loc.x = j; sb.loc.y = i;
                        sb.radius = 1.41f * (exp(tmin) + s*tdelta);
                        sb.ss_val = s;
                        blobs.push_back(sb);
                    }
                }
            }
        }
        
    }
    
    
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

