#pragma once

#include <string>
#include "vision/rectangle.h"

namespace tinyGL
{
    class Display : public iRect
{
public:
    
      Display();
    Display(int posX, int posY, int width, int height, const char * title);
    Display(const iRect display_box, const char * title);

    virtual ~Display();
    Display(const Display & other) = delete;
    Display(Display && other) = delete;
    Display & operator=(const Display & other) = delete;
    Display & operator=(Display && other) = delete;

    virtual void init();
    virtual void display();
    virtual void reshape(int width, int height);
    virtual void keyboard(unsigned char key, int x, int y);
    virtual void special(int key, int x, int y);
    virtual void mouse(int button, int state, int x, int y);
    virtual void motion(int x, int y);
    virtual void passiveMotion(int x, int y);

    std::string title;
    int id;

    bool needsRedisplay;
    
    void norm (fPair& _t)
    {
        _t += _t;
        _t[0] /= width();
        _t[1] /= height();
        _t -= fPair (1.0f, 1.0f);
    }
    
 
  
protected:
    const float kBackgroundGray;
};
}