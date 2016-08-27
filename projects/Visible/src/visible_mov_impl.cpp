//
//  visible_mov_impl.cpp
//  Visible
//
//  Created by Arman Garakani on 5/13/16.
//
//

#include <stdio.h>

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

int movContext::getIndex ()
{
    return mMovieIndexPosition;
}

void movContext::setIndex (int mark)
{
    if( m_movie->isPlaying() )
        m_movie->stop();
    
    mMovieIndexPosition = (mark % m_fc);
    m_movie->seekToFrame(mMovieIndexPosition);
}

void movContext::setup()
{
    // Browse for the movie file
    loadMovieFile (getOpenFilePath());
    
    clear_movie_params();
    
    if( m_valid )
    {
        string max = to_string( m_movie->getDuration() );
        const std::function<void (int)> setter = std::bind(&movContext::setIndex, this, std::placeholders::_1);
        const std::function<int ()> getter = std::bind(&movContext::getIndex, this);
        mMovieParams.addParam ("Mark", setter, getter);
        mMovieParams.addSeparator();
        mMovieParams.addParam( "Zoom", &mMovieCZoom, "min=0.1 max=+10 step=0.1" );
        
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

void movContext::loadMovieFile( const boost::filesystem::path &moviePath )
{
    m_valid = false;
    
    
    if ( ! moviePath.empty () )
    {
        ci_console () << moviePath.string ();
        
        
        try {
            
            m_movie = qtime::MovieSurface::create( moviePath.string() );
            m_valid = m_movie->isPlayable ();
            
            mFrameSet = qTimeFrameCache::create (m_movie);
            
            if (m_valid)
            {
                getWindow()->setTitle( moviePath.filename().string() );
                mMovieParams = params::InterfaceGl( "Movie Controller", vec2( 90, 160 ) );
                
                ci_console() << "Dimensions:" <<m_movie->getWidth() << " x " <<m_movie->getHeight() << std::endl;
                ci_console() << "Duration:  " <<m_movie->getDuration() << " seconds" << std::endl;
                ci_console() << "Frames:    " <<m_movie->getNumFrames() << std::endl;
                ci_console() << "Framerate: " <<m_movie->getFramerate() << std::endl;
                getWindow()->getApp()->setFrameRate(m_movie->getFramerate() / 3);

                
                mScreenSize = vec2(std::fabs(m_movie->getWidth()), std::fabs(m_movie->getHeight()));
//                getWindow()->getApp()->setWindowSize(mScreenSize.x, mScreenSize.y);
                mSurface = Surface8u::create (int32_t(mScreenSize.x), int32_t(mScreenSize.y), true);
                
                mS = cv::Mat(mScreenSize.x, mScreenSize.y, CV_32F);
                mSS = cv::Mat(mScreenSize.x, mScreenSize.y, CV_32F);
                
                
                m_fc = m_movie->getNumFrames ();
                m_movie->setLoop( true, false);
                m_movie->seekToStart();
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
    //    handleTime(mMousePos);
}

void movContext::mouseDrag( MouseEvent event )
{
    mMousePos = event.getPos();
    mMouseIsDragging  = true;
    //    handleTime(mMousePos);
}

void movContext::handleTime( vec2 pos )
{
    m_movie->seekToTime( (m_movie->getDuration() * (float)pos.x ) / (float)getWindowWidth () );
}

void movContext::keyDown( KeyEvent event )
{
    if( event.getChar() == 'f' ) {
        setFullScreen( ! isFullScreen() );
    }
    else if( event.getChar() == 'o' ) {
        loadMovieFile( getOpenFilePath() );
    }
    
    // these keys only make sense if there is an active movie
    if( m_movie ) {
        if( event.getCode() == KeyEvent::KEY_LEFT ) {
            
            if( m_movie->isPlaying() )
                m_movie->stop();
            setIndex (mMovieIndexPosition - 1);
        }
        if( event.getCode() == KeyEvent::KEY_RIGHT ) {
            if( m_movie->isPlaying() )
                m_movie->stop();
            
            setIndex (mMovieIndexPosition + 1);
        }
        
        //        else if( event.getChar() == 's' ) {
        //            if( mSurface ) {
        //                fs::path savePath = getSaveFilePath();
        //                if( ! savePath.empty() ) {
        //                    writeImage( savePath, *mSurface );
        //                }
        //            }
        //        }
        
        
        if( event.getChar() == ' ' ) {
            if( m_movie->isPlaying() )
                m_movie->stop();
            else
                m_movie->play();
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
    //  auto waves = mWaveformPlot.getWaveforms ();
    //	if (have_sampler () ) mSamplePlayer->seek( waves[0].sections() * xPos / mGraphDisplayRect.getWidth()() );
    
    if (is_valid()) mMovieIndexPosition = movContext::Normal2Index ( getWindowBounds(), xPos, m_fc);
}


bool movContext::is_valid ()
{
    return m_valid;
}

static void channelMaskImage( const Channel8uRef &mask, SurfaceRef target )
{
    float val;
    auto maskIter = mask->getIter(); // using const because we're not modifying it
    Surface::Iter targetIter( target->getIter() ); // not using const because we are modifying it
    while( maskIter.line() && targetIter.line() ) { // line by line
        while( maskIter.pixel() && targetIter.pixel() ) { // pixel by pixel
            {
                auto mv = maskIter.v();
                auto rv = targetIter.r();
                if (mv > 64)
                    targetIter.r() = 255; //rv + ((255 - rv) * mv) / 255;
            }
        }
    }
}


void movContext::update ()
{
    time_spec_t new_time = qtimeAvfLink::MovieBaseGetCurrentTime(m_movie);
    
    fPair trim (10, 10);
    uint8_t edge_magnitude_threshold = 10;
    
    if (m_movie->checkNewFrame())
    {
        ip::flipVertical(*m_movie->getSurface(), mSurface.get());
        ip::flipHorizontal(mSurface.get());
        
        bool missOrHit = mFrameSet->loadFrame(mSurface, new_time);
        m_index = mFrameSet->currentIndex(new_time);
        
        //    if (missOrHit)
        //   std::cout << "L @" << new_time.secs() << std::endl;
        //    else
        //       std::cout << "H @" << new_time.secs() << std::boolalpha <<  mFrameSet->checkFrame(new_time) << std::endl;
        
        
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

//            Channel8uRef mask = Channel8u::create(fromOcv(mS));
//            channelMaskImage(mask, mSurface);
//            cv_drawCross(mS, com, Scalar(255,0,0),24);
//            cv_drawCross(mS, dcom, Scalar(255,0,0),16);
//            imshow("Diff", mS);
//            std::cout << center << std::endl;
        }
        
          mS = gm;
        mSS = surfm;
        
        
        
    }
    
}

void movContext::draw ()
{
//	gl::ScopedModelMatrix matScope;
    
    if( ( ! m_movie ) || ( ! mSurface ) )
        return;
//    gl::clear(Color::gray (0.1f));
    
    mImage = gl::Texture::create(*mSurface);
    mImage->setMagFilter(GL_NEAREST_MIPMAP_NEAREST);
    gl::draw (mImage, getWindowBounds());
    vec2 mid = (mCom + m_max_motion) / vec2(2.0f,2.0f);
    float len = distance(mCom, m_max_motion);
    
    if (m_index < 1) return;
    
//    gl::pushModelView();
    {
        gl::ScopedColor color (ColorA(0.5f, 0.5f, 1.0f, 0.5f));
        gl::drawSolidEllipse(mid, len, len / 2.0f);
//        gl::drawStrokedCircle( m_max_motion, 8);
//        gl::drawStrokedCircle( mCom, 8);
//        gl::drawStrokedCircle( mid, 24);
    }
//    gl::popModelView();
//    mMovieParams.draw();
    
}

