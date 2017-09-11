
#pragma once

#include <string>

namespace rph {

class Object {
  public:
    Object():mIsDead(false){};
    ~Object(){};
    
    virtual void update( float deltaTime = 0.0f){};
    virtual void draw(){};
    virtual void die(){ mIsDead = true; }
    virtual bool isDead(){ return mIsDead; }
    
    std::string id;
  protected:
    bool         mIsDead;
    
    
};
    
}
