


#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdelete-non-virtual-dtor"
#pragma GCC diagnostic ignored "-Wunused-private-field"
#pragma GCC diagnostic ignored "-Wcomma"


#include "VisibleApp.h"



#if __used__
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



bool VisibleAppControl::ThreadsShouldStop = false;

namespace {
const std::vector<std::string>& supported_mov_extensions = { ".lif", ".mov", ".mp4", ".ts", ".avi"};
}
using namespace ci;
using namespace ci::app;
using namespace std;
using namespace boost;
namespace bfs=boost::filesystem;


void VisibleApp::QuitApp(){
    quit();
}

void VisibleApp::DrawSettings() {
    if(! isValid()) return;
    ImGui::SetNextWindowPos(ImVec2(0, 36), ImGuiCond_Always);
    if (ImGui::Begin(mCurrentContentName.c_str(), &show_settings_)) {
        static bool first_time = true;
        if (first_time) {
            ImGui::SetNextTreeNodeOpen(true);
            first_time = false;
        }
        // @ todo: add layoput information about the LIF file

        if (isLifFile()) {
            m_selected_lif_serie_index = -1;
            m_custom_type = false;
            static int sSelected = -1;
            auto series = mBrowser->get_all_series();
            /*
              Format a descriptor line for selectable
               Draw the channel configuration:
                ---- ---- ----
                ---- ---- ----
                ---- ----
                ----
             */
            if (ImGui::TreeNode("Select Serie")) {
                for (auto i = 0; i < m_sections.size(); i++)
                {
                    if (ImGui::Selectable(m_sections[i].c_str(), sSelected == i))
                                        sSelected = i;
                    
                    lif_serie_data& serie =  series[i];
                    auto width = serie.dimensions()[0];
                    auto height = serie.dimensions()[1];
                    ImGui::Text(" %s %lu x %lu (%d) (%d)", serie.name().c_str(), width, height, serie.channelCount(), serie.timesteps());
                }
                ImGui::TreePop();
            }
            if (sSelected >= 0 && sSelected < m_sections.size()){
                m_selected_lif_serie_index = sSelected;
                m_selected_lif_serie_name = m_sections[m_selected_lif_serie_index];
                static int clicked = 0;
                ImGui::Text("%s", " Click to Load ");
                ImGui::SameLine();
                if (ImGui::Button(m_selected_lif_serie_name.c_str()))
                    clicked++;
                if (clicked & 1)
                {
                    load_lif_serie(m_selected_lif_serie_name);
                }
            } // End Selected check & run
        } // IsLifFile
        ImGui::Spacing();
    }// Settings
  
    ImGui::End();
}


void VisibleApp::ShowStyleEditor(ImGuiStyle* ref)
{
    // You can pass in a reference ImGuiStyle structure to compare to, revert to and save to (else it compares to an internally stored reference)
    ImGuiIO &io = ImGui::GetIO();
    ImGuiStyle& style = ImGui::GetStyle();
    static ImGuiStyle ref_saved_style;

    // Default to using internal storage as reference
    static bool init = true;
    if (init && ref == NULL)
        ref_saved_style = style;
    init = false;
    if (ref == NULL)
        ref = &ref_saved_style;

  //  ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.50f);
    ImGui::Text(" Choose Color Theme");
    ImGui::SameLine();
    if (ImGui::ShowStyleSelector("##Selector"))
        ref_saved_style = style;
    ImGui::Text(" Set Global Scale ");
    ImGui::SameLine();
    ImGui::DragFloat(" 0.0 -- 2.0 ", &io.FontGlobalScale, 0.005f, 0.3f, 2.0f, "%.2f");
 
}

void VisibleApp::DrawMainMenu(){
    
    ImGui::PushItemWidth(ImGui::GetFontSize() * -12);
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::Button("Open "))
            {
                nfdchar_t *outPath = NULL;
                nfdresult_t result = NFD_OpenDialog("lif,mov,mp4", NULL, &outPath);
                if (result == NFD_OKAY) {
                    bfs::path fout(outPath);
                    auto msg = "Selected " + fout.string();
                    vlogger::instance().console()->info(msg);
                    auto dotext = identify_file(fout, "");
                    /*
                     * Create the browser for the file type.
                     * We do it here as it is naturally blocked operation.
                     */
                    if(isLifFile()){
                        vlogger::instance().console()->info(fout.string() + " Ok " );
                        m_sections.clear();
                        auto num = list_lif_series(m_sections);
                        assert(num == m_sections.size());
                    }
                    else{
                        vlogger::instance().console()->error("%s not a lif file", fout.string());
                    }
                    free(outPath);
                }
                else {
                }
            }
            if (ImGui::Button("Process"))
            {
                if (mContext) std::dynamic_pointer_cast<lifContext>(mContext)->process_async();
            }
            
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("Views"))
        {
            ImGui::MenuItem( "Log", nullptr, &show_logs_);
            ImGui::MenuItem( "Help", nullptr, &show_help_);
            ImGui::MenuItem( "About", nullptr, &show_about_);
            ShowStyleEditor();
            ImGui::EndMenu();
        }
    }
    ImGui::EndMainMenuBar();
    //Draw the log if desired
    if(show_logs_){
        visual_log.Draw("Log", &show_logs_);
    }
    
    if(show_about_) //ImGui::OpenPopup("Help");
    {
        ImGui::ScopedWindow window( "About");
        std::string buildN =  boost::any_cast<const string&>(mPlist.find("CFBundleVersion")->second);
        buildN = "Visible ( build: " + buildN + " ) ";
//        if(ImGui::BeginPopupModal("Help", &show_help_)){
            ImGui::TextColored(ImVec4(0.92f, 0.18f, 0.29f, 1.00f), "%s", buildN.c_str());
            ImGui::Text("Arman Garakani, Darisa LLC");
            ImGui::Text("This App is Built With OpenCv, Boost, Cinder and ImGui Libraries ");
    }
    DrawSettings();
//    DrawImGuiDemos();
}
    



void VisibleApp::windowClose()
{
    WindowRef win = getWindow();
}





void VisibleApp::DrawImGuiMetrics() { ImGui::ShowMetricsWindow(); }

void VisibleApp::DrawImGuiDemos() {
    ImGui::ShowDemoWindow(&show_imgui_demos_);
}


void VisibleApp::DrawStatusBar(float width, float height, float pos_x,
                               float pos_y) {
    // Draw status bar (no docking)
    ImGui::SetNextWindowSize(ImVec2(width, height), ImGuiCond_Always);
    ImGui::SetNextWindowPos(ImVec2(pos_x, pos_y), ImGuiCond_Always);
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


bool VisibleApp::shouldQuit()
{
    return true;
}

void VisibleApp::setup_ui(){
    WindowRef ww = getWindow ();
    m_imgui_options = ImGui::Options();
    ImGui::Initialize(m_imgui_options
                      .window(ww)
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
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsLight();
    io.FontGlobalScale = 2.0;
    
    
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiDockNodeFlags_PassthruCentralNode;
    io.ConfigDockingWithShift = true;
    io.ConfigDockingTransparentPayload = true;
    io.ConfigViewportsNoAutoMerge = true;
    
    
}
    
#define ADD_ERR_AND_RETURN(sofar,addition)\
sofar += addition;\
VAPPLOG_INFO(sofar.c_str());\
return false;
    
void VisibleApp::setup()
{
    setup_ui();
    
    mRootOutputDir = VisibleAppControl::get_visible_app_support_directory();
    mRunAppPath = ci::app::getAppPath();
    const bfs::path plist = mRunAppPath / "Visible.app/Contents/Info.plist";
    if (exists (mRunAppPath)){
        std::ifstream stream(plist.c_str(), std::ios::binary);
        Plist::readPlist(stream, mPlist);
        mBuildn =  boost::any_cast<const string&>(mPlist.find("CFBundleVersion")->second);
    }
    
//        VisibleAppControl::setup_text_loggers(mRootOutputDir, "Visible Log " );
    VisibleAppControl::setup_loggers(mRootOutputDir, visual_log, " Visible Log ");
    
    //   if(mVisibleScope== nullptr){
    mVisibleScope = gl::Texture::create( loadImage( loadResource(VISIBLE_SCOPE  )));
    
    std::cout << getDisplay()->getWidth() << " , " << getDisplay()->getHeight() << std::endl;
    
    setWindowSize(APP_WIDTH,APP_HEIGHT);
    setFrameRate( 60 );
    setWindowPos(getWindowSize()/6);
    
    WindowRef ww = getWindow ();
    if( mVisibleScope ){
        gl::draw( mVisibleScope, getWindowBounds() );
    }
    
    getSignalShouldQuit().connect( std::bind( &VisibleApp::shouldQuit, this ) );
    
    getWindow()->getSignalMove().connect( std::bind( &VisibleApp::windowMove, this ) );
    getWindow()->getSignalDisplayChange().connect( std::bind( &VisibleApp::displayChange, this ) );
//    getWindow()->getSignalDraw().connect([&]{draw();});
    getWindow()->getSignalClose().connect(std::bind( &VisibleApp::windowClose, this) );
    getWindow()->getSignalResize().connect(std::bind( &VisibleApp::resize, this) );
    
    getSignalDidBecomeActive().connect( [this] { update_log ( "App became active." ); } );
    getSignalWillResignActive().connect( [this] { update_log ( "App will resign active." ); } );
    
    
    // Create an invisible folder for storage
    mUserStorageDirPath = getHomeDirectory()/".Visible";
    if (!bfs::exists( mUserStorageDirPath)) bfs::create_directories(mUserStorageDirPath);
    
    for( auto display : Display::getDisplays() )
     {
         mGlobalBounds.include(display->getBounds());
     }
}





std::string identify_extension(const bfs::path& bpath){
    
    if (bpath.empty() || exists(bpath) == false || bpath.filename_is_dot() || bpath.filename_is_dot()){
        std::string msg = bpath.string() + " is not a valid path to a file ";
        vlogger::instance().console()->info(msg);
        return "";
    }
    
    auto extension = bfs::path(bpath).extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
    return extension;
}


std::string VisibleApp::identify_file(const bfs::path& bpath, const std::string& custom_type){
    
    std::lock_guard<std::mutex> lock(m_mutex);
    m_is_valid_file = false;
    m_is_lif_file = false;
    m_is_mov_file = false;
    
    if (bpath.empty() || exists(bpath) == false || bpath.filename_is_dot() || bpath.filename_is_dot()){
        std::string msg = bpath.string() + " is not a valid path to a file ";
        vlogger::instance().console()->info(msg);
        return "";
    }
    
    auto extension = bfs::path(bpath).extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
    mDotExtension = identify_extension(bpath);
    if (mDotExtension != ""){
        m_is_valid_file = std::find( supported_mov_extensions.begin(), supported_mov_extensions.end(), mDotExtension) != supported_mov_extensions.end();
        m_is_lif_file = m_is_valid_file && extension == ".lif";
        m_is_mov_file = m_is_valid_file && (extension == ".mov" || extension == ".mp4");
        if (m_is_valid_file){
            mCurrentContent = bpath;
            mCurrentContentName = bpath.filename().string();
        }
    }
    
    return mDotExtension;
    
}

size_t VisibleApp::list_lif_series(std::vector<std::string>& names){
    assert(m_is_lif_file);
    assert(exists(mCurrentContent));
    
    mBrowser =  lif_browser::create(mCurrentContent.string());
    std::strstream msg;
    names = mBrowser->names ();
    
    for (auto & se : names)
        msg << std::endl << se;
    msg << std::endl;
    
    std::string tmp = msg.str();
    VAPPLOG_INFO(tmp.c_str());
    
    return names.size();
}

bool VisibleApp::load_lif_serie(const std::string& serie){
    if( mContext && mContext->is_valid()) return true;
    
    auto bpath_path = mCurrentContent;
    VisibleAppControl::make_result_cache_directory_for_lif (bpath_path, mBrowser);
    auto cache_path = VisibleAppControl::get_visible_cache_directory();
    auto stem = mCurrentContent.stem();
    cache_path = cache_path / stem;
    std::string cmds = " [ " + serie + " ] ";
    
    auto indexItr = mBrowser->name_to_index_map().find(serie);
    if (indexItr != mBrowser->name_to_index_map().end()){
        auto serie = mBrowser->get_serie_by_index(indexItr->second);
        std::strstream msg;
        msg << serie << std::endl;
        VAPPLOG_INFO(msg.str());
        
        mViewerWindow = getWindow();
        mContext = std::make_shared<lifContext>(mViewerWindow,serie,cache_path, mCurrentContentName);
        
        if (mContext->is_valid()){
            cmds += "  Ok ";
        }
        
        VAPPLOG_INFO(cmds.c_str());
        update();
        
        mViewerWindow->setTitle (cmds + " Visible build: " + mBuildn);
        mFont = Font( "Menlo", 18 );
        //        mSize = vec2( getWindowWidth(), getWindowHeight() / 12);
        // ci::ThreadSetup threadSetup; // instantiate this if you're talking to Cinder from a secondary thread
        return true;
    }
    ADD_ERR_AND_RETURN(cmds, " Serie Not Found ")
}

#if 0
    else if (exists_with_extenstion && is_valid_extension && is_video_content){
        
        
        VisibleAppControl::setup_loggers(root_output_dir, visual_log, bfs::path(bpath).filename().string());
        
        cvVideoPlayer::ref vref = cvVideoPlayer::create(bfs::path(bpath));
        WindowRef ww = getWindow ();
        
        auto bpath_path = bfs::path(bpath);
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
    


void VisibleApp::windowMove()
{
    //  update_log("window pos: " + toString(getWindow()->getPos()));
}


void VisibleApp::update_log (const std::string& msg)
{
    if (msg.length() > 2)
        VAPPLOG_INFO(msg.c_str());
}


void VisibleApp::mouseDrag( MouseEvent event )
{
    
    if (mContext) mContext->mouseDrag(event);
    else
        cinder::app::App::mouseDrag(event);
}

void VisibleApp::mouseMove( MouseEvent event )
{
    if (mContext) mContext->mouseMove(event);
    else
        cinder::app::App::mouseMove(event);
}


void VisibleApp::mouseUp( MouseEvent event )
{
    if (mContext) mContext->mouseUp(event);
    else
        cinder::app::App::mouseUp(event);
}

void VisibleApp::mouseDown( MouseEvent event )
{
    if(mContext)
        mContext->mouseDown(event);
    else
        cinder::app::App::mouseDown(event);
}

void VisibleApp::displayChange()
{
    update_log ( "window display changed: " + to_string(getWindow()->getDisplay()->getBounds()));
    update_log ( "ContentScale = " + to_string(getWindowContentScale()));
    update_log ( "getWindowCenter = " + to_string(getWindowCenter()));
}



void VisibleApp::keyDown( KeyEvent event )
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


void VisibleApp::update()
{
    // ImGuiIO &io = ImGui::GetIO();
    
    if (mContext && mContext->is_valid()) mContext->update ();
    DrawMainMenu();
}


void VisibleApp::draw ()
{
    gl::clear( Color::gray( 0.67f ) );
    if (mContext && mContext->is_valid()){
        mContext->draw ();
    }
}



void VisibleApp::resize ()
{
    //  mSize = vec2( getWindowWidth(), getWindowHeight() / 12);
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
//        settings->setWindowSize(lifContext::startup_display_size().x, lifContext::startup_display_size().y);
    settings->setFrameRate( 60 );
    settings->setResizable( true );
    settings->setResizable( true );
}




    // settings fn from top of file:
    CINDER_APP( VisibleApp, RendererGl, prepareSettings )
    
#pragma GCC diagnostic pop
    
    //int main( int argc, char* argv[] )
    //{
    //    cinder::app::RendererRef renderer( new RendererGl );
    //    cinder::app::AppMac::main<VisibleRunApp>( renderer, "VisibleRunApp", argc, argv, nullptr); // prepareSettings);
    //    return 0;
    //}
    
    
