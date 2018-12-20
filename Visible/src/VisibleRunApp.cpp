


#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdelete-non-virtual-dtor"
#pragma GCC diagnostic ignored "-Wunused-private-field"
#pragma GCC diagnostic ignored "-Wcomma"

#include "core/core.hpp"
#include "core/file_system.hpp"
#include "Plist.hpp"
#include "otherIO/lifFile.hpp"
#include "algo_Lif.hpp"
#include "LifContext.h"

#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/Context.h"
#include "cinder/gl/gl.h"
#include "cinder/ImageIo.h"
//#include "cinder/Utilities.h"
//#include "cinder/app/Platform.h"
//#include "cinder/Url.h"
//#include "cinder/System.h"
#include <iostream>
#include <fstream>
#include <string>
#include <mutex>

#include "cinder/Log.h"
#include "cinder/CinderAssert.h"

#include <map>
#include "CinderImGui.h"
#include "gui_handler.hpp"
#include "gui_base.hpp"
#include "app_logger.hpp"
#include "logger.hpp"
#include "VisibleApp.h"

//#include "console.h"

#define APP_WIDTH 1024
#define APP_HEIGHT 768

#pragma GCC diagnostic pop


namespace {


bool check_input (const string &filename){
 
    auto check_file_and_size = svl::io::check_file(filename);
    if(! check_file_and_size.first) return check_file_and_size.first;
    
    boost::filesystem::path bpath(filename);
    auto boost_file_and_size = svl::io::existsFile(bpath);
    if(! boost_file_and_size) return boost_file_and_size;

    auto bsize = boost::filesystem::file_size(bpath);
    return check_file_and_size.second == bsize;
}
imGuiLog visualLog;
void setup_loggers (){
   using imgui_sink_mt = spdlog::sinks::imGuiLogSink<std::mutex> ;
    
    // Setup APP LOG
    auto logging_container = logging::get_mutable_logging_container();
    logging_container->add_sink(std::make_shared<logging::sinks::platform_sink_mt>());
    logging_container->add_sink(std::make_shared<logging::sinks::daily_file_sink_mt>("Log", 23, 59));
    logging_container->add_sink(std::make_shared<imgui_sink_mt>(visualLog));
   // logging_container->add_sink(std::make_shared<spdlog::sinks::stdout_sink_mt>(vlogger::instance().console()));
    auto combined_logger = std::make_shared<spdlog::logger>("Log", logging_container);
    //register it if you need to access it globally
    spdlog::register_logger(combined_logger);
}

}

namespace VisibleRunAppControl{
    /**
     When this is set to false the threads managed by this program will stop and join the main thread.
     */
    bool ThreadsShouldStop = false;
    
    /**
     Logger which will show output on the Log window in the application.
     */
    imGuiLog app_log;
    
}

using namespace ci;
using namespace ci::app;
using namespace std;


void VisibleRunApp::QuitApp(){
    ImGui::DestroyContext();
    // fg::ThreadsShouldStop = true;
    quit();
}



void VisibleRunApp::SetupGUIVariables(){}

void VisibleRunApp::DrawGUI(){
    // Draw the menu bar
    {
        ui::ScopedMainMenuBar menuBar;
        
        if( ui::BeginMenu( "File" ) ){
            //if(ui::MenuItem("Fullscreen")){
            //    setFullScreen(!isFullScreen());
            //}
            if(ui::MenuItem("Looping", "S")){
                mContext->loop_no_loop_button();
            }
            ui::MenuItem("Help", nullptr, &showHelp);
            if(ui::MenuItem("Quit", "ESC")){
                QuitApp();
            }
            ui::EndMenu();
        }
        
        if( ui::BeginMenu( "View" ) ){
            ui::MenuItem( "General Settings", nullptr, &showGUI );
            //    ui::MenuItem( "Edge Detection", nullptr, &edgeDetector.showGUI );
            //    ui::MenuItem( "Face Detection", nullptr, &faceDetector.showGUI );
            ui::MenuItem( "Log", nullptr, &showLog );
            //ui::MenuItem( "PS3 Eye Settings", nullptr, &showWindowWithMenu );
            ui::EndMenu();
        }
        
        //ui::SameLine(ui::GetWindowWidth() - 60); ui::Text("%4.0f FPS", ui::GetIO().Framerate);
        ui::SameLine(ui::GetWindowWidth() - 60); ui::Text("%4.1f FPS", getAverageFps());
    }
    
    //Draw general settings window
    if(showGUI)
    {
        ui::Begin("General Settings", &showGUI, ImGuiWindowFlags_AlwaysAutoResize);
        
        ui::SliderInt("Cover Percent", &convergence, 0, 100);
        
        ui::End();
    }
    
    //Draw the log if desired
    if(showLog){
        visualLog.Draw("Log", &showLog);
    }
    
    if(showHelp) ui::OpenPopup("Help");
    //ui::ScopedWindow window( "Help", ImGuiWindowFlags_AlwaysAutoResize );
    std::string buildN =  boost::any_cast<const string&>(mPlist.find("CFBundleVersion")->second);
    buildN = "Visible ( build: " + buildN + " ) ";
    if(ui::BeginPopupModal("Help", &showHelp)){
        ui::TextColored(ImVec4(0.92f, 0.18f, 0.29f, 1.00f), "%s", buildN.c_str());
        ui::Text("Arman Garakani, Darisa LLC");
        if(ui::Button("Copy")) ui::LogToClipboard();
        ui::SameLine();
        //  ui::Text("github.com/");
        ui::LogFinish();
        ui::Text("");
        ui::Text("Mouse over any"); ShowHelpMarker("We did it!"); ui::SameLine(); ui::Text("to show help.");
        ui::Text("Ctrl+Click any slider to set its value manually.");
        ui::EndPopup();
    }
    
}

bool VisibleRunApp::shouldQuit()
{
    return true;
}



void VisibleRunApp::setup()
{
    ui::initialize(ui::Options()
                   .itemSpacing(vec2(6, 6)) //Spacing between widgets/lines
                   .itemInnerSpacing(vec2(10, 4)) //Spacing between elements of a composed widget
                   .color(ImGuiCol_Button, ImVec4(0.86f, 0.93f, 0.89f, 0.39f)) //Darken the close button
                   .color(ImGuiCol_Border, ImVec4(0.86f, 0.93f, 0.89f, 0.39f))
                   //  .color(ImGuiCol_TooltipBg, ImVec4(0.27f, 0.57f, 0.63f, 0.95f))
                   );
    
  
    setup_loggers();

    
    
    const fs::path& appPath = ci::app::getAppPath();
    const fs::path plist = appPath / "VisibleRun.app/Contents/Info.plist";
    if (exists (appPath)){
        std::ifstream stream(plist.c_str(), std::ios::binary);
        Plist::readPlist(stream, mPlist);
    }
    // Get the LIF file and Serie from the command line arguments
    // args[0] = Run App Path
    // args[1] = LIF file full path
    // args[3] = Serie name
    m_args = getCommandLineArgs();

    if(m_args.size() == 1){
        auto some_path = getOpenFilePath(); //"", extensions);
        if (! some_path.empty() || exists(some_path)){
            m_args.push_back(some_path.string());
        }
        else{
            std::string msg = some_path.string() + " is not a valid path to a file ";
            vlogger::instance().console()->info(msg);
        }
    }
        
        
    for( auto display : Display::getDisplays() )
    {
        mGlobalBounds.include(display->getBounds());
    }
    
    setWindowPos(getWindowSize()/3);
    WindowRef ww = getWindow ();
    std::string bpath = m_args[1];
    auto cmds = m_args[1];

    
    if(! exists(bpath)){
        cmds += " Does Not Exist ";
        APPLOG_INFO(cmds.c_str());
    }
    else{
        mBrowser = lif_browser::create(bpath);
        mBrowser->get_series_info();
        
        auto indexItr = mBrowser->name_to_index_map().find(m_args[2]);
        if (indexItr != mBrowser->name_to_index_map().end()){
            auto serie = mBrowser->get_serie_by_index(indexItr->second);
            
            mContext = std::unique_ptr<lifContext>(new lifContext (ww,serie));
    
            if (mContext->is_valid()){
                cmds += " [ " + m_args[2] + " ] ";
                cmds += "  Ok ";
            }
            APPLOG_INFO(cmds.c_str());
            mContext->resize();
            mContext->seekToStart();
            mContext->play();
        }
       
    }
 
    
 
    std::string buildN =  boost::any_cast<const string&>(mPlist.find("CFBundleVersion")->second);
    ww->setTitle ( cmds + " Visible build: " + buildN);
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

static int n = 0;
static float minRadius = 1;
void VisibleRunApp::draw ()
{
    gl::clear( Color::gray( 0.5f ) );
    if (mContext && mContext->is_valid()) mContext->draw ();
    DrawGUI();
    ui::SliderInt( "Circles", &n, 0, 500 );
    ui::SliderFloat( "Min Radius", &minRadius, 1, 499 );
}



void VisibleRunApp::resize ()
{
    mSize = vec2( getWindowWidth(), getWindowHeight() / 12);
    if (mContext && mContext->is_valid()) mContext->resize ();
    
}


void prepareSettings( App::Settings *settings )
{
    const auto &args = settings->getCommandLineArgs();
    if(args.size() > 1){
        auto ok = check_input(args[1]);
        std::cout << "File is " << std::boolalpha << ok << std::endl;
    }
    
    settings->setHighDensityDisplayEnabled();
    settings->setWindowSize(lifContext::startup_display_size().x, lifContext::startup_display_size().y);
    settings->setFrameRate( 60 );
    settings->setResizable( true );
    //    settings->setWindowSize( 640, 480 );
    //    settings->setFullScreen( false );
    //    settings->setResizable( true );
}




// settings fn from top of file:
 CINDER_APP( VisibleRunApp, RendererGl, prepareSettings )


//int main( int argc, char* argv[] )
//{
//    cinder::app::RendererRef renderer( new RendererGl );
//    cinder::app::AppMac::main<VisibleRunApp>( renderer, "VisibleRunApp", argc, argv, nullptr); // prepareSettings);
//    return 0;
//}


