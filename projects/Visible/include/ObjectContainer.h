
#pragma once
#include <vector>
#include "Object.h"

namespace rph {

class ObjectContainer {
  
public:
    ObjectContainer(){};
    ~ObjectContainer(){};
    
    virtual void update( float deltaTime = 0.0f, int beginIndex = 0, int endIndex = 0x7fffffff);
    virtual void draw(int beginIndex = 0, int endIndex = 0x7fffffff);
    
    void    addChild(Object *obj){ if(obj != NULL) mChildren.push_back(obj); }
    void    addChild( Object *obj, int index){ if(obj != NULL) mChildren.insert( mChildren.begin()+index, obj ); };
    
    int getNumChildren(){ return mChildren.size(); }
//    bool  contains(const DisplayObject &obj){};
    
    Object*   getChildAt(int index){ return mChildren[index]; }
    std::vector<Object *>* getChildren(){ return &mChildren; }
//    DisplayObject   getChildByName(std::string name){};
//    int getChildIndex(DisplayObject child){};
    
//    vector<DisplayObject>  getObjectsUnderPoint(ci::vec2 point){};
    
//    DisplayObject removeChild(DisplayObject child){};
    Object* removeChildAt(int index);
    void    removeChildren(int beginIndex = 0, int endIndex = 0x7fffffff);
//    void setChildIndex(DisplayObject child, int index){};
//    void swapChildren(DisplayObject child1, DisplayObject child2){};
    void            swapChildrenAt(int index1, int index2){};
    
    std::vector<Object *> mChildren;
  protected:
  private:
    
    
};
    
}
