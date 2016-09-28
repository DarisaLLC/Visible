
#ifndef SEGMENT_IMAGE
#define SEGMENT_IMAGE

#include <cstdlib>
#include "disjoint-set.h"
#include <iostream>
#include <memory>
#include <vector>
#include "vision/histo.h"
#include "vision/sparsehist.h"
#include "core/stl_utils.hpp"


#include "CinderOpenCV.h"




template<typename T, typename ...Args>
std::unique_ptr<T> make_unique( Args&& ...args )
{
    return std::unique_ptr<T>( new T( std::forward<Args>(args)... ) );
}

template<typename T>
inline T square(const T &x) { return x*x; };


#define THRESHOLD(size, c) (c/size)


typedef struct {
    float w;
    int a, b;
} edge;

bool operator<(const edge &a, const edge &b) {
    return a.w < b.w;
}


using namespace cv;
using namespace ci;
using namespace std;

/*
 * Segment an image
 *
 * Returns a color image representing the segmentation.
 *
 * im: image to segment.
 * sigma: to smooth the image.
 * c: constant for treshold function.
 * min_size: minimum component size (enforced by post-processing stage).
 * num_ccs: number of connected components in the segmentation.
 */


class ssSegmenter
{
public:
    ssSegmenter (const std::vector<cv::Mat>& channels,
                 float sigma, float c, int min_size)
    : mChannels (channels), mDone (false), mMinSize(min_size), mColorDone(false), mHistDone(false)
    {
        mWidth = channels[0].size().width;
        mHeight = channels[0].size().height;
        int isize = channels[0].size().width * channels[0].size().height;
        mEdges.resize (4*isize);
        int num = compute_similarities ();
        
        // Sort similarities
        
        std::sort (mEdges.begin(), mEdges.end());
        
        // segment
        segment_graph(isize, num, c);
        
        
        // post process small components
        for (int i = 0; i < num; i++) {
            int a = mUniverse->find(mEdges[i].a);
            int b = mUniverse->find(mEdges[i].b);
            if ((a != b) && ((mUniverse->size(a) < min_size) || (mUniverse->size(b) < min_size)))
                mUniverse->join(a, b);
        }
        
        mComponents = mUniverse->num_sets();
        mColorized = cv::Mat(channels[0].size().height , channels[0].size().width, CV_8UC(3));
        mOutput = cv::Mat(channels[0].size().height , channels[0].size().width, CV_32SC(1));
        
        
        int ww = width();
        for( int i = 0; i < height() ; i++ )
            for( int j = 0; j < width() ; j++ )
            {
                int comp = mUniverse->find(i * ww + j);
                mOutput.at<int>(i,j) = comp;
            }
        
        
        
        mDone = true;
    }
    
    bool isDone () const { return mDone; }
    const std::vector<cv::Mat>& output () const { return mChannels; }
    const int32_t components () const { return mComponents; }
    const int32_t width () const { return mWidth; }
    const int32_t height () const { return mHeight; }
    const cv::Mat random_colorization () const { colorize(); return mColorized; }
    const cv::Mat segmented_output() const { return mOutput; }
    const sparse_histogram histogram () const { gen_histogram (); return mSpHist; }
    
    
private:
    
    mutable sparse_histogram mSpHist;
    int mWidth, mHeight;
    int mMinSize;
    std::vector<cv::Mat> mChannels;
    mutable cv::Mat mColorized;
    mutable cv::Mat mOutput;
    mutable int32_t mComponents;
    mutable bool mDone, mColorDone, mHistDone;
    std::vector<edge> mEdges;
    std::unique_ptr<universe> mUniverse;
    
    // dissimilarity measure between pixels
    static inline float diff_(const std::vector<cv::Mat>& channels, int x1, int y1, int x2, int y2)
    {
        float sum = 0.0f;
        for (auto cc : channels)
        {
            sum += square(cc.at<float>(y1,x1) - cc.at<float>(y2,x2));
        }
        return std::sqrt(sum);
    }
    
    static inline float diff(const std::vector<cv::Mat>& channels, int x1, int y1, int x2, int y2)
    {
        float c1 = channels[0].at<float>(y1,x1);
        if (c1 < 0) c1 += 255.0f;
        float c2 = channels[0].at<float>(y2,x2);
        if (c2 < 0) c2 += 255.0f;
        c2 = c1 - c2;
        if (c2 < 0) c2 += 255.0f;
        c2 = fmod (c2, 256);
        return std::sqrt(c2);
    }
    
    int compute_similarities ()
    {
        int num = 0;
        int ww = width();
        int hh = height();
        for (int y = 0; y < hh ; y++)
        {
            for (int x = 0; x < ww ; x++)
            {
                if ( x < ww - 1)
                {
                    mEdges[num].a = y * ww + x;
                    mEdges[num].b = y * ww + (x+1);
                    mEdges[num].w = diff(mChannels, x, y, x+1, y);
                    num++;
                }
                if ( y < hh - 1)
                {
                    mEdges[num].a = y * ww + x;
                    mEdges[num].b = (y+1) * ww + x;
                    mEdges[num].w = diff(mChannels, x, y, x, y+1);
                    num++;
                }
                if ((x < ww-1) && (y < hh-1))
                {
                    mEdges[num].a = y * ww + x;
                    mEdges[num].b = (y+1) * ww + (x+1);
                    mEdges[num].w = diff(mChannels, x, y, x+1, y+1);
                    num++;
                }
                
                if ((x < ww-1) && y > 0)
                {
                    mEdges[num].a = y * ww + x;
                    mEdges[num].b = (y-1) * ww + (x+1);
                    mEdges[num].w = diff(mChannels, x, y, x+1, y-1);
                    num++;
                }
            }
        }
        return num;
    }
    
    void segment_graph(int num_vertices, int num_edges, float c)
    {
        
        // make a disjoint-set forest
        mUniverse = make_unique<universe> (num_vertices);
        
        // init thresholds
        std::vector<float> threshold (num_vertices, THRESHOLD(1,c));
        
        
        // for each edge, in non-decreasing weight order...
        for (int i = 0; i < num_edges; i++) {
            edge *pedge = &mEdges[i];
            
            // components conected by this edge
            int a = mUniverse->find(pedge->a);
            int b = mUniverse->find(pedge->b);
            if (a != b) {
                if ((pedge->w <= threshold[a]) &&
                    (pedge->w <= threshold[b])) {
                    mUniverse->join(a, b);
                    a = mUniverse->find(a);
                    threshold[a] = pedge->w + THRESHOLD(mUniverse->size(a), c);
                }
            }
        }
    }
    
    void colorize () const
    {
        if (isDone() && mColorDone) return;
        else if (isDone() && !mColorDone)
        {
            RNG rng(12345);
            int ww = width();
            
            vector<Vec3b> colorTab;
            for( auto i = 0; i < width()*height() ; i++ )
            {
                uint8_t r = (uint8_t) random ();
                uint8_t g = (uint8_t) random ();
                uint8_t b = (uint8_t) random ();
                
                colorTab.push_back(Vec3b((uchar)b, (uchar)g, (uchar)r));
            }
            
            for( int i = 0; i < height() ; i++ )
                for( int j = 0; j < width() ; j++ )
                {
                    int comp = mUniverse->find(i * ww + j);
                    mColorized.at<Vec3b>(i,j)= colorTab[comp];
                }
            
            mColorDone = true;;
        }
    }
    
    void gen_histogram () const
    {
        if (isDone() && mHistDone) return;
        else if (isDone() && !mHistDone)
        {
            
            for (int y = 0; y < height(); y++) {
                for (int x = 0; x < width(); x++) {
                    int comp = mUniverse->find(y * width() + x);
                    mSpHist.add(comp);
                }
            }
            mHistDone = true;
        }
    }
    
    
    
    
};

#endif
