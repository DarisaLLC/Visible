 
#include "opencv2/stitching.hpp"
#include "guiContext.h"
#include "core/stl_utils.hpp"

using namespace boost;


namespace
{
    
    std::ostream& ci_console ()
    {
        return AppBase::get()->console();
    }
    
    double				ci_getElapsedSeconds()
    {
        return AppBase::get()->getElapsedSeconds();
    }
    
}

guiContext::guiContext (ci::app::WindowRef& ww)
: mWindow (ww), m_valid (false), m_type (null_viewer)
{
    if (ww)
        ww->setUserData (this);
}

guiContextRef guiContext::getRef ()
{
    return shared_from_this();
}

guiContext::~guiContext ()
{
   // std::cout << " guiContext Dtor called " << std::endl;
}

// u implementation does nothing
void guiContext::mouseDown( MouseEvent event ) {}
void guiContext::mouseMove( MouseEvent event ) {}
void guiContext::mouseUp( MouseEvent event ) {}
void guiContext::mouseDrag( MouseEvent event ) {}
void guiContext::keyDown( KeyEvent event ) {}

void guiContext::normalize_point (vec2& pos, const ivec2& size)
{
    pos.x = pos.x / size.x;
    pos.y = pos.y / size.y;
}


const std::string & guiContext::getName() const { return mName; }
void guiContext::setName (const std::string& name) { mName = name; }
guiContext::Type guiContext::context_type () const { return m_type; }
bool guiContext::is_context_type (const Type t) const { return m_type == t; }
bool guiContext::is_valid () const { return m_valid; }


///////////////  Main Viewer ////////////////////

mainContext::mainContext(ci::app::WindowRef& ww, const boost::filesystem::path& dp)
: guiContext (ww)
{
    m_valid = false;
    m_type = Type::null_viewer;
    
}


