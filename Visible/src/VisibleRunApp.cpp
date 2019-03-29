


#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdelete-non-virtual-dtor"
#pragma GCC diagnostic ignored "-Wunused-private-field"
#pragma GCC diagnostic ignored "-Wcomma"


#include "VisibleApp.h"


#define APP_WIDTH 1024
#define APP_HEIGHT 768

#pragma GCC diagnostic pop

// /Users/arman/Library/Application Support


bool VisibleAppControl::ThreadsShouldStop = false;


using namespace ci;
using namespace ci::app;
using namespace std;


void VisibleRunApp::QuitApp(){
    ImGui::DestroyContext();
    // fg::ThreadsShouldStop = true;
    quit();
}



void VisibleRunApp::SetupGUIVariables(){
    //Set up global preferences for the GUI
    //http://anttweakbar.sourceforge.net/doc/tools:anttweakbar:twbarparamsyntax
    GetGUI().DefineGlobal("iconalign=horizontal");
}

void VisibleRunApp::DrawGUI(){
    // Draw the menu bar
    {
        ui::ScopedMainMenuBar menuBar;
        
        if( ui::BeginMenu( "File" ) ){
            
            // Not Working Yet
            //if(ui::MenuItem("Fullscreen")){
            //    setFullScreen(!isFullScreen());
           // }

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
        
        ui::SameLine(ui::GetWindowWidth() - 60); ui::Text("%4.1f FPS", getAverageFps());
    }
    
  
    
    //Draw general settings window
    if(showGUI)
    {
//        ui::Begin("General Settings", &showGUI, ImGuiWindowFlags_AlwaysAutoResize);
//        ui::End();
    }
    
    //Draw the log if desired
    if(showLog){
        visual_log.Draw("Log", &showLog);
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

void VisibleRunApp::setup_ui(){
    ui::initialize(ui::Options()
                   .itemSpacing(vec2(6, 6)) //Spacing between widgets/lines
                   .itemInnerSpacing(vec2(10, 4)) //Spacing between elements of a composed widget
                   .color(ImGuiCol_Button, ImVec4(0.86f, 0.93f, 0.89f, 0.39f)) //Darken the close button
                   .color(ImGuiCol_Border, ImVec4(0.86f, 0.93f, 0.89f, 0.39f))
                   //  .color(ImGuiCol_TooltipBg, ImVec4(0.27f, 0.57f, 0.63f, 0.95f))
                   );
    ImGuiStyle* st = &ImGui::GetStyle();
    ImGui::StyleColorsLightGreen(st);
    
    setWindowPos(getWindowSize()/3);
}
/*
 * Browse the LIF file and dispatch VisibleRun with the selected chapter
 * Setsp Up Loggers
 * Loads PLIST ?? Maybe not necessary
 * @todo: use JSON
 // Get the LIF file and Serie from the command line arguments
 // args[0] = Run App Path
 // args[1] = content file full path
 // args[2] = Serie name
 // args[3] = content_type if not assume default from the content file
 */


#define ADD_ERR_AND_RETURN(sofar,addition)\
sofar += addition;\
VAPPLOG_INFO(sofar.c_str());

void VisibleRunApp::setup()
{
    const fs::path root_output_dir = vac::get_runner_app_support_directory();
    const fs::path& appPath = ci::app::getAppPath();
    const fs::path plist = appPath / "VisibleRun.app/Contents/Info.plist";
    if (exists (appPath)){
        std::ifstream stream(plist.c_str(), std::ios::binary);
        Plist::readPlist(stream, mPlist);
    }
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
    
  
    std::string bpath = m_args[1];
    auto cmds = m_args[1];
  
    
    static std::string cok = "chapter_ok";
    static std::string ccok = "chapter_ok_custom_content_ok";
    static std::string cokcnk = "chapter_ok_custom_content_not_recognized";
    static std::string used_dialog = "selected_by_dialog_using first_chapter";
    static std::string list_chapters = "listing chapters";
    
    std::string buildN =  boost::any_cast<const string&>(mPlist.find("CFBundleVersion")->second);
    cmds = cmds + " Visible build: " + buildN;
    
    // @todo enumerate args and implement in JSON
    // Custom Content for LIF files is only IDLab 
    if(exists(bpath)){
        
        bool just_list_chapters = m_args.size() == 3 && m_args[2] == "list";
        bool selected_by_dialog_no_custom_content_no_chapter = m_args.size() == 2;
        bool chapter_ok =  m_args.size() == 3;
        bool custom_id_exists = m_args.size() == 4;
        bool known_custom_type =  custom_id_exists && lifIO::isKnownCustomContent(m_args[3]);
        bool chapter_ok_unknown_custom_content = custom_id_exists && ! known_custom_type;
        std::string info;
        mContentType  = "";
        
        if (just_list_chapters)
              VisibleAppControl::setup_text_loggers(root_output_dir, fs::path(bpath).filename().string());
        else
            VisibleAppControl::setup_loggers(root_output_dir, visual_log, fs::path(bpath).filename().string());
        
        if (just_list_chapters){
            mBrowser =  lif_browser::create(bpath);
            info = list_chapters;
        }
        else if (selected_by_dialog_no_custom_content_no_chapter){
            mBrowser =  lif_browser::create(bpath);
            info = selected_by_dialog_no_custom_content_no_chapter;
        }
        else if ( known_custom_type) {
            mContentType = m_args[3];
            mBrowser = lif_browser::create(bpath, mContentType);
            info = ccok;
        }
        else if (chapter_ok_unknown_custom_content){
            mBrowser =  lif_browser::create(bpath);
            info = cokcnk + "  " + m_args[3];
        }
        else if (chapter_ok){
            mBrowser =  lif_browser::create(bpath);
            info = cok;
        }else{
            assert(0);
        }
        
        VAPPLOG_INFO(info.c_str());
        
        // browser set is allows other threads to work during its setup
        bool browser_is_ready = mBrowser && ! mBrowser->names().empty();
        
        
        if(! browser_is_ready){
            ADD_ERR_AND_RETURN(cmds, " No Chapters or Series "){
                quit();
            }
        }

        
        if(just_list_chapters){
           std::strstream msg;
            for (auto & se : mBrowser->get_all_series  ()){
                msg << se << std::endl;
                std::cout << se << std::endl;
            }
            std::string tmp = msg.str();
            VAPPLOG_INFO(tmp.c_str());
            quit();
        }

        auto bpath_path = fs::path(bpath);
        VisibleAppControl::make_result_cache_if_needed (mBrowser, bpath_path);
        auto cache_path = VisibleAppControl::get_visible_cache_directory();
        auto stem = bpath_path.stem();
        cache_path = cache_path / stem;
        
        auto chapter = mBrowser->names()[0];
        if (selected_by_dialog_no_custom_content_no_chapter){ // Selected by File Dialog
            m_args.push_back(chapter);
        }
        auto indexItr = mBrowser->name_to_index_map().find(m_args[2]);
        if (indexItr != mBrowser->name_to_index_map().end()){
            auto serie = mBrowser->get_serie_by_index(indexItr->second);
            WindowRef ww = getWindow ();
            mContext = std::unique_ptr<lifContext>(new lifContext (ww,serie, cache_path));
            
            if (mContext->is_valid()){
                cmds += " [ " + m_args[2] + " ] ";
                cmds += "  Ok ";
            }
            setup_ui();
            
            VAPPLOG_INFO(cmds.c_str());
            update();

            ww->setTitle ( cmds + " Visible build: " + buildN);
            mFont = Font( "Menlo", 18 );
            mSize = vec2( getWindowWidth(), getWindowHeight() / 12);
            // ci::ThreadSetup threadSetup; // instantiate this if you're talking to Cinder from a secondary thread
            getSignalShouldQuit().connect( std::bind( &VisibleRunApp::shouldQuit, this ) );
            getSignalDidBecomeActive().connect( [this] { update_log ( "App became active." ); } );
            getSignalWillResignActive().connect( [this] { update_log ( "App will resign active." ); } );
            getWindow()->getSignalDisplayChange().connect( std::bind( &VisibleRunApp::displayChange, this ) );
            gl::enableVerticalSync();

        }else{
            ADD_ERR_AND_RETURN(cmds, " No Chapters or Series ")
        }
    }
    ADD_ERR_AND_RETURN(cmds, " Path not valid ");
}
    


void VisibleRunApp::update_log (const std::string& msg)
{
    if (msg.length() > 2)
        VAPPLOG_INFO(msg.c_str());
}


void VisibleRunApp::mouseDown( MouseEvent event )
{
    if(mContext)
        mContext->mouseDown(event);
    else
        cinder::app::App::mouseDown(event);
}

void VisibleRunApp::displayChange()
{
    update_log ( "window display changed: " + to_string(getWindow()->getDisplay()->getBounds()));
    update_log ( "ContentScale = " + to_string(getWindowContentScale()));
                update_log ( "getWindowCenter = " + to_string(getWindowCenter()));
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

//static int n = 0;
//static float minRadius = 1;
void VisibleRunApp::draw ()
{
    gl::clear( Color::gray( 0.67f ) );
    if (mContext && mContext->is_valid()){
        mContext->draw ();
        DrawGUI();
    }

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
        auto ok = VisibleAppControl::check_input(args[1]);
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


