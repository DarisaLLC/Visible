#pragma once

#include "tinyGL.h"
#include <map>
#include <vector>
#include <condition_variable>
#include <mutex>
#include "core/singleton.hpp"

namespace tinyGL
{
class DisplayManager : public singleton<DisplayManager>
{
public:
    DisplayManager();

    void init(int argc, char ** argv)
    {
        m_argc = argc;
        m_argv = argv;
    }
    void run();

    void addNewDisplay(Display * display) ;
    void startNewDisplays();
    void checkNeedsRedisplay();

    std::map<int, Display *> displays;

    //do not define these two
    DisplayManager(const DisplayManager &) = delete;
    DisplayManager & operator=(const DisplayManager &) = delete;

private:
    void startDisplay(Display * display);
    int m_argc;
    char** m_argv;

    std::vector<Display *> mNewDisplays;
    mutable std::mutex mNewDisplaysMutex;
};
}