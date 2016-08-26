#ifndef _gabor_
#define _gabor_

#include "opencv2/core/core.hpp"

namespace cv
{
    
#define uPI 3.141592653589793238462643383279502884197169399375105820974944
#define u2PI 6.2831853071795864769252867665590057683943388015061
    
    float GaborFilterValue(int x,
                           int y,
                           float lambda,
                           float theta,
                           float phi,
                           float sigma,
                           float gamma)
    {
        float xx =  x * cos(theta) + y * sin(theta);
        float yy = -x * sin(theta) + y * cos(theta);
        
        float envelopeVal = exp( - ( (xx*xx + gamma*gamma* yy*yy) / (2.0f * sigma*sigma) ) );
        
        float carrierVal = cos( 2.0f * (float)uPI * xx / lambda + phi);
        
        float g = envelopeVal * carrierVal;
        
        return g;
    }
    
    
    double round(double d)
    {
        return floor(d + 0.5);
    }
    
    
    
    cv::Mat CreateGaborFilterImage(int side, float wavelength, float orientation, float phaseoffset, float gaussvar, float aspectratio)
    {
        // Create an empty image
        
        cv::Mat img = cv::Mat::zeros(2*side+10,2*side+10, CV_8UC3);
        
        
        // Display gabor filter values
        float MinGaborValue = std::numeric_limits<float>::max();
        float MaxGaborValue = std::numeric_limits<float>::min();
        for (int dy=-side; dy<=side; dy++)
        {
            for (int dx=-side; dx<=side; dx++)
            {
                float val = GaborFilterValue(dx,dy, wavelength, orientation, phaseoffset, gaussvar, aspectratio);
                
                uint8_t R=0;
                uint8_t G=0;
                uint8_t B=0;
                if (val>0.0f)
                    R = (uint8_t)round(val * 255.0f);
                else
                    B = (uint8_t)round(-val * 255.0f);

                img.at<Vec3b>(side+dy, side+dx)[0] = B;
                img.at<Vec3b>(side+dy, side+dx)[1] = G;
                img.at<Vec3b>(side+dy, side+dx)[2] = R;
                
                if (val<MinGaborValue)
                    MinGaborValue = val;
                if (val>MaxGaborValue)
                    MaxGaborValue = val;
                
            }
        }
        return img;
    }
}


#endif


