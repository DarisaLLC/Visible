/*
 * Code Copyright 2011 Darisa LLC
 */



#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdelete-non-virtual-dtor"
#pragma GCC diagnostic ignored "-Wunused-private-field"
#pragma GCC diagnostic ignored "-Wcomma"

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

#define APP_WIDTH 1024
#define APP_HEIGHT 768

using namespace ci;
using namespace ci::app;
using namespace std;


#if 0
void prepareSettings( App::Settings *settings )
{
    settings->setHighDensityDisplayEnabled();
    settings->setWindowSize(APP_WIDTH,APP_HEIGHT);
    settings->setFrameRate( 60 );
    settings->setResizable( true );
}

#endif


class VisibleApp : public App, public SingletonLight<VisibleApp> {
  public:
    
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
    
	vector<Item>			mItems;
	vector<Item>::iterator	mMouseOverItem;
	vector<Item>::iterator	mNewMouseOverItem;
	vector<Item>::iterator	mSelectedItem;
	
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
};


void VisibleApp::create_lif_viewer (const int serie_index)
{
    Window::Format format( RendererGl::create() );
    WindowRef ww = createConnectedWindow(format);
    mContexts.push_back(std::unique_ptr<lifContext>(new lifContext(ww, mLifRef, serie_index)));
    
}

WindowRef VisibleApp::createConnectedWindow(Window::Format& format)
{
    WindowRef win = createWindow( Window::Format().size( APP_WIDTH, APP_HEIGHT ) );
    win->getSignalClose().connect( std::bind( &VisibleApp::windowClose, this ) );
    win->getSignalMouseDown().connect( std::bind( &VisibleApp::windowMouseDown, this, std::placeholders::_1 ) );
    win->getSignalDisplayChange().connect( std::bind( &VisibleApp::displayChange, this ) );
    return win;
    
}


void VisibleApp::windowMouseDown( MouseEvent &mouseEvt )
{
    //  update_log ( "Mouse down in window" );
}


void VisibleApp::fileDrop( FileDropEvent event )
{
    auto imageFile = event.getFile(0);
    
    const fs::path& file = imageFile;
    
    if (! exists(file) ) return;
    
 
    if (file.has_extension())
    {
        std::string ext = file.extension().string ();
        if (ext.compare(".lif") == 0)
            initData(file);
        return;
    }
}


bool VisibleApp::shouldQuit()
{
    return true;
}

//
void VisibleApp::setup()
{
#if defined (  HOCKEY_SUPPORT )
    hockeyAppSetup hockey;
#endif
    
    const fs::path& appPath = ci::app::getAppPath();
    const fs::path plist = appPath / "Visible.app/Contents/Info.plist";
    std::ifstream stream(plist.c_str(), std::ios::binary);
    Plist::readPlist(stream, mPlist);
    
    
    for( auto display : Display::getDisplays() )
    {
    }
    
    setWindowPos(getWindowSize()/4);
    getWindow()->setAlwaysOnTop();
    WindowRef ww = getWindow ();
    std::string buildN =  boost::any_cast<const string&>(mPlist.find("CFBundleVersion")->second);
    ww->setTitle ("Visible ( build: " + buildN + " ) ");
    mFont = Font( "Menlo", 18 );
    mSize = vec2( getWindowWidth(), getWindowHeight() / 12);
    
	// fonts
	Font smallFont = Font( "TrebuchetMS-Bold", 16 );
	Font bigFont   = Font( "TrebuchetMS-Bold", 72 );
	Item::setFonts( smallFont, bigFont );
	
	// title text
	TextLayout layout;
	layout.clear( ColorA( 1, 1, 1, 0 ) );
	layout.setFont( bigFont );
	layout.setColor( Color::white() );
	layout.addLine( "Icelandic Colors" );
	mTitleTex		= gl::Texture2d::create( layout.render( true ) );
	
	// positioning
	mLeftBorder		= 50.0f;
	mTopBorder		= 375.0f;
	mItemSpacing	= 22.0f;
	
	// create items
    std::vector<std::string> extensions = {"lif"};
    auto platform = ci::app::Platform::get();
    
    auto home_path = platform->getHomeDirectory();
    vlogger::instance().console()->info(home_path.string());
    auto some_path = getOpenFilePath(); //"", extensions);
    if (! some_path.empty() || exists(some_path))
        initData(some_path);
    else{
            std::string msg = some_path.string() + " is not a valid path to a file ";
            vlogger::instance().console()->info(msg);
    }
	
	// textures and colors
	mFgImage		= gl::Texture2d::create( APP_WIDTH / 2, APP_HEIGHT / 2 );
	mBgImage		= gl::Texture2d::create( APP_WIDTH / 2, APP_HEIGHT / 2 );
	mFgAlpha		= 1.0f;
	mBgColor		= Color::white();
	
	// swatch graphics (square and blurry square)
    auto palette_path = platform->getResourceDirectory();
    std::string small =  "swatchSmall.png";
    std::string large =  "swatchLarge.png";
    auto small_path = palette_path / small ;
	mSwatchSmallTex	= gl::Texture2d::create( loadImage( small_path ) );
    auto large_path = palette_path / large ;
	mSwatchLargeTex	= gl::Texture2d::create( loadImage( large_path ) );
	
	// state
	mMouseOverItem		= mItems.end();
	mNewMouseOverItem	= mItems.end();
	mSelectedItem		= mItems.begin();
	mSelectedItem->select( timeline(), mLeftBorder );
    
    getSignalShouldQuit().connect( std::bind( &VisibleApp::shouldQuit, this ) );
    
    getWindow()->getSignalMove().connect( std::bind( &VisibleApp::windowMove, this ) );
    getWindow()->getSignalDisplayChange().connect( std::bind( &VisibleApp::displayChange, this ) );
    getWindow()->getSignalDraw().connect(std::bind( &VisibleApp::draw, this) );
    getWindow()->getSignalClose().connect(std::bind( &VisibleApp::windowClose, this) );
    getWindow()->getSignalResize().connect(std::bind( &VisibleApp::resize, this) );
    
    getSignalDidBecomeActive().connect( [this] { update_log ( "App became active." ); } );
    getSignalWillResignActive().connect( [this] { update_log ( "App will resign active." ); } );
    
    getWindow()->getSignalDisplayChange().connect( std::bind( &VisibleApp::displayChange, this ) );
    
}


void VisibleApp::windowMove()
{
    //  update_log("window pos: " + toString(getWindow()->getPos()));
}

void VisibleApp::initData( const fs::path &path )
{
    if(! exists(path))
    {
        std::string msg = path.string() + " is not a valid path to a file ";
        vlogger::instance().console()->info(msg);
    }
    mCurrentLifFilePath = path;
    mLifRef = lif_browser::create(path);
    
    auto series = mLifRef->internal_serie_infos();

	for( vector<internal_serie_info>::const_iterator serieIt = series.begin(); serieIt != series.end(); ++serieIt )
		createItem( *serieIt, int(serieIt - series.begin()) );
}

void VisibleApp::createItem( const internal_serie_info &serie, int index )
{
    string title				= serie.name;
    string desc					= " Channels: " + to_string(serie.channelCount) +
                                  "   width: " + to_string(serie.dimensions[0]) +
                                  "   height: " + to_string(serie.dimensions[1]);
    
    auto platform = ci::app::Platform::get();
    auto palette_path = platform->getResourceDirectory();
    string paletteFilename        = "palette.png";
    palette_path = palette_path / paletteFilename;
    Surface palette = Surface( loadImage(  palette_path ) ) ;
    gl::Texture2dRef image = gl::Texture::create(Surface( ImageSourceRef( new ImageSourceCvMat( serie.poster ))));
	vec2 pos( mLeftBorder, mTopBorder + index * mItemSpacing );
	Item item = Item(index, pos, title, desc, palette );
	mItems.push_back( item );
	mImages.push_back( image );
   
}

void VisibleApp::mouseMove( MouseEvent event )
{
    guiContext  *data = getWindow()->getUserData<guiContext>();
    if(data)
        data->mouseMove(event);
    else
    {
        mNewMouseOverItem = mItems.end();

        for( vector<Item>::iterator itemIt = mItems.begin(); itemIt != mItems.end(); ++itemIt ) {
            if( itemIt->isPointIn( event.getPos() ) && !itemIt->getSelected() ) {
                mNewMouseOverItem = itemIt;

                break;
            }
        }
        
        if( mNewMouseOverItem == mItems.end() ){
            if( mMouseOverItem != mItems.end() && mMouseOverItem != mSelectedItem ){
                mMouseOverItem->mouseOff( timeline() );
                mMouseOverItem = mItems.end();
            }
        } else {
        
            if( mNewMouseOverItem != mMouseOverItem && !mNewMouseOverItem->getSelected() ){
                if( mMouseOverItem != mItems.end() && mMouseOverItem != mSelectedItem )
                    mMouseOverItem->mouseOff( timeline() );
                mMouseOverItem = mNewMouseOverItem;
                mMouseOverItem->mouseOver( timeline() );
            }
        }
        
        if( mMouseOverItem != mItems.end() && mMouseOverItem != mSelectedItem ){
            mFgImage = mImages[mMouseOverItem->mIndex];
      //      mFgAlpha = 0.0f;
       //     timeline().apply( &mFgAlpha, 1.0f, 0.4f, EaseInQuad() );
        }
    }
}

void VisibleApp::mouseDown( MouseEvent event )
{
    guiContext  *data = getWindow()->getUserData<guiContext>();
    if(data)
        data->mouseDown(event);
    else
    {
        if( mMouseOverItem != mItems.end() ){
            vector<Item>::iterator prevSelected = mSelectedItem;
            mSelectedItem = mMouseOverItem;
            
            // deselect previous selected item
            if( prevSelected != mItems.end() && prevSelected != mMouseOverItem ){
                prevSelected->deselect( timeline() );
                mBgImage = mFgImage;
                mBgColor = Color::white();
                timeline().apply( &mBgColor, Color::black(), 0.4f, EaseInQuad() );
            }
            
            // select current mouseover item
            mSelectedItem->select( timeline(), mLeftBorder );
            mFgImage = mImages[mSelectedItem->mIndex];
            mFgAlpha = 0.0f;
            timeline().apply( &mFgAlpha, 1.0f, 0.4f, EaseInQuad() );
            mMouseOverItem = mItems.end();
            mNewMouseOverItem = mItems.end();
            
            // Open a LIF Context for this serie
           create_lif_viewer(mSelectedItem->mIndex);
        }
    }
}

void VisibleApp::update()
{
    guiContext  *data = getWindow()->getUserData<guiContext>();
    
    if (data && data->is_valid()) data->update ();
    else {
        for( vector<Item>::iterator itemIt = mItems.begin(); itemIt != mItems.end(); ++itemIt ) {
            itemIt->update();
        }
    }
}

void VisibleApp::draw()
{
    // Note: this function is called once per frame for EACH WINDOW
    gl::ScopedViewport scpViewport( ivec2( 0 ), getWindowSize() );
    
    guiContext  *data = getWindow()->getUserData<guiContext>();
    
    bool valid_data = data != nullptr && data->is_valid ();
    
    if (valid_data){
          gl::clear( Color::gray( 0.5f ) );
         data->draw();
    }
    else {
        
        // clear out the window with black
        gl::clear( Color( 0, 0, 0 ) );
        gl::enableAlphaBlending();
        
        gl::setMatricesWindowPersp( getWindowSize() );
        
        // draw background image
        if( mBgImage ){
            gl::color( mBgColor );
            gl::draw( mBgImage, getWindowBounds() );
        }
        
        // draw foreground image
        if( mFgImage ){
            Rectf bounds (mFgImage->getBounds());
            bounds = bounds.getCenteredFit(getWindowBounds(), false);
            gl::color( ColorA( 1.0f, 1.0f, 1.0f, mFgAlpha ) );
            gl::draw( mFgImage, bounds );
        }
        
        // draw swatches
        gl::context()->pushTextureBinding( mSwatchLargeTex->getTarget(), mSwatchLargeTex->getId(), 0 );
        if( mSelectedItem != mItems.end() )
            mSelectedItem->drawSwatches();
        
        gl::context()->bindTexture( mSwatchLargeTex->getTarget(), mSwatchLargeTex->getId(), 0 );
        for( vector<Item>::const_iterator itemIt = mItems.begin(); itemIt != mItems.end(); ++itemIt ) {
          //  if( ! itemIt->getSelected() )
                itemIt->drawSwatches();
        }
        gl::context()->popTextureBinding( mSwatchLargeTex->getTarget(), 0 );
        
        // turn off textures and draw bgBar
        for( vector<Item>::const_iterator itemIt = mItems.begin(); itemIt != mItems.end(); ++itemIt ) {
            itemIt->drawBgBar();
        }
        
        // turn on textures and draw text
        gl::color( Color( 1.0f, 1.0f, 1.0f ) );
        for( vector<Item>::const_iterator itemIt = mItems.begin(); itemIt != mItems.end(); ++itemIt ) {
            itemIt->drawText();
        }
    }
}


void VisibleApp::update_log (const std::string& msg)
{
    if (msg.length() > 2)
        std::cout << msg.c_str ();
}

void VisibleApp::displayChange()
{
 //   update_log ( "window display changed: " + toString(getWindow()->getDisplay()->getBounds()));
 //   console() << "ContentScale = " << getWindowContentScale() << endl;
 //   console() << "getWindowCenter() = " << getWindowCenter() << endl;
}


void VisibleApp::windowClose()
{
    WindowRef win = getWindow();
}



void VisibleApp::mouseDrag( MouseEvent event )
{
    guiContext  *data = getWindow()->getUserData<guiContext>();
    if(data)
        data->mouseDrag(event);
    else
        cinder::app::App::mouseDrag(event);
}




void VisibleApp::mouseUp( MouseEvent event )
{
    guiContext  *data = getWindow()->getUserData<guiContext>();
    if(data)
        data->mouseUp(event);
    else
        cinder::app::App::mouseUp(event);
}


void VisibleApp::keyDown( KeyEvent event )
{
    guiContext  *data = getWindow()->getUserData<guiContext>();
    if(data)
        data->keyDown(event);
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



void VisibleApp::resize ()
{
    mSize = vec2( getWindowWidth(), getWindowHeight() / 12);
    guiContext  *data = getWindow()->getUserData<guiContext>();
    
    if (data && data->is_valid()) data->resize ();
    
}

CINDER_APP( VisibleApp, RendererGl( RendererGl::Options().msaa( 4 ) ), []( App::Settings *settings ) {
	settings->setWindowSize( APP_WIDTH, APP_HEIGHT );
} )


#pragma GCC diagnostic pop
