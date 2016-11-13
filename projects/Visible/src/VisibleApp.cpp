
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

WindowRef VisibleApp::getNewWindow (ivec2 size)
{
    return createWindow( Window::Format().size(size));
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

   if (data && data->is_valid()) data->update ();
    
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
