#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"

#include "ColorPalette.h"

template<typename R>
bool is_ready(std::future<R> const& f)
{ return f.valid() && f.wait_for(std::chrono::seconds(0)) == std::future_status::ready; }

using namespace ci;
using namespace ci::app;
using namespace std;

class ColorQuantizationApp : public App {
  public:
	void setup() override;
	void update() override;
	void draw() override;
	void fileDrop( FileDropEvent event ) override;
	
	std::vector<Color8u> mRandomSampleColors;
	std::vector<Color8u> mMedianCutColors;
	
	col::AsyncPalette mRandomAsyncPalette, mMedianAsyncPalette;
};

void ColorQuantizationApp::setup()
{
	gl::enableAlphaBlending();
}

void ColorQuantizationApp::update()
{
	if( is_ready( mRandomAsyncPalette ) ) {
		mRandomSampleColors = mRandomAsyncPalette.get();
	}
	
	if( is_ready( mMedianAsyncPalette ) ) {
		mMedianCutColors = mMedianAsyncPalette.get();
	}
}

void drawColorCircle( std::vector<Color8u> colors ) {
	if( colors.empty() )
		return;
	
	float radius = 100.0f;
	int idx = 0;
	for( auto& color : colors ) {
		gl::color( color );
		float angle = - idx / float( colors.size() ) * 2.0f * M_PI;
		vec2 pos( radius * cos( angle ), radius * sin( angle ) );
		gl::drawSolidCircle( pos, 60.0f );
		++idx;
	}
}

void ColorQuantizationApp::draw()
{
	gl::clear( Color::gray( 0.5f * ( sin(app::getElapsedSeconds()) + 1.0f ) ) );
	
	Font font("Arial", 16 );
	ColorA col( ColorA::white() );
	
	gl::ScopedMatrices push;
	gl::translate( getWindowWidth() / 4, getWindowHeight()/2 );
	drawColorCircle( mRandomSampleColors );
	gl::drawStringCentered( "Random", vec2( 0, - getWindowHeight()/2.15f ), col, font );
	
	gl::translate( getWindowWidth() / 2, 0 );
	drawColorCircle( mMedianCutColors );
	gl::drawStringCentered( "Median cut", vec2( 0, - getWindowHeight()/2.15f ), col, font );
}

std::vector<Color8u> getPalette( const fs::path& imageFile, size_t nb, bool random )
{
	try {
		col::PaletteGenerator generator( Surface8u( loadImage( loadFile( imageFile ) ) ) );
		return ( random ) ? generator.randomPalette( nb, 0.35f ) : generator.medianCutPalette( nb );
	}
	catch( ... ) {
		app::console() << "Palette generation failed." << std::endl;
	}
	return {};
}

void ColorQuantizationApp::fileDrop( FileDropEvent event )
{
	auto file = event.getFile(0);
	size_t nb = 64;
	
	mMedianAsyncPalette = std::async(std::launch::async, getPalette, file, 64, false );
	mRandomAsyncPalette = std::async(std::launch::async, getPalette	, file, 64, true );
}

CINDER_APP( ColorQuantizationApp, RendererGl, []( App::Settings * settings )
{
	int height = 450;
	settings->setWindowSize( 2 * height, height );
	settings->setHighDensityDisplayEnabled();
} )
