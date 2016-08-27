#include "InteractiveObject.h"

#include "cinder/gl/gl.h"
#include "cinder/app/App.h"
#include "cinder/Function.h"
#include "cinder/gl/Texture.h"
#include "cinder/gl/TextureFont.h"

using namespace ci;
using namespace ci::app;
using namespace std;

InteractiveObject::InteractiveObject( const Rectf& rect ){
    this->rect = rect;
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
    gl::drawSolidRect( rect );
    gl::color( strokeColor );
    gl::drawStrokedRect( rect );
}

void InteractiveObject::pressed(){
    console() << "pressed" << endl;
}

void InteractiveObject::pressedOutside(){
    console() << "pressed outside" << endl;
}

void InteractiveObject::released(){
    console() << "released" << endl;
}

void InteractiveObject::releasedOutside(){
    console() << "released outside" << endl;
}

void InteractiveObject::rolledOver(){
    console() << "rolled over" << endl;
}

void InteractiveObject::rolledOut(){
    console() << "rolled out" << endl;
}

void InteractiveObject::dragged(){
    console() << "dragged" << endl;
}

bool InteractiveObject::update_norm_position ( ci::app::MouseEvent& event )
{
    const vec2 mousePos = event.getPos();
    mNormPos = vec2(mousePos.x / rect.getWidth (), mousePos.y / rect.getHeight () );
    return rect.contains( mousePos );
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

