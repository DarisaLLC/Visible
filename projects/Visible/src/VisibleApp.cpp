
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

using namespace ci;
using namespace ci::app;
using namespace std;





static uContext sNullViewer;
std::shared_ptr<uContext> sNullViewerRef (&sNullViewer, null_deleter ());




class VisibleApp : public App
{
public:
    
  
    void prepareSettings( Settings *settings );
    void setup();
    void create_matrix_viewer ();
    void create_clip_viewer ();
    void create_qmovie_viewer ();
    void create_image_dir_viewer ();
    
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
    void window_close ();
    
      
    params::InterfaceGlRef         mTopParams;
    
    Rectf                       mGraphDisplayRect;
    Rectf                       mMovieDisplayRect;
    Rectf                       mMenuDisplayRect;
    Rectf                       mLogDisplayRect;
    CameraPersp				mCam;
    
    std::list <std::shared_ptr<uContext> > mContexts;
    bool mImageSequenceDataLoaded;
    bool mImageDataLoaded;
    bool mShowCenterOfMotionSignal;
    bool mShowMultiSnapShot;
    bool mShowMultiSnapShotAndData;
    bool mPaused;
    

    
};

// The window-specific data for each window
class WindowData {
public:
    WindowData()
    : mColor( Color( CM_HSV, randFloat(), 0.8f, 0.8f ) ) // a random color
    {}
    
    Color			mColor;
    list<vec2>		mPoints; // the points drawn into this window
};
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



void VisibleApp::window_close()
{
    console() << "Closing " << getWindow() << std::endl;
}



void VisibleApp::prepareSettings( Settings *settings )
{
    settings->setWindowSize( 848, 564 );
    settings->setFrameRate( 60 );
    settings->setResizable( false );
    
//    settings->setWindowSize( 640, 480 );
//    settings->setFullScreen( false );
//    settings->setResizable( true );
 }



void VisibleApp::resize ()
{
}


//
// We allow one movie and multiple clips or matrix view.
//
//
void VisibleApp::create_matrix_viewer ()
{
    mContexts.push_back(std::shared_ptr<uContext>(new matContext(createWindow( Window::Format().size(mMovieDisplayRect.getSize())))));
}

//
// We allow one movie and multiple clips or matrix view.
//
//
void VisibleApp::create_image_dir_viewer ()
{
    mContexts.push_back(std::shared_ptr<uContext>(new imageDirContext(createWindow( Window::Format().size(mMovieDisplayRect.getSize())))));
}


//////
// Remove existing viewers. Needs better design & implementation
//    auto new_end = std::remove_if(mContexts.begin(), mContexts.end(),
//                                  [](const std::shared_ptr<uContext>& cx)
//                                 { return cx->is_context_type(uContext::Type::qtime_viewer); });
//    mContexts.erase (new_end, mContexts.end());



// Create a clip viewer. Go through container of viewers, if there is a movie view, connect onMarked signal to it
void VisibleApp::create_clip_viewer ()
{
    std::shared_ptr<uContext> cw(std::shared_ptr<uContext>(new clipContext(createWindow( Window::Format().size(mGraphDisplayRect.getSize())))));
    
    if (! cw->is_valid()) return;
    
    for (std::shared_ptr<uContext> uip : mContexts)
    {
        if (uip->is_context_type(uContext::Type::qtime_viewer))
        {
            uip->signalMarker.connect(std::bind(&clipContext::onMarked, static_cast<clipContext*>(cw.get()), std::placeholders::_1));
            cw->signalMarker.connect(std::bind(&clipContext::onMarked, static_cast<clipContext*>(uip.get()), std::placeholders::_1));
        }
    }
    mContexts.push_back(cw);
}

// Create a movie viewer. Go through container of viewers, if there is a clip view, connect onMarked signal to it
void VisibleApp::create_qmovie_viewer ()
{
    std::shared_ptr<uContext> mw(new movContext(createWindow( Window::Format().size(mMovieDisplayRect.getSize()))));
    
    if (! mw->is_valid()) return;
    
    for (std::shared_ptr<uContext> uip : mContexts)
    {
        if (uip->is_context_type(uContext::Type::clip_viewer))
        {
            uip->signalMarker.connect(std::bind(&movContext::onMarked, static_cast<movContext*>(mw.get()), std::placeholders::_1));
            mw->signalMarker.connect(std::bind(&clipContext::onMarked, static_cast<clipContext*>(uip.get()), std::placeholders::_1));
        }
    }
    mContexts.push_back(mw);
}


void VisibleApp::setup()
{
    mPaused = mShowMultiSnapShotAndData = mShowMultiSnapShot = mShowCenterOfMotionSignal = false;
    mImageDataLoaded = mImageSequenceDataLoaded = false;
    
    ci::ThreadSetup threadSetup; // instantiate this if you're talking to Cinder from a secondary thread
    // Setup our default camera, looking down the z-axis
    mCam.lookAt( vec3( -20, 0, 0 ), vec3(0.0f,0.0f,0.0f) );
  
    
     // Setup the parameters
    mTopParams = params::InterfaceGl::create( "Visible", ivec2( 250, 300 ) );
//	mTopParams = params::InterfaceGl::create( getWindow(), "Select", toPixels( vec2( 200, 400)), color );

    mTopParams->addSeparator();
	mTopParams->addButton( "Import Movie", std::bind( &VisibleApp::create_qmovie_viewer, this ) );
  //  mTopParams->addSeparator();
  // 	mTopParams->addButton( "Import SS Matrix", std::bind( &VisibleApp::create_matrix_viewer, this ) );
    mTopParams->addSeparator();
   	mTopParams->addButton( "Import Time Series ", std::bind( &VisibleApp::create_clip_viewer, this ) );
    mTopParams->addSeparator();
    mTopParams->addSeparator();
   	mTopParams->addButton( "Import Image Directory ", std::bind( &VisibleApp::create_image_dir_viewer, this ) );
    
    mTopParams->addSeparator();
    
//    mTopParams->addParam( "Show Multi Snap Shot ", &mShowMultiSnapShot);
//    mTopParams->addParam( "Show Multi Snap Shot and Data ", &mShowMultiSnapShotAndData);
//    mTopParams->addParam( "Pause ", &mPaused );

    getWindow()->getSignalDraw().connect(std::bind( &VisibleApp::draw, this) );
    getWindow()->getSignalClose().connect(std::bind( &VisibleApp::window_close, this) );
    const vec2 c_ul (0.0,0.0);
    const vec2& c_lr = getWindowBounds().getSize();
    vec2 c_mr (c_lr.x, (c_lr.y - c_ul.y) / 2);
    vec2 c_ml (c_ul.x, (c_lr.y - c_ul.y) / 2);
    mGraphDisplayRect = Area (c_ul, c_mr);
    mMovieDisplayRect = Area (c_ml, c_lr);
    
    

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
   if (data) data->update ();
    
}

void VisibleApp::draw ()
{
    gl::clear( Color( 0.3f, 0.3f, 0.3f ) );
    
    uContext  *data = getWindow()->getUserData<uContext>();
    
    bool valid_data = data != nullptr && data->context_type() != uContext::null_viewer;
    
    if (valid_data) data->draw();
    else
        mTopParams->draw ();
    
}


// This line tells Cinder to actually create the application
CINDER_APP( VisibleApp, RendererGl (RendererGl ::Options().msaa( 4 ) ) )
