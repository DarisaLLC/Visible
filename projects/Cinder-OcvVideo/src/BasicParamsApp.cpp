#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/Timeline.h"
#include "cinder/Rand.h"
#include "resources.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class BasicAppendTweenApp : public App {
public:
    void setup() override;
    void setDestinations();
    void startTweening();
    void mouseDown( MouseEvent event ) override;
    void draw() override;
    
    Anim<vec2>		mPos;
    	gl::TextureRef	mTexture;
    int				mNumDestinations;
    vector<vec2>	mDestinations;
};

void BasicAppendTweenApp::setup()
{
    auto img = loadResource( RES_IMAGE);
    
    mTexture = gl::Texture::create( loadImage( img ) ), gl::Texture::Format().loadTopDown() ;
    
    mPos().x = getWindowWidth()/16.0f;
    mPos().y = getWindowHeight()/2.0f;
    mNumDestinations = 10;
    setDestinations();
    startTweening();
}

void BasicAppendTweenApp::mouseDown( MouseEvent event )
{
    setDestinations();
    startTweening();
}

void BasicAppendTweenApp::setDestinations()
{
    // clear all destinations
    mDestinations.clear();
    mPos().x = getWindowWidth()/16.0f;
    mPos().y = getWindowHeight()/2.0f;
    // start from current position
  //  mDestinations.push_back( mPos );
    // add more destinations
    float scale = getWindowWidth() /(float)(mNumDestinations);
    
    for( int i=0; i<mNumDestinations; i++ ){
        vec2 v( mPos().x + i * scale , mPos().y + sin( i * scale)*getWindowHeight()/8.0f);
        mDestinations.push_back( v );
    }
}

void BasicAppendTweenApp::startTweening()
{
    timeline().apply( &mPos, (vec2)mDestinations[0], 0.5f, EaseInOutQuad() );
    for( int i=1; i<mNumDestinations; i++ ){
        timeline().appendTo( &mPos, (vec2)mDestinations[i], 0.5f, EaseInOutQuad() );
    }
}


void BasicAppendTweenApp::draw()
{
    gl::clear( Color( 0.2f, 0.2f, 0.2f ) );
    gl::enableAlphaBlending();
    
    for( int i=0; i<mNumDestinations; i++ ){
        gl::color( Color( 1, 0, 0 ) );
        gl::drawSolidCircle( mDestinations[i], 3.0f );
        Rectf r (mDestinations[i] - vec2(13.0,13.0), mDestinations[i] + vec2(13.0,13.0));
        gl::draw (mTexture, r);
        
        if( i > 0 ){
            gl::color( ColorA( 1, 0, 0, 0.25f ) );
            gl::drawLine( mDestinations[i], mDestinations[i-1] );
        }
    }
    
    gl::color( Color::white() );
    gl::drawSolidCircle( mPos, 20.0f );
}


CINDER_APP( BasicAppendTweenApp, RendererGl )