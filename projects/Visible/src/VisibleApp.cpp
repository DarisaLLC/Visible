
#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/Camera.h"
#include "cinder/params/Params.h"
#include "cinder/Rand.h"
#include "ui_contexts.h"
#include "boost/filesystem.hpp"

using namespace ci;
using namespace ci::app;
using namespace std;








class VisibleApp : public App
{
public:
    
    void prepareSettings( Settings *settings );
    void setup();
    void create_matrix_viewer ();
    void create_clip_viewer ();
    void create_qmovie_viewer ();
    void create_imageFolder_viewer ();
    
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
    
    std::vector<std::shared_ptr<uContext> > mContexts;
    bool mImageSequenceDataLoaded;
    bool mImageDataLoaded;
    bool mShowCenterOfMotionSignal;
    bool mShowMultiSnapShot;
    bool mShowMultiSnapShotAndData;
    bool mPaused;
    
    static boost::filesystem::path browseToFolder ()
    {
        return Platform::get()->getFolderPath (Platform::get()->getHomeDirectory());
    }
    
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
    settings->setWindowSize( 640, 480 );
    settings->setFullScreen( false );
    settings->setResizable( true );
 }



void VisibleApp::resize ()
{
}

void VisibleApp::create_matrix_viewer ()
{
    mContexts.push_back(std::shared_ptr<uContext>(new matContext(createWindow( Window::Format().size(mMovieDisplayRect.getSize())))));
}
void VisibleApp::create_clip_viewer ()
{
    mContexts.push_back(std::shared_ptr<uContext>(new clipContext(createWindow( Window::Format().size(mGraphDisplayRect.getSize())))));
}
void VisibleApp::create_qmovie_viewer ()
{
    mContexts.push_back(std::shared_ptr<uContext>(new movContext(createWindow( Window::Format().size(mMovieDisplayRect.getSize())))));
}

void VisibleApp::create_imageFolder_viewer ()
{
    mContexts.push_back(std::shared_ptr<uContext>(new idirContext(createWindow( Window::Format().size(mMovieDisplayRect.getSize())))));
}

void VisibleApp::setup()
{
    mPaused = mShowMultiSnapShotAndData = mShowMultiSnapShot = mShowCenterOfMotionSignal = false;
    mImageDataLoaded = mImageSequenceDataLoaded = false;
    
    ci::ThreadSetup threadSetup; // instantiate this if you're talking to Cinder from a secondary thread
    // Setup our default camera, looking down the z-axis
    mCam.lookAt( vec3( -20, 0, 0 ), vec3(0.0f,0.0f,0.0f) );
    const ColorA &color = ColorA( 0.3f, 0.3f, 0.3f, 0.4f );
  
    
     // Setup the parameters
    mTopParams = params::InterfaceGl::create( "Visible Options", ivec2( 250, 300 ) );
//	mTopParams = params::InterfaceGl::create( getWindow(), "Select", toPixels( vec2( 200, 400)), color );

    mTopParams->addSeparator();
    mTopParams->addButton( "Import Image Folder", std::bind( &VisibleApp::create_imageFolder_viewer, this ) );
    mTopParams->addSeparator();
	mTopParams->addButton( "Import Quicktime Movie", std::bind( &VisibleApp::create_qmovie_viewer, this ) );
  //  mTopParams->addSeparator();
  // 	mTopParams->addButton( "Import SS Matrix", std::bind( &VisibleApp::create_matrix_viewer, this ) );
    mTopParams->addSeparator();
   	mTopParams->addButton( "Import Result ", std::bind( &VisibleApp::create_clip_viewer, this ) );

    
    mTopParams->addSeparator();
    
    mTopParams->addParam( "Image Sequence Loaded", &mImageSequenceDataLoaded );
    mTopParams->addParam( "Image Data Loaded", &mImageDataLoaded );
    mTopParams->addParam( "Show Motion Center", &mShowCenterOfMotionSignal);
    mTopParams->addParam( "Show Multi Snap Shot ", &mShowMultiSnapShot);
    mTopParams->addParam( "Show Multi Snap Shot and Data ", &mShowMultiSnapShotAndData);
    
    mTopParams->addParam( "Pause ", &mPaused );

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
    
    if (data) data->draw();
    else
        mTopParams->draw ();
    
}


// This line tells Cinder to actually create the application
CINDER_APP( VisibleApp, RendererGl )
