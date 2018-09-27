
#pragma once

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Woverloaded-virtual"

#include "DisplayObject.h"

class Square : public rph::DisplayObject2D{
public:
    Square(){};
    ~Square(){};
    
    virtual void    setup();
    virtual void    update();
    virtual void    draw();
    
    void fadeOutAndDie();
};
    
#pragma GCC diagnostic pop
