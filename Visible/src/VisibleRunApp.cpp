


#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdelete-non-virtual-dtor"
#pragma GCC diagnostic ignored "-Wunused-private-field"
#pragma GCC diagnostic ignored "-Wcomma"

#include "cinder_opencv.h"
#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/Context.h"
#include "cinder/gl/gl.h"
#include "cinder/ImageIo.h"
#include "cinder/Utilities.h"
#include "cinder/app/Platform.h"
#include "cinder/Url.h"
#include "cinder/System.h"
#include <iostream>
#include <fstream>
#include <string>
#include "otherIO/lifFile.hpp"
#include "algo_Lif.hpp"
#include "Item.h"
#include "logger.hpp"


#if defined (  HOCKEY_SUPPORT )
#include "hockey_etc_cocoa_wrappers.h"
#endif

#include "LifContext.h"
#include "core/core.hpp"
#include "Plist.hpp"
#include <map>
#include "CinderImGui.h"
#include "gui_handler.hpp"
#include "gui_base.hpp"
#include "logger.hpp"


//#include "console.h"

#define APP_WIDTH 1024
#define APP_HEIGHT 768




using namespace ci;
using namespace ci::app;
using namespace std;



class VisibleRunApp : public App, public gui_base
{
public:
    
 //   VisibleRunApp();
  //  ~VisibleRunApp();
    
    virtual void SetupGUIVariables() override {}
    virtual void DrawGUI() override {}
    virtual void QuitApp() {}
    
    void prepareSettings( Settings *settings );
    void setup()override;
    void mouseDown( MouseEvent event )override;
    void mouseMove( MouseEvent event )override;
    void mouseUp( MouseEvent event )override;
    void mouseDrag( MouseEvent event )override;
    void keyDown( KeyEvent event )override;
    
    void update()override;
    void draw()override;
    void resize()override;
    void windowMove();
    void windowClose();
    void windowMouseDown( MouseEvent &mouseEvt );
    void displayChange();
    void update_log (const std::string& msg);
    
    bool shouldQuit();
    
private:
    std::vector<std::string> m_args;
  //  params::InterfaceGlRef         mTopParams;
    vec2				mSize;
    Font				mFont;
    std::string			mLog;
    
    Rectf						mGlobalBounds;
    map<string, boost::any> mPlist;
    
    mutable std::unique_ptr<lifContext> mContext;
    mutable lif_browser::ref mBrowser;
    
  
};


bool VisibleRunApp::shouldQuit()
{
    return true;
}



//
// We allow one movie and multiple clips or matrix view.
//
//
//void VisibleRunApp::create_lif_viewer (const boost::filesystem::path& pp)
//{
//    Window::Format format( RendererGl::create() );
//    WindowRef ww = createConnectedWindow(format);
//    mContexts.push_back(std::unique_ptr<lifContext>(new lifContext(ww, pp)));
//}
//



void VisibleRunApp::setup()
{
    const fs::path& appPath = ci::app::getAppPath();
    const fs::path plist = appPath / "VisibleRun.app/Contents/Info.plist";
    std::ifstream stream(plist.c_str(), std::ios::binary);
    Plist::readPlist(stream, mPlist);
  
    // Get the LIF file and Serie from the command line arguments
    // args[0] = Run App Path
    // args[1] = LIF file full path
    // args[3] = Serie name
    m_args = getCommandLineArgs();
    if( ! m_args.empty() ) {
        std::cout <<  "command line args: " ;
        for( size_t i = 0; i < m_args.size(); i++ )
            std::cout << "\t[" << i << "] " << m_args[i] << endl;
    }
#if 0
 auto some_path = getOpenFilePath(); //"", extensions);
    if (! some_path.empty() || exists(some_path)){
        std::cout << m_args[1] << std::endl;
        std::cout << some_path.string() << std::endl;
        auto cmp = m_args[1].compare(some_path.string());
        std::cout << cmp << std::endl;
      //  m_args[1] = some_path.string();
    }
    else{
        std::string msg = some_path.string() + " is not a valid path to a file ";
        vlogger::instance().console()->info(msg);
    }
#endif
        
        
    for( auto display : Display::getDisplays() )
    {
        mGlobalBounds.include(display->getBounds());
    }
    
    setWindowPos(getWindowSize()/3);
    
  
    WindowRef ww = getWindow ();
  
    std::string cmds = m_args[1] + " [ " +  m_args[2] + " ] ";
    
    std::string bpath = m_args[1];
    auto parts = split(bpath, getPathSeparator(), true);
    for (auto part : parts)
        std::cout << part << std::endl;
    
    if(! exists(bpath))
        cmds += " Does Not Exist ";
    else{
        mBrowser = lif_browser::create(bpath);
        mContext = std::unique_ptr<lifContext>(new lifContext (ww,mBrowser, 0));
        if (mContext->is_valid()) cmds += "  Ok ";
    }
   // auto bItr = mBrowser->name_to_index_map().find(m_args[2]);
   // if (bItr != mBrowser->name_to_index_map().end())
   // {
   //     auto index = 0; //bItr->second;
   //     mContext = std::unique_ptr<lifContext>(new lifContext (ww,mBrowser, index));
   //     if (mContext->is_valid()) cmds += "  Ok ";
   // }
    
    
 
    std::string buildN =  boost::any_cast<const string&>(mPlist.find("CFBundleVersion")->second);
    ww->setTitle (cmds + " Visible build: " + buildN);
    mFont = Font( "Menlo", 18 );
    mSize = vec2( getWindowWidth(), getWindowHeight() / 12);
    
   // ci::ThreadSetup threadSetup; // instantiate this if you're talking to Cinder from a secondary thread
    
  
    getSignalShouldQuit().connect( std::bind( &VisibleRunApp::shouldQuit, this ) );
    
#if 1
    getWindow()->getSignalMove().connect( std::bind( &VisibleRunApp::windowMove, this ) );
    getWindow()->getSignalDisplayChange().connect( std::bind( &VisibleRunApp::displayChange, this ) );
    getWindow()->getSignalDraw().connect(std::bind( &VisibleRunApp::draw, this) );
    getWindow()->getSignalClose().connect(std::bind( &VisibleRunApp::windowClose, this) );
    getWindow()->getSignalResize().connect(std::bind( &VisibleRunApp::resize, this) );
#endif
    
    getSignalDidBecomeActive().connect( [this] { update_log ( "App became active." ); } );
    getSignalWillResignActive().connect( [this] { update_log ( "App will resign active." ); } );
    
    getWindow()->getSignalDisplayChange().connect( std::bind( &VisibleRunApp::displayChange, this ) );
    
    gl::enableVerticalSync();
    
  
    
}


void VisibleRunApp::update_log (const std::string& msg)
{
    if (msg.length() > 2)
        std::cout << msg.c_str ();
}

void VisibleRunApp::windowMouseDown( MouseEvent &mouseEvt )
{
  update_log ( "Mouse down in window" );
}

void VisibleRunApp::windowMove()
{
    update_log("window pos: " + ci::toString(getWindow()->getPos()));
}

void VisibleRunApp::displayChange()
{
    update_log ( "window display changed: " + to_string(getWindow()->getDisplay()->getBounds()));
    console() << "ContentScale = " << getWindowContentScale() << endl;
    console() << "getWindowCenter() = " << getWindowCenter() << endl;
}


void VisibleRunApp::windowClose()
{
}

void VisibleRunApp::mouseMove( MouseEvent event )
{
    if(mContext)
        mContext->mouseMove(event);
    else
        cinder::app::App::mouseMove(event);
    
}


void VisibleRunApp::mouseDrag( MouseEvent event )
{
    if(mContext)
        mContext->mouseDrag(event);
    else
        cinder::app::App::mouseDrag(event);
}


void VisibleRunApp::mouseDown( MouseEvent event )
{
    if(mContext)
        mContext->mouseDown(event);
    else
        cinder::app::App::mouseDown(event);
}


void VisibleRunApp::mouseUp( MouseEvent event )
{
    if(mContext)
        mContext->mouseUp(event);
    else
        cinder::app::App::mouseUp(event);
}


void VisibleRunApp::keyDown( KeyEvent event )
{
    if(mContext)
        mContext->keyDown(event);
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


void VisibleRunApp::update()
{
    if (mContext && mContext->is_valid()) mContext->update ();
    
}

void VisibleRunApp::draw ()
{
    gl::clear( Color::gray( 0.5f ) );
    if (mContext && mContext->is_valid()) mContext->draw ();
}



void VisibleRunApp::resize ()
{
    mSize = vec2( getWindowWidth(), getWindowHeight() / 12);
    if (mContext && mContext->is_valid()) mContext->resize ();
    
}


void prepareSettings( App::Settings *settings )
    {
  
        
        settings->setHighDensityDisplayEnabled();
        settings->setWindowSize(lifContext::startup_display_size().x, lifContext::startup_display_size().y);
        settings->setFrameRate( 60 );
        settings->setResizable( true );
        //    settings->setWindowSize( 640, 480 );
        //    settings->setFullScreen( false );
        //    settings->setResizable( true );
    }
                                                         

// This line tells Cinder to actually create the application
CINDER_APP( VisibleRunApp, RendererGl( RendererGl::Options().msaa( 4 ) ), []( App::Settings *settings ) {
    settings->setWindowSize( APP_WIDTH, APP_HEIGHT );
} )


#pragma GCC diagnostic pop
