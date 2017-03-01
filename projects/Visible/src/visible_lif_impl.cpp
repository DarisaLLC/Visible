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
#include "cinder/app/App.h"
#include "cinder/gl/gl.h"
#include "cinder/Timeline.h"
#include "cinder/Timer.h"
#include "cinder/Camera.h"
#include "cinder/qtime/Quicktime.h"
#include "cinder/params/Params.h"
#include "cinder/ImageIo.h"
#include "CinderOpenCV.h"
#include "Cinder/ip/Blend.h"
#include "opencv2/highgui.hpp"
#include "vision/opencv_utils.hpp"
#include "vision/histo.h"
#include "core/stl_utils.hpp"
#include "cinder/ip/Flip.h"
#include "otherIO/lifFile.hpp"
#include <strstream>
#include <algorithm>
#include <future>
#include <mutex>
#include "async_producer.h"
#include "cinder_xchg.hpp"
#include "visible_layout.hpp"

using namespace ci;
using namespace ci::app;
using namespace std;
using namespace svl;


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
    
    static layout vl (ivec2 (960, 540), ivec2 (10, 10));

}


/////////////  lifContext Implementation  ////////////////


lifContext::lifContext(WindowRef& ww, const boost::filesystem::path& dp)
: visualContext(ww), mPath (dp)
{
    m_valid = false;
    m_type = Type::lif_file_viewer;
    
    if (mPath.string().empty())
        mPath = getOpenFilePath();
    
    m_valid = ! mPath.string().empty() && exists(mPath);
    
    if (is_valid())
    {
        mWindow->setTitle( mPath.filename().string() );
        mWindow->setSize(vl.desired_window_size());
        mFont = Font( "Menlo", 12 );
        mSize = vec2( getWindowWidth(), getWindowHeight() / 12);
    }
    
    setup ();
}




void lifContext::looping (bool what)
{
    mMovieLoop = what;
}


bool lifContext::looping ()
{
    return mMovieLoop;
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

void lifContext::loop_no_loop_button ()
{
    if (! have_movie () ) return;
    if (looping())
        looping(false);
    else
        looping(true);
}

void lifContext::onMarked ( marker_info& t)
{
    pause ();
    seekToFrame((int)(t.norm_pos.x *= getNumFrames () ));
    
    
}

bool lifContext::have_movie ()
{
    return m_lifRef && m_current_serie_ref >= 0 && mFrameSet;
}

void lifContext::seekToEnd ()
{
    mMovieIndexPosition = getNumFrames() - 1;
}

void lifContext::seekToStart ()
{
    mMovieIndexPosition = 0;
}

int lifContext::getNumFrames ()
{
    return m_fc;
}

int lifContext::getCurrentFrame ()
{
    return mMovieIndexPosition;
}


void lifContext::seekToFrame (int mark)
{
    mMovieIndexPosition = mark;
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
        
        m_selected_serie_index = 0;
        m_current_serie_ref = std::shared_ptr<lifIO::LifSerie>();
        
        mMovieParams.addParam( "Series ", m_series_names, &m_selected_serie_index )
        //        .keyDecr( "[" )
        //        .keyIncr( "]" )
        .updateFn( [this]
                  {
                      if (m_selected_serie_index >= 0 && m_selected_serie_index < m_series_names.size() )
                      {
                          m_serie = m_series_book[m_selected_serie_index];
                          m_current_serie_ref = std::shared_ptr<lifIO::LifSerie>(&m_lifRef->getSerie(m_selected_serie_index), stl_utils::null_deleter());
                          loadCurrentSerie ();
                          console() << "selected serie updated: " << m_series_names [m_selected_serie_index] << endl;
                      }
                  });
        
        
        mMovieParams.addSeparator();
        {
            const std::function<void (int)> setter = std::bind(&lifContext::seekToFrame, this, std::placeholders::_1);
            const std::function<int ()> getter = std::bind(&lifContext::getCurrentFrame, this);
            mMovieParams.addParam ("Current Time Step", setter, getter);
        }
        
        mMovieParams.addSeparator();
        mMovieParams.addButton("Play / Pause ", bind( &lifContext::play_pause_button, this ) );
        mMovieParams.addSeparator();
        mMovieParams.addButton(" Loop ", bind( &lifContext::loop_no_loop_button, this ) );
        
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
    mMovieIndexPosition = 0;
    mMovieRate = 1.0f;
    mMoviePlay = false;
    mMovieLoop = false;
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



tracksD1_t get_mean_luminance (const std::shared_ptr<qTimeFrameCache>& frames, const std::vector<std::string>& names,
                               bool test_data = false)
{

    tracksD1_t tracks;
    tracks.resize (names.size ());
    for (auto tt = 0; tt < names.size(); tt++)
        tracks[tt].first = names[tt];

    // If it is 3 channels. We will use multiple window
    int64_t fn = 0;
    while (frames->checkFrame(fn))
    {
        auto su8 = frames->getFrame(fn++);
        
        auto channels = names.size();

        std::vector<roiWindow<P8U> > rois;
        switch (channels)
        {
            case 1:
            {
                auto m1 = svl::NewRedFromSurface(su8);
                rois.emplace_back(m1);
                break;
            }
            case 3:
            {
                auto m3 = svl::NewMultiFromSurface (su8, names, fn);
                for (auto cc = 0; cc < m3.planes(); cc++)
                    rois.emplace_back(m3.plane(cc));
                break;
            }
        }

        assert (rois.size () == tracks.size());
        
        // Now get average intensity for each channel
        int index = 0;
        for (roiWindow<P8U> roi : rois)
        {
            index_time_t ti;
            ti.first = fn;
            timed_double_t res;
            res.first = ti;
            auto nmg = histoStats::mean(roi) / 256.0;
            res.second = (! test_data) ? nmg :  (((float) fn) / frames->count() );
            tracks[index++].second.emplace_back(res);
        }
        
        // TBD: call progress reporter
    }

    
    return tracks;
}


void lifContext::loadCurrentSerie ()
{
    if ( ! (m_lifRef || ! m_current_serie_ref) )
        return;
    
    try {
        
        mFrameSet = qTimeFrameCache::create (*m_current_serie_ref);
        
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
            
            ivec2 window_size (vl.desired_window_size());
            setWindowSize(window_size);
            int channel_count = (int) tm.getNumChannels();

            {
                std::lock_guard<std::mutex> lock(m_track_mutex);
                vl.plot_rects(m_track_rects);
                
                assert (m_track_rects.size() >= channel_count);
                
                m_tracks.resize (0);
                
                for (int cc = 0; cc < channel_count; cc++)
                {
                    m_tracks.push_back( Graph1DRef (new graph1D (m_current_serie_ref->getChannels()[cc].getName(),
                                                                 m_track_rects [cc])));
                }
            }
            
            // Launch Average Luminance Computation
            m_async_luminance_tracks = std::async(std::launch::async, get_mean_luminance,
                                                  mFrameSet, m_serie.channel_names, false);
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
            seekToFrame (getCurrentFrame() - 1);
        }
        if( event.getCode() == KeyEvent::KEY_RIGHT ) {
            pause ();
            seekToFrame (getCurrentFrame() + 1);
        }
        if( event.getChar() == 'l' ) {
            loop_no_loop_button();
        }
        
        if( event.getChar() == ' ' ) {
            play_pause_button();
        }
        
        if (event.getChar() == KeyEvent::KEY_v)
        {
            // Toggle vertical sync.
            if( gl::isVerticalSyncEnabled() )
                gl::enableVerticalSync( false );
            else
                gl::enableVerticalSync();
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
    ivec2 ivf = vl.image_frame_size();
    ivec2 tl = vl.trim();
    
    if (m_serie.channelCount == 1)
        ivf.y /= 3;
    
    ivec2 lr = tl + ivf;
    return Rectf (tl, lr);
}



//void lifContext::seek( size_t xPos )
//{
//    if (is_valid()) mMovieIndexPosition = lifContext::Normal2Index ( getWindowBounds(), xPos, getNumFrames () );
//}


bool lifContext::is_valid () { return m_valid && is_context_type(guiContext::lif_file_viewer); }

void lifContext::resize ()
{
    mSize = vec2( getWindowWidth(), getWindowHeight() / 12);
    vl.plot_rects(m_track_rects);
    for (int cc = 0; cc < m_tracks.size(); cc++)
    {
        m_tracks[cc]->setRect (m_track_rects[cc]);
    }
}
void lifContext::update ()
{
    if ( is_ready (m_async_luminance_tracks))
    {
        m_luminance_tracks = m_async_luminance_tracks.get();
        assert (m_luminance_tracks.size() == m_tracks.size ());
        for (int cc = 0; cc < m_luminance_tracks.size(); cc++)
        {
            m_tracks[cc]->setup(m_luminance_tracks[cc]);
        }
    }
    
    if (! have_movie () ) return;
    
    if (getCurrentFrame() >= getNumFrames())
    {
        if (! looping () ) pause ();
        else
            seekToStart();
    }
    
    mSurface = mFrameSet->getFrame(getCurrentFrame());
    
    if (mMoviePlay ) seekToFrame (getCurrentFrame() + 1);
    
}


void lifContext::draw_info ()
{
    if (! m_lifRef) return;
    
    
    std::strstream msg;
    msg << std::boolalpha << " Loop " << looping() << std::boolalpha << " Play " << mMoviePlay << " F " << setw(12) << int( getCurrentFrame ());
    std::string frame_str = msg.str();
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
        
        if (m_serie.channelCount)
        {
            for (int cc = 0; cc < m_tracks.size(); cc++)
            {
                m_tracks[cc]->draw();
            }
        }
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
