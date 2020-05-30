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

//#include "MovContext.h"
#include "lif_content.hpp"
//#include "algo_Mov.hpp"
#include "Item.h"
#include "logger/logger.hpp"
#include "core/core.hpp"
#include "core/singleton.hpp"
#include "core/file_system.hpp"
#include "otherIO/lifFile.hpp"
#include "Plist.hpp"

#if defined (  HOCKEY_SUPPORT )
#include "hockey_etc_cocoa_wrappers.h"
#endif

#include "LifContext.h"
#include "MovContext.h"
#include "imGuiLogger.hpp"
#include "visible_logger_macro.h"
#include "imGuiCustom/ImSequencer.h"
#include "imGuiCustom/visibleSequencer.h"
#include "imGuiCustom/imgui_wrapper.h"
#include "Resources.h"
#include "nfd.h"

#define APP_WIDTH 1024
#define APP_HEIGHT 768

using namespace ci;
using namespace ci::app;
using namespace std;

namespace VisibleAppControl{
/**
 When this is set to false the threads managed by this program will stop and join the main thread.
 */
extern bool ThreadsShouldStop;
static constexpr const char c_visible_app_support[] = "Library/Application Support/net.darisallc.Visible";
static constexpr const char c_visible_runner_app_support[] = "Library/Application Support/net.darisallc.VisibleRun";
static constexpr const char c_visible_cache_folder_name [] = ".Visible";
bool check_input (const string &filename);

bfs::path get_visible_app_support_directory ();
bfs::path get_runner_app_support_directory ();
bfs::path get_visible_cache_directory ();

bool setup_loggers (const bfs::path app_support_dir,  imGuiLog& visualLog, std::string id_name);
bool setup_text_loggers (const bfs::path app_support_dir,  std::string id_name);

bfs::path make_result_cache_entry_for_content_file (const boost::filesystem::path& path);
bool make_result_cache_directory_for_lif (const boost::filesystem::path& path, const lif_browser::ref& lif_ref);

static const std::string LIF_CUSTOM = "IDLab_0";
}

namespace vac = VisibleAppControl;

class VisibleApp : public App {
public:
    
    virtual void DrawGUI();
    virtual void QuitApp();
    //   void prepareSettings( Settings *settings );
    void setup() override;
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
private:
    bfs::path getLoggingDirectory ();
    void initData( const bfs::path &path );
    void createItem( const lif_serie_data &serie, int serieNumber );
    
    void fileDrop( FileDropEvent event ) override;
    
    bool shouldQuit();
    void update_log (const std::string& msg);
    WindowRef createConnectedWindow (Window::Format& format);
    void dispatch_lif_viewer (const int serie_index);
    
    vector<Item>            mItems;
    vector<Item>::iterator    mMouseOverItem;
    vector<Item>::iterator    mNewMouseOverItem;
    vector<Item>::iterator    mSelectedItem;
    
    vector<gl::Texture2dRef>    mImages;
    
    float mLeftBorder;
    float mTopBorder;
    float mItemSpacing;
    
    gl::Texture2dRef mVisibleScope;
    gl::Texture2dRef mTitleTex;
    gl::Texture2dRef mBgImage;
    gl::Texture2dRef mFgImage;
    gl::Texture2dRef mSwatchSmallTex, mSwatchLargeTex;
    gl::Texture2dRef mDropFileTextTexture;
    
    Anim<float> mFgAlpha;
    Anim<Color> mBgColor;
    
    lif_browser::ref mLifRef;
    mutable std::list <std::unique_ptr<guiContext> > mContexts;
    map<string, boost::any> mPlist;
    Font                mFont;
    std::string            mLog;
    vec2                mSize;
    bfs::path            mCurrentLifFilePath;
    bfs::path            mUserStorageDirPath;
    bfs::path            mRunAppPath;
    
    std::string         mFileName;
    std::string         mFileExtension;
    std::string         mRunAppAppString;
    
    int convergence = 0;
    bool m_isIdLabLif = false;
    bool showLog = false;
    bool showHelp = false;
    bool showOverlay = false;
    bool showGUI = false;
    
    imGuiLog app_log;
    
    
    
};

class VisibleRunApp : public App
{
public:
    
    
    virtual void DrawGUI();
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
    
private:
    // return null for unacceptable file or dot extension, i.e. ".lif" or ".mov" or ".mp4", etc
    std::string identify_file(const bfs::path& mfile, const std::string& content_type);
    bool load_lif_serie(const std::string& serie);
    size_t list_lif_series(std::vector<std::string>& names);
    
    float DrawMainMenu ();
    void DrawImGuiMetrics();
    void DrawImGuiDemos ();
    void DrawStatusBar(float width, float height, float pos_x, float pos_y);
    void DrawLogView();
    void DrawSettings();
    void DrawDocksDebug();
    
    void setup_media_file(const bfs::path&);
    bfs::path            mRootOutputDir;
    bfs::path            mCurrentUserHomePath;
    bfs::path            mUserStorageDirPath;
    bfs::path            mRunAppPath;
    bfs::path            mCurrentContent;
    std::string         mDotExtension;
    std::string         mCurrentContentName;
    
    bfs::path getLoggingDirectory ();
    bool            m_is_lif_file;
    bool            m_is_valid_file;
    bool            m_is_mov_file;
    vec2                mSize;
    Font                mFont;
    std::string            mLog;
    
    int m_selected_lif_serie_index;
    std::string m_selected_lif_serie_name;
    bool m_custom_type;
    
    Rectf            mGlobalBounds;
    map<string, boost::any> mPlist;
    
    mutable std::shared_ptr<sequencedImageContext> mContext;
    mutable lif_browser::ref mBrowser;
    
    bool show_docks_debug_{false};
    bool show_logs_{true};
    bool show_settings_{true};
    bool show_imgui_metrics_{false};
    bool show_imgui_demos_{false};
    
    gl::Texture2dRef mVisibleScope;
    
    std::string         mFileName;
    std::string         mFileExtension;
    std::string         mBuildn;
    lifIO::ContentType_t mContentType; // "" denotes canonical LIF file
    bool m_isIdLabLif = false;
    void setup_ui ();
    
    imGuiLog visual_log;
    
    
    
    
};

#endif /* VisibleApp_h */
