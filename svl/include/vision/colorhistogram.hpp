#if !defined COLHISTOGRAM
#define COLHISTOGRAM


#include "opencv2/highgui.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/imgproc.hpp"
#include <vector>
#include <array>


using namespace std;
using namespace cv;


class ColorSpaceHistogram
{
public:
    typedef uint8_t pixel_t;
    typedef std::vector<std::vector<std::vector<float> > > hist3d_t;
    
    int bins () const
    {
        static int bb = 256;
        return bb;
    }
    
    int factor () const
    {
        return 1000.0;
    }
    
    ColorSpaceHistogram(const cv::Mat& src, bool ChromocityNorm = true)
    {
        mChannels = {0,1,2};
        mSizes = {bins(), bins(), bins()};
        float top = static_cast<float>(bins()-1);
        mRanges = {0,top,0,top,0,top};
        _currentHist.create (3, &mSizes[0], CV_32F);
        assert(src.channels() >= 3);
        deep_copy = src.clone ();
    }
    
    void run ()
    {
        /// check if we have all
        //        calcHist( &hsv, 1, channels, cv::Mat(), // do not use mask
        //                 hist, 2, histSize, ranges,
        //                 true, // the histogram is uniform
        //                 false );
        
        static int hist_size [] = {256, 256, 256};
        static int channels [] {0, 1, 2};
        static float range_0 [] {0, 255};
        
        const float* ranges [] = { range_0,range_0,range_0 };
        
        calcHist(&deep_copy, 1,
                 channels, cv::Mat(),
                 _currentHist, 3, hist_size, ranges,
                 true, false);
        
        // Normalize: the sum of all the bins will equal to 100
       // cv::normalize(_currentHist, _currentHist, factor(), 0, NORM_L1);
    }
    
    
    const cv::SparseMat spaceHistogram () const
    {
        return _currentHist;
    }
    
    void check_against (const cv::SparseMat& other)
    {
        double correlation = compareHist(_currentHist,  other,CV_COMP_CORREL);
        double chisquared = compareHist(_currentHist,  other,CV_COMP_CHISQR);
        double intersect = compareHist(_currentHist,  other,CV_COMP_INTERSECT);
        double bhattacharyya = compareHist(_currentHist, other,CV_COMP_BHATTACHARYYA);
        
        std::cout <<  "Comparison Results " << " Corr: " <<
        correlation << " ChiSq: " << chisquared <<
        " Intersect: " << intersect << " Bhatt: " << bhattacharyya  << std::endl;
    }
    
    // TBD: Fix this
    friend ostream & operator<<(ostream & ous, const ColorSpaceHistogram & dis)
    {
        cv::SparseMatConstIterator_<float> it = dis.spaceHistogram().begin<float>();
        cv::SparseMatConstIterator_<float> it_end = dis.spaceHistogram().end<float>();
        
        double s = 0;
        int dims = dis.spaceHistogram().dims();
        for(; it != it_end; ++it)
        {
            // print element indices and the element value
            const SparseMat::Node* n = it.node();
            if (!n) continue;
            printf("(");
            for(int i = 0; i < dims; i++)
                printf("%d%s", n->idx[i], i < dims-1 ? ", " : ")");
            printf(": %g\n", it.value<float>());
            s += *it;
        }
        printf("Element sum is %g\n", s);

        
        return ous;
    }
    
 
    float * range () const
    {
        static float rn [] {0, 255};
        return rn;
    }

    int * channels () const
    {
        static int chans [] {0, 1, 2};
        return chans;
    }
    int * dimensions () const
    {
         static int histSize [] = {bins(), bins(), bins() };
        return histSize;
    }
    
    float ** ranges () const
    {
        static float* rns [] = { range(),range(),range() };
        return rns;
    }
    
//    
//    const 3dhist_t& getHistogram3d() const
//    {
//        
//        ofxCvGrayscaleImage r, g, b;
//        r.setFromPixels(img.getPixels().getChannel(0));
//        g.setFromPixels(img.getPixels().getChannel(1));
//        b.setFromPixels(img.getPixels().getChannel(2));
//        CvHistogram* hist;
//        IplImage *iplImage, **plane;
//        IplImage* planes[] = {r.getCvImage(), g.getCvImage(), b.getCvImage()};
//        int hist_size[] = { numBins, numBins, numBins };
//        float range[] = { 0, 255 };
//        float* ranges[] = { range, range, range };
//        hist = cvCreateHist( 3, hist_size, CV_HIST_ARRAY, ranges, 1 );
//        cvCalcHist( planes, hist, 0, 0 );
//        cvNormalizeHist( hist, 1.0 );
//        for (int ir=0; ir<numBins; ir++) {
//            for (int ig=0; ig<numBins; ig++) {
//                for (int ib=0; ib<numBins; ib++) {
//                    histogram[ir][ig][ib] = cvQueryHistValue_3D(hist, ir, ig, ib);
//                }
//            }
//        }    
//        return histogram;
//    }
private:
    mutable hist3d_t mHist3d;
    std::vector<int> mChannels;
    std::vector<float> mRanges;
    std::vector<int> mSizes;
    
    cv::Size mBounds;
    // @todo
//    bool uniform;
//    bool accumulate;
//    bool norm_chromocity;
    std::vector<cv::Mat> mhists;
    cv::Mat mHistImage;
    cv::Mat deep_copy;
    mutable cv::SparseMat _currentHist;
    
    void create_3dhist (hist3d_t& h3d)
    {
        h3d.resize (0);
        for (int ir=0; ir<bins(); ir++) {
            vector<vector<float > > hist1;
            for (int ig=0; ig<bins(); ig++) {
                vector<float> hist2;
                hist2.resize(bins());
                hist1.push_back(hist2);
            }
            h3d.push_back(hist1);
        }
    }
    
    
    
};

class ColorChannelHistogram
{
    
    
public:
    ColorChannelHistogram(const cv::Mat& src) : histSize (256), uniform(true), accumulate(false)
    {
        vector<cv::Mat> bgr_planes(src.channels());
        /// Separate the image in 3 places ( B, G and R )
        split( src, bgr_planes );
        run (bgr_planes);
    }
    
    ColorChannelHistogram(const vector<cv::Mat>& bgr_planes) : histSize (256), uniform(true), accumulate(false)
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
    
    
    
    const cv::Mat display_image () const
    {
        return mHistImage;
    }
    
private:
    cv::Size mBounds;
    int histSize;
    bool uniform;
    bool accumulate;
    std::vector<cv::Mat> mhists;
    cv::Mat mHistImage;
    
    float entropy (int index)
    {
        cv::Mat logP;
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
        cv::calcHist( &channel, 1, 0, cv::Mat(), dirty, 1, &histSize, &histRange, true, false);
        return dirty.clone();
        
    }
    
    void run (const vector<cv::Mat> bgr_planes)
    {
        
        /// Set the ranges ( for B,G,R) )
        float range[] = { 0, 256 } ;
        const float* histRange = { range };
        
        
        cv::Mat b_hist, g_hist, r_hist;
        
        /// Compute the histograms:
        calcHist( &bgr_planes[0], 1, 0, cv::Mat(), b_hist, 1, &histSize, &histRange, uniform, accumulate );
        calcHist( &bgr_planes[1], 1, 0, cv::Mat(), g_hist, 1, &histSize, &histRange, uniform, accumulate );
        calcHist( &bgr_planes[2], 1, 0, cv::Mat(), r_hist, 1, &histSize, &histRange, uniform, accumulate );
        
        // Draw the histograms for B, G and R
        int hist_w = 512; int hist_h = 400;
        int bin_w = cvRound( (double) hist_w/histSize );
        
        mHistImage = cv::Mat ( hist_h, hist_w, CV_8UC3, Scalar( 0,0,0) );
        
        /// Normalize the result to [ 0, histImage.rows ]
    normalize(b_hist, b_hist, 0, mHistImage.rows, NORM_MINMAX, -1, cv::Mat() );
        normalize(g_hist, g_hist, 0, mHistImage.rows, NORM_MINMAX, -1, cv::Mat() );
        normalize(r_hist, r_hist, 0, mHistImage.rows, NORM_MINMAX, -1, cv::Mat() );
        
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
