

#include <cstdio>
#include <glut/glut.h>
#include "tinyGLManager.h"

using namespace tinyGL;

const int kDefaultWidth = 400;
const int kDefaultHeight = 300;


static void DisplayFunc()
{
    int id = glutGetWindow();
    DisplayManager & dm = DisplayManager::get_mutable_instance();
    if (dm.displays.find(id) != dm.displays.end())
    {
        dm.displays[id]->display();
    }
}

static void ReshapeFunc(int width, int height)
{
    int id = glutGetWindow();
    DisplayManager & dm = DisplayManager::get_mutable_instance();
    if (dm.displays.find(id) != dm.displays.end())
    {
        dm.displays[id]->reshape(width, height);
    }
}

static void KeyboardFunc(unsigned char key, int x, int y)
{
    int id = glutGetWindow();
    DisplayManager & dm = DisplayManager::get_mutable_instance();
    if (dm.displays.find(id) != dm.displays.end())
    {
        dm.displays[id]->keyboard(key, x, y);
    }
}

static void SpecialFunc(int key, int x, int y)
{
    int id = glutGetWindow();
    DisplayManager & dm = DisplayManager::get_mutable_instance();
    if (dm.displays.find(id) != dm.displays.end())
    {
        dm.displays[id]->special(key, x, y);
    }
}

static void MouseFunc(int button, int state, int x, int y)
{
    int id = glutGetWindow();


    DisplayManager & dm = DisplayManager::get_mutable_instance();
    if (dm.displays.find(id) != dm.displays.end())
    {
        dm.displays[id]->mouse(button, state, x, y);
    }
}

static void MotionFunc(int x, int y)
{
    int id = glutGetWindow();
    DisplayManager & dm = DisplayManager::get_mutable_instance();
    if (dm.displays.find(id) != dm.displays.end())
    {
        dm.displays[id]->motion(x, y);
    }
}

static void PassiveMotionFunc(int x, int y)
{
    int id = glutGetWindow();
    DisplayManager & dm = DisplayManager::get_mutable_instance();
    if (dm.displays.find(id) != dm.displays.end())
    {
        dm.displays[id]->passiveMotion(x, y);
    }
}

static void CloseFunc()
{
    int id = glutGetWindow();
    DisplayManager & dm = DisplayManager::get_mutable_instance();
    if (dm.displays.find(id) != dm.displays.end())
    {
        Display * d = dm.displays[id];
        delete d;
        dm.displays.erase(id);
    }
}

static void TimerFunc(int value)
{
    /* if (Commands::Instance()->quit)
    {
        Commands::Instance()->spaceKeyPressed.notify_all();
        if (_isatty(_fileno(stdin)))
        {
            HANDLE console = GetStdHandle(STD_INPUT_HANDLE);
            CloseHandle(console);
        }
        glutLeaveMainLoop();
    }*/
    DisplayManager & dm = DisplayManager::get_mutable_instance();
    dm.startNewDisplays();
    dm.checkNeedsRedisplay();
    glutTimerFunc(value, TimerFunc, value);
}

void DisplayManager::run()
{
    glutInit( &m_argc, m_argv);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
//    glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_CONTINUE_EXECUTION);

    // GLUT requires at least one window to be created before allowing calls below
    Display * d = new (std::nothrow) Display(0, 0, kDefaultWidth, kDefaultHeight, "tinyGL");
    if (d != NULL)
    {
        startDisplay(d);
        glutHideWindow();

        glutTimerFunc(1, TimerFunc, 1);
        
        try {
             glutMainLoop();
        } catch (Display::normal_return& e) {
            return;
        }
       
    }
}

void DisplayManager::addNewDisplay(Display * display)
{
    mNewDisplaysMutex.lock();
    mNewDisplays.push_back(display);
    mNewDisplaysMutex.unlock();
}

void DisplayManager::startNewDisplays()
{
    mNewDisplaysMutex.lock();
    for (auto it = mNewDisplays.begin(); it != mNewDisplays.end(); ++it)
    {
        startDisplay(*it);
    }
    mNewDisplays.clear();
    mNewDisplaysMutex.unlock();
}

void DisplayManager::checkNeedsRedisplay()
{
    for (auto it = displays.begin(); it != displays.end(); ++it)
    {
        if (it->second->needsRedisplay)
        {
            glutPostWindowRedisplay(it->second->id);
            it->second->needsRedisplay = false;
        }
    }
}

void DisplayManager::startDisplay(Display * display)
{
    if (! display->isNull())
    {
        glutInitWindowSize(display->width(), display->height());
    }
    else
    {
        glutInitWindowSize(kDefaultWidth, kDefaultHeight);
    }
    glutInitWindowPosition(display->ul().x(), display->ul().y());
    int id = glutCreateWindow(display->title.c_str());
    glutDisplayFunc(DisplayFunc);
    glutReshapeFunc(ReshapeFunc);
    glutKeyboardFunc(KeyboardFunc);
    glutSpecialFunc(SpecialFunc);
    glutMouseFunc(MouseFunc);
    glutMotionFunc(MotionFunc);
    glutPassiveMotionFunc(PassiveMotionFunc);
    glutWMCloseFunc(CloseFunc);
    display->init();
    display->id = id;
    displays[id] = display;
}


DisplayManager::DisplayManager()
{
}
