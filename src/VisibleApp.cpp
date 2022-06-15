


#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdelete-non-virtual-dtor"
#pragma GCC diagnostic ignored "-Wunused-private-field"
#pragma GCC diagnostic ignored "-Wint-in-bool-context"
#pragma GCC diagnostic ignored "-Wcomma"
#pragma GCC diagnostic ignored "-Wshorten-64-to-32"
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"

#include "VisibleApp.h"
#include "oiio_utils.hpp"
#include "imGuiCustom/imgui_panel.hpp"
#include "imgui.h"



	//static ImGuiDockNodeFlags opt_flags = ImGuiDockNodeFlags_None;

bool VisibleAppControl::ThreadsShouldStop = false;

namespace {
	const std::vector<std::string>& supported_mov_extensions = { ".lif", ".mov", ".mp4", ".ts", ".avi", ".tif"};
}
using namespace ci;
using namespace ci::app;
using namespace std;
using namespace boost;
namespace bfs=boost::filesystem;
using pipeline = visibleContext::pipeline;


void VisibleApp::QuitApp(){
	quit();
}

void VisibleApp::DrawInputPanel() {
	
	ImGui::ScopedWindow window( " Input ");
	if(! isValid()) return;
	
	ImGui::BeginGroupPanel(" Input ", {0, 0});
	
	static bool first_time = true;
	if (first_time) {
		ImGui::SetNextItemOpen(true);
		first_time = false;
	}
	if (isOiiOFile()){
		static int mov_clicked = 0;
		ImGui::SameLine();
		ImGui::NewLine();
		if (ImGui::Button(mCurrentContentName.c_str()))
			mov_clicked++;
		if (mov_clicked & 1){
			load_oiio_file();
		}
		if (ImGui::IsItemHovered()){
			ImGui::BeginTooltip();
			ImGui::Text("%s", " Click to Load ");
			ImGui::EndTooltip();
		}
			//  dump_all_extra_attributes(mInputSpec);
		ImGui::BulletText("Image Size (width: %d, height: %d)", mInputSpec.width, mInputSpec.height);
		ImGui::BulletText("TimeSteps %d", mInput->nsubimages());
		ImGui::BulletText("Data Format: %s", mInputSpec.format.c_str());
		ImGui::BulletText("Number of Channels: %d", mInputSpec.nchannels);
		for (auto nn = 0; nn < mInputSpec.nchannels; nn++)
		ImGui::BulletText("Channel(%d) %s", nn, mInputSpec.channelnames[nn].c_str());
	}
	
	ImGui::BeginGroup();
	bool changed = false;
	changed |= ImGui::InputFloat(" Magnification X ", &m_magnification, 0.1, 2.0, "%.3f" );
	if (changed)
		m_magnification = svl::math<float>::clamp(m_magnification, 0.1f, 100.0f);
	changed = false;
	changed |= ImGui::InputFloat(" Display FPS ", &m_displayFPS, 0.1, 2.0, "%.3f" );
	if (changed)
		m_displayFPS = svl::math<float>::clamp(m_displayFPS, 0.1f, 100.0f);
	ImGui::EndGroup();
	
	
	ImGui::EndGroupPanel();
	
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
		//    ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove
	if (ImGui::BeginMainMenuBar()){
		
		if (ImGui::BeginMenu("File")){
			if(ImGui::MenuItem("Open", "CTRL+O")){
				nfdchar_t *outPath = NULL;
				nfdresult_t result = NFD_OpenDialog("tif,mov,mp4", NULL, &outPath);
				if (result == NFD_OKAY) {
					bfs::path fout(outPath);
					auto msg = "Selected " + fout.string();
					vlogger::instance().console()->info(msg);
					auto dotext = identify_file(fout, "");
					if(isOiiOFile()){
						vlogger::instance().console()->info(fout.string() + " Ok " );
						m_sections.clear();
						auto num = list_oiio_channels(m_sections);
						assert(num == m_sections.size());
					}
					else{
						vlogger::instance().console()->error("%s wrong file type", fout.string());
					}
					free(outPath);
				}
			}
			if(ImGui::MenuItem("Process", "CTRL+P")){
				if (mContext && mContext->is_valid()) mContext->process_async(); //std::dynamic_pointer_cast<lifContext>(mContext)->process_async();
			}
			if(ImGui::MenuItem("Quit", "CTRL+P")){

				quit();
			}
			ImGui::EndMenu();
		}
		
		if (ImGui::BeginMenu("Views")){
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
	
	if(show_about_){
		ImGui::ScopedWindow window( "About");
		std::string buildN =  boost::any_cast<const string&>(mPlist.find("CFBundleVersion")->second);
		buildN = "Visible ( build: " + buildN + " ) ";
		ImGui::TextColored(ImVec4(0.92f, 0.18f, 0.29f, 1.00f), "%s", buildN.c_str());
		ImGui::Text("Arman Garakani, Darisa LLC");
		ImGui::Text("This App is Built With OpenImageIO, OpenCv, Boost, Cinder and ImGui Libraries ");
	}
	DrawInputPanel();
//	DrawImGuiDemos();
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
				 ImGuiWindowFlags_AlwaysAutoResize );
	
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
		//    io.ConfigDockingWithShift = true;
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
	
	VisibleAppControl::setup_text_loggers(mRootOutputDir, "Visible Log " );
	//VisibleAppControl::setup_loggers(mRootOutputDir, visual_log, " Visible Log ");
	
		//   if(mVisibleScope== nullptr){
	mVisibleScope = gl::Texture::create( loadImage( loadResource(VISIBLE_SCOPE  )));
	
	std::cout << getDisplay()->getWidth() << " , " << getDisplay()->getHeight() << std::endl;
	
	setWindowSize(getDisplay()->getWidth(), getDisplay()->getHeight());
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
	m_is_oiio_file = false;
	
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
		m_is_oiio_file = m_is_valid_file && (extension == ".mov" || extension == ".mp4" || extension == ".tif");
		if (m_is_valid_file){
			mCurrentContent = bpath;
			mCurrentContentName = bpath.filename().string();
		}
	}
	
	return mDotExtension;
	
}



size_t VisibleApp::list_oiio_channels(std::vector<std::string>& channel_names){
	assert(m_is_oiio_file);
	assert(exists(mCurrentContent));
	std::strstream msg;
	channel_names.resize(0);
	mCurrentContentU = ustring (mCurrentContent.string());
	mInput.reset(new ImageBuf(mCurrentContentU));
	mInput->init_spec(mCurrentContentU, 0, 0);  // force it to get the spec, not read
	mInputSpec = mInput->spec();
	channel_names = mInputSpec.channelnames;
	return channel_names.size();
}


bool VisibleApp::load_oiio_file(){
	if( mContext && mContext->is_valid()) return true;
	auto bpath_path = mCurrentContent;
	auto stem = mCurrentContent.stem();
	std::string cmds = " [ " + stem.string() + " ] ";
	
	m_mspec = mediaSpec (mInputSpec.width, mInputSpec.height,
						 1, 1000.0, mInput->nsubimages(),
						 mInputSpec.channelnames);
	
	if (exists(mCurrentContent)){
		WindowRef ww = getWindow ();
		ww->getApp()->setFrameRate(m_displayFPS);
		auto cache_path = VisibleAppControl::make_result_cache_entry_for_content_file(bpath_path);
		mContext =  std::make_shared<visibleContext> (ww, mInput, m_mspec, cache_path, bpath_path, magnification(), displayFPS(),
													  visibleContext::pipeline::cardiac);
		update();
		
		ww->setTitle ( cmds + " Visible build: " + mBuildn);
		mFont = Font( "Menlo", 18 );
		return true;
	}
	ADD_ERR_AND_RETURN(cmds, " Path not valid ");
}


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
}


void VisibleApp::draw ()
{
	gl::clear( Color::gray( 0.67f ) );
	if (mContext && mContext->is_valid()){
		if (GImGui != nullptr && GImGui->CurrentWindow != nullptr)
			mContext->draw ();
	}
	if (GImGui != nullptr && GImGui->CurrentWindow != nullptr)
		DrawMainMenu();
	
	
}



void VisibleApp::resize ()
{
		//    ImGuiViewport*  imvp = GetMainViewport();
		//    std::cout << imvp->Pos.x << "," << imvp->Pos.y << imvp->Size.x  << "," << imvp->Size.y << std::endl;
		//    std::cout << getWindowSize().x << "," << getWindowSize().y << std::endl;
	
	assert(GImGui != nullptr && GImGui->CurrentWindow != nullptr);
	ImGui::SetCurrentContext(GImGui);
	
	ImGuiIO& io    = ImGui::GetIO();
	auto ws = getWindowSize();
	io.DisplaySize    = ImVec2(ws.x, ws.y);
	
		//  mSize = vec2( getWindowWidth(), getWindowHeight() / 12);
	if (mContext && mContext->is_valid()) mContext->resize ();
	
}

#if CHECK
void VisibleApp::prepareSettings( App::Settings *settings )
{
	const auto &args = settings->getCommandLineArgs();
	if(args.size() > 1){
		auto ok = VisibleAppControl::check_input(args[1]);
		std::cout << "File is " << std::boolalpha << ok << std::endl;
	}
	
	settings->setHighDensityDisplayEnabled();
		//        settings->setWindowSize(lifContext::startup_display_size().x, lifContext::startup_display_size().y);
	settings->setResizable( true );
}
#endif



	// settings fn from top of file:
CINDER_APP( VisibleApp, RendererGl )

//#pragma GCC diagnostic pop

	//int main( int argc, char* argv[] )
	//{
	//    cinder::app::RendererRef renderer( new RendererGl );
	//    cinder::app::AppMac::main<VisibleRunApp>( renderer, "VisibleRunApp", argc, argv, nullptr); // prepareSettings);
	//    return 0;
	//}


