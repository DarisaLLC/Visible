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

#include "gradient.h"

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


/////////////  movContext Implementation  ////////////////

movContext::movContext(const uContextRef& parent , const boost::filesystem::path& dp)
: uContext (parent), mPath (dp)
{
    m_valid = false;
    m_type = Type::clip_viewer;
    
    setup ();
    if (is_valid())
    {
        app::WindowRef new_win = VisibleApp::instance().createWindow ();
        setWindowRef ( new_win );
        mCbMouseDown = mWindow->getSignalMouseDown().connect( std::bind( &movContext::mouseDown, this, std::placeholders::_1 ) );
        mCbMouseDrag = mWindow->getSignalMouseDrag().connect( std::bind( &movContext::mouseDrag, this, std::placeholders::_1 ) );
        mCbMouseUp = mWindow->getSignalMouseUp().connect( std::bind( &movContext::mouseUp, this, std::placeholders::_1 ) );
        mCbMouseMove = mWindow->getSignalMouseMove().connect( std::bind( &movContext::mouseMove, this, std::placeholders::_1 ) );
        mCbKeyDown = mWindow->getSignalKeyDown().connect( std::bind( &movContext::keyDown, this, std::placeholders::_1 ) );
        getWindow()->setTitle( mPath.filename().string() );
    }
    
}





void movContext::play ()
{
    if (! have_movie() || m_movie->isPlaying() ) return;
    m_movie->play ();
}

void movContext::pause ()
{
    if (! have_movie() || ! m_movie->isPlaying() ) return;
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

bool movContext::have_movie ()
{
    return m_movie != nullptr && m_valid;
}

int movContext::getIndex ()
{
    return mMovieIndexPosition;
}

void movContext::onMarked ( marker_info& t)
{
    pause ();
    setIndex((int)(t.norm_pos.x *= m_fc));
    

}
void movContext::setIndex (int mark)
{
    pause ();
    mMovieIndexPosition = (mark % m_fc);
    m_movie->seekToFrame(mMovieIndexPosition);
}

float movContext::getZoom ()
{
    return mMovieCZoom;
}

void movContext::setZoom (float nv)
{
    mMovieCZoom = nv;
    update ();
}


void movContext::setup()
{
    // Browse for the movie file
    loadMovieFile ();
    
    clear_movie_params();
    
    if( m_valid )
    {
       	m_type = Type::qtime_viewer;
        
        mButton_title_index = 0;
        string max = to_string( m_movie->getDuration() );
        {
        const std::function<void (int)> setter = std::bind(&movContext::setIndex, this, std::placeholders::_1);
        const std::function<int ()> getter = std::bind(&movContext::getIndex, this);
        mMovieParams.addParam ("Mark", setter, getter);
        }
        mMovieParams.addSeparator();
        {
            const std::function<void (bool)> setter = std::bind(&movContext::setShowMotionCenter, this, std::placeholders::_1);
            const std::function<bool (void)> getter = std::bind(&movContext::getShowMotionCenter, this);

            mMovieParams.addParam( "Show Mc", setter, getter);
        }
        
        mMovieParams.addSeparator();
        mMovieParams.addButton("Play / Pause ", bind( &movContext::play_pause_button, this ) );
        
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
    mPrevMoviePosition = mMoviePosition;
    mMovieIndexPosition = 0;
    mPrevMovieIndexPosition = -1;
    mMovieRate = 1.0f;
    mPrevMovieRate = mMovieRate;
    mMoviePlay = false;
    mPrevMoviePlay = mMoviePlay;
    mMovieLoop = false;
    mPrevMovieLoop = mMovieLoop;
    mMovieCZoom=1.0f;
}



void movContext::loadMovieFile()
{
    m_valid = false;
    
    // Browse for the result file
    mPath = getOpenFilePath();
    
    if (mPath.string().empty() || ! exists(mPath) )
    {
        std::cout << mPath.string() << " Does not exist Or User cancelled " << std::endl;
    }
    
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
                
                m_fc = m_movie->getNumFrames ();
                m_movie->setLoop( true, false);
                m_movie->seekToStart();
                // Do not play at start 
                play();
                
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



void movContext::mouseUp( MouseEvent event )
{
    mMouseIsDown = false;
    mMouseIsDragging = false;
}


vec2 movContext::texture_to_display_zoom()
{
    Rectf textureBounds = mImage->getBounds();
    return vec2(m_display_rect.getWidth() / textureBounds.getWidth(),m_display_rect.getHeight() / textureBounds.getHeight());
}

void movContext::seek( size_t xPos )
{
    if (is_valid()) mMovieIndexPosition = movContext::Normal2Index ( getWindowBounds(), xPos, m_fc);
}


bool movContext::is_valid () { return m_valid && is_context_type(uContext::qtime_viewer); }


void movContext::update ()
{
    if (! have_movie () )
        return;
    
    time_spec_t new_time = qtimeAvfLink::MovieBaseGetCurrentTime(m_movie);
    
    fPair trim (10, 10);
    uint8_t edge_magnitude_threshold = 10;
    
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
            getLuminanceCenterOfMass (mS, dcom);
            vec2 m (dcom.x , dcom.y );
            normalize_point(m, mSurface->getSize());
            m = m - (mCom - m_prev_com);
            m_max_motion = m;
        }
        
          mS = gm;
        mSS = surfm;
        
        
        
    }
    
}

void movContext::draw ()
{
    if( ! have_movie()  || ( ! mSurface ) )
        return;
    
    mImage = gl::Texture::create(*mSurface);
    mImage->setMagFilter(GL_NEAREST_MIPMAP_NEAREST);
    gl::draw (mImage, getWindowBounds());

    vec2 com = mCom * vec2(getWindowSize().x,getWindowSize().y);
    vec2 mmm = m_max_motion * vec2(getWindowSize().x,getWindowSize().y);
    vec2 mid = (com + mmm) / vec2(2.0f,2.0f);
    
    float len = distance(mCom, m_max_motion);
    
    if (m_index < 1) return;
    
    if (getShowMotionCenter ())
    {
        gl::ScopedColor color (ColorA(0.5f, 0.5f, 1.0f, 0.5f));
        gl::drawSolidEllipse(mid, len, len / 2.0f);
    }
    mMovieParams.draw();
    
}



////////   Adding Tracks 


// Create a clip viewer. Go through container of viewers, if there is a movie view, connect onMarked signal to it
void movContext::add_scalar_track(const std::string& name, const boost::filesystem::path& path)
{
    // Get a clip context from the path
    uContextRef new_ts ( new clipContext (getRef(), path));
    auto parent_size = getWindowRef()->getSize();
    auto parent_pos = getWindowRef()->getPos();
    parent_pos.y += parent_size.y;
    parent_size.y = 100;
    new_ts->getWindowRef()->setSize (parent_size);
    new_ts->getWindowRef()->setPos (parent_pos);
    getRef()->signalMarker.connect(std::bind(&clipContext::onMarked, static_cast<clipContext*>(new_ts.get()), std::placeholders::_1));
    new_ts->getRef()->signalMarker.connect(std::bind(&movContext::onMarked, static_cast<movContext*>(new_ts.get()), std::placeholders::_1));
}



//////
// Remove existing viewers. Needs better design & implementation
//    auto new_end = std::remove_if(mContexts.begin(), mContexts.end(),
//                                  [](const std::shared_ptr<uContext>& cx)
//                                 { return cx->is_context_type(uContext::Type::qtime_viewer); });
//    mContexts.erase (new_end, mContexts.end());



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
    std::shared_ptr<uContext> mw(new movContext(createWindow( Window::Format().size(mMovieDisplayRect.getSize()))));
    
    if (! mw->is_valid()) return;
    
    for (std::shared_ptr<uContext> uip : mContexts)
    {
        if (uip->is_context_type(uContext::Type::clip_viewer))
        {
            uip->signalMarker.connect(std::bind(&movContext::onMarked, static_cast<movContext*>(mw.get()), std::placeholders::_1));
            mw->signalMarker.connect(std::bind(&clipContext::onMarked, static_cast<clipContext*>(uip.get()), std::placeholders::_1));
        }
    }
    mContexts.push_back(mw);
}

#endif
