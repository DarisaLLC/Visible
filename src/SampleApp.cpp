#include "cinder_opencv.h"

#include "cinder/Capture.h"
#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"

#include "boost/filesystem.hpp"
#include "spdlog/spdlog.h"

using namespace ci;
using namespace ci::app;

namespace boost {
	namespace fs = filesystem;
}

class SampleApp : public App
{
  public:
	void setup();
	void update();
	void draw();

	CaptureRef mCapture;
	gl::TextureRef mTexture;
};

void SampleApp::setup()
{
	try
	{
		mCapture = Capture::create( 640, 480 );
		mCapture->start();
	}
	catch ( ... )
	{
		console() << "Failed to initialize capture" << std::endl;
	}

	spdlog::info("Welcome to spdlog version {}.{}.{}  !", SPDLOG_VER_MAJOR, SPDLOG_VER_MINOR, SPDLOG_VER_PATCH);
	spdlog::shutdown();

	boost::fs::path p( getAppPath().string() );

	if ( boost::fs::exists( p ) )
	{
		if ( boost::fs::is_regular_file( p ) )
		{
			app::console() << p << " size is " << boost::fs::file_size( p ) << '\n';
		}
		else if ( is_directory( p ) )
		{
			app::console() << p << " is a directory\n";
		}
		else
		{
			app::console() << p << " exists, but is neither a regular file nor a directory\n";
		}
	}
}

void SampleApp::update()
{
	if ( mCapture && mCapture->checkNewFrame() )
	{
	  cv::Mat input( cinder::toOcv( *mCapture->getSurface(), CV_8UC1 ) ), output;

		cv::Sobel( input, output, CV_8UC1, 1, 0 );

		mTexture = gl::Texture::create( cinder::fromOcv( output ), gl::Texture::Format().loadTopDown() );
	}
}

void SampleApp::draw()
{
	gl::clear();
	if ( mTexture )
	{
		gl::draw( mTexture );
	}
}

CINDER_APP( SampleApp, RendererGl )
