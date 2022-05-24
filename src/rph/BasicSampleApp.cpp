#include "cinder/app/AppNative.h"
#include "cinder/gl/gl.h"
#include "cinder/Rand.h"

#include "rph/DisplayObjectContainer.h"

#include "Square.h"
#include "Circle.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class BasicSampleApp : public AppNative {
  public:
	void setup();
	void mouseDown( MouseEvent event );	
	void update();
	void draw();
    
    rph::DisplayObjectContainer mContainer;
    
};

void BasicSampleApp::setup()
{
    ci::gl::enableAlphaBlending();
}

void BasicSampleApp::mouseDown( MouseEvent event )
{
    if(Rand::randFloat() > 0.5){
        Circle *c = new Circle();
        c->setRegPoint(rph::DisplayObject2D::RegistrationPoint::CENTERCENTER);
        c->setColor(Color(Rand::randFloat(),Rand::randFloat(),Rand::randFloat()));
        c->setScale(Rand::randInt(50,100));
        c->setPos( event.getPos() );
        c->fadeOutAndDie();
        mContainer.addChild(c);
    }else{
        Square *s = new Square();
        s->setRegPoint(rph::DisplayObject2D::RegistrationPoint::CENTERCENTER);
        s->setColor(Color(Rand::randFloat(),Rand::randFloat(),Rand::randFloat()));
        s->setSize(Rand::randInt(50,100),Rand::randInt(50,100));
        s->setPos( event.getPos() );
        s->fadeOutAndDie();
        mContainer.addChild(s);
    }
}

void BasicSampleApp::update()
{
    mContainer.update();
}

void BasicSampleApp::draw()
{
	// clear out the window with black
	gl::clear( Color( 0, 0, 0 ) );
    mContainer.draw();
}

CINDER_APP_NATIVE( BasicSampleApp, RendererGl )
