#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"
#include "cinder/Surface.h"
#include "cinder/Capture.h"
#include "cinder/Log.h"
#include "cinder/Rand.h"

//#include "cinder/params/Params.h"

#include "CinderOpenCV.h"

#define SIZE 512

using namespace ci;
using namespace ci::app;
using namespace std;

struct TouchPoint {
    TouchPoint() {}
    TouchPoint( const vec2 &initialPt, const Color &color ) : mColor( color ), mTimeOfDeath( -1.0 )
    {
        mLine.push_back( initialPt );
    }
    
    void addPoint( const vec2 &pt ) { mLine.push_back( pt ); }
    
    void draw() const
    {
        if( mTimeOfDeath > 0 ) // are we dying? then fade out
            gl::color( ColorA( mColor, ( mTimeOfDeath - getElapsedSeconds() ) / 2.0f ) );
        else
            gl::color( mColor );
        
        gl::draw( mLine );
    }
    
    void startDying() { mTimeOfDeath = getElapsedSeconds() + 2.0f; } // two seconds til dead
    
    bool isDead() const { return getElapsedSeconds() > mTimeOfDeath; }
    
    PolyLine2f		mLine;
    Color			mColor;
    float			mTimeOfDeath;
};


class ColorTrackApp : public App {
public:
    static void	prepareSettings( Settings *settings );
    void setup() override;
    void mouseDown(MouseEvent event) override;
    // void	mouseDrag( MouseEvent event ) override;
    
    void	touchesBegan( TouchEvent event ) override;
    void	touchesMoved( TouchEvent event ) override;
    void	touchesEnded( TouchEvent event ) override;
    void update() override;
    void draw() override;
    
    void setTrackingHSV();
    void increment ();
    
    Surface8uRef   mImage;
    
    vector<cv::Point2f> mCenters;
    vector<float> mRadius;
    vector<deque<float> > mTracks;
    
    double mApproxEps;
    int mCannyThresh;
    
    CaptureRef     mCapture;
    gl::TextureRef mCaptureTex;
    
    ColorA      mPickedColor;
    ColorA      mResultColor;
    int         mCurrentIndex;
    Color8u     mColors[3];
    cv::Scalar  mColorMin;
    cv::Scalar  mColorMax;
    Rectf       mGraph;
    vec2        mTol;
   
private:
    map<uint32_t,TouchPoint>	mActivePoints;
    list<TouchPoint>			mDyingPoints;
    
    //params::InterfaceGlRef		mParams;
};

void ColorTrackApp::prepareSettings( Settings *settings )
{
    settings->setFrameRate( 30.0f );
    // On mobile, if you disable multitouch then touch events will arrive via mouseDown(), mouseDrag(), etc.
    settings->setMultiTouchEnabled( true );
}

void ColorTrackApp::setup()
{
    CI_LOG_I( "MT: " << System::hasMultiTouch() << " Max points: " << System::getMaxMultiTouchPoints() );
    
    mGraph = Rectf (0,(float)getWindowHeight()-50,(float)getWindowWidth() / 5, 50.0f);
    try {
        mCapture = Capture::create(640, 480);
        mCapture->start();
    }
    catch( ... ) {
        console() << "Failed to initialize capture" << std::endl;
    }
    
    mColors[0] = Color8u(324/2, 255, 255);
    mColors[1] = Color8u(210/2, 255, 255);
    mColors[2] = Color8u(55/2, 255, 255);
    mCurrentIndex = 0;
    mApproxEps = 1.0;
    mCannyThresh = 200;
    mTracks.resize (1);
    
    mPickedColor = ColorA( 0, 1, 1, 0.7f );
    mTol.x = mTol.y = 10;
    setTrackingHSV();
    
    
}

void drawResultBars (const std::vector<std::deque<float> > &buffer, const Rectf &bounds, bool drawFrame, const ci::ColorA &color )
{
    gl::color( color );
    gl::drawSolidCircle( bounds.getCenter(), 10.0f, 100);

}

void drawResultBuffer( const std::vector<std::deque<float> > &buffer, const Rectf &bounds, bool drawFrame, const ci::ColorA &color )
{
    gl::ScopedGlslProg glslScope( getStockShader( gl::ShaderDef().color() ) );
    
    gl::color( color );
    
    const float waveHeight = bounds.getHeight() / (float)buffer.size();
    
    const float xScale = bounds.getWidth() / (float)buffer[0].size();
    
    float yOffset = bounds.y1;
    for( size_t ch = 0; ch < buffer.size(); ch++ ) {
        PolyLine2f waveform;
        if(buffer[ch].empty()) continue;
        
        const float *channel = &buffer[ch][0];
        float x = bounds.x1;
        for( size_t i = 0; i < buffer[ch].size(); i++ ) {
            x += xScale;
            float y = ( 1 - ( channel[i] * 0.5f + 0.5f ) ) * waveHeight + yOffset;
            waveform.push_back( vec2( x, y ) );
        }
        
        if( ! waveform.getPoints().empty() )
            gl::draw( waveform );
        
        yOffset += waveHeight;
    }
    
    if( drawFrame ) {
        gl::color( color.r, color.g, color.b, color.a * 0.6f );
        gl::drawStrokedRect( bounds );
    }
}

void ColorTrackApp::mouseDown(MouseEvent event)
{
    if( mImage && mImage->getBounds().contains( event.getPos() ) ) {
        increment();
    }
}

void ColorTrackApp::increment()
{
    mCurrentIndex++;
    mCurrentIndex = mCurrentIndex % 3;
    setTrackingHSV();
    for (auto t : mTracks) t.clear();
}
void ColorTrackApp::setTrackingHSV()
{
    vec3 colorHSV = mColors[mCurrentIndex]; //.get(CM_HSV);
    mColorMin = cv::Scalar(colorHSV.x-mTol.x, 100, 100);
    mColorMax = cv::Scalar(colorHSV.x+mTol.y, 255, 255);
}

void ColorTrackApp::update()
{
    if( mCapture && mCapture->checkNewFrame() ) {
        mImage = mCapture->getSurface();
        mCaptureTex = gl::Texture::create(*mImage );
        
        // do some CV
        cv::Mat inputMat( toOcv( *mImage) );
        
        cv::resize(inputMat, inputMat, cv::Size(320, 240) );
        cv::Scalar lmean, lstd;
        cv::Mat inputHSVMat, frameTresh;
        cv::cvtColor(inputMat, inputHSVMat, CV_BGR2HSV);
        
        cv::inRange(inputHSVMat, mColorMin, mColorMax, frameTresh);
        
        
        cv::medianBlur(frameTresh, frameTresh, 7);
        
        cv::Mat cannyMat;
        cv::Canny(frameTresh, cannyMat, mCannyThresh, mCannyThresh*2.f, 3 );
        
        vector< std::vector<cv::Point> >  contours;
        cv::findContours(cannyMat, contours, CV_RETR_LIST, CV_CHAIN_APPROX_SIMPLE);
        
        mCenters = vector<cv::Point2f>(contours.size());
        mRadius = vector<float>(contours.size());
        for( int i = 0; i < contours.size(); i++ )
        {
            std::vector<cv::Point> approxCurve;
            cv::approxPolyDP(contours[i], approxCurve, mApproxEps, true);
            cv::minEnclosingCircle(approxCurve, mCenters[i], mRadius[i]);
        }
        
        cv::meanStdDev(inputHSVMat,lmean, lstd, frameTresh);
        float hscore = 1.0f - std::abs(lmean[0] - mColors[mCurrentIndex][0]) / mColors[mCurrentIndex][0];
        mResultColor = hscore > 0.92 ? ColorA(0.0f,hscore,0.0f) : ColorA(1.0f,0.0f,0.0f);
    }
}

void ColorTrackApp::touchesBegan( TouchEvent event )
{
    CI_LOG_I( event );
    
    for( const auto &touch : event.getTouches() ) {
        Color newColor( CM_HSV, Rand::randFloat(), 1, 1 );
        mActivePoints.insert( make_pair( touch.getId(), TouchPoint( touch.getPos(), newColor ) ) );
    }
}

void ColorTrackApp::touchesMoved( TouchEvent event )
{
    CI_LOG_I( event );
    for( const auto &touch : event.getTouches() ) {
        mActivePoints[touch.getId()].addPoint( touch.getPos() );
    }
}

void ColorTrackApp::touchesEnded( TouchEvent event )
{
    CI_LOG_I( event );
    for( const auto &touch : event.getTouches() ) {
        mActivePoints[touch.getId()].startDying();
        mDyingPoints.push_back( mActivePoints[touch.getId()] );
        mActivePoints.erase( touch.getId() );
    }
    increment();
}


void ColorTrackApp::draw()
{
    gl::clear( Color( 0.1f, 0.1f, 0.1f ) );
    
    gl::color(Color::white());
    
    if(mCaptureTex)
    {
        gl::ScopedModelMatrix modelScope;
        
#if 0 //defined( CINDER_COCOA_TOUCH )
        // change iphone to landscape orientation
        gl::rotate( M_PI / 2 );
        gl::translate( 0, - getWindowWidth() );
        Rectf flippedBounds( 0, 0, getWindowHeight(), getWindowWidth() );
        gl::draw( mCaptureTex, flippedBounds );
#else
        gl::draw( mCaptureTex );
#endif
        
         if(! mTracks[0].empty()) mTracks[0].pop_front();
        
        gl::color(mResultColor);
        
        for( int i = 0; i < mCenters.size(); i++ )
        {
            vec2 nc = fromOcv(mCenters[i])*2.f;
#if 0 // defined( CINDER_COCOA_TOUCH )
            vec2 center (nc.x, nc.y - getWindowWidth() );
#else
            vec2 center = nc;
#endif
            
            //   gl::begin(GL_POINTS);
            //   gl::vertex( center );
            //   gl::end();
            
            gl::drawStrokedCircle(center, mRadius[i]*2.f );
        }
    }
    

   
}


CINDER_APP( ColorTrackApp, RendererGl, ColorTrackApp::prepareSettings)
