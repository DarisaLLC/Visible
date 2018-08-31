    //
    //  localvariance.cpp
    //  irec_framework
    //
    //  Created by Arman Garakani on 1/3/14.
    //  Copyright (c) 2014 Arman Garakani. All rights reserved.
    //


#include "vision/localvariance.h"
#include <opencv2/imgproc/imgproc.hpp>  // cvtColor
#include "opencv2/core/core_c.h"
using namespace cv;


namespace svl
{
    localVAR::localVAR (Size filter_size)
    {
        m_fsize = filter_size;
        m_maxVar = std::numeric_limits<float>::min ();
        m_minVar = std::numeric_limits<float>::max ();
        clear_cache ();
    }

    void localVAR::convert_or_not (const cv::Mat& _image) const
    {
        IplImage img = _image;
        IplImage single = m_single;
        switch (_image.channels ())
        {
            case 3: // RGB only for now
                cv::cvtColor(_image, m_single,CV_BGR2GRAY);
                break;
            case 1:
                cvCopy (&img, &single);
                break;
            default:
                CV_Assert (false);

        }

    }

    void localVAR::clear_cache () const
    {
        m_isize.width = m_isize.height = -1;
        reset();
    }

    void localVAR::reset () const
    {
        m_var = -1.0;
        m_maxVar = std::numeric_limits<float>::min ();
        m_minVar = std::numeric_limits<float>::max ();

    }

    bool localVAR::use_cached_images (const cv::Mat& _image) const
    {
        return  m_isize == _image.size ();
    }

    void localVAR::allocate_images (const cv::Mat& _image) const
    {
        if (! use_cached_images (_image))
        {
                // Allocate intergral image: 1 bigger in each dimension
            m_ss = cv::Mat(cv::Size(_image.size().width + 1, _image.size().height + 1), CV_64FC1);
            m_s = cv::Mat(cv::Size(_image.size().width + 1, _image.size().height + 1), CV_32SC1);
            m_var = cv::Mat(cv::Size(_image.size().width + 1, _image.size().height + 1), CV_32FC1);
            m_single = cv::Mat(_image.size(),CV_8UC1);
            m_isize = _image.size ();
        }
        reset ();

    }

    bool localVAR::process(const cv::Mat& _image, cv::Mat& result) const
    {
        if (m_fsize.width < 2 || m_fsize.height < 2) return false;
        
            // Allocate intergral image: 1 bigger in each dimension
        allocate_images (_image);
        convert_or_not (_image);
        cv::integral (m_single, m_s, m_ss);

        int w = m_fsize.width; int h = m_fsize.height;
        int n = w * h;
        int n_n_1 = n > 1 ? n * (n - 1) : 1; // handle the 1x1 case for completeness
        int half_k_w = w / 2;
        int half_k_h = h / 2;
        IplImage I = m_s;
        IplImage S = m_ss;
        IplImage V2 = m_var;

        int32_t* sumptr = (int32_t *) I.imageData;
        double* sqptr = (double *) S.imageData;
        float* varPtr = (float *) V2.imageData;
        int processed_h = m_isize.height - h + 1;
        int processed_w = m_isize.width - w + 1;
        int v2_wspixels = V2.widthStep / sizeof(float);
        int s_wspixels = I.widthStep / sizeof(int32_t);
        int ss_wspixels = S.widthStep / sizeof(double);

        for (int y = 0; y < processed_h; y++)
        {
            float* outPtr = varPtr + (y+half_k_h)*v2_wspixels + (half_k_w);

            for (int x = 0; x < processed_w; x++)
            {
                int32_t * sumptr1,*sumptr2,*sumptr3,*sumptr4;

                sumptr1 = sumptr +  y*s_wspixels+ x;
                sumptr2 = sumptr +  (y+h)*s_wspixels+ (x+w);
                sumptr3 = sumptr +  y*s_wspixels+ (x+w);
                sumptr4 = sumptr +  (y+h)*s_wspixels+ x;

                double* sqptr1,*sqptr2,*sqptr3,*sqptr4;

                sqptr1 = sqptr +  y*ss_wspixels + x;
                sqptr2 = sqptr +  (y+h)*ss_wspixels + (x+w);
                sqptr3 = sqptr +  y*ss_wspixels + (x+w);
                sqptr4 = sqptr +  (y+h)*ss_wspixels + x;
                
                int var = n * floor (*sqptr1+*sqptr2-*sqptr3-*sqptr4);
                int sum = *sumptr1+*sumptr2-*sumptr3-*sumptr4;
                var = (var - sum*sum) / n_n_1;
                m_minVar = (var < m_minVar) ? var : m_minVar;
                m_maxVar = (var > m_maxVar) ? var : m_maxVar;
                *outPtr++ = var;
                
            }
        }
        result = m_var(cv::Rect(half_k_w,half_k_h,processed_w,processed_h));
        return true;
    }
    
}



