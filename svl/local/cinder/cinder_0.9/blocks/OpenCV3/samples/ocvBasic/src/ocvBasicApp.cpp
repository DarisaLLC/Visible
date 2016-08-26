#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/ImageIo.h"

#include "CinderOpenCv.h"

#include "Resources.h"

using namespace ci;
using namespace ci::app;

class ocvBasicApp : public App {
  public:
	void setup() override;
	void draw() override;
	
	gl::TextureRef	mTexture;
};

void ocvBasicApp::setup()
{
	// The included image is copyright Trey Ratcliff
	// http://www.flickr.com/photos/stuckincustoms/4045813826/
	
	ci::Surface8u surface( loadImage( loadAsset( "dfw.jpg" ) ) );
	cv::Mat input( toOcv( surface ) );
	cv::Mat output;

	cv::medianBlur( input, output, 11 );
// Uncomment these to try different operations
//	cv::Sobel( input, output, CV_8U, 0, 1 );
//	cv::threshold( input, output, 128, 255, CV_8U );

	mTexture = gl::Texture::create( fromOcv( output ) );
}   

void ocvBasicApp::draw()
{
	gl::clear();
	gl::draw( mTexture );
}

CINDER_APP( ocvBasicApp, RendererGl )