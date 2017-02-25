#pragma once

#include "cinder/Rect.h"
#include "cinder/Color.h"
#include "cinder/app/MouseEvent.h"
#include "cinder/Function.h"
#include "time_index.h"
#include <iostream>

#include <vector>






class marker_info
{
public:
    enum event_type { move = 0, down = move+1, num_types = down+1 };

    cinder::vec2 norm_pos;
    float norm_time;
    float val;
    int32_t index;
    index_time_t timestamp;
    event_type et;
    
    friend std::ostream& operator<< (std::ostream& std_stream, marker_info& t)
    {
        
        std_stream << "Normalized Position:" << t.norm_pos.x << " x " << t.norm_pos.y << std::endl;
        std_stream << "Value:  " << t.val <<  std::endl;
        std_stream << "Event:    " << ((t.et == event_type::move) ? "move" : "down") << std::endl;
        std_stream << "Index:    " << t.index << std::endl;
        return std_stream;
    }
    
};




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
    void norm_pos (vec2& nn) const { mNormPos = nn; }
    
    
    ci::Rectf getRect () const { return m_rect; }
    void setRect (const ci::Rectf& new_rect) { m_rect = new_rect; }
    
    ci::Color pressedColor, idleColor, overColor, strokeColor;
    
protected:
    ci::Rectf m_rect;
    bool mPressed, mOver;
    ci::CallbackMgr< void(InteractiveObjectEvent) > mEvents;
    mutable vec2 mNormPos;
    bool update_norm_position ( ci::app::MouseEvent& event  );
};

