#ifndef _Visible_App_
#define _Visible_App_

#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/Camera.h"
#include "cinder/params/Params.h"
#include "cinder/Rand.h"
#include "ui_contexts.h"
#include "boost/filesystem.hpp"
#include <functional>
#include <list>
#include "core/stl_utils.hpp"
#include "app_utils.hpp"
#include "core/core.hpp"
#include <memory>

using namespace ci;
using namespace ci::app;

typedef app_utils::WindowMgr<WindowRef, uContextRef> VisWinMgr;

struct VisibleCentral : SingletonLight<VisibleCentral>
{
    VisibleCentral () {}

    WindowRef getConnectedWindow (Window::Format& format);

};

#endif

