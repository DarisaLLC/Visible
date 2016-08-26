#if !defined COLHISTOGRAM
#define COLHISTOGRAM


#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>
#include <vector>



using namespace std;
using namespace cv;

class ColorHistogram
{

 
public:
    ColorHistogram(const cv::Mat& src) : histSize (256), uniform(true), accumulate(false)
    {
        vector<cv::Mat> bgr_planes;
        /// Separate the image in 3 places ( B, G and R )
        split( src, bgr_planes );
        run (bgr_planes);
    }
    
    ColorHistogram(const vector<cv::Mat>& bgr_planes) : histSize (256), uniform(true), accumulate(false)
    {
        mBounds = bgr_planes[0].size();
        run (bgr_planes);
    }
    
    
    const cv::Mat channel_histogram (unsigned index)
    {
        assert(!mhists.empty());
        index = index % mhists.size();
        return mhists[index];
    }
    
    const cv::Mat channel_overexposed (int index)
    {
        assert(!mhists.empty());
        index = index % mhists.size();        
        return mOver_hists[index];
    }
    
    
    
private:
    cv::Size mBounds;
    int histSize;
    bool uniform;
    bool accumulate;
    std::vector<cv::Mat> mhists;
    cv::Mat mHistImage;
    std::vector<cv::Mat> mOver_planes;
    std::vector<cv::Mat> mOver_hists;

    float entropy (int index)
    {
        Mat logP;
        cv::log(channel_histogram(index),logP);
        cv::Mat norm = channel_histogram(index)/ ((float)mBounds.width*mBounds.height);
        float entropy = -1*sum(norm.mul(logP)).val[0];
        return entropy;
    }
    cv::Mat get_channel_hist (const cv::Mat& channel)
    {
        int histSize = 256;
        float range[] = { 0, 256 } ;
        const float* histRange = { range };
        cv::Mat dirty;
        calcHist( &channel, 1, 0, Mat(), dirty, 1, &histSize, &histRange, true, false);
        return dirty.clone();
        
    }
    
    void run (const vector<Mat> bgr_planes)
    {
        for (auto cc = 0; cc < bgr_planes.size(); cc++)
        {
            // hist for the channel
            mhists.push_back(get_channel_hist(bgr_planes[cc]));
            // Mat for over plane
            mOver_planes.push_back(cv::Mat());
            // hist for the over
            cv::inRange (255, 255, bgr_planes[cc], mOver_planes[cc]);
            mOver_hists.push_back(get_channel_hist(mOver_planes[cc]));
        }
        
        /// Set the ranges ( for B,G,R) )
        float range[] = { 0, 256 } ;
        const float* histRange = { range };
        
        
        Mat b_hist, g_hist, r_hist;
        
        /// Compute the histograms:
        calcHist( &bgr_planes[0], 1, 0, Mat(), b_hist, 1, &histSize, &histRange, uniform, accumulate );
        calcHist( &bgr_planes[1], 1, 0, Mat(), g_hist, 1, &histSize, &histRange, uniform, accumulate );
        calcHist( &bgr_planes[2], 1, 0, Mat(), r_hist, 1, &histSize, &histRange, uniform, accumulate );
        
        // Draw the histograms for B, G and R
        int hist_w = 512; int hist_h = 400;
        int bin_w = cvRound( (double) hist_w/histSize );
        
        mHistImage = Mat ( hist_h, hist_w, CV_8UC3, Scalar( 0,0,0) );
        
        /// Normalize the result to [ 0, histImage.rows ]
        normalize(b_hist, b_hist, 0, mHistImage.rows, NORM_MINMAX, -1, Mat() );
        normalize(g_hist, g_hist, 0, mHistImage.rows, NORM_MINMAX, -1, Mat() );
        normalize(r_hist, r_hist, 0, mHistImage.rows, NORM_MINMAX, -1, Mat() );
        
        for (unsigned ii = 0; ii < 256; ii++)
        {
            std::cout << b_hist.at<float>(ii) << "  ,  " << g_hist.at<float>(ii) << "  ,  " << r_hist.at<float>(ii) << std::endl;
        }
        
        /// Draw for each channel
        for( int i = 1; i < histSize; i++ )
        {
            line( mHistImage, cv::Point( bin_w*(i-1), hist_h - cvRound(b_hist.at<float>(i-1)) ) ,
                 cv::Point( bin_w*(i), hist_h - cvRound(b_hist.at<float>(i)) ),
                 Scalar( 255, 0, 0), 2, 8, 0  );
            line( mHistImage, cv::Point( bin_w*(i-1), hist_h - cvRound(g_hist.at<float>(i-1)) ) ,
                 cv::Point( bin_w*(i), hist_h - cvRound(g_hist.at<float>(i)) ),
                 Scalar( 0, 255, 0), 2, 8, 0  );
            line( mHistImage, cv::Point( bin_w*(i-1), hist_h - cvRound(r_hist.at<float>(i-1)) ) ,
                 cv::Point( bin_w*(i), hist_h - cvRound(r_hist.at<float>(i)) ),
                 Scalar( 0, 0, 255), 2, 8, 0  );
        }
        
        mhists.push_back (r_hist.clone());
        mhists.push_back (g_hist.clone());
        mhists.push_back (b_hist.clone());
        
      
        
    }
    
    
    
   };




#endif
