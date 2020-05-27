


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

namespace {
const std::vector<std::string>& supported_mov_extensions = { ".lif", ".mov", ".mp4", ".ts", ".avi"};
}
using namespace ci;
using namespace ci::app;
using namespace std;


void VisibleRunApp::QuitApp(){
    quit();
}

#if 0
static void ShowHelpMarker(const char* desc)
{
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(450.0f);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}
#endif

static ImGuiDockNodeFlags opt_flags = ImGuiDockNodeFlags_None;


float VisibleRunApp::DrawMainMenu() {
  auto menu_height = 0.0f;

  if (ImGui::BeginMainMenuBar()) {
    // todo: Call the derived class to draw the application specific menus inside the
    // menu bar
//    DrawInsideMainMenu();

    // Last menu bar items are the ones we create as baseline (i.e. the docking
    // control menu and the debug menu)

    if (ImGui::BeginMenu("Docking")) {
      // Disabling fullscreen would allow the window to be moved to the front of
      // other windows, which we can't undo at the moment without finer window
      // depth/z control.
      // ImGui::MenuItem("Fullscreen", NULL, &opt_fullscreen_persistant);

      if (ImGui::MenuItem("Flag: NoSplit", "",
                          (opt_flags & ImGuiDockNodeFlags_NoSplit) != 0))
        opt_flags ^= ImGuiDockNodeFlags_NoSplit;
      if (ImGui::MenuItem(
              "Flag: NoDockingInCentralNode", "",
              (opt_flags & ImGuiDockNodeFlags_NoDockingInCentralNode) != 0))
        opt_flags ^= ImGuiDockNodeFlags_NoDockingInCentralNode;
      if (ImGui::MenuItem(
              "Flag: PassthruDockspace", "",
              (opt_flags & ImGuiDockNodeFlags_PassthruDockspace) != 0))
        opt_flags ^= ImGuiDockNodeFlags_PassthruDockspace;

      ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Debug")) {
      if (ImGui::MenuItem("Show Logs", "CTRL+SHIFT+L", &show_logs_)) {
   //     DrawLogView();
      }
      if (ImGui::MenuItem("Show Docks Debug", "CTRL+SHIFT+D",
                          &show_docks_debug_)) {
        DrawDocksDebug();
      }
      if (ImGui::MenuItem("Show Settings", "CTRL+SHIFT+S", &show_settings_)) {
 //       DrawSettings();
      }

      ImGui::Separator();

      if (ImGui::MenuItem("Show ImGui Metrics", "CTRL+SHIFT+M",
                          &show_imgui_metrics_)) {
        DrawImGuiMetrics();
      }
      if (ImGui::MenuItem("Show ImGui Demos", "CTRL+SHIFT+G",
                          &show_imgui_demos_)) {
        DrawImGuiDemos();
      }

      ImGui::EndMenu();
    }
    menu_height = ImGui::GetWindowSize().y;

    ImGui::EndMainMenuBar();
  }

  return menu_height;
}


void VisibleRunApp::DrawDocksDebug() {
  if (ImGui::Begin("Docks", &show_docks_debug_)) {
    ImGui::LabelText(
        "TODO",
        "Get docking information from ImGui and populate this once the ImGui "
        "programmatic access to docking is published as a stable API");
    // TODO: generate docking information
  }
  ImGui::End();
}

void VisibleRunApp::DrawImGuiMetrics() { ImGui::ShowMetricsWindow(); }

void VisibleRunApp::DrawImGuiDemos() {
  ImGui::ShowDemoWindow(&show_imgui_demos_);
}


void VisibleRunApp::DrawStatusBar(float width, float height, float pos_x,
                                    float pos_y) {
  // Draw status bar (no docking)
  ImGui::SetNextWindowSize(ImVec2(width, height), ImGuiSetCond_Always);
  ImGui::SetNextWindowPos(ImVec2(pos_x, pos_y), ImGuiSetCond_Always);
  ImGui::Begin("statusbar", nullptr,
               ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings |
                   ImGuiWindowFlags_NoBringToFrontOnFocus |
                   ImGuiWindowFlags_NoResize);

  // Call the derived class to add stuff to the status bar
 // DrawInsideStatusBar(width - 45.0f, height);

 // ImGui::SameLine(ui::GetWindowWidth() - 60); ui::Text("%4.1f FPS", getAverageFps());
  // Draw the common stuff
  ImGui::SameLine(width - 45.0f);
// Font = Font( "Menlo", 18 ); //font(Font::FAMILY_PROPORTIONAL);
//  font.Normal().Regular().SmallSize();
//  ImGui::PushFont(font.ImGuiFont());
  ImGui::Text("FPS: %ld", std::lround(ImGui::GetIO().Framerate));
  ImGui::End();
}

void VisibleRunApp::DrawGUI(){
     ImGuiViewport *viewport = ImGui::GetMainViewport();

     if (ImGui::GetIO().DisplaySize.y > 0) {
       auto menu_height = DrawMainMenu();

       bool opt_fullscreen = isFullScreen();
       // We are using the ImGuiWindowFlags_NoDocking flag to make the parent
       // window not dockable into, because it would be confusing to have two
       // docking targets within each others.
       ImGuiWindowFlags window_flags =
           ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
       if (opt_fullscreen) {
         auto dockSpaceSize = viewport->Size;
         dockSpaceSize.y -= 16.0f;  // remove the status bar
         ImGui::SetNextWindowPos(viewport->Pos);
         ImGui::SetNextWindowSize(dockSpaceSize);
         ImGui::SetNextWindowViewport(viewport->ID);
         ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
         ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
         window_flags |= ImGuiWindowFlags_NoTitleBar |
                         ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
                         ImGuiWindowFlags_NoMove;
         window_flags |=
             ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
       }

       // When using ImGuiDockNodeFlags_PassthruDockspace, DockSpace() will render
       // our background and handle the pass-thru hole, so we ask Begin() to not
       // render a background.
       if (opt_flags & ImGuiDockNodeFlags_PassthruDockspace)
         window_flags |= ImGuiWindowFlags_NoBackground;

       ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
       ImGui::Begin("DockSpace Demo", nullptr, window_flags);
       ImGui::PopStyleVar();

       if (opt_fullscreen) ImGui::PopStyleVar(2);

       // Dockspace
       ImGuiIO &io = ImGui::GetIO();
       if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable) {
         ImGuiID dockspace_id = ImGui::GetID("MyDockspace");
         ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), opt_flags);
       } else {
         // TODO: emit a log message
       }

       //
       // Status bar
       //
       DrawStatusBar(viewport->Size.x, 16.0f, 0.0f, viewport->Size.y - menu_height);

   //    if (show_logs_) DrawLogView();
   //    if (show_settings_) DrawSettings();
   //    if (show_docks_debug_) DrawDocksDebug();
       if (show_imgui_metrics_) DrawImGuiMetrics();
       if (show_imgui_demos_) DrawImGuiDemos();
     }

     ImGui::End();
    
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
    float dpiScale = 1.f;
    auto& style = ImGui::GetStyle();
    style.WindowBorderSize = 1.f * dpiScale;
    style.FrameBorderSize = 1.f * dpiScale;
    style.FrameRounding = 5.f * dpiScale;
    style.ScrollbarSize *= dpiScale;
    style.Colors[ImGuiCol_ScrollbarBg] = ImVec4( 1, 1, 1, 0.03f );
    style.Colors[ImGuiCol_WindowBg] = ImVec4( 0.129f, 0.137f, 0.11f, 1.f );
    
    style.AntiAliasedFill = false;
    style.AntiAliasedLines = false;
    setWindowPos(getWindowSize()/4);
    
    ImGuiIO &io = ImGui::GetIO();
    //    static auto imguiSettingsPath =
    //        asap::fs::GetPathFor(asap::fs::Location::F_IMGUI_SETTINGS).string();
    //    io.IniFilename = imguiSettingsPath.c_str();
    
    //
    // Various flags controlling ImGui IO behavior
    //
    
    // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    // Enable Gamepad Controls
    // io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    // Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    // Enable Multi-Viewport
    // io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    // NOTE: Platform Windows
    // io.ConfigFlags |= ImGuiConfigFlags_ViewportsNoTaskBarIcons;
    // io.ConfigFlags |= ImGuiConfigFlags_ViewportsNoMerge;
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
    
    bool exists_with_extenstion = exists(bpath) && fs::path(bpath).has_extension();
    if (! exists_with_extenstion){
        ADD_ERR_AND_RETURN(cmds, " Path not valid or missing extension ");
    }
    // @todo enumerate args and implement in JSON
    // Custom Content for LIF files is only IDLab
    std::string extension = fs::path(bpath).extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
    bool is_valid_extension = std::find( supported_mov_extensions.begin(), supported_mov_extensions.end(), extension) != supported_mov_extensions.end();
    //    bool is_video_content = extension == ".mov" || extension == ".mp4";
    bool is_lif_content = extension == ".lif";
    
    
    if(exists_with_extenstion && is_valid_extension && is_lif_content){
        
        bool just_list_chapters = m_args.size() == 3 && is_lif_content && m_args[2] == "list";
        bool selected_by_dialog_no_custom_content_no_chapter = m_args.size() == 2;
        bool chapter_ok =  m_args.size() >= 3;
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
        VisibleAppControl::make_result_cache_directory_for_lif (bpath_path, mBrowser);
        auto cache_path = VisibleAppControl::get_visible_cache_directory();
        auto stem = bpath_path.stem();
        cache_path = cache_path / stem;
        
        auto chapter = mBrowser->names()[0];
        if (selected_by_dialog_no_custom_content_no_chapter){ // Selected by File Dialog
            m_args.push_back(chapter);
        }
        std::cout << mBrowser->name_to_index_map() << std::endl;
        
        auto indexItr = mBrowser->name_to_index_map().find(m_args[2]);
        if (indexItr != mBrowser->name_to_index_map().end()){
            auto serie = mBrowser->get_serie_by_index(indexItr->second);
            WindowRef ww = getWindow ();
            mContext = std::make_shared<lifContext>(ww,serie,cache_path);
            
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
#if 0
    else if (exists_with_extenstion && is_valid_extension && is_video_content){
        
        
        VisibleAppControl::setup_loggers(root_output_dir, visual_log, fs::path(bpath).filename().string());
        
        cvVideoPlayer::ref vref = cvVideoPlayer::create(fs::path(bpath));
        WindowRef ww = getWindow ();
        
        auto bpath_path = fs::path(bpath);
        auto cache_path = VisibleAppControl::make_result_cache_entry_for_content_file(bpath_path);
        mContext = std::unique_ptr<sequencedImageContext>(new movContext (ww, vref, cache_path));
        
        if (mContext->is_valid()){ cmds += " [ " + m_args[1] + " ] ";cmds += "  Ok ";}
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
        ADD_ERR_AND_RETURN(cmds, " Path not valid ");
    }
#endif
    
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


