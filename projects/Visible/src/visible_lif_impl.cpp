//
//  visible_mov_impl.cpp
//  Visible
//
//  Created by Arman Garakani on 5/13/16.
//
//

#include <stdio.h>
#include "VisibleApp.h"
#include "guiContext.h"
#include "stl_util.hpp"
#include "cinder/app/App.h"
#include "cinder/gl/gl.h"
#include "cinder/Timeline.h"
#include "cinder/Timer.h"
#include "cinder/Camera.h"
#include "cinder/qtime/Quicktime.h"
#include "cinder/params/Params.h"
#include "cinder/ImageIo.h"
#include "qtime_frame_cache.hpp"
#include "CinderOpenCV.h"
#include "Cinder/ip/Blend.h"
#include "opencv2/highgui.hpp"
#include "vision/opencv_utils.hpp"
#include "cinder/ip/Flip.h"
#include "otherIO/lifFile.hpp"


using namespace ci;
using namespace ci::app;
using namespace std;

extern float  MovieBaseGetCurrentTime(cinder::qtime::MovieSurfaceRef& movie);

namespace
{
    
    
    std::ostream& ci_console ()
    {
        return AppBase::get()->console();
    }
    
    double				ci_getElapsedSeconds()
    {
        return AppBase::get()->getElapsedSeconds();
    }


    inline ivec2 trim () { return vec2(10,10); }
    vec2 trim_norm () { return vec2(trim().x, trim().y) / vec2(getWindowWidth(), getWindowHeight()); }
    inline ivec2 desired_window_size () { return ivec2 (960 + 2 * trim().x, 540 + 2 * trim().y); }
    inline ivec2 canvas_size () { return getWindowSize() - trim() - trim (); }
    inline ivec2 image_frame_size () { return ivec2 ((canvas_size().x * 2) / 3, (canvas_size().y * 3) / 4); }
    inline ivec2 plot_frame_size () { return ivec2 ((canvas_size().x * 1) / 3, (canvas_size().y * 3) / 4); }
    inline vec2 canvas_norm_size () { return vec2(canvas_size().x, canvas_size().y) / vec2(getWindowWidth(), getWindowHeight()); }
    inline Rectf text_norm_rect () { return Rectf(0.0, 1.0 - 0.125, 1.0, 0.125); }
    
    
}


/////////////  lifContext Implementation  ////////////////

lifContext::lifContext(WindowRef& ww, const boost::filesystem::path& dp)
: guiContext(ww), mPath (dp)
{
    m_valid = false;
    m_type = Type::lif_file_viewer;
    
    if (mPath.string().empty())
        mPath = getOpenFilePath();

    m_valid = ! mPath.string().empty() && exists(mPath);
    
    if (is_valid())
    {
        mWindow->setTitle( mPath.filename().string() );
        mWindow->setSize(desired_window_size());
        mFont = Font( "Menlo", 32 );
        mSize = vec2( getWindowWidth(), getWindowHeight() / 12);
    }

    setup ();
}





void lifContext::play ()
{
    if (! have_movie() || mMoviePlay ) return;
    mMoviePlay = true;
}

void lifContext::pause ()
{
    if (! have_movie() || ! mMoviePlay ) return;
    mMoviePlay = false;
}

void lifContext::play_pause_button ()
{
    if (! have_movie () ) return;
    if (mMoviePlay)
        pause ();
    else
        play ();
}

void lifContext::onMarked ( marker_info& t)
{
    pause ();
    setIndex((int)(t.norm_pos.x *= m_fc));
    
    
}

bool lifContext::have_movie ()
{
    return m_lifRef && m_selected_serie >= 0 && m_selected_serie < m_lifRef->getNbSeries() && mFrameSet;
}

int lifContext::getIndex ()
{
    if (m_fc)
        return mMovieIndexPosition % m_fc;
    return m_fc;
}

bool lifContext::incrementIndex()
{
    return setIndex(getIndex()+1);
    
}

bool lifContext::setIndex (int mark)
{
    bool lastc = mMovieIndexPosition == m_fc - 1;
    mMovieIndexPosition = (mark % m_fc);
    return lastc;
}

vec2 lifContext::getZoom ()
{
    return m_zoom;
}

void lifContext::setZoom (vec2 zoom)
{
    m_zoom = zoom;
    update ();
}


void lifContext::setup()
{
    mMovieParams = params::InterfaceGl( "Lif Player ", vec2( 260, 260) );
    mMovieParams.setPosition(getWindowSize() / 2);

    
    // Load the validated movie file
    loadLifFile ();
    
    clear_movie_params();
    
    if( m_valid )
    {
       	m_type = Type::qtime_viewer;
        mMovieParams.addSeparator();

        m_series_names.clear ();
        for (auto ss = 0; ss < m_series_book.size(); ss++)
            m_series_names.push_back (m_series_book[ss].name);
        
        
        // Add an enum (list) selector.
        m_selected_serie = 0;
        mMovieParams.addParam( "Series ", m_series_names, &m_selected_serie )
//        .keyDecr( "[" )
//        .keyIncr( "]" )
        .updateFn( [this] { console() << "selected serie updated: " << m_series_names [m_selected_serie] << endl; loadCurrentSerie (); } );
        
        
        mMovieParams.addSeparator();
        {
            const std::function<void (int)> setter = std::bind(&lifContext::setIndex, this, std::placeholders::_1);
            const std::function<int ()> getter = std::bind(&lifContext::getIndex, this);
            mMovieParams.addParam ("Current Time Step", setter, getter);
        }
            
        mMovieParams.addSeparator();
        mMovieParams.addButton("Play / Pause ", bind( &lifContext::play_pause_button, this ) );
        
//        {
//            const std::function<void (float)> setter = std::bind(&lifContext::setZoom, this, std::placeholders::_1);
//            const std::function<float (void)> getter = std::bind(&lifContext::getZoom, this);
//            mMovieParams.addParam( "Zoom", setter, getter);
//        }
        
    }
}

void lifContext::clear_movie_params ()
{
    mMoviePosition = 0.0f;
    mPrevMoviePosition = mMoviePosition;
    mMovieIndexPosition = 0;
    mPrevMovieIndexPosition = -1;
    mMovieRate = 1.0f;
    mPrevMovieRate = mMovieRate;
    mMoviePlay = false;
    mPrevMoviePlay = mMoviePlay;
    mMovieLoop = false;
    mPrevMovieLoop = mMovieLoop;
    m_zoom.x = m_zoom.y = 1.0f;
}


void lifContext::loadLifFile ()
{
    if ( ! mPath.empty () )
    {
        ci_console () << mPath.string ();
        
        try {
            
            m_lifRef =  std::shared_ptr<lifIO::LifReader> (new lifIO::LifReader (mPath.string()));
            get_series_info (m_lifRef);
            
        }
        catch( ... ) {
            ci_console() << "Unable to load the movie." << std::endl;
            return;
        }
        
    }
}


void lifContext::loadCurrentSerie ()
{
    if ( ! (m_lifRef && m_selected_serie >= 0 && m_selected_serie < m_lifRef->getNbSeries()))
        return;
        
    try {
            
            m_serie = m_series_book[m_selected_serie];
            mFrameSet = qTimeFrameCache::create (m_lifRef->getSerie(m_selected_serie));
       
        
            std::stringstream str;
        
            std::cout << m_serie << std::endl;
        
            if (m_valid)
            {
                getWindow()->setTitle( mPath.filename().string() );
        
                ci_console() <<  m_series_book.size() << "  Series  " << std::endl;

                const tiny_media_info tm = mFrameSet->media_info ();
                getWindow()->getApp()->setFrameRate(tm.getFramerate() / 5.0);

                mScreenSize = tm.getSize();
                m_fc = tm.getNumFrames ();
                
                mSurface = Surface8u::create (int32_t(mScreenSize.x), int32_t(mScreenSize.y), true);
                
                // Set window size according to layout
                //  Channel             Data
                //  1
                //  2
                //  3
                //  Create 2x mag for images
                ivec2 window_size (desired_window_size());
                setWindowSize(window_size);
                texture_to_display_zoom();
                
            }
        }
    catch( const std::exception &ex ) {
        console() << ex.what() << endl;
          return;
    }
}

void lifContext::mouseDown( MouseEvent event )
{
    mMetaDown = event.isMetaDown();
    mMouseIsDown = true;
}

void lifContext::mouseMove( MouseEvent event )
{
    mMousePos = event.getPos();
}

void lifContext::mouseDrag( MouseEvent event )
{
    mMousePos = event.getPos();
    mMouseIsDragging  = true;
}


void lifContext::keyDown( KeyEvent event )
{
    if( event.getChar() == 'f' ) {
        setFullScreen( ! isFullScreen() );
    }
    else if( event.getChar() == 'o' ) {
        loadLifFile();
    }
    
    // these keys only make sense if there is an active movie
    if( have_movie () ) {
        if( event.getCode() == KeyEvent::KEY_LEFT ) {
            pause();
           setIndex (mMovieIndexPosition - 1);
        }
        if( event.getCode() == KeyEvent::KEY_RIGHT ) {
            pause ();
            setIndex (mMovieIndexPosition + 1);
        }
        
        
        if( event.getChar() == ' ' ) {
            play_pause_button();
        }
        
    }
}



void lifContext::mouseUp( MouseEvent event )
{
    mMouseIsDown = false;
    mMouseIsDragging = false;
}

void  lifContext::update_log (const std::string& msg)
{
    if (msg.length() > 2)
        mLog = msg;
    TextBox tbox = TextBox().alignment( TextBox::RIGHT).font( mFont ).size( mSize ).text( mLog );
    tbox.setColor( Color( 1.0f, 0.65f, 0.35f ) );
    tbox.setBackgroundColor( ColorA( 0.3f, 0.3f, 0.3f, 0.4f )  );
    ivec2 sz = tbox.measure();
    mTextTexture = gl::Texture2d::create( tbox.render() );
}

Rectf lifContext::get_image_display_rect ()
{
    ivec2 ivf = image_frame_size();
    ivec2 tl = trim();
    
    if (m_serie.channelCount == 1)
        ivf.y /= 3;
    
    ivec2 lr = tl + ivf;
    return Rectf (tl, lr);
}

Rectf lifContext::get_plotting_display_rect ()
{
    ivec2 pvf = plot_frame_size();
    ivec2 tl = trim();
    tl.x += image_frame_size().x;
    
    if (m_serie.channelCount == 1)
        pvf.y /= 3;
    
    ivec2 lr = tl + pvf;
    return Rectf (tl, lr);

}

vec2 lifContext::texture_to_display_zoom()
{
    Rectf image (0.0f, 0.0f, mScreenSize.x, mScreenSize.y);
    Rectf window = getWindowBounds();
    
    // Trim display window

    float sx =  image.getWidth() / window.getWidth();
    float sy = image.getHeight() / window.getHeight();
    
    float aspect_same = 2.0f / (sx + sy);
    
    
    float w = window.getWidth() * aspect_same;
    float h = window.getHeight() * aspect_same;
    float ox = (window.getWidth() - w) / 2.0;
    float oy = (window.getHeight() - h) / 2.0;
    image.set(20, 20, ox + w, oy + h);
    m_display_rect = image;

    return vec2(aspect_same, aspect_same);
}

void lifContext::seek( size_t xPos )
{
    if (is_valid()) mMovieIndexPosition = lifContext::Normal2Index ( getWindowBounds(), xPos, m_fc);
}


bool lifContext::is_valid () { return m_valid && is_context_type(guiContext::lif_file_viewer); }

void lifContext::resize ()
{
    mSize = vec2( getWindowWidth(), getWindowHeight() / 12);
    
  //  if (! have_movie () ) return;
//    m_zoom = texture_to_display_zoom();
    
}
void lifContext::update ()
{
    if (! have_movie () ) return;
    
     mSurface = mFrameSet->getFrame(getIndex());

    if (mMoviePlay) incrementIndex ();
    
}

void lifContext::draw_info ()
{
    if (! m_lifRef) return;

    std::string frame_str = string( "Frame: " + to_string( int( getIndex() ) ) );
    std::string seri_str = m_serie.info();
    
    gl::setMatricesWindow( getWindowSize() );
    
    gl::ScopedBlendAlpha blend_;
    TextLayout layoutR;
    
    layoutR.clear( ColorA::gray( 0.2f, 0.5f ) );
    layoutR.setFont( Font( "Arial", 18 ) );
    layoutR.setColor( Color::white() );
    layoutR.setLeadingOffset( 3 );
    layoutR.addRightLine( seri_str  );
    update_log (frame_str);
    
    auto texR = gl::Texture::create( layoutR.render( true ) );
    gl::draw( texR, vec2( 10, 10 ) );
    
    if (mTextTexture)
    {
        Rectf textrect (0.0, getWindowHeight() - mTextTexture->getHeight(), getWindowWidth(), getWindowHeight());
        gl::draw(mTextTexture, textrect);
    }
        
}


void lifContext::draw ()
{


    if( have_movie()  && mSurface )
    {
        Rectf dr = get_image_display_rect();
        
        switch(m_serie.channelCount)
        {
            case 1:
        mImage = gl::Texture::create(*mSurface);
        mImage->setMagFilter(GL_NEAREST_MIPMAP_NEAREST);
        gl::draw (mImage, dr);
                break;
            case 3:
                mImage = gl::Texture::create(*mSurface);
                mImage->setMagFilter(GL_NEAREST_MIPMAP_NEAREST);
                gl::draw (mImage, dr);
                break;
        }
        draw_info ();
    }
    mMovieParams.draw();
    
    
}



////////   Adding Tracks 


// Create a clip viewer. Go through container of viewers, if there is a movie view, connect onMarked signal to it
void lifContext::add_scalar_track(const boost::filesystem::path& path)
{
    Window::Format format( RendererGl::create() );
    WindowRef win = VisibleCentral::instance().getConnectedWindow(format);
    std::shared_ptr<clipContext> new_ts ( new clipContext (win, path));
    auto parent_size = mWindow->getSize();
    auto parent_pos = mWindow->getPos();
    parent_pos.y += parent_size.y;
    parent_size.y = 100;
    win->setSize (parent_size);
    win->setPos (parent_pos);
    signalMarker.connect(std::bind(&clipContext::onMarked, static_cast<clipContext*>(new_ts.get()), std::placeholders::_1));
    new_ts->signalMarker.connect(std::bind(&lifContext::onMarked, static_cast<lifContext*>(this), std::placeholders::_1));
    VisibleCentral::instance().contexts().push_back(new_ts);
    
//    VisWinMgr::key_t kk;
//    bool kept = VisWinMgr::instance().makePair(win, new_ts, kk);
//    ci_console() << "Time Series Window/Context registered: " << std::boolalpha << kept << std::endl;

}





#if 0
    std::shared_ptr<guiContext> cw(std::shared_ptr<guiContext>(new clipContext(createWindow( Window::Format().size(mGraphDisplayRect.getSize())))));
    
    if (! cw->is_valid()) return;
    
    for (std::shared_ptr<guiContext> uip : mContexts)
    {
        if (uip->is_context_type(guiContext::Type::qtime_viewer))
        {
            uip->signalMarker.connect(std::bind(&clipContext::onMarked, static_cast<clipContext*>(cw.get()), std::placeholders::_1));
            cw->signalMarker.connect(std::bind(&clipContext::onMarked, static_cast<clipContext*>(uip.get()), std::placeholders::_1));
        }
    }
    mContexts.push_back(cw);
}

// Create a movie viewer. Go through container of viewers, if there is a clip view, connect onMarked signal to it
void VisibleApp::create_qmovie_viewer ()
{
    std::shared_ptr<guiContext> mw(new lifContext(createWindow( Window::Format().size(mMovieDisplayRect.getSize()))));
    
    if (! mw->is_valid()) return;
    
    for (std::shared_ptr<guiContext> uip : mContexts)
    {
        if (uip->is_context_type(guiContext::Type::clip_viewer))
        {
            uip->signalMarker.connect(std::bind(&lifContext::onMarked, static_cast<lifContext*>(mw.get()), std::placeholders::_1));
            mw->signalMarker.connect(std::bind(&clipContext::onMarked, static_cast<clipContext*>(uip.get()), std::placeholders::_1));
        }
    }
    mContexts.push_back(mw);
}

#endif
