#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/Camera.h"
#include "cinder/params/Params.h"
#include "cinder/Rand.h"
#include "cinder/Utilities.h"
#include "cinder/Log.h"
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
using namespace std;

#include "VisibleApp.h"

using namespace csv;

namespace
{
    
    std::ostream& ci_console ()
    {
        return App::get()->console();
    }
    
    double				ci_getElapsedSeconds()
    {
        return App::get()->getElapsedSeconds();
    }
    
    uint32_t ci_getNumWindows ()
    {
        return App::get()->getNumWindows();
    }
}


void prepareSettings( App::Settings *settings )
{
    settings->setHighDensityDisplayEnabled();
    settings->setWindowSize( 848, 564 );
    settings->setFrameRate( 60 );
    settings->setResizable( false );
    
    //    settings->setWindowSize( 640, 480 );
    //    settings->setFullScreen( false );
    //    settings->setResizable( true );
}

class VisibleApp : public App, public SingletonLight<VisibleApp>
{
public:
    
    
    void prepareSettings( Settings *settings );
    void setup();
    void create_qmovie_viewer ();
    void create_movie_dir_viewer ();
    void create_clip_viewer ();
    void create_lif_viewer ();
    
    void mouseDown( MouseEvent event );
    void mouseMove( MouseEvent event );
    void mouseUp( MouseEvent event );
    void mouseDrag( MouseEvent event );
    void keyDown( KeyEvent event );
    
    //void fileDrop( FileDropEvent event );
    void update();
    void draw();
    void close_main();
    void resize();
    void windowMove();
    void windowClose();
    void windowMouseDown( MouseEvent &mouseEvt );
    void displayChange();
    
    bool shouldQuit();
    WindowRef createConnectedWindow (Window::Format& format);
   
    
    params::InterfaceGlRef         mTopParams;
    
    Rectf                       mGraphDisplayRect;
    Rectf                       mMovieDisplayRect;
    Rectf                       mMenuDisplayRect;
    Rectf                       mLogDisplayRect;
    CameraPersp				mCam;
    

    bool mImageSequenceDataLoaded;
    bool mImageDataLoaded;
    bool mShowCenterOfMotionSignal;
    bool mShowMultiSnapShot;
    bool mShowMultiSnapShotAndData;
    bool mPaused;
    

    mutable std::list <std::shared_ptr<uContext> > mContexts;
    
};

WindowRef  VisibleCentral::getConnectedWindow (Window::Format& format )
{
    return VisibleApp::instance().createConnectedWindow(format);
}

std::list <std::shared_ptr<uContext> >& VisibleCentral::contexts ()
{
    return VisibleApp::instance().mContexts;
}

WindowRef VisibleApp::createConnectedWindow(Window::Format& format)
{
    WindowRef win = createWindow( format );
    win->getSignalClose().connect( std::bind( &VisibleApp::windowClose, this ) );
    win->getSignalMouseDown().connect( std::bind( &VisibleApp::windowMouseDown, this, std::placeholders::_1 ) );
    //   win->getSignalDraw().connect( std::bind( &VisibleApp::windowDraw, this ) );
    return win;

}


bool VisibleApp::shouldQuit()
{
    return true;
}

//
// We allow one movie and multiple clips or matrix view.
// And movie of a directory
//
void VisibleApp::create_qmovie_viewer ()
{
    Window::Format format( RendererGl::create() );
    WindowRef ww = createConnectedWindow(format);
    mContexts.push_back(std::shared_ptr<movContext>( new movContext(ww) ) );
}


//
// We allow one movie and multiple clips or matrix view.
//
//
void VisibleApp::create_lif_viewer ()
{
    Window::Format format( RendererGl::create() );
    WindowRef ww = createConnectedWindow(format);
    mContexts.push_back(std::shared_ptr<lifContext>(new lifContext(ww)));
}


//
// We allow one movie and multiple clips or matrix view.
//
//
void VisibleApp::create_movie_dir_viewer ()
{
    Window::Format format( RendererGl::create() );
    WindowRef ww = createConnectedWindow(format);
    mContexts.push_back(std::shared_ptr<movDirContext>(new movDirContext(ww)));
}

//
// We allow one movie and multiple clips or matrix view.
//
//
void VisibleApp::create_clip_viewer ()
{
    Window::Format format( RendererGl::create() );
    WindowRef ww = createConnectedWindow(format);
    mContexts.push_back(std::shared_ptr<clipContext>(new clipContext(ww)));
}


void VisibleApp::setup()
{
    WindowRef ww = getWindow ();
    
    mPaused = mShowMultiSnapShotAndData = mShowMultiSnapShot = mShowCenterOfMotionSignal = false;
    mImageDataLoaded = mImageSequenceDataLoaded = false;
    
    ci::ThreadSetup threadSetup; // instantiate this if you're talking to Cinder from a secondary thread
    // Setup our default camera, looking down the z-axis
    mCam.lookAt( vec3( -20, 0, 0 ), vec3(0.0f,0.0f,0.0f) );
  
    
     // Setup the parameters
    mTopParams = params::InterfaceGl::create( "Visible", ivec2( 250, 300 ) );

    mTopParams->addSeparator();
    mTopParams->addButton( "Import Movie", std::bind( &VisibleApp::create_qmovie_viewer, this) );
  //  mTopParams->addSeparator();
  // 	mTopParams->addButton( "Import SS Matrix", std::bind( &VisibleApp::create_matrix_viewer, this ) );
   
    mTopParams->addSeparator();
   	mTopParams->addButton( "Import LIF  ", std::bind( &VisibleApp::create_lif_viewer, this ) );
    
    mTopParams->addSeparator();
   	mTopParams->addButton( "Import Image Directory ", std::bind( &VisibleApp::create_movie_dir_viewer, this ) );
    
    mTopParams->addSeparator();

    
    mTopParams->addSeparator();
    mTopParams->addButton( "Import A CSV File", std::bind( &VisibleApp::create_clip_viewer, this ) );
    mTopParams->addSeparator();
    
    getSignalShouldQuit().connect( std::bind( &VisibleApp::shouldQuit, this ) );
    
    getWindow()->getSignalMove().connect( std::bind( &VisibleApp::windowMove, this ) );
    getWindow()->getSignalDisplayChange().connect( std::bind( &VisibleApp::displayChange, this ) );
    getWindow()->getSignalDraw().connect(std::bind( &VisibleApp::draw, this) );
    getWindow()->getSignalClose().connect(std::bind( &VisibleApp::windowClose, this) );
    
    getSignalDidBecomeActive().connect( [] { CI_LOG_V( "App became active." ); } );
    getSignalWillResignActive().connect( [] { CI_LOG_V( "App will resign active." ); } );
    
}

void VisibleApp::windowMouseDown( MouseEvent &mouseEvt )
{
    CI_LOG_V( "Mouse down in window" );
}

void VisibleApp::windowMove()
{
    CI_LOG_V( "window pos: " << getWindow()->getPos() );
}

void VisibleApp::displayChange()
{
    CI_LOG_V( "window display changed: " << getWindow()->getDisplay()->getBounds() );
}


void VisibleApp::windowClose()
{
    WindowRef win = getWindow();
    CI_LOG_V( "Closing " << getWindow() );
}

void VisibleApp::mouseMove( MouseEvent event )
{
    uContext  *data = getWindow()->getUserData<uContext>();
    if(data)
        data->mouseMove(event);
    else
        cinder::app::App::mouseMove(event);

}


void VisibleApp::mouseDrag( MouseEvent event )
{
    uContext  *data = getWindow()->getUserData<uContext>();
    if(data)
        data->mouseDrag(event);
    else
        cinder::app::App::mouseDrag(event);
}


void VisibleApp::mouseDown( MouseEvent event )
{
    uContext  *data = getWindow()->getUserData<uContext>();
    if(data)
        data->mouseDown(event);
    else
        cinder::app::App::mouseDown(event);
}


void VisibleApp::mouseUp( MouseEvent event )
{
    uContext  *data = getWindow()->getUserData<uContext>();
    if(data)
        data->mouseUp(event);
    else
        cinder::app::App::mouseUp(event);
}


void VisibleApp::keyDown( KeyEvent event )
{
    uContext  *data = getWindow()->getUserData<uContext>();
    if(data)
        data->keyDown(event);
    else
    {
        if( event.getChar() == 'f' ) {
            // Toggle full screen when the user presses the 'f' key.
            setFullScreen( ! isFullScreen() );
        }
        else if( event.getCode() == KeyEvent::KEY_ESCAPE ) {
            // Exit full screen, or quit the application, when the user presses the ESC key.
            if( isFullScreen() )
                setFullScreen( false );
            else
                quit();
        }
    }
    
}


void VisibleApp::update()
{
   uContext  *data = getWindow()->getUserData<uContext>();

  if (data && data->is_valid()) data->update ();
    
}

void VisibleApp::draw ()
{
    gl::clear( Color( 0.3f, 0.3f, 0.3f ) );
    
    uContext  *data = getWindow()->getUserData<uContext>();
    
    bool valid_data = data != nullptr && data->is_valid ();
    
    if (valid_data) data->draw();
    else
        mTopParams->draw ();
    
}



void VisibleApp::resize ()
{
    uContext  *data = getWindow()->getUserData<uContext>();
    
    if (data && data->is_valid()) data->resize ();
    
}


// This line tells Cinder to actually create the application
CINDER_APP( VisibleApp, RendererGl (RendererGl ::Options().msaa( 4 ) ), prepareSettings )
