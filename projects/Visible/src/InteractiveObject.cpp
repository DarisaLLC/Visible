#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcomma"



#include "InteractiveObject.h"

#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"
#include "cinder/gl/TextureFont.h"

using namespace ci;
using namespace ci::app;
using namespace std;

//#define DEBUGIO

InteractiveObject::InteractiveObject( const Rectf& rect )
: m_rect (rect)
{
    pressedColor = Color( 1.0f, 0.0f, 0.0f );
    idleColor = Color( 0.7f, 0.7f, 0.7f );
    overColor = Color( 1.0f, 1.0f, 1.0f );
    strokeColor = Color( 0.0f, 0.0f, 0.0f );
    mPressed = false;
    mOver = false;
}

InteractiveObject::~InteractiveObject(){    
}

void InteractiveObject::draw(){
    if( mPressed ){
        gl::color( pressedColor );
    } else if( mOver ){
        gl::color( overColor );
    } else {
        gl::color( idleColor );
    }
    gl::drawSolidRect( getRect () );
    gl::color( strokeColor );
    gl::drawStrokedRect( getRect () );
}

void InteractiveObject::pressed(){
#if defined(DEBUGIO)
    console() << "pressed" << endl;
#endif
}

void InteractiveObject::pressedOutside(){
#if defined(DEBUGIO)
    console() << "pressed outside" << endl;
#endif
}

void InteractiveObject::released(){
#if defined(DEBUGIO)
    console() << "released" << endl;
#endif
}

void InteractiveObject::releasedOutside(){
#if defined(DEBUGIO)
    console() << "released outside" << endl;
#endif
}

void InteractiveObject::rolledOver(){
#if defined(DEBUGIO)
    console() << "rolled over" << endl;
#endif
}

void InteractiveObject::rolledOut(){
#if defined(DEBUGIO)
    console() << "rolled out" << endl;
#endif
}

void InteractiveObject::dragged(){
#if defined(DEBUGIO)
    console() << "dragged" << endl;
#endif    
}

bool InteractiveObject::update_norm_position ( ci::app::MouseEvent& event )
{
    const vec2 mousePos = event.getPos();
    mNormPos = vec2(mousePos.x / getRect().getWidth (), mousePos.y / getRect().getHeight () );
    return getRect().contains( mousePos );
}

void InteractiveObject::mouseDown( MouseEvent& event )
{
    if( update_norm_position ( event ) ){
        mPressed = true;
        mOver = false;
        pressed();
        mEvents.call( InteractiveObjectEvent( this, InteractiveObjectEvent::Pressed ) );
    } else {
        pressedOutside();
        mEvents.call( InteractiveObjectEvent( this, InteractiveObjectEvent::PressedOutside ) );
    }
}

void InteractiveObject::mouseUp( MouseEvent& event )
{
    if( update_norm_position ( event ) ){
        if( mPressed ){
            mPressed = false;
            mOver = true;
            released();
            mEvents.call( InteractiveObjectEvent( this, InteractiveObjectEvent::Released ) );
        }
    } else {
        mPressed = false;
        mOver = false;
        mEvents.call( InteractiveObjectEvent( this, InteractiveObjectEvent::ReleasedOutside ) );
    }
}

void InteractiveObject::mouseDrag( MouseEvent& event ){
    if( mPressed && update_norm_position ( event ) ){
        mPressed = true;
        mOver = false;
        dragged();
        mEvents.call( InteractiveObjectEvent( this, InteractiveObjectEvent::Dragged ) );
    }
}

void InteractiveObject::mouseMove( MouseEvent& event ){
    if( update_norm_position ( event ) ){
        if( mOver == false ){
            mPressed = false;
            mOver = true;
            rolledOver();
            mEvents.call( InteractiveObjectEvent( this, InteractiveObjectEvent::RolledOver ) );
        }
    } else {
        if( mOver ){
            mPressed = false;
            mOver = false;
            rolledOut();
            mEvents.call( InteractiveObjectEvent( this, InteractiveObjectEvent::RolledOut ) );
        }
    }
}

void InteractiveObject::removeListener( CallbackId callbackId ){
    mEvents.unregisterCb( callbackId );
}

#pragma GCC diagnostic pop
