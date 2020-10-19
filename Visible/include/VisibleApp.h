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

#include "cinder_cv/cinder_opencv.h"
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


#include "Item.h"
#include "logger/logger.hpp"
#include "core/core.hpp"
#include "core/singleton.hpp"
#include "core/file_system.hpp"

#include "Plist.hpp"

#include "mediaInfo.h"
#include "LifContext.h"
#include "imGuiLogger.hpp"
#include "visible_logger_macro.h"
#include "imGuiCustom/ImSequencer.h"
#include "imGuiCustom/visibleSequencer.h"
#include "imGuiCustom/imgui_wrapper.h"
#include "Resources.h"
#include "nfd.h"

#define APP_WIDTH 1536
#define APP_HEIGHT 896

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
    VisibleApp() { ImGui::CreateContext(); }
    ~VisibleApp() { ImGui::DestroyContext (); }
    
    virtual void QuitApp();
    
    void prepareSettings( Settings *settings );
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
    void launchViewer ();
    
private:
    
    std::shared_ptr<ImageBuf> mInput;
    ImageSpec mInputSpec;
    mediaSpec m_mspec;
    
    // return null for unacceptable file or dot extension, i.e. ".lif" or ".mov" or ".mp4", etc
    std::string identify_file(const bfs::path& mfile, const std::string& content_type);
    bool load_oiio_file();
    size_t list_oiio_channels(std::vector<std::string>& channel_names);

    void DrawMainMenu ();
    void DrawImGuiMetrics();
    void DrawImGuiDemos ();
    void DrawStatusBar(float width, float height, float pos_x, float pos_y);
    void DrawLogView();
    void DrawSettings();
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
