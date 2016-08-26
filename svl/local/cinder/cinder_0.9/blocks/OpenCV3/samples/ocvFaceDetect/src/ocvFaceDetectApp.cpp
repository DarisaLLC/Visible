#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/Capture.h"

#include "CinderOpenCv.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class ocvFaceDetectApp : public App {
 public:
	void setup() override;

	void updateFaces( const SurfaceRef &cameraImage );
	void update() override;
	
	void draw() override;

	CaptureRef			mCapture;
	gl::TextureRef		mCameraTexture;
	
	cv::CascadeClassifier	mFaceCascade, mEyeCascade;
	vector<Rectf>			mFaces, mEyes;
};

void ocvFaceDetectApp::setup()
{
	mFaceCascade.load( getAssetPath( "haarcascade_frontalface_alt.xml" ).string() );
	mEyeCascade.load( getAssetPath( "haarcascade_eye.xml" ).string() );	

	Capture::DeviceRef device;
#if defined( CINDER_COCOA_TOUCH )
	for( auto dev : Capture::getDevices() )
		if( dev->isFrontFacing() )
			device = dev;
#endif
	mCapture = Capture::create( 640, 480, device );
	mCapture->start();
}

void ocvFaceDetectApp::updateFaces( const SurfaceRef &cameraImage )
{
	const int calcScale = 2; // calculate the image at half scale

	// create a grayscale copy of the input image
	cv::Mat grayCameraImage( toOcv( *cameraImage, CV_8UC1 ) );

	// scale it to half size, as dictated by the calcScale constant
	int scaledWidth = cameraImage->getWidth() / calcScale;
	int scaledHeight = cameraImage->getHeight() / calcScale;
	cv::Mat smallImg( scaledHeight, scaledWidth, CV_8UC1 );
	cv::resize( grayCameraImage, smallImg, smallImg.size(), 0, 0, cv::INTER_LINEAR );
	
	// equalize the histogram
	cv::equalizeHist( smallImg, smallImg );

	// clear out the previously deteced faces & eyes
	mFaces.clear();
	mEyes.clear();

	// detect the faces and iterate them, appending them to mFaces
	vector<cv::Rect> faces;
	mFaceCascade.detectMultiScale( smallImg, faces );
	for( vector<cv::Rect>::const_iterator faceIter = faces.begin(); faceIter != faces.end(); ++faceIter ) {
		Rectf faceRect( fromOcv( *faceIter ) );
		faceRect *= calcScale;
		mFaces.push_back( faceRect );
		
		// detect eyes within this face and iterate them, appending them to mEyes
		vector<cv::Rect> eyes;
		mEyeCascade.detectMultiScale( smallImg( *faceIter ), eyes );
		for( vector<cv::Rect>::const_iterator eyeIter = eyes.begin(); eyeIter != eyes.end(); ++eyeIter ) {
			Rectf eyeRect( fromOcv( *eyeIter ) );
			eyeRect = eyeRect * calcScale + faceRect.getUpperLeft();
			mEyes.push_back( eyeRect );
		}
	}
}

void ocvFaceDetectApp::update()
{
	if( mCapture->checkNewFrame() ) {
		auto surface = mCapture->getSurface();
		mCameraTexture = gl::Texture::create( *surface, gl::Texture::Format().loadTopDown() );
		updateFaces( surface );
	}
}

void ocvFaceDetectApp::draw()
{
	if( ! mCameraTexture )
		return;

	gl::setMatricesWindow( getWindowSize() );
	gl::enableAlphaBlending();
	
	// draw the webcam image
	gl::color( Color( 1, 1, 1 ) );
	gl::draw( mCameraTexture );
	
	// draw the faces as transparent yellow rectangles
	gl::color( ColorA( 1, 1, 0, 0.45f ) );
	for( vector<Rectf>::const_iterator faceIter = mFaces.begin(); faceIter != mFaces.end(); ++faceIter )
		gl::drawSolidRect( *faceIter );
	
	// draw the eyes as transparent blue ellipses
	gl::color( ColorA( 0, 0, 1, 0.35f ) );
	for( vector<Rectf>::const_iterator eyeIter = mEyes.begin(); eyeIter != mEyes.end(); ++eyeIter )
		gl::drawSolidCircle( eyeIter->getCenter(), eyeIter->getWidth() / 2 );
}

CINDER_APP( ocvFaceDetectApp, RendererGl )
