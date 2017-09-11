

#pragma once
#include "DisplayObject.h"

class Circle : public rph::DisplayObject2D{
public:
    Circle(){
        setSize(1,1);
    };
    ~Circle(){};
    
    virtual void    setup();
    virtual void    update();
    virtual void    draw();
    
    void fadeOutAndDie();
};
