//
//  VisibleApp.h
//  Visible
//
//  Created by Arman Garakani on 10/15/18.
//

#ifndef VisibleApp_h
#define VisibleApp_h
/*
 * Code Copyright 2011 Darisa LLC
 */

#include <OpenImageIO/imagebuf.h>
#include <OpenImageIO/imagecache.h>
#include <OpenImageIO/imageio.h>

#define float16_t opencv_broken_float16_t
#include "cinder_cv/cinder_opencv.h"
#undef float16_t


#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/ImageIo.h"
#include "cinder/Utilities.h"
#include "cinder/app/Platform.h"

#include <iostream>
#include <fstream>
#include <string>
#include <map>


#include "logger/logger.hpp"
#include "core/core.hpp"
#include "core/singleton.hpp"
#include "core/file_system.hpp"

#include "Plist.hpp"
#include "imgui.h"
#include "imgui_internal.h"

#include "mediaInfo.h"
#include "LifContext.h"
#include "imGuiLogger.hpp"
#include "visible_logger_macro.h"
#include "imgui_wrapper.h"
#include "Resources.h"
#include "imGuiCustom/imfilebrowser.h"
#include "implot.h"

//#define APP_WIDTH 1536
//#define APP_HEIGHT 896

using namespace boost;
namespace bfs=boost::filesystem;

using namespace ci;
using namespace ci::app;
using namespace std;
using namespace OIIO;

namespace VisibleAppControl{
    /**
     When this is set to false the threads managed by this program will stop and join the main thread.
     */
    extern bool ThreadsShouldStop;
    static constexpr const char c_visible_app_support[] = "Library/Application Support/net.darisallc.Visible";
    static constexpr const char c_visible_cache_folder_name [] = ".Visible";
    bool check_input (const string &filename);

    bfs::path get_visible_app_support_directory ();
    bfs::path get_visible_cache_directory ();

    bool setup_loggers (const bfs::path app_support_dir,  imGuiLog& visualLog, std::string id_name);
    bool setup_text_loggers (const bfs::path app_support_dir,  std::string id_name);

    bfs::path make_result_cache_entry_for_content_file (const boost::filesystem::path& path);
}


class VisibleApp : public App
{
public:
	VisibleApp() { ImGui::CreateContext(); ImPlot::CreateContext(); m_displayFPS = 1.0f;}
	~VisibleApp() { if(mCachePtr) mCachePtr->close_all (); ImPlot::DestroyContext(); ImGui::DestroyContext (); }
    
    virtual void QuitApp();
    
    static void prepareSettings( Settings *settings );
    void setup()override;
    void mouseMove( MouseEvent event ) override;
    void mouseDrag( MouseEvent event ) override;
    void mouseDown( MouseEvent event ) override;
    void mouseUp( MouseEvent event )override;
    void keyDown( KeyEvent event )override;
    void update() override;
    void draw() override;
    void windowClose();
    void windowMove();
    void windowMouseDown( MouseEvent &mouseEvt );
    void displayChange();
    void resize() override;
    void fileOpen ();
    
    void update_log (const std::string& msg);
    
    bool shouldQuit();
    bool isValid() const { return m_is_valid_file; }
    bool isOiiOFile() const { return isValid() && m_is_oiio_file; }
	void magnification (const float& mmag) const { m_magnification = mmag; }
	float magnification () const { return m_magnification; }
	void displayFPS (const float& fps) const { m_displayFPS = fps; }
	float displayFPS () const { return m_displayFPS; }
	
	
    void launchViewer ();
    
private:
    ImGui::FileBrowser m_fileDialog;
    std::shared_ptr<ImageBuf> mInput;
    ImageCache* mCachePtr = nullptr;
    ImageSpec mInputSpec;
    mediaSpec m_mspec;
	mutable float m_magnification;
	mutable float m_displayFPS;
	
    
    // return null for unacceptable file or dot extension, i.e. ".lif" or ".mov" or ".mp4", etc
    std::string identify_file(const bfs::path& mfile, const std::string& content_type);
    bool load_oiio_file();
    size_t list_oiio_channels(std::vector<std::string>& channel_names);

    void DrawMainMenu ();
    void DrawImGuiMetrics();
    void DrawImGuiDemos ();
    void DrawStatusBar(float width, float height, float pos_x, float pos_y);
    void DrawLogView();
    void DrawInputPanel();
    void ShowStyleEditor(ImGuiStyle* ref = NULL);   
    
    void setup_media_file(const bfs::path&);
    bfs::path            mRootOutputDir;
    bfs::path            mCurrentUserHomePath;
    bfs::path            mUserStorageDirPath;
    bfs::path            mRunAppPath;
    bfs::path            mCurrentContent;
    std::string         mDotExtension;
    std::string         mCurrentContentName;
    ustring             mCurrentContentU;
    
    bfs::path getLoggingDirectory ();
    bool            m_is_oiio_file{false};
    bool            m_is_valid_file{false};
    Font                mFont;
    std::string            mLog;
    
    std::vector<std::string> m_sections;
    int m_selected_lif_serie_index;
    std::string m_selected_lif_serie_name;
    bool m_custom_type;
    
    Rectf            mGlobalBounds;
    map<string, boost::any> mPlist;
    
    mutable std::shared_ptr<sequencedImageContext> mContext;
    app::WindowRef mViewerWindow;
    
    bool show_logs_{true};
    bool show_settings_{true};
    bool show_help_{true};
    bool show_about_{true};
    bool show_imgui_metrics_{false};
    bool show_imgui_demos_{true};
    
    gl::Texture2dRef mVisibleScope;
    
    std::string         mFileName;
    std::string         mFileExtension;
    std::string         mBuildn;
    bool m_isIdLabLif = false;
    void setup_ui ();
    
    imGuiLog visual_log;
    ImGui::Options m_imgui_options;
    
    // Threading Lock etc
    std::mutex m_mutex;
    
};

#endif /* VisibleApp_h */
