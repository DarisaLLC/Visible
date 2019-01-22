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


#include "cinder_opencv.h"
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

#include "algo_Lif.hpp"
#include "Item.h"
#include "logger.hpp"
#include "core/core.hpp"
#include "core/singleton.hpp"
#include "core/file_system.hpp"
#include "otherIO/lifFile.hpp"
#include "Plist.hpp"

#if defined (  HOCKEY_SUPPORT )
#include "hockey_etc_cocoa_wrappers.h"
#endif

#include "LifContext.h"
#include "gui_handler.hpp"
#include "gui_base.hpp"
#include "imGuiLogger.hpp"
#include "visible_logger_macro.h"
#include "imGuiCustom/ImSequencer.h"
#include "imGuiCustom/visibleSequencer.h"

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
    
    bool check_input (const string &filename);
    
    fs::path get_visible_app_directory ();
    fs::path get_runner_app_directory ();
    
    bool setup_loggers (const fs::path app_support_dir,  imGuiLog& visualLog, std::string id_name);
    bool setup_text_loggers (const fs::path app_support_dir,  std::string id_name);
    
    static const std::string LIF_CUSTOM = "IDLab_0";
}

namespace vac = VisibleAppControl;

class VisibleApp : public App, public gui_base {
public:
    
    virtual void SetupGUIVariables() override;
    virtual void DrawGUI() override;
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
    
private:
    fs::path getLoggingDirectory ();
    void initData( const fs::path &path );
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
    
    gl::Texture2dRef mTitleTex;
    gl::Texture2dRef mBgImage;
    gl::Texture2dRef mFgImage;
    gl::Texture2dRef mSwatchSmallTex, mSwatchLargeTex;
    
    Anim<float> mFgAlpha;
    Anim<Color> mBgColor;
    
    lif_browser::ref mLifRef;
    mutable std::list <std::unique_ptr<guiContext> > mContexts;
    map<string, boost::any> mPlist;
    Font                mFont;
    std::string            mLog;
    vec2                mSize;
    fs::path            mCurrentLifFilePath;
    std::string         mFileName;
    std::string         mFileExtension;
    
    int convergence = 0;
    bool m_isIdLabLif = false;
    bool showLog = false;
    bool showHelp = false;
    bool showOverlay = false;
    
    imGuiLog app_log;
};

class VisibleRunApp : public App, public gui_base
{
public:

 //   VisibleRunApp();
  //  ~VisibleRunApp();

    virtual void SetupGUIVariables() override;
    virtual void DrawGUI() override;
    virtual void QuitApp();

    void prepareSettings( Settings *settings );
    void setup()override;
    void mouseDown( MouseEvent event )override;
    void keyDown( KeyEvent event )override;

    void update()override;
    void draw()override;
    void resize()override;
    void displayChange();
    void update_log (const std::string& msg);

    bool shouldQuit();

private:
    fs::path getLoggingDirectory ();
    std::vector<std::string> m_args;
    vec2                mSize;
    Font                mFont;
    std::string            mLog;

    Rectf                        mGlobalBounds;
    map<string, boost::any> mPlist;

    mutable std::unique_ptr<lifContext> mContext;
    mutable lif_browser::ref mBrowser;
    
    bool showLog = false;
    bool showHelp = false;
    bool showOverlay = false;
    int convergence = 0;
    std::string         mFileName;
    std::string         mFileExtension;
    lifIO::ContentType_t mContentType; // "" denotes canonical LIF file
    bool m_isIdLabLif = false;
    void setup_ui ();
    
    imGuiLog visual_log;
    

  
    
};

#endif /* VisibleApp_h */
