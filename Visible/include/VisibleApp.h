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
#include "core/singleton.hpp"
#include "hockey_etc_cocoa_wrappers.h"
#include "LifContext.h"
#include "core/core.hpp"
#include "Plist.hpp"
#include <map>
#include "CinderImGui.h"
#include "gui_handler.hpp"
#include "gui_base.hpp"
#include "app_logger.hpp"

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
    extern AppLog app_log;
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
    
    void initData( const fs::path &path );
    void createItem( const internal_serie_info &serie, int serieNumber );
    
    void fileDrop( FileDropEvent event ) override;
    
    bool shouldQuit();
    void update_log (const std::string& msg);
    WindowRef createConnectedWindow (Window::Format& format);
    void create_lif_viewer (const int serie_index);
    
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
    
    bool swapEyes = false;
    
    int convergence = 0;
    
    bool showLog = false;
    bool showHelp = false;
    bool showOverlay = false;
};



#endif /* VisibleApp_h */