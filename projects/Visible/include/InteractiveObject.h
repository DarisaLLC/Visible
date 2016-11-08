#pragma once

//#include "vf_types.h"
#include "cinder/Rect.h"
#include "cinder/Color.h"
#include "cinder/app/MouseEvent.h"
#include "cinder/Function.h"
#include <vector>





using namespace ci;
//using namespace ci::app;
using namespace std;
class InteractiveObject;

class InteractiveObjectEvent: public ci::app::Event
{
public:
    enum EventType{Pressed, PressedOutside, Released, ReleasedOutside, RolledOut, RolledOver, Dragged, Changed};
    
    InteractiveObjectEvent( InteractiveObject *sender, EventType type )
    {
        this->sender = sender;
        this->type = type;
    }
    
    InteractiveObject *sender;
    EventType type;
};

class InteractiveObject
{
public:
    InteractiveObject( const ci::Rectf& rect );
    virtual ~InteractiveObject();
    
    virtual void draw();
    
    virtual void pressed();
    virtual void pressedOutside();
    virtual void released();
    virtual void releasedOutside();
    virtual void rolledOver();
    virtual void rolledOut();
    virtual void dragged();
    
    void mouseDown( ci::app::MouseEvent& event );
    void mouseUp( ci::app::MouseEvent& event );
    void mouseDrag( ci::app::MouseEvent& event );
    void mouseMove( ci::app::MouseEvent& event );
    
    template< class T >
    ci::CallbackId addListener( T* listener, void (T::*callback)(InteractiveObjectEvent) )
    {
        return mEvents.registerCb( std::bind1st( std::mem_fun( callback ), listener ) );
    }
    
    void removeListener( ci::CallbackId callId );

    const vec2& norm_pos () const { return mNormPos; }
    
    
    ci::Rectf rect;
    ci::Color pressedColor, idleColor, overColor, strokeColor;
    
protected:
    bool mPressed, mOver;
    ci::CallbackMgr< void(InteractiveObjectEvent) > mEvents;
    vec2 mNormPos;
    bool update_norm_position ( ci::app::MouseEvent& event  );
};

