#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/Camera.h"
#include "cinder/params/Params.h"
#include "cinder/Rand.h"
#include "cinder/Utilities.h"
#include "cinder/Log.h"
#include "guiContext.h"
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
    void create_qmovie_viewer (const boost::filesystem::path& pp = boost::filesystem::path ());
    void create_movie_dir_viewer (const boost::filesystem::path& pp = boost::filesystem::path ());
    void create_clip_viewer (const boost::filesystem::path& pp = boost::filesystem::path ());
    void create_lif_viewer (const boost::filesystem::path& pp = boost::filesystem::path ());
    
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
    
	void fileDrop( FileDropEvent event ) override;
    
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
    
    gl::TextureRef		mTextTexture;
    vec2				mSize;
    Font				mFont;
    std::string			mLog;
    
    mutable std::list <std::shared_ptr<guiContext> > mContexts;
    
};

WindowRef  VisibleCentral::getConnectedWindow (Window::Format& format )
{
    return VisibleApp::instance().createConnectedWindow(format);
}

std::list <std::shared_ptr<guiContext> >& VisibleCentral::contexts ()
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
void VisibleApp::create_qmovie_viewer (const boost::filesystem::path& pp)
{
    Window::Format format( RendererGl::create() );
    WindowRef ww = createConnectedWindow(format);
    mContexts.push_back(std::shared_ptr<movContext>( new movContext(ww, pp) ) );
}


//
// We allow one movie and multiple clips or matrix view.
//
//
void VisibleApp::create_lif_viewer (const boost::filesystem::path& pp)
{
    Window::Format format( RendererGl::create() );
    WindowRef ww = createConnectedWindow(format);
    mContexts.push_back(std::shared_ptr<lifContext>(new lifContext(ww, pp)));
}


//
// We allow one movie and multiple clips or matrix view.
//
//
void VisibleApp::create_movie_dir_viewer (const boost::filesystem::path& pp)
{
    Window::Format format( RendererGl::create() );
    WindowRef ww = createConnectedWindow(format);
    mContexts.push_back(std::shared_ptr<movDirContext>(new movDirContext(ww, pp)));
}

//
// We allow one movie and multiple clips or matrix view.
//
//
void VisibleApp::create_clip_viewer (const boost::filesystem::path& pp)
{
    Window::Format format( RendererGl::create() );
    WindowRef ww = createConnectedWindow(format);
    mContexts.push_back(std::shared_ptr<clipContext>(new clipContext(ww, pp)));
}

void VisibleApp::fileDrop( FileDropEvent event )
{
    auto file = event.getFile(0);

    const fs::path& imageFile = file;
    
    if (! exists(file) ) return;
    
    if (is_directory(file))
    {
        create_movie_dir_viewer(file);
        return;
    }
    
    if (file.has_extension())
    {
        std::string ext = file.extension().string ();
        switch (ext)
    case: ".lif":
        create_lif_viewer(file);
        return;
        
    case: ".mov":
        create_qmovie_viewer(file);
        return;
    }
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
    
    getSignalDidBecomeActive().connect( [] { update_log ( "App became active." ); } );
    getSignalWillResignActive().connect( [] { update_log ( "App will resign active." ); } );
    
}


void VisibleApp::update_log (const std::string& msg)
{
    if (msg.length() > 2)
        mLog = msg;
    TextBox tbox = TextBox().alignment( TextBox::RIGHT).font( mFont ).size( mSize ).text( mLog );
    tbox.setColor( Color( 1.0f, 0.65f, 0.35f ) );
    tbox.setBackgroundColor( ColorA( 0.3f, 0.3f, 0.3f, 0.4f )  );
    //    ivec2 sz = tbox.measure();
    mTextTexture = gl::Texture2d::create( tbox.render() );
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
    guiContext  *data = getWindow()->getUserData<guiContext>();
    if(data)
        data->mouseMove(event);
    else
        cinder::app::App::mouseMove(event);

}


void VisibleApp::mouseDrag( MouseEvent event )
{
    guiContext  *data = getWindow()->getUserData<guiContext>();
    if(data)
        data->mouseDrag(event);
    else
        cinder::app::App::mouseDrag(event);
}


void VisibleApp::mouseDown( MouseEvent event )
{
    guiContext  *data = getWindow()->getUserData<guiContext>();
    if(data)
        data->mouseDown(event);
    else
        cinder::app::App::mouseDown(event);
}


void VisibleApp::mouseUp( MouseEvent event )
{
    guiContext  *data = getWindow()->getUserData<guiContext>();
    if(data)
        data->mouseUp(event);
    else
        cinder::app::App::mouseUp(event);
}


void VisibleApp::keyDown( KeyEvent event )
{
    guiContext  *data = getWindow()->getUserData<guiContext>();
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
   guiContext  *data = getWindow()->getUserData<guiContext>();

  if (data && data->is_valid()) data->update ();
    
}

void VisibleApp::draw ()
{
    gl::clear( Color( 0.3f, 0.3f, 0.3f ) );
    
    guiContext  *data = getWindow()->getUserData<guiContext>();
    
    bool valid_data = data != nullptr && data->is_valid ();
    
    if (valid_data) data->draw();
    else
        mTopParams->draw ();
    
    
    if (mTextTexture)
    {
        Rectf textrect (0.0, getWindowHeight() - mTextTexture->getHeight(), getWindowWidth(), getWindowHeight());
        gl::draw(mTextTexture, textrect);
    }
    
    
}



void VisibleApp::resize ()
{
    guiContext  *data = getWindow()->getUserData<guiContext>();
    
    if (data && data->is_valid()) data->resize ();
    
}


// This line tells Cinder to actually create the application
CINDER_APP( VisibleApp, RendererGl (RendererGl ::Options().msaa( 4 ) ), prepareSettings )
