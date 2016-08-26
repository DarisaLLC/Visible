#include "cinder/app/App.h"

#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"
#include "cinder/Surface.h"
#include "cinder/ImageIo.h"
#include "cinder/cairo/Cairo.h"
#include "cinder/Capture.h"
#include "cinder/params/Params.h"

#include "CinderOpenCV.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class MainApp : public AppNative {
 public:
	void setup();
    void mouseDown(MouseEvent event);
    void update();
	void draw();
    
    void setTrackingHSV();
    
    Surface8u   mImage;
    
    vector<cv::Point2f> mCenters;
    vector<float> mRadius;
    
    double mApproxEps;
    int mCannyThresh;

    Capture     mCapture;
    gl::Texture mCaptureTex;

    ColorA      mPickedColor;
    cv::Scalar  mColorMin;
    cv::Scalar  mColorMax;
    
	params::InterfaceGl		mParams;
};

void MainApp::setup()
{
    
    setWindowSize(640, 480);

    try {
        mCapture = Capture( 640, 480 );
        mCapture.start();
    }
    catch( ... ) {
        console() << "Failed to initialize capture" << std::endl;
    }

    mApproxEps = 1.0;
    mCannyThresh = 200;

    mPickedColor = Color8u(255, 0, 0);
    setTrackingHSV();

    // Setup the parameters
    mParams = params::InterfaceGl( "Parameters", Vec2i( 200, 150 ) );
    mParams.addParam( "Picked Color", &mPickedColor, "readonly=1" );
    
    
    
}


void MainApp::mouseDown(MouseEvent event)
{
    if( mImage && mImage.getBounds().contains( event.getPos() ) ) {
        
        mPickedColor = mImage.getPixel( event.getPos() );
        setTrackingHSV();
    }
}

void MainApp::setTrackingHSV()
{
    Color8u col = Color( mPickedColor );
    Vec3f colorHSV = col.get(CM_HSV);
    colorHSV.x *= 179;
    colorHSV.y *= 255;
    colorHSV.z *= 255;
    
    mColorMin = cv::Scalar(colorHSV.x-5, colorHSV.y -50, colorHSV.z -50);
    mColorMax = cv::Scalar(colorHSV.x+5, 255, 255);
    
}

void MainApp::update()
{
    if( mCapture && mCapture.checkNewFrame() ) {
        mImage = mCapture.getSurface();
        mCaptureTex = gl::Texture( mImage );
        
        // do some CV
        cv::Mat inputMat( toOcv( mImage) );
        
        cv::resize(inputMat, inputMat, cv::Size(320, 240) );
        
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
	}
}

void MainApp::draw() 
{
    gl::clear( Color( 0.1f, 0.1f, 0.1f ) );

    gl::color(Color::white());

    if(mCaptureTex) {
        gl::draw(mCaptureTex);
        
        gl::color(Color::white());
        
        for( int i = 0; i < mCenters.size(); i++ )
        {
            Vec2f center = fromOcv(mCenters[i])*2.f;
            gl::begin(GL_POINTS);
            gl::vertex( center );
            gl::end();
            
            gl::drawStrokedCircle(center, mRadius[i]*2.f );
        }
    }
    
    // Draw the interface
	params::InterfaceGl::draw();
}


CINDER_APP_NATIVE( MainApp, RendererGl )
