#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/Camera.h"
#include "cinder/params/Params.h"
#include "cinder/Rand.h"
#include "cinder/Utilities.h"
#include "cinder/Log.h"
#include "cinder/Display.h"
#include "guiContext.h"
#include "LifContext.h"
#include "boost/filesystem.hpp"
#include "boost/any.hpp"
#include <functional>
#include <list>
#include "core/stl_utils.hpp"
#include "app_utils.hpp"
#include "core/core.hpp"
#include "Plist.hpp"
#include <memory>
#include <functional>

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
    void setup()override;
    void create_qmovie_viewer (const boost::filesystem::path& pp = boost::filesystem::path ());
    void create_movie_dir_viewer (const boost::filesystem::path& pp = boost::filesystem::path ());
    void create_result_viewer (const boost::filesystem::path& pp = boost::filesystem::path ());
    void create_clip_viewer (const boost::filesystem::path& pp = boost::filesystem::path ());
    void create_lif_viewer (const boost::filesystem::path& pp = boost::filesystem::path ());
    
    void mouseDown( MouseEvent event )override;
    void mouseMove( MouseEvent event )override;
    void mouseUp( MouseEvent event )override;
    void mouseDrag( MouseEvent event )override;
    void keyDown( KeyEvent event )override;
    
    //void fileDrop( FileDropEvent event );
    void update()override;
    void draw()override;
    void close_main();
    void resize()override;
    void windowMove();
    void windowClose();
    void windowMouseDown( MouseEvent &mouseEvt );
    void displayChange();
    void update_log (const std::string& msg);
    
    void fileDrop( FileDropEvent event ) override;
    
    bool shouldQuit();
    WindowRef createConnectedWindow (Window::Format& format);
    
    params::InterfaceGlRef         mTopParams;
    
    
    gl::TextureRef		mTextTexture;
    vec2				mSize;
    vec2                mDisplayTL;
    Font				mFont;
    std::string			mLog;
    
    Rectf						mGlobalBounds;
    map<string, boost::any> mPlist;
    
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
//    win->getSignalDisplayChange().connect( std::bind( &VisibleApp::update, this ) );
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
// And movie of a directory
//
void VisibleApp::create_result_viewer (const boost::filesystem::path& pp)
{
    Window::Format format( RendererGl::create() );
    WindowRef ww = createConnectedWindow(format);
    mContexts.push_back(std::shared_ptr<imageDirContext>( new imageDirContext(ww, pp) ) );
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
    auto imageFile = event.getFile(0);
    
    const fs::path& file = imageFile;
    
    if (! exists(file) ) return;
    
    if (is_directory(file))
    {
        create_movie_dir_viewer(file);
        return;
    }
    
    if (file.has_extension())
    {
        std::string ext = file.extension().string ();
        if (ext.compare(".lif") == 0)
            create_lif_viewer(file);
        else if (ext.compare(".mov"))
            create_qmovie_viewer(file);
        return;
    }
}

void VisibleApp::setup()
{
    const fs::path& appPath = ci::app::getAppPath();
    const fs::path plist = appPath / "Visible.app/Contents/Info.plist";
    std::ifstream stream(plist.c_str(), std::ios::binary);
    Plist::readPlist(stream, mPlist);
    
    
    for( auto display : Display::getDisplays() )
    {
        mGlobalBounds.include(display->getBounds());
        CI_LOG_V( "display name: '" << display->getName() << "', bounds: " << display->getBounds() );
    }
    
    ci::Area windowArea = getDisplay()->getBounds();
    mDisplayTL = windowArea.getUL();
    setWindowPos(getWindowSize()/3);
    
    WindowRef ww = getWindow ();
    std::string buildN =  boost::any_cast<const string&>(mPlist.find("CFBundleVersion")->second);
    ww->setTitle ("Visible build: " + buildN);
    mFont = Font( "Menlo", 18 );
    mSize = vec2( getWindowWidth(), getWindowHeight() / 12);
    
    ci::ThreadSetup threadSetup; // instantiate this if you're talking to Cinder from a secondary thread
    
    // Setup the parameters
    mTopParams = params::InterfaceGl::create( "Visible", ivec2( getWindowWidth()/2, getWindowHeight()/2 ) );
    
    mTopParams->addSeparator();
    mTopParams->addButton( "Import Movie", std::bind( &VisibleApp::create_qmovie_viewer, this, boost::filesystem::path () ) );
    //  mTopParams->addSeparator();
    // 	mTopParams->addButton( "Import SS Matrix", std::bind( &VisibleApp::create_matrix_viewer, this ) );
    
    mTopParams->addSeparator();
   	mTopParams->addButton( "Import LIF  ", std::bind( &VisibleApp::create_lif_viewer, this, boost::filesystem::path () ) );
    mTopParams->addSeparator();
    mTopParams->addButton( "Import Movie", std::bind( &VisibleApp::create_qmovie_viewer, this, boost::filesystem::path () ) );
    
#if 0
    mTopParams->addSeparator();
   	mTopParams->addButton( "Import Image Directory ", std::bind( &VisibleApp::create_movie_dir_viewer, this, boost::filesystem::path () ) );
    
    mTopParams->addSeparator();
    
    mTopParams->addSeparator();
   	mTopParams->addButton( "View Sorted Result ", std::bind( &VisibleApp::create_result_viewer, this, boost::filesystem::path () ) );
    
    mTopParams->addSeparator();
    
    
    mTopParams->addSeparator();
    mTopParams->addButton( "Import A CSV File", std::bind( &VisibleApp::create_clip_viewer, this, boost::filesystem::path () ) );
    mTopParams->addSeparator();

#endif
    getSignalShouldQuit().connect( std::bind( &VisibleApp::shouldQuit, this ) );
    
    getWindow()->getSignalMove().connect( std::bind( &VisibleApp::windowMove, this ) );
    getWindow()->getSignalDisplayChange().connect( std::bind( &VisibleApp::displayChange, this ) );
    getWindow()->getSignalDraw().connect(std::bind( &VisibleApp::draw, this) );
    getWindow()->getSignalClose().connect(std::bind( &VisibleApp::windowClose, this) );
    getWindow()->getSignalResize().connect(std::bind( &VisibleApp::resize, this) );
    
    getSignalDidBecomeActive().connect( [this] { update_log ( "App became active." ); } );
    getSignalWillResignActive().connect( [this] { update_log ( "App will resign active." ); } );
    
    getWindow()->getSignalDisplayChange().connect( std::bind( &VisibleApp::displayChange, this ) );
    
    gl::enableVerticalSync();
    
    auto cistrs = getCommandLineArgs();
    for (auto li : cistrs)
        std::cout << li << std::endl;
    
}


void VisibleApp::update_log (const std::string& msg)
{
    if (msg.length() > 2)
        std::cout << msg.c_str ();
}

void VisibleApp::windowMouseDown( MouseEvent &mouseEvt )
{
    update_log ( "Mouse down in window" );
}

void VisibleApp::windowMove()
{
    update_log("window pos: " + toString(getWindow()->getPos()));
}

void VisibleApp::displayChange()
{
    ci::Area windowArea = getDisplay()->getBounds();
    mDisplayTL = windowArea.getUL();
    
    update_log ( "window display changed: " + toString(getWindow()->getDisplay()->getBounds()));
    console() << "ContentScale = " << getWindowContentScale() << endl;
    console() << "getWindowCenter() = " << getWindowCenter() << endl;
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
    gl::clear( Color::gray( 0.5f ) );
    
    guiContext  *data = getWindow()->getUserData<guiContext>();
    
    bool valid_data = data != nullptr && data->is_valid ();
    
    if (valid_data) data->draw();
    else
        mTopParams->draw ();

   
 
#if 0
    gl::enableAlphaBlending();
    
    Rectf scaledBounds = mGlobalBounds;
    if( mGlobalBounds.getAspectRatio() > getWindowAspectRatio() )
        scaledBounds.scaleCentered( vec2( 1, mGlobalBounds.getAspectRatio() / getWindowAspectRatio() ) );
    else
        scaledBounds.scaleCentered( vec2(getWindowAspectRatio() / mGlobalBounds.getAspectRatio(), 1 ) );
//    scaledBounds.scaleCentered( 1.1f );
//    gl::clear( Color( 0, 0, 0 ) );
    gl::setMatrices( CameraOrtho( scaledBounds.getLowerLeft().x, scaledBounds.getLowerRight().x, scaledBounds.getLowerLeft().y, scaledBounds.getUpperLeft().y, -1, 1 ) );
    
    gl::translate(mDisplayTL);
#endif
    
    

    
    
}



void VisibleApp::resize ()
{
    mSize = vec2( getWindowWidth(), getWindowHeight() / 12);
    guiContext  *data = getWindow()->getUserData<guiContext>();
    
    if (data && data->is_valid()) data->resize ();
    
}


// This line tells Cinder to actually create the application
CINDER_APP( VisibleApp, RendererGl, prepareSettings )
