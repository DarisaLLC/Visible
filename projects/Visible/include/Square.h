
#pragma once

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
    

