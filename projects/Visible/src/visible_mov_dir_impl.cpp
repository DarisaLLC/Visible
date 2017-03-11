
#include <stdio.h>
#include "VisibleApp.h"
#include "guiContext.h"
#include "core/stl_utils.hpp"
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
#include "directoryPlayer.h"
#include <strstream>

#include "gradient.h"

using namespace ci;
using namespace ci::app;
using namespace std;


static boost::filesystem::path browseToFolder ()
{
    return Platform::get()->getFolderPath (Platform::get()->getHomeDirectory());
}

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
    
    inline ivec2 expected_window_size () { return vec2 (960, 540); }
    inline ivec2 trim () { return vec2(10,10); }
    vec2 trim_norm () { return vec2(trim().x, trim().y) / vec2(getWindowWidth(), getWindowHeight()); }
    inline ivec2 desired_window_size () { return expected_window_size() + trim () + trim (); }
    inline ivec2 canvas_size () { return getWindowSize() - trim() - trim (); }
    inline ivec2 image_frame_size () { return ivec2 (canvas_size().x / 2, (canvas_size().y * 3) / 4); }
    inline ivec2 plot_frame_size () { return ivec2 (canvas_size().x / 2, (canvas_size().y * 3) / 4); }
    inline vec2 canvas_norm_size () { return vec2(canvas_size().x, canvas_size().y) / vec2(getWindowWidth(), getWindowHeight()); }
    inline Rectf text_norm_rect () { return Rectf(0.0, 1.0 - 0.125, 1.0, 0.125); }
    
    
    
}


/////////////  movDirContext Implementation  ////////////////



movDirContext::movDirContext(WindowRef& ww, const boost::filesystem::path& dp)
: guiContext(ww), mPath (dp)
{
    m_valid = false;
    m_type = Type::movie_dir_viewer;
    
    if (dp.string().empty())
    {
        auto dirPath = browseToFolder ();
        mPath = dirPath;
    }
    
    m_valid = ! source_path().string().empty() && exists(source_path());
    
    setup ();
    if (is_valid())
    {
        mWindow->setTitle( source_path().filename().string() );
        
        mWindow->setSize(desired_window_size());
        mFont = Font( "Menlo", 12 );
        mSize = vec2( getWindowWidth(), getWindowHeight() / 12);
        
    }
    
    set_dir_info ();
    
}

void movDirContext::set_dir_info (const std::string extension , double fps, const std::string anonymous_format )
{
    m_extension = extension;
    m_anonymous_format = anonymous_format;
    m_fps = fps;
}

const  boost::filesystem::path movDirContext::source_path () const
{
    return mPath;
}

void movDirContext::seekToStart()
{
    if ( have_movie () ) m_Dm->seekToStart ();
}


void movDirContext::seekToEnd()
{
    if ( have_movie () ) m_Dm->seekToEnd ();
}

void movDirContext::seekToFrame ( const size_t frame )
{
    if (! have_movie () ) return;
    pause ();
    m_Dm->seekToFrame(frame % m_Dm->getNumFrames());
}

double  movDirContext::getDuration() const
{
    return m_Dm->getDuration ();
}

size_t movDirContext::getNumFrames () const
{
    return m_Dm->getNumFrames ();
}

void movDirContext::play ()
{
    if (! have_movie() || mMoviePlay ) return;
    mMoviePlay = true;
}

void movDirContext::pause ()
{
    if (! have_movie() || ! mMoviePlay ) return;
    mMoviePlay = false;
}

void movDirContext::play_pause_button ()
{
    if (! have_movie () ) return;
    if (mMoviePlay)
        pause ();
    else
        play ();
}


void movDirContext::loop_no_loop_button ()
{
    if (! have_movie () ) return;
    if (m_Dm->looping())
        m_Dm->looping(false);
    else
        m_Dm->looping(true);
}



bool movDirContext::have_movie () const
{
    return m_Dm != nullptr && m_valid;
}

size_t movDirContext::getCurrentFrame () const
{
    return this->have_movie() ? m_Dm->getCurrentFrame () : -1;
}

void movDirContext::onMarked ( marker_info& t)
{
    pause ();
    seekToFrame ((size_t)(t.norm_pos.x *= m_Dm->getNumFrames()));
    
    
}

vec2 movDirContext::getZoom ()
{
    return m_zoom;
}

void movDirContext::setZoom (vec2 zoom)
{
    m_zoom = zoom;
    update ();
}


void movDirContext::setup()
{
    
    clear_movie_params();
    
    loadMovieFile();
    
    if( m_Dm && m_valid )
    {
        
        getWindow()->setTitle( mPath.filename().string() );
        mMovieParams = params::InterfaceGl( "Directory Player", vec2( getWindowWidth()/2, getWindowHeight()/2 ) );
        
        
        std::string max = to_string( m_Dm->getDuration() );
        {
            const std::function<void (int)> setter = std::bind(&directoryPlayer::seekToFrame, m_Dm, std::placeholders::_1);
            const std::function<int ()> getter = std::bind(&directoryPlayer::getCurrentFrame, m_Dm);
            mMovieParams.addParam (" Mark ", setter, getter);
        }
        
        mMovieParams.addSeparator();
        mMovieParams.addButton(" Play / Pause ", bind( &movDirContext::play_pause_button, this ) );
        mMovieParams.addSeparator();
        mMovieParams.addButton(" Loop ", bind( &movDirContext::loop_no_loop_button, this ) );
        
        
    }
}

void movDirContext::clear_movie_params ()
{
    seekToStart ();
    mMoviePlay = false;
    mMovieLoop = false;
    m_zoom.x = m_zoom.y = 1.0f;
}



void movDirContext::loadMovieFile()
{
    if ( ! mPath.empty () )
    {
        ci_console () << mPath.string ();
        
        try {
            
            
            m_Dm = directoryPlayer::create (source_path(), m_extension, getWindow()->getApp()->getFrameRate() , m_anonymous_format);
            
            if (m_valid)
            {
                
                std::strstream msg;
                msg << "Duration:  " <<getDuration() << " seconds" << std::endl;
                msg << "Frames:    " <<getNumFrames() << std::endl;
                msg << "Framerate: " <<getFrameRate() << std::endl;
             //   update_log(msg.str());
                
                m_Dm->setPlayRate (1.0);
                ivec2 window_size (desired_window_size());
                setWindowSize(window_size);
                seekToStart();
                
            }
        }
        catch( ... ) {
            ci_console() << "Unable to load the movie." << std::endl;
            return;
        }
        
    }
}

void movDirContext::mouseDown( MouseEvent event )
{
    mMetaDown = event.isMetaDown();
    mMouseIsDown = true;
}

void movDirContext::mouseMove( MouseEvent event )
{
    mMousePos = event.getPos();
}

void movDirContext::mouseDrag( MouseEvent event )
{
    mMousePos = event.getPos();
    mMouseIsDragging  = true;
}


void movDirContext::keyDown( KeyEvent event )
{
    if( event.getChar() == 'f' ) {
        setFullScreen( ! isFullScreen() );
    }
    else if( event.getChar() == 'o' ) {
        loadMovieFile();
    }
    
    // these keys only make sense if there is an active movie
    if( m_Dm ) {
        if( event.getCode() == KeyEvent::KEY_LEFT ) {
            pause();
            seekToFrame(getCurrentFrame () - 1);
        }
        if( event.getCode() == KeyEvent::KEY_RIGHT ) {
            pause ();
            seekToFrame (getCurrentFrame() + 1);
        }
        
        
        if( event.getChar() == ' ' ) {
            play_pause_button();
        }
        
    }
}



void movDirContext::mouseUp( MouseEvent event )
{
    mMouseIsDown = false;
    mMouseIsDragging = false;
}






void movDirContext::resize ()
{
    mSize = vec2( getWindowWidth(), getWindowHeight() / 12);
}
void movDirContext::update ()
{
    if (! have_movie () )
        return;
    
    // Update text texture with most recent text
    auto num_str = to_string( int( m_Dm->getCurrentFrame ()));
    std::string frame_str = mMoviePlay ? string( "Playing: " + num_str) : string ("Paused: " + num_str);
    update_log (frame_str);
    
    if (mMoviePlay )
        m_Dm->update ();
}

void movDirContext::draw ()
{
    if (! have_movie () )
        return;
    
    Rectf dr = get_image_display_rect();
    
    m_Dm->draw (dr);
    
    draw_info ();
    mMovieParams.draw();
    
}

void movDirContext::update_log (const std::string& msg)
{
    if (msg.length() > 2)
        mLog = msg;
    TextBox tbox = TextBox().alignment( TextBox::RIGHT).font( mFont ).size( mSize ).text( mLog );
    tbox.setColor( Color( 1.0f, 0.65f, 0.35f ) );
    tbox.setBackgroundColor( ColorA( 0.3f, 0.3f, 0.3f, 0.4f )  );
//    ivec2 sz = tbox.measure();
    mTextTexture = gl::Texture2d::create( tbox.render() );
}

Rectf movDirContext::get_image_display_rect ()
{
    ivec2 ivf = image_frame_size();
    ivec2 tl = trim();
    
    ivec2 lr = tl + ivf;
    return Rectf (tl, lr);
}


void movDirContext::draw_info ()
{
    if (! m_Dm || ! m_Dm->getTexture () ) return;
    
    // Setup for putting text on
    
    gl::setMatricesWindow( getWindowSize() );
    gl::ScopedBlendAlpha blend_;
    
    // Adding Text on the Right
    //    TextLayout layoutR;
    //
    //    layoutR.clear( ColorA::gray( 0.2f, 0.5f ) );
    //    layoutR.setFont( Font( "Arial", 18 ) );
    //    layoutR.setColor( Color::white() );
    //    layoutR.setLeadingOffset( 3 );
    //    layoutR.addRightLine( other_str  );
    //    auto texR = gl::Texture::create( layoutR.render( true ) );
    //    gl::draw( texR, vec2( 10, 10 ) );
    
    
    if (mTextTexture)
    {
        Rectf textrect (0.0, getWindowHeight() - mTextTexture->getHeight(), getWindowWidth(), getWindowHeight());
        gl::draw(mTextTexture, textrect);
    }
    
}



#if 0

////////   Adding Tracks


// Create a clip viewer. Go through container of viewers, if there is a movie view, connect onMarked signal to it
void movDirContext::add_scalar_track(const boost::filesystem::path& path)
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
    new_ts->signalMarker.connect(std::bind(&movDirContext::onMarked, static_cast<movDirContext*>(this), std::placeholders::_1));
    VisibleCentral::instance().contexts().push_back(new_ts);
    
    //    VisWinMgr::key_t kk;
    //    bool kept = VisWinMgr::instance().makePair(win, new_ts, kk);
    //    ci_console() << "Time Series Window/Context registered: " << std::boolalpha << kept << std::endl;
    
}






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
    std::shared_ptr<guiContext> mw(new movDirContext(createWindow( Window::Format().size(mMovieDisplayRect.getSize()))));
    
    if (! mw->is_valid()) return;
    
    for (std::shared_ptr<guiContext> uip : mContexts)
    {
        if (uip->is_context_type(guiContext::Type::clip_viewer))
        {
            uip->signalMarker.connect(std::bind(&movDirContext::onMarked, static_cast<movDirContext*>(mw.get()), std::placeholders::_1));
            mw->signalMarker.connect(std::bind(&clipContext::onMarked, static_cast<clipContext*>(uip.get()), std::placeholders::_1));
        }
    }
    mContexts.push_back(mw);
}

#endif
