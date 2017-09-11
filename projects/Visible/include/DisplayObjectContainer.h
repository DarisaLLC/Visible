
#pragma once
#include "DisplayObject.h"
#include "ObjectContainer.h"

namespace rph {

    class DisplayObjectContainer : public DisplayObject2D, public ObjectContainer {
  
      public:
        DisplayObjectContainer(){};
        ~DisplayObjectContainer(){};
        virtual void update(float deltaTime = 0.0f, int beginIndex = 0, int endIndex = 0x7fffffff){ObjectContainer::update(deltaTime, beginIndex, endIndex);}
        virtual void draw(int beginIndex = 0, int endIndex = 0x7fffffff){ObjectContainer::draw(beginIndex, endIndex);}
    };
    
}
