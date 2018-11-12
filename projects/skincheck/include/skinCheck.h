#pragma once

#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/capture.h"
#include "cinder/gl/Context.h"
#include "cinder/gl/Texture.h"
#include "cinder/gl/gl.h"
#include "cinder/Xml.h"
#include "cinder/Timeline.h"
#include "cinder/ImageIo.h"
#include "cinder/Thread.h"
#include "cinder/log.h"
#include "cinder/ConcurrentCircularBuffer.h"
#include "cinder/gl/TextureFont.h"
#include "cinder/gl/Sync.h"
#include "cinder/CinderResources.h"
#include <vector>
#include <map>
#include <functional>
#include <numeric>
#include <cmath>
#include "cinder/Rect.h"
#include "CinderOpenCV.h"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui.hpp"
#include "vision/localvariance.h"
#include "touchaction.h"

using namespace ci;
using namespace ci::app;
using namespace std;


void cb_test_mark (int i, bool b)
{
    string start_marker = b ? "<" : " ";
    string end_marker = b ? ">" : " ";
    std::cout << start_marker << i << end_marker;
}




class skinCheckApp : public App
{
public:
    typedef std::function<void()> overlay_producer;
    ~skinCheckApp();

    void cleanup ();
    void setup() override;
    void update() override;
    void draw() override;
    void captureThreadFn( gl::ContextRef sharedGlContext );
    
#if defined( CINDER_MAC )
    void mouseMove( MouseEvent event ) override
    {
        mTouchPos = event.getPos();
    }
#endif
    
    void configureFrameRate ();
    
    bool focus_at (float lens_location);
    int focus_mode () const;
    bool focus_mode (int new_mode);
    
    
    static void prepareSettings( skinCheckApp::Settings *settings );
private:
    void setupVisuals ();
    void infoTapped ();
    void thirdTapped ();
    
    std::string pixelInfo( const ivec2& );
    std::map<uint32_t, overlay_producer> mProducerMap;
    float mPad;
    CaptureRef			mCapture;
    SurfaceRef             mSurface;
    int mCount;
    vec2 mPos, mTouchPos;
    double					mLastTime;
    double					mStartTime;
    double                  mFps;
    GLsync                  mFence;
    float               mScore;
    std::vector<float> mScores;
    std::string         mBlur;
    Font				mFont;
    gl::TextureFontRef	mTextureFont;
    std::vector<cv::Mat> mB_G_R;
    cv::Mat mBGR;
    std::vector<Area> mFocusAreas;
    std::vector<Rectf> mFocusRects;
    Area mCurrentDisplayArea;
    Area mCurrentCaptureArea;
    float mActiveRadius;
    std::bitset<8> mChecks;
    vector<touch> mCenters;
    std::unique_ptr<ConcurrentCircularBuffer<gl::TextureRef> >	mTextures;
    gl::TextureRef                                      mOverlay;
    cv::Mat mGray;
    bool					mShouldQuit;
    shared_ptr<thread>		mThread;
    gl::TextureRef			mTexture, mLastTexture;
    std::atomic<bool>       mFocusStepMode;
    
    
    
    void set_check (int i, bool b)
    {
        mChecks[i] = b;
        cb_test_mark(i, b);
    }
    
    
    overlay_producer overex_producer;
    overlay_producer shadow_producer;
    overlay_producer edge_producer;
    overlay_producer blur_producer;
    
    void overex_producer_impl ();
    void shadow_producer_impl ();
    void edge_producer_impl ();
    void blur_producer_impl ();
    
    
    bool update_touch ();
    bool check_touch (const vec2&);
    void compute_blur (const cv::Mat& src, const std::vector<Area>& rois, const int pad, std::vector<float>& scores);
    
    int which (vec2 position);
    void render_dynamic_content ();
    void render_static_content ();
    
    void printDevices();
    void update_overlay (cv::Mat& );
    void clear_overlay ();
    
    static void compute_shadow_overlay (const std::vector<cv::Mat>& bgr, cv::Mat& mask);
    static void compute_overex_overlay (const std::vector<cv::Mat>& bgr, cv::Mat& mask);
    static void compute_skin_overlay (const std::vector<cv::Mat>& bgr, cv::Mat& mask);
    static void compute_edge_overlay (const std::vector<cv::Mat>& bgr, cv::Mat& mask);
    
    void norm2image (const Rectf& norm, Area& img)
    {
        
        vec2 ipos (getWindowWidth() * norm.getX1(), getWindowHeight()* norm.getY1());
        vec2 jpos (getWindowWidth() * norm.getX2(), getWindowHeight()* norm.getY2());
        
        img = Area (ipos, jpos);
    }
    
    void norm2image (const vec2& norm, vec2& img)
    {
        img.x = getWindowWidth() * norm.x;
        img.y = getWindowHeight()* norm.y;
    }
    
    void image2norm(const vec2& img, vec2& norm)
    {
        norm.x = img.x / ((float)getWindowWidth());
        norm.y = img.y / ((float)getWindowHeight());
    }
};
