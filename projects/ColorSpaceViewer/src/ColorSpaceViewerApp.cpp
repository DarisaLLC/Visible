#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "ColorPalette.h"
#include "cinder/gl/Batch.h"
#include "cinder/CameraUi.h"
#include "cinder/Camera.h"
#include "cinder/Log.h"
#include <iostream>
#include <vector>
#include "async_producer.h"

using namespace ci;
using namespace ci::app;

// The window-specific data for each window
class LogColorWindow {
public:
    LogColorWindow() : m_ok(false)
    {
        
    }
    
    void setup()
    {
        assert(mDisWindow);
        mDisWindow->setSize(desired_window_size());
        m_name = "Log B/R Log G/R";
        mDisWindow->setTitle(m_name.c_str());

        m_rect = plot_rect();
        m_ok = false;
    }
    
    
    void setXform(vec2& ctr, vec2& scale)
    {
        m_scale.x = mDisWindow->getWidth() / scale.x;
        m_scale.y = mDisWindow->getHeight() / scale.y;
        m_trans = mDisWindow->getBounds().getCenter();
    }
    
    void createPlot ()
    {
        ColorA pcol (46/255.,211/255.,251/255.0, 1.0);
        
        mBatch = gl::VertBatch::create(GL_POINTS);
        mBatch->begin(GL_POINTS);
        for (auto ii = 0; ii < mLogChrom.first.size(); ii++)
        {
            mBatch->color( pcol );
            mBatch->vertex((mLogChrom.first[ii]*getWindowWidth()/2),
                      (mLogChrom.second[ii]*getWindowHeight()/2));
        }
        mBatch->end();
    }
    
    void draw ()
    {
        gl::clear(Color( 62/255.0, 82 / 255.0, 100 / 255.0 ));
        gl::enableAlphaBlending();
        
        cinder::gl::setMatricesWindow( getWindowWidth(), getWindowHeight() );
        cinder::gl::translate( getWindowCenter() );
        {
            gl::ScopedColor cl (1.0, 1.0, 1.0, 0.5);
            cinder::gl::drawLine(vec2(-getWindowWidth()/2, 0), vec2(getWindowWidth()/2, 0));
            cinder::gl::drawLine(vec2(0,-getWindowHeight()/2), vec2(0, getWindowHeight()/2));
        }
        if (mBatch) mBatch->draw();

    }
    
    


    void windowClose();
    void windowMouseDown( MouseEvent &mouseEvt );
    
    void mouseDown( MouseEvent event ) {}
    void mouseMove( MouseEvent event ) {}
    void mouseUp( MouseEvent event ) {}
    void mouseDrag( MouseEvent event ) {}
    void keyDown( KeyEvent event ) {}
    
	cinder::gl::VertBatchRef	mBatch;
    
    ci::app::WindowRef mDisWindow;
    std::string m_name;
    col::log_chrom_t  mLogChrom;
    gl::TextureRef m_texture;
    vec2 m_scale;
    vec2 m_trans;
    Rectf m_rect;
    bool m_ok;
    inline ivec2 canvas_size () { return vec2 (300, 300); }
    inline ivec2 trim () { return vec2(10,10); }
    vec2 trim_norm () { return vec2(trim().x, trim().y) / vec2(getWindowWidth(), getWindowHeight()); }
    inline ivec2 desired_window_size () { return canvas_size () + trim () + trim (); }
    
    inline ivec2 plot_frame_size () { return ivec2 (canvas_size().x, canvas_size().y); }
    inline Rectf plot_rect () { return Rectf(trim(), trim() + plot_frame_size()); }
    
    
};

class ColorSpaceViewerApp : public App {
public:
    void setup() override;
    void update() override;
    void draw() override;
    void first_window_draw();
    void fileDrop( FileDropEvent event ) override;
    void windowClose();
    void windowMouseDown( MouseEvent &mouseEvt );
    
    void mouseDown( MouseEvent event )override;
    void mouseMove( MouseEvent event )override;
    void mouseUp( MouseEvent event )override;
    void mouseDrag( MouseEvent event )override;
    void keyDown( KeyEvent event )override;
    
    bool shouldQuit();
    
    std::vector<ci::Color8u> mColorList;
    std::vector<vec2> mLogChrom;
    col::result_pair_t m_results;
    
    col::AsyncPalette mAsyncPalette;
    
    CameraPersp		mCam;
    CameraUi		mCamUi;
    ci::app::WindowRef createConnectedWindow (Window::Format& format);
    
    ci::app::WindowRef mSecondWindow;
    std::shared_ptr<LogColorWindow> mLogColorWindow;
    
};



ci::app::WindowRef ColorSpaceViewerApp::createConnectedWindow(Window::Format& format)
{
    ci::app::WindowRef win = createWindow( format );
    win->getSignalClose().connect( std::bind( &ColorSpaceViewerApp::windowClose, this ) );
    win->getSignalMouseDown().connect( std::bind( &ColorSpaceViewerApp::windowMouseDown, this, std::placeholders::_1 ) );
    //    win->getSignalDisplayChange().connect( std::bind( &ColorSpaceViewerApp::update, this ) );
    return win;
    
}


bool ColorSpaceViewerApp::shouldQuit()
{
    return true;
}


void ColorSpaceViewerApp::setup()
{
    for( auto display : Display::getDisplays() )
        CI_LOG_V( "display name: '" << display->getName() << "', bounds: " << display->getBounds() );
    
    
    std::string title = "Drop an Image file and use wheel to zoom";
    getWindow()->setTitle(title);
    mCamUi = CameraUi( &mCam, getWindow() );
    mCam.lookAt( vec3( 0.2f, 0.4f, 1.0f ), vec3(0.0f, 0.425f, 0.0f) );
    mCam.setNearClip( 0.01f );
    mCam.setFarClip( 100.0f );
    
    Window::Format format( RendererGl::create() );
    mSecondWindow = createConnectedWindow(format);
    mLogColorWindow = std::make_shared<LogColorWindow>();
    mSecondWindow->setUserData(mLogColorWindow.get());
    mLogColorWindow->mDisWindow = mSecondWindow;
    mLogColorWindow->setup();
    
}

void ColorSpaceViewerApp::windowMouseDown( MouseEvent &mouseEvt )
{
    ci::app::WindowRef win = getWindow();
    CI_LOG_V( "Closing " << getWindow() );
    
}

void ColorSpaceViewerApp::windowClose()
{
    ci::app::WindowRef win = getWindow();
    CI_LOG_V( "Closing " << getWindow() );
}

void ColorSpaceViewerApp::mouseMove( MouseEvent event )
{
    auto  *data = getWindow()->getUserData<LogColorWindow>();
    if(data)
        data->mouseMove(event);
    else
        cinder::app::App::mouseMove(event);
    
}


void ColorSpaceViewerApp::mouseDrag( MouseEvent event )
{
    auto  *data = getWindow()->getUserData<LogColorWindow>();
    if(data)
        data->mouseDrag(event);
    else
        cinder::app::App::mouseDrag(event);
}


void ColorSpaceViewerApp::mouseDown( MouseEvent event )
{
    auto  *data = getWindow()->getUserData<LogColorWindow>();
    if(data)
        data->mouseDown(event);
    else
        cinder::app::App::mouseDown(event);
}


void ColorSpaceViewerApp::mouseUp( MouseEvent event )
{
    auto  *data = getWindow()->getUserData<LogColorWindow>();
    if(data)
        data->mouseUp(event);
    else
        cinder::app::App::mouseUp(event);
}


void ColorSpaceViewerApp::keyDown( KeyEvent event )
{
    auto  *data = getWindow()->getUserData<LogColorWindow>();
    if(data)
        data->keyDown(event);
    {
        if( event.getChar() == 'f' ) {
            // Toggle full screen when the user presses the 'f' key.
            setFullScreen( ! isFullScreen() );
        }
        else if( event.getCode() == KeyEvent::KEY_ESCAPE ) {
            // Exit full screen, or quit the application, when the user presses the ESC k/Users/arman/Pictures/hue.pngey.
            if( isFullScreen() )
                setFullScreen( false );
            else
                quit();
        }
    }
    
}


void ColorSpaceViewerApp::update()
{
    if( is_ready( mAsyncPalette ) ) {
        m_results = mAsyncPalette.get();
        std::pair<float,float>& mmx = std::get<2>(m_results);
        std::pair<float,float>& mmy = std::get<3>(m_results);
        mmx.first -= 0.5;mmx.second += 0.5;
        mmy.first -= 0.5;mmy.second += 0.5;
        vec2 scale (mmx.first - mmx.second + 1.0f,
                    mmy.first - mmy.second + 1.0f);
        vec2 ctr (0.5, 0.5);
        mLogColorWindow->setXform(ctr,scale);
        mLogColorWindow->mLogChrom = std::get<1>(m_results);
        mColorList = std::get<0>(m_results);
        mLogColorWindow->createPlot ();
    }
}


void ColorSpaceViewerApp::first_window_draw()
{
    gl::clear();
    gl::enableDepthRead();
    gl::enableDepthWrite();
    
    // set 3D camera matrices
    gl::pushMatrices();
    gl::setMatrices( mCam );
    
    auto lambert = gl::ShaderDef().lambert().color();
    auto shader = gl::getStockShader( lambert );
    shader->bind();
    
    gl::drawCoordinateFrame ();
    
    for( auto& color : mColorList ) {
        
        vec3 offsett( color.r / 256.0, color.g/ 256.0, color.b/256.0);
        gl::pushModelMatrix();
        gl::translate( offsett );
        gl::color( Color ( CM_RGB, color.r / 256.0, color.g/ 256.0, color.b/256.0));
        gl::drawSphere( vec3(), 0.067f);
        gl::popModelMatrix();
    }
    
    // restore 2D rendering
    gl::popMatrices();
    gl::disableDepthWrite();
    gl::disableDepthRead();
    
}

void ColorSpaceViewerApp::draw ()
{
    if (getWindow() == mSecondWindow)
        mLogColorWindow->draw();
    else
        first_window_draw ();
}

col::result_pair_t  getPalette( const fs::path& imageFile, size_t nb, bool random )
{
    try {
        col::PaletteGenerator generator( Surface8u( loadImage( loadFile( imageFile ) ) ) );
        const auto clist = generator.randomPalette(256);
        //        const auto clist = generator.colorList();
        return std::make_tuple (clist, generator.logChrom(), generator.MinMaxX(), generator.MinMaxY());
    }
    catch( ... ) {
        app::console() << "Palette generation failed." << std::endl;
    }
    return {};
}

void ColorSpaceViewerApp::fileDrop( FileDropEvent event )
{
    auto file = event.getFile(0);
    mAsyncPalette = std::async(std::launch::async, getPalette, file, 256, false );
    
}

CINDER_APP( ColorSpaceViewerApp, RendererGl )
