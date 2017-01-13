//
//  visible_mov_impl.cpp
//  Visible
//
//  Created by Arman Garakani on 5/13/16.
//
//

#include <stdio.h>
#include "VisibleApp.h"
#include "ui_contexts.h"
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
#include "opencv_utils.hpp"
#include "cinder/ip/Flip.h"
#include "otherIO/lifFile.hpp"


#include "gradient.h"

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
    
    
    
}


/////////////  lifContext Implementation  ////////////////

lifContext::lifContext(WindowRef& ww, const boost::filesystem::path& dp)
: uContext(ww), mPath (dp)
{
    m_valid = false;
    m_type = Type::lif_file_viewer;
    
    if (mPath.string().empty())
        mPath = getOpenFilePath();

    m_valid = ! mPath.string().empty() && exists(mPath);
    
    setup ();
    if (is_valid())
    {
        mWindow->setTitle( mPath.filename().string() );
    }
    
}





void lifContext::play ()
{
    if (! have_movie() || m_movie->isPlaying() ) return;
    m_movie->play ();
}

void lifContext::pause ()
{
    if (! have_movie() || ! m_movie->isPlaying() ) return;
    m_movie->stop ();
}

void lifContext::play_pause_button ()
{
    if (! have_movie () ) return;
    if (m_movie->isPlaying())
        pause ();
    else
        play ();
}

bool lifContext::have_movie ()
{
    return m_movie != nullptr && m_valid;
}

int lifContext::getIndex ()
{
    return mMovieIndexPosition;
}

void lifContext::onMarked ( marker_info& t)
{
    pause ();
    setIndex((int)(t.norm_pos.x *= m_fc));
  

}
void lifContext::setIndex (int mark)
{
    pause ();
    mMovieIndexPosition = (mark % m_fc);
    m_movie->seekToFrame(mMovieIndexPosition);
 
    
    
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
    
//    VisWinMgr::key_t kk;
//    uContextRef fthis = getRef();
//    bool kept = VisWinMgr::instance().makePair(new_win, fthis, kk);
//    ci_console() << "Movie Window/Context registered: " << std::boolalpha << kept << std::endl;
    
    // Load the validated movie file
    loadLifFile ();
    
    clear_movie_params();
    
    if( m_valid )
    {
       	m_type = Type::qtime_viewer;
        
        mMovieParams.addSeparator();
        mMovieParams.addButton( "Import Time Series ", std::bind( &lifContext::add_scalar_track_get_file, this ) );
        mMovieParams.addSeparator();
        
        mButton_title_index = 0;
        string max = to_string( m_movie->getDuration() );
        {
        const std::function<void (int)> setter = std::bind(&lifContext::setIndex, this, std::placeholders::_1);
        const std::function<int ()> getter = std::bind(&lifContext::getIndex, this);
        mMovieParams.addParam ("Mark", setter, getter);
        }
        mMovieParams.addSeparator();
        {
            const std::function<void (bool)> setter = std::bind(&lifContext::setShowMotionCenter, this, std::placeholders::_1);
            const std::function<bool (void)> getter = std::bind(&lifContext::getShowMotionCenter, this);

            mMovieParams.addParam( "Show Mc", setter, getter);
        }
        mMovieParams.addSeparator();
        {
            const std::function<void (bool)> setter = std::bind(&lifContext::setShowMotionBubble, this, std::placeholders::_1);
            const std::function<bool (void)> getter = std::bind(&lifContext::getShowMotionBubble, this);
            
            mMovieParams.addParam( "Show Mb", setter, getter);
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
            
            m_movie = qtime::MovieSurface::create( mPath.string() );
            m_valid = m_movie->isPlayable ();
            
            mFrameSet = qTimeFrameCache::create (m_movie);
            
            if (m_valid)
            {
                getWindow()->setTitle( mPath.filename().string() );
                mMovieParams = params::InterfaceGl( "Movie Controller", vec2( 90, 160 ) );
                
                ci_console() << "Dimensions:" <<m_movie->getWidth() << " x " <<m_movie->getHeight() << std::endl;
                ci_console() << "Duration:  " <<m_movie->getDuration() << " seconds" << std::endl;
                ci_console() << "Frames:    " <<m_movie->getNumFrames() << std::endl;
                ci_console() << "Framerate: " <<m_movie->getFramerate() << std::endl;
                getWindow()->getApp()->setFrameRate(m_movie->getFramerate() / 3);

                mScreenSize = vec2(std::fabs(m_movie->getWidth()), std::fabs(m_movie->getHeight()));
        
                mSurface = Surface8u::create (int32_t(mScreenSize.x), int32_t(mScreenSize.y), true);
                
                mS = cv::Mat(mScreenSize.x, mScreenSize.y, CV_32F);
                mSS = cv::Mat(mScreenSize.x, mScreenSize.y, CV_32F);
                
                texture_to_display_zoom();
                
                
                m_fc = m_movie->getNumFrames ();
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
    if( m_movie ) {
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


vec2 lifContext::texture_to_display_zoom()
{
    Rectf image (0.0f, 0.0f, mScreenSize.x, mScreenSize.y);
    Rectf window = getWindowBounds();
    float sx = window.getWidth() / image.getWidth();
    float sy = window.getHeight() / image.getHeight();
    
    if ( sx < 1.1f || sy < 1.1f )
    {
        getWindow()->setSize((int32_t) (mScreenSize.x*1.15f), (int32_t) (mScreenSize.y*1.15f));
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

void lifContext::seek( size_t xPos )
{
    if (is_valid()) mMovieIndexPosition = lifContext::Normal2Index ( getWindowBounds(), xPos, m_fc);
}


bool lifContext::is_valid () { return m_valid && is_context_type(uContext::qtime_viewer); }

void lifContext::resize ()
{
    m_zoom = texture_to_display_zoom();
    
}
void lifContext::update ()
{
    if (! have_movie () )
        return;
    
    time_spec_t new_time = qtimeAvfLink::MovieBaseGetCurrentTime(m_movie);
    
    fPair trim (10, 10);
    uint8_t edge_magnitude_threshold = 5;
    
    if (m_movie->checkNewFrame())
    {
       ip::flipVertical(*m_movie->getSurface(), mSurface.get());
        ip::flipHorizontal(mSurface.get());
        
        mFrameSet->loadFrame(mSurface, new_time);
        m_index = mFrameSet->currentIndex(new_time);
        
        mSurface = mFrameSet->getFrame(new_time);
        cv::Mat surfm = toOcv( *mSurface);
        cv::Mat gm (mSurface->getWidth(), mSurface->getHeight(), CV_8U);
        cvtColor(surfm, gm, cv::COLOR_RGB2GRAY);
        cv::GaussianBlur(gm, gm, cv::Size(11,11), 5.00);
        roiWindow<P8U> rw;
        NewFromOCV(gm, rw);
        roiWindow<P8U> roi (rw.frameBuf(), trim.first, trim.second, rw.width() - 2*trim.first, rw.height() - 2*trim.second );
        fPair center;
        GetMotionCenter(roi, center, edge_magnitude_threshold);
        center += trim;
        m_prev_com = mCom;
        mCom = vec2(center.first, center.second);
        
        cv::Point2f com (mCom.x , mCom.y );
        normalize_point(mCom, mSurface->getSize());

        if (m_index > 0)
        {
            cv::absdiff(gm, mS, mS);
            cv::Point2f dcom;
            getLuminanceCenterOfMass (gm, dcom);
            vec2 m (dcom.x , dcom.y );
            normalize_point(m, mSurface->getSize());
            m = m - (mCom - m_prev_com);
            m_max_motion = m;
        }
        
          mS = gm;
        mSS = surfm;
        
        
        
    }
    
}

void lifContext::draw ()
{
   
    if( ! have_movie()  || ( ! mSurface ) )
    {
        std::cout << " no have movie or surface " << std::endl;
        return;
    }
    
    mImage = gl::Texture::create(*mSurface);
    
      mImage->setMagFilter(GL_NEAREST_MIPMAP_NEAREST);
    gl::draw (mImage, m_display_rect);

    vec2 com = mCom * vec2(getWindowSize().x,getWindowSize().y);
    vec2 pcom = m_prev_com * vec2(getWindowSize().x,getWindowSize().y);
    vec2 mmm = m_max_motion * vec2(getWindowSize().x,getWindowSize().y);
    vec2 mid = (com + mmm) / vec2(2.0f,2.0f);
    
    float len = distance(pcom, com);
    
    if (m_index < 1) return;
    
    if (getShowMotionCenter ())
    {
        gl::ScopedColor color (ColorA(1.0f, 0.5f, 1.0f, 0.5f));
        gl::drawVector(vec3(pcom.x,pcom.y,0), vec3(com.x, com.y, 128));
    }
    if (getShowMotionBubble ())
    {
        gl::ScopedColor color (ColorA(0.5f, 0.5f, 1.0f, 0.5f));
        gl::drawSolidEllipse(mid, len, len / 2.0f);
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
    std::shared_ptr<uContext> cw(std::shared_ptr<uContext>(new clipContext(createWindow( Window::Format().size(mGraphDisplayRect.getSize())))));
    
    if (! cw->is_valid()) return;
    
    for (std::shared_ptr<uContext> uip : mContexts)
    {
        if (uip->is_context_type(uContext::Type::qtime_viewer))
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
    std::shared_ptr<uContext> mw(new lifContext(createWindow( Window::Format().size(mMovieDisplayRect.getSize()))));
    
    if (! mw->is_valid()) return;
    
    for (std::shared_ptr<uContext> uip : mContexts)
    {
        if (uip->is_context_type(uContext::Type::clip_viewer))
        {
            uip->signalMarker.connect(std::bind(&lifContext::onMarked, static_cast<lifContext*>(mw.get()), std::placeholders::_1));
            mw->signalMarker.connect(std::bind(&clipContext::onMarked, static_cast<clipContext*>(uip.get()), std::placeholders::_1));
        }
    }
    mContexts.push_back(mw);
}

#endif
