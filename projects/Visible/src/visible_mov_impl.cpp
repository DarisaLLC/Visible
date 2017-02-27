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
#include "opencv_utils.hpp"
#include "cinder/ip/Flip.h"
#include <strstream>
#include "gradient.h"

using namespace ci;
using namespace ci::app;
using namespace std;

extern float  MovieBaseGetCurrentTime(cinder::qtime::MovieSurfaceRef& movie);

/// Layout for this widget. Poor design !!

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
    inline vec2& image_frame_size_norm (){ static vec2 ni (0.67, 0.75); return ni;}
    inline ivec2 image_frame_size ()
    {
        static vec2& np = image_frame_size_norm ();
        return ivec2 (np.x * expected_window_size().x, np.y * expected_window_size().y);
    }
    
    
    inline vec2 plots_frame_position_norm ()
    {
        vec2 np = vec2 (image_frame_size_norm().x + trim_norm().x, trim_norm().y);
        return np;
    }
    inline vec2 image_frame_position_norm ()
    {
        vec2 np = vec2 (trim_norm().x, trim_norm().y);
        return np;
    }
    
    inline vec2 plots_frame_position ()
    {
        vec2 np = vec2 (plots_frame_position_norm().x * expected_window_size().x, plots_frame_position_norm().y * expected_window_size().y);
        return np;
    }
    inline vec2 image_frame_position ()
    {
        vec2 np = vec2 (image_frame_position_norm().x * expected_window_size().x, image_frame_position_norm().y * expected_window_size().y);
        return np;
    }
    
    
    inline vec2 single_plot_size_norm (){ vec2 np = vec2 (1.0 - image_frame_size_norm().x, 0.25); return np;}
    inline vec2 plots_frame_size_norm (){ vec2 np = vec2 (1.0 - image_frame_size_norm().x,
                                                          3 * single_plot_size_norm().y); return np;}
    
    
    inline ivec2 plots_frame_size () { return ivec2 ((canvas_size().x * plots_frame_size_norm().x),
                                                     (canvas_size().y * plots_frame_size_norm().y)); }
    
    inline ivec2 single_plot_size () { return ivec2 ((canvas_size().x * single_plot_size_norm().x),
                                                     (canvas_size().y * single_plot_size_norm().y)); }
    
    inline Rectf image_frame_rect ()
    {
        vec2 tl = image_frame_position();
        vec2 br = vec2 (tl.x + image_frame_size().x, tl.y + image_frame_size().y);
        
        return Rectf (tl, br);
    }
    
    inline vec2 canvas_norm_size () { return vec2(canvas_size().x, canvas_size().y) / vec2(getWindowWidth(), getWindowHeight()); }
    inline Rectf text_norm_rect () { return Rectf(0.0, 1.0 - 0.125, 1.0, 0.125); }
    inline void plot_rects (std::vector<Rectf>& plots )
    {
        plots.resize(3);
        auto plot_tl = plots_frame_position();
        auto plot_size = single_plot_size();
        
        plots[0] = Rectf (plot_tl, vec2 (plot_tl.x + plot_size.x, plot_tl.y + plot_size.y));
        plots[1] = Rectf (vec2(plots[0].getUpperLeft().x, plots[0].getUpperLeft().y + plot_size.y),
                          vec2(plots[0].getLowerRight().x, plots[0].getLowerRight().y + plot_size.y));
        plots[2] = Rectf (vec2(plots[1].getUpperLeft().x, plots[1].getUpperLeft().y + plot_size.y),
                          vec2(plots[1].getLowerRight().x, plots[1].getLowerRight().y + plot_size.y));
    }
    
}


/////////////  movContext Implementation  ////////////////

movContext::movContext(WindowRef& ww, const boost::filesystem::path& dp)
: visualContext(ww), mPath (dp)
{
    m_valid = false;
    m_type = Type::qtime_viewer;
    
    if (mPath.string().empty())
        mPath = getOpenFilePath();

    m_valid = ! mPath.string().empty() && exists(mPath);
    
    if (is_valid())
    {
        mWindow->setTitle( mPath.filename().string() );
        mWindow->setSize(desired_window_size());
        mFont = Font( "Menlo", 12 );
        mSize = vec2( getWindowWidth(), getWindowHeight() / 12);
    }
    setup ();
}



const  boost::filesystem::path movContext::source_path () const
{
    return mPath;
}



void movContext::looping (bool what)
{
    m_movie->setLoop(what);
    mMovieLoop = what;
}


bool movContext::looping ()
{
    return mMovieLoop;
}

void movContext::play ()
{
    if (! m_movie->isPlaying() )
        m_movie->play ();
}

void movContext::pause ()
{
    if (m_movie->isPlaying())
        m_movie->stop ();
}

void movContext::play_pause_button ()
{
    if (! have_movie () ) return;
    if (m_movie->isPlaying())
        pause ();
    else
        play ();
}

void movContext::loop_no_loop_button ()
{
    if (! have_movie () || ! m_movie->isPlaying()) return;
    if (looping())
        looping(false);
    else
        looping(true);
}
bool movContext::have_movie ()
{
    return m_movie != nullptr && m_valid;
}

void movContext::seekToEnd ()
{
    m_movie->seekToEnd();
}

void movContext::seekToStart ()
{
    m_movie->seekToStart();
}

int movContext::getNumFrames ()
{
    return m_fc;
}

int movContext::getCurrentFrame ()
{
    auto ft = m_movie->getCurrentTime();
    return std::floor(m_movie->getFramerate() * ft);

}

void movContext::seekToFrame (int mark)
{
    m_movie->seekToFrame (mark);
}



void movContext::onMarked ( marker_info& t)
{
    seekToFrame(std::floor(t.norm_pos.x *= getNumFrames ()));
}


vec2 movContext::getZoom ()
{
    return m_zoom;
}

void movContext::setZoom (vec2 zoom)
{
    m_zoom = zoom;
    update ();
}


void movContext::setup()
{
    
//    VisWinMgr::key_t kk;
//    guiContextRef fthis = getRef();
//    bool kept = VisWinMgr::instance().makePair(new_win, fthis, kk);
//    ci_console() << "Movie Window/Context registered: " << std::boolalpha << kept << std::endl;
    
    // Load the validated movie file
    loadMovieFile ();
    
    clear_movie_params();
    
    if( m_valid )
    {
       	m_type = Type::qtime_viewer;
        
        
        mButton_title_index = 0;
        string max = to_string( m_movie->getDuration() );
        {
        const std::function<void (int)> setter = std::bind(&movContext::seekToFrame, this, std::placeholders::_1);
        const std::function<int ()> getter = std::bind(&movContext::getCurrentFrame, this);
        mMovieParams.addParam ("Mark", setter, getter);
        }
        mMovieParams.addSeparator();
        {
            const std::function<void (bool)> setter = std::bind(&movContext::setShowMotionCenter, this, std::placeholders::_1);
            const std::function<bool (void)> getter = std::bind(&movContext::getShowMotionCenter, this);

            mMovieParams.addParam( "Show Mc", setter, getter);
        }
        mMovieParams.addSeparator();
        {
            const std::function<void (bool)> setter = std::bind(&movContext::setShowMotionBubble, this, std::placeholders::_1);
            const std::function<bool (void)> getter = std::bind(&movContext::getShowMotionBubble, this);
            
            mMovieParams.addParam( "Show Mb", setter, getter);
        }
        
        mMovieParams.addSeparator();
        mMovieParams.addButton("Play / Pause ", bind( &movContext::play_pause_button, this ) );
        mMovieParams.addSeparator();
        mMovieParams.addButton(" Loop ", bind( &movContext::loop_no_loop_button, this ) );
        
//        {
//            const std::function<void (float)> setter = std::bind(&movContext::setZoom, this, std::placeholders::_1);
//            const std::function<float (void)> getter = std::bind(&movContext::getZoom, this);
//            mMovieParams.addParam( "Zoom", setter, getter);
//        }
        
    }
}

void movContext::clear_movie_params ()
{
    mMoviePosition = 0.0f;
    mMovieIndexPosition = 0;
    mMovieRate = 1.0f;
    mMoviePlay = false;
    mMovieLoop = false;
    m_zoom.x = m_zoom.y = 1.0f;
}



void movContext::loadMovieFile()
{
    if ( ! mPath.empty () )
    {
        ci_console () << mPath.string ();
        
        try {
            
            m_movie = qtime::MovieSurface::create( mPath.string() );
            m_valid = m_movie->isPlayable ();
            
            mFrameSet = qTimeFrameCache::create (m_movie);
            
            if (m_valid)
            {
                getWindow()->setTitle( mPath.filename().string() );
                mMovieParams = params::InterfaceGl( "Movie Player", vec2( 260, 260 ) );

                // TBD: wrap these into media_info
                m_fc = m_movie->getNumFrames ();
                mScreenSize = vec2(std::fabs(m_movie->getWidth()), std::fabs(m_movie->getHeight()));
                ivec2 window_size (desired_window_size());
                setWindowSize(window_size);
                mSurface = Surface8u::create (int32_t(mScreenSize.x), int32_t(mScreenSize.y), true);
                getWindow()->getApp()->setFrameRate(m_movie->getFramerate() / 3);
                
                ci_console() << "Dimensions:" <<m_movie->getWidth() << " x " <<m_movie->getHeight() << std::endl;
                ci_console() << "Duration:  " <<m_movie->getDuration() << " seconds" << std::endl;
                ci_console() << "Frames:    " <<m_movie->getNumFrames() << std::endl;
                ci_console() << "Framerate: " <<m_movie->getFramerate() << std::endl;


                // Setup Plot area
                    std::lock_guard<std::mutex> lock(m_track_mutex);
                    plot_rects(m_track_rects);
                    
                    assert (m_track_rects.size() >= 1);
                    m_tracks.resize (0);
                    
                    m_tracks.push_back( Graph1DRef (new graph1D ("Plot 1",  m_track_rects [0])));

   
                

                m_movie->setLoop( true, false);
                m_movie->seekToStart();
                // Do not play at start 
                m_movie->play();
                
                // Percent trim from all sides.
                m_max_motion.x = m_max_motion.y = 0.1;
                
            }
        }
        catch( ... ) {
            ci_console() << "Unable to load the movie." << std::endl;
            return;
        }
        
    }
}

void movContext::mouseDown( MouseEvent event )
{
    mMetaDown = event.isMetaDown();
    mMouseIsDown = true;
}

void movContext::mouseMove( MouseEvent event )
{
    mMousePos = event.getPos();
}

void movContext::mouseDrag( MouseEvent event )
{
    mMousePos = event.getPos();
    mMouseIsDragging  = true;
}


void movContext::keyDown( KeyEvent event )
{
    if( event.getChar() == 'f' ) {
        setFullScreen( ! isFullScreen() );
    }
    else if( event.getChar() == 'o' ) {
        loadMovieFile();
    }
    
    // these keys only make sense if there is an active movie
    if( m_movie ) {
        if( event.getCode() == KeyEvent::KEY_LEFT ) {
            pause();
            seekToFrame (getCurrentFrame() - 1);
        }
        if( event.getCode() == KeyEvent::KEY_RIGHT ) {
            pause();
            seekToFrame (getCurrentFrame() + 1);
        }
        
        
        if( event.getChar() == ' ' ) {
            play_pause_button();
        }
        
    }
}



void movContext::mouseUp( MouseEvent event )
{
    mMouseIsDown = false;
    mMouseIsDragging = false;
}



void movContext::seek( size_t xPos )
{
    if (is_valid()) mMovieIndexPosition = movContext::Normal2Index ( getWindowBounds(), xPos, m_fc);
}


bool movContext::is_valid () { return m_valid && is_context_type(guiContext::qtime_viewer); }

void movContext::resize ()
{
    mSize = vec2( getWindowWidth(), getWindowHeight() / 12);
    plot_rects(m_track_rects);
    for (int cc = 0; cc < m_tracks.size(); cc++)
    {
        m_tracks[cc]->setRect (m_track_rects[cc]);
    }

    
}
void movContext::update ()
{
    if (! have_movie () )
        return;
    
    time_spec_t new_time = qtimeAvfLink::MovieBaseGetCurrentTime(m_movie);
    
    fPair trim (10, 10);

    
    if (m_movie->checkNewFrame())
    {
       ip::flipVertical(*m_movie->getSurface(), mSurface.get());
       ip::flipHorizontal(mSurface.get());
        
        mFrameSet->loadFrame(mSurface, new_time);
        m_index = mFrameSet->currentIndex(new_time);
        mSurface = mFrameSet->getFrame(new_time);
    }
    
    update_log();
    
}

void movContext::draw ()
{
     Rectf dr = get_image_display_rect();
    
    if( ! have_movie()  || ( ! mSurface ) )
    {
        std::cout << " no have movie or surface " << std::endl;
        return;
    }
    
    
    mImage = gl::Texture::create(*mSurface);
    mImage->setMagFilter(GL_NEAREST_MIPMAP_NEAREST);
    gl::draw (mImage, dr);
    
    
    draw_info ();
    
    for(Graph1DRef gg : m_tracks)
        gg->draw ();
    
    mMovieParams.draw();
    
}



////////   Info Log

void  movContext::update_log (const std::string& message)
{
    
    std::strstream msg;
    msg << " Looping: " << (looping() ? " On" : "Off");
    msg << " Playing: " << (m_movie->isPlaying() ? "Yes" : " No");
    msg << " | - F  " << setw(8) << getCurrentFrame () << std::endl;
    mLog = msg.str() + message;
    
    TextBox tbox = TextBox().alignment( TextBox::RIGHT).font( mFont ).size( mSize ).text( mLog );
    tbox.setColor( Color( 1.0f, 0.65f, 0.35f ) );
    tbox.setBackgroundColor( ColorA( 0.3f, 0.3f, 0.3f, 0.4f )  );
    ivec2 sz = tbox.measure();
    mTextTexture = gl::Texture2d::create( tbox.render() );
}

Rectf movContext::get_image_display_rect ()
{
    ivec2 ivf = image_frame_size();
    ivec2 tl = trim();
    
    ivf.y /= 3;
    
    ivec2 lr = tl + ivf;
    return Rectf (tl, lr);
}


void movContext::draw_info ()
{
    if (! have_movie () ) return;
    

    std::string seri_str = mPath.filename().string();
    
    gl::setMatricesWindow( getWindowSize() );
    
    gl::ScopedBlendAlpha blend_;
    TextLayout layoutL;
    
    layoutL.clear( ColorA::gray( 0.2f, 0.5f ) );
    layoutL.setFont( Font( "Arial", 18 ) );
    layoutL.setColor( Color::white() );
    layoutL.setLeadingOffset( 3 );
    layoutL.addRightLine( seri_str);
    
    auto texR = gl::Texture::create( layoutL.render( true ) );
    gl::draw( texR, vec2( 10, 10 ) );
    gl::clearColor(ColorA::gray( 0.2f, 0.5f ));
    
    if (mTextTexture)
    {
        Rectf textrect (0.0, getWindowHeight() - mTextTexture->getHeight(), getWindowWidth(), getWindowHeight());
        gl::draw(mTextTexture, textrect);
    }
    
}



// Create a clip viewer. Go through container of viewers, if there is a movie view, connect onMarked signal to it
void movContext::add_scalar_track(const boost::filesystem::path& path)
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
    new_ts->signalMarker.connect(std::bind(&movContext::onMarked, static_cast<movContext*>(this), std::placeholders::_1));
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
    std::shared_ptr<guiContext> mw(new movContext(createWindow( Window::Format().size(mMovieDisplayRect.getSize()))));
    
    if (! mw->is_valid()) return;
    
    for (std::shared_ptr<guiContext> uip : mContexts)
    {
        if (uip->is_context_type(guiContext::Type::clip_viewer))
        {
            uip->signalMarker.connect(std::bind(&movContext::onMarked, static_cast<movContext*>(mw.get()), std::placeholders::_1));
            mw->signalMarker.connect(std::bind(&clipContext::onMarked, static_cast<clipContext*>(uip.get()), std::placeholders::_1));
        }
    }
    mContexts.push_back(mw);
}

#endif
