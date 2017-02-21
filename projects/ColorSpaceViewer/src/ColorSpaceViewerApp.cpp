#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "ColorPalette.h"
#include "cinder/CameraUi.h"
#include "cinder/Camera.h"
using namespace ci;
using namespace ci::app;


template<typename R>
bool is_ready(std::future<R> const& f)
{ return f.valid() && f.wait_for(std::chrono::seconds(0)) == std::future_status::ready; }


class ColorSpaceViewerApp : public App {
public:
    void setup() override;
    void update() override;
    void draw() override;
    void fileDrop( FileDropEvent event ) override;
    
    std::vector<Color8u> mRandomSampleColors;
    std::vector<Color8u> mMedianCutColors;
    
    col::AsyncPalette mRandomAsyncPalette, mMedianAsyncPalette;
    
    CameraPersp		mCam;
    CameraUi		mCamUi;
    
};

void ColorSpaceViewerApp::setup()
{
    gl::enableAlphaBlending();
    gl::enableDepthRead();
    gl::enableDepthWrite();
    
    mCamUi = CameraUi( &mCam, getWindow() );
    mCam.lookAt( vec3( 0.2f, 0.4f, 1.0f ), vec3(0.0f, 0.425f, 0.0f) );
    mCam.setNearClip( 0.01f );
    mCam.setFarClip( 100.0f );

}

void ColorSpaceViewerApp::update()
{
    if( is_ready( mRandomAsyncPalette ) ) {
        mRandomSampleColors = mRandomAsyncPalette.get();
    }
    
    if( is_ready( mMedianAsyncPalette ) ) {
        mMedianCutColors = mMedianAsyncPalette.get();
    }
}


void ColorSpaceViewerApp::draw()
{
    gl::clear();
    gl::enableDepthRead();
    gl::enableDepthWrite();
    
    gl::setMatrices( mCam );
    
    auto lambert = gl::ShaderDef().lambert().color();
    auto shader = gl::getStockShader( lambert );
    shader->bind();
    
    gl::drawCoordinateFrame ();
    
    for( auto& color : mRandomSampleColors ) {
        
        vec3 offsett( color.r / 256.0, color.g/ 256.0, color.b/256.0);
        gl::pushModelMatrix();
        gl::translate( offsett );
        gl::color( Color ( CM_RGB, color.r / 256.0, color.g/ 256.0, color.b/256.0));
        gl::drawSphere( vec3(), 0.067f);
        gl::popModelMatrix();
        

    }
    
//    int numSpheres = 64;
//    float maxAngle = M_PI * 7;
//    float spiralRadius = 1;s
//    float height = 3;
//    
//    
//    for( int s = 0; s < numSpheres; ++s ) {
//        float rel = s / (float)numSpheres;
//        float angle = rel * maxAngle;
//        float y = rel * height;
//        float r = rel * spiralRadius * spiralRadius;
//        vec3 offset( r * cos( angle ), y, r * sin( angle ) );
//        
//        gl::pushModelMatrix();
//        gl::translate( offset );
//        gl::color( Color( CM_HSV, rel, 1, 1 ) );
//        gl::drawSphere( vec3(), 0.1f, 30 );
//        gl::popModelMatrix();
//    }
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

void ColorSpaceViewerApp::fileDrop( FileDropEvent event )
{
    auto file = event.getFile(0);
    size_t nb = 64;
    
    mMedianAsyncPalette = std::async(std::launch::async, getPalette, file, 64, false );
    mRandomAsyncPalette = std::async(std::launch::async, getPalette	, file, 64, true );
}

CINDER_APP( ColorSpaceViewerApp, RendererGl )
