#include "tinyGL.h"
#include <glut/glut.h>
#include "vision/rectangle.h"

using namespace tinyGL;

Display::Display()
    : title("")
    , id(0)
    , needsRedisplay(false)
    , kBackgroundGray((float)0x35 / (float)0xFF)
{
}


Display::Display(int posX, int posY, int width, int height, const char * title)
    : iRect(posX, posY, width, height)
    , title(title)
    , id(0)
    , needsRedisplay(false)
    , kBackgroundGray((float)0x35 / (float)0xFF)
{
}

Display::Display(const iRect display_box, const char * title)
: iRect (display_box)
, title(title)
, id(0)
, needsRedisplay(false)
, kBackgroundGray((float)0x35 / (float)0xFF)
{
}

Display::~Display()
{
}

void Display::init()
{
}

void Display::display()
{
    glutSwapBuffers();
}

void Display::reshape(int new_width, int new_height)
{
    this->width (new_width);
    this->height (new_height);
    glutPostWindowRedisplay(id);
}

void Display::keyboard(unsigned char key, int x, int y)
{
#if 0
    switch (key)
    {
    case 'q':
        Commands::Instance()->quit = true;
        break;
    case 's':
        Commands::Instance()->continueLoop = false;
        printf("Skipped out of loop\n");
        fflush(stdout);
        break;
    case ' ':
        Commands::Instance()->spaceKeyPressed.notify_all();
        break;
    }
#endif
    glutPostWindowRedisplay(id);
}

void Display::special(int key, int x, int y)
{
    glutPostWindowRedisplay(id);
}

void Display::mouse(int button, int state, int x, int y)
{
    glutPostWindowRedisplay(id);
}

void Display::motion(int x, int y)
{
}

void Display::passiveMotion(int x, int y)
{
}



