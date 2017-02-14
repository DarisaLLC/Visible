
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
#include "directoryPlayer.h"

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
    
    
    
}


/////////////  movDirContext Implementation  ////////////////



movDirContext::movDirContext(WindowRef& ww, const boost::filesystem::path& dp)
                            : guiContext(ww), mPath (dp)
{
    m_valid = false;
    m_type = Type::qtime_viewer;

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
    
    
    m_Dm = directoryPlayer::create (source_path(), m_extension, m_fps, m_anonymous_format);
    
    if( m_Dm && m_valid )
    {
       	m_type = Type::qtime_viewer;

#if 0
        mMovieParams.addSeparator();
        mMovieParams.addButton( "Import Time Series ", std::bind( &movDirContext::add_scalar_track_get_file, this ) );
        mMovieParams.addSeparator();
        mButton_title_index = 0;
#endif
        

        std::string max = to_string( m_Dm->getDuration() );
        {
        const std::function<void (int)> setter = std::bind(&movDirContext::seekToFrame, this, std::placeholders::_1);
        const std::function<int ()> getter = std::bind(&movDirContext::getCurrentFrame, this);
        mMovieParams.addParam ("Mark", setter, getter);
        }
#if 0
        mMovieParams.addSeparator();
        {
            const std::function<void (bool)> setter = std::bind(&movDirContext::setShowMotionCenter, this, std::placeholders::_1);
            const std::function<bool (void)> getter = std::bind(&movDirContext::getShowMotionCenter, this);

            mMovieParams.addParam( "Show Mc", setter, getter);
        }
        mMovieParams.addSeparator();
        {
            const std::function<void (bool)> setter = std::bind(&movDirContext::setShowMotionBubble, this, std::placeholders::_1);
            const std::function<bool (void)> getter = std::bind(&movDirContext::getShowMotionBubble, this);
            
            mMovieParams.addParam( "Show Mb", setter, getter);
        }
#endif
        
        mMovieParams.addSeparator();
        mMovieParams.addButton("Play / Pause ", bind( &movDirContext::play_pause_button, this ) );
        
//        {
//            const std::function<void (float)> setter = std::bind(&movDirContext::setZoom, this, std::placeholders::_1);
//            const std::function<float (void)> getter = std::bind(&movDirContext::getZoom, this);
//            mMovieParams.addParam( "Zoom", setter, getter);
//        }
        
    }
}

void movDirContext::clear_movie_params ()
{
    seekToStart ();
    mMoviePlay = false;
    mPrevMoviePlay = mMoviePlay;
    mMovieLoop = false;
    mPrevMovieLoop = mMovieLoop;
    m_zoom.x = m_zoom.y = 1.0f;
}



void movDirContext::loadMovieFile()
{
    if ( ! mPath.empty () )
    {
        ci_console () << mPath.string ();
        
        try {
            

            if (m_valid)
            {
                getWindow()->setTitle( mPath.filename().string() );
                mMovieParams = params::InterfaceGl( "Movie Controller", vec2( 90, 160 ) );
                
              //  ci_console() << "Dimensions:" <<m_movie->getWidth() << " x " <<m_movie->getHeight() << std::endl;
                ci_console() << "Duration:  " <<getDuration() << " seconds" << std::endl;
                ci_console() << "Frames:    " <<getNumFrames() << std::endl;
                ci_console() << "Framerate: " <<getFrameRate() << std::endl;
                getWindow()->getApp()->setFrameRate(m_Dm->getFrameRate() / 3);

                mScreenSize = vec2(640, 480);
        
                texture_to_display_zoom();
                
                
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


vec2 movDirContext::texture_to_display_zoom()
{
    if (! have_movie ()) return vec2(0,0);
    vec2 textureSize (m_Dm->getTexture()->getWidth(), m_Dm->getTexture()->getWidth());
    Rectf image (0.0f, 0.0f, textureSize.x, textureSize.y);
    Rectf window = getWindowBounds();
    float sx = window.getWidth() / image.getWidth();
    float sy = window.getHeight() / image.getHeight();
    
    if ( sx < 1.1f || sy < 1.1f )
    {
        getWindow()->setSize((int32_t) (textureSize.x*1.15f), (int32_t) (textureSize.y*1.15f));
        sx = window.getWidth() / image.getWidth();
        sy = window.getHeight() / image.getHeight();
    }
    float w = image.getWidth() * sx;
    float h = image.getHeight() * sy;
    float ox = -0.5 * ( w - window.getWidth());
    float oy = -0.5 * ( h - window.getHeight());
    image.set(ox, oy, ox + w, oy + h);
    m_display_rect = image;

    return vec2(sx, sy);
}





void movDirContext::resize ()
{
    m_zoom = texture_to_display_zoom();
    
}
void movDirContext::update ()
{
    if (! have_movie () )
        return;
    m_Dm->update ();
}

void movDirContext::draw ()
{
   
    if (! have_movie () )
        return;
    m_Dm->draw (getWindowBounds ());
    
  
    mMovieParams.draw();
    
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
