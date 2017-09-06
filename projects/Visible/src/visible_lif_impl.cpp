//
//  visible_lif_impl.cpp
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
#include "cinder/qtime/QuickTime.h"
#include "cinder/params/Params.h"
#include "cinder/ImageIo.h"
#include "CinderOpenCV.h"
#include "cinder/ip/Blend.h"
#include "opencv2/highgui.hpp"
#include "cinder/ip/Flip.h"
#include "otherIO/lifFile.hpp"
#include <strstream>
#include <algorithm>
#include <future>
#include <mutex>
#include "async_producer.h"
#include "cinder_xchg.hpp"
#include "visible_layout.hpp"
#include "vision/opencv_utils.hpp"
#include "vision/histo.h"
#include "core/stl_utils.hpp"
#include "sm_producer.h"
#include "algo_registry.hpp"
#include "tracker.h"

using namespace ci;
using namespace ci::app;
using namespace std;
using namespace svl;


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
    
    static layout vl ( ivec2 (10, 10));
    
    
    class algo_processor : public SingletonLight<algo_processor>
    {
    public:
        algo_processor ()
        {
            m_sm = std::shared_ptr<sm_producer> ( new sm_producer () );
        }
        
    private:
        const smProducerRef sm () const { return m_sm; }
        
        bool load_channels_from_images (const std::shared_ptr<qTimeFrameCache>& frames, bool LIF_data = true,
                                        uint8_t channel = 2)
        {
            // If it LIF data We will use multiple window.
            int64_t fn = 0;
            m_channel_images.clear();
            m_channel_images.resize (3);
            m_rois.resize (0);
            
            std::vector<std::string> names = {"Red", "Green","Blue"};
            
            while (frames->checkFrame(fn))
            {
                auto su8 = frames->getFrame(fn++);
                if (LIF_data)
                {
                    auto m3 = svl::NewRefMultiFromSurface (su8, names, fn);
                    for (auto cc = 0; cc < m3->planes(); cc++)
                        m_channel_images[cc].emplace_back(m3->plane(cc));
                    
                    // Assumption: all have the same 3 channel concatenated structure
                    // Fetch it only once
                    if (m_rois.empty())
                    {
                        for (auto cc = 0; cc < m3->planes(); cc++)
                        {
                            const iRect& ir = m3->roi(cc);
                            m_rois.emplace_back(vec2(ir.ul().first, ir.ul().second), vec2(ir.lr().first, ir.lr().second));
                        }
                        
                        
                    }
                }
                else
                {
                    // assuming 3 channels
                    for (auto cc = 0; cc < 3; cc++)
                    {
                        auto m1 = svl::NewChannelFromSurfaceAtIndex(su8,cc);
                        m_channel_images[cc].emplace_back(m1);
                    }
                }
            }
            
        }
        
        timed_double_t computeIntensityStatisticsResults (const roiWindow<P8U>& roi)
        {
            index_time_t ti;
            timed_double_t res;
            res.first = ti;
            res.second = histoStats::mean(roi) / 256.0;
            return res;
        }
        
        void entropiesToTracks (smProducerRef& sp, trackD1_t& track)
        {
            auto entropies = sp->shannonProjection ();
            auto medianLevel = sp->medianLeveledProjection();
            sm_producer::sMatrixProjection_t::const_iterator bee = entropies.begin();
            sm_producer::sMatrixProjection_t::const_iterator mee = medianLevel.begin();
            for (auto ss = 0; bee != entropies.end() && ss < frame_count(); ss++, bee++, mee++)
            {
                index_time_t ti;
                ti.first = ss;
                timed_double_t res;
                res.first = ti;
                //                res.second = *bee;
                res.second = *mee;
                track.second.emplace_back(res);
            }
        }
        
        size_t frame_count () const
        {
            if (m_channel_images[0].size() == m_channel_images[1].size() && m_channel_images[1].size() == m_channel_images[2].size())
                return m_channel_images[0].size();
            else return 0;
        }
        
    public:
         vector_of_trackD1s_t run (const std::shared_ptr<qTimeFrameCache>& frames, const std::vector<std::string>& names,
                        // TBD and 3 callbacks from algo registry to call
                        bool test_data = false)
        {
            
            load_channels_from_images(frames);
            
             vector_of_trackD1s_t  tracks;
            tracks.resize (names.size ());
            for (auto tt = 0; tt < names.size(); tt++)
                tracks[tt].first = names[tt];
            
            // Run Histogram on channels 0 and 1
            // Filling up tracks 0 and 1
            // Now Do Aci on the 3rd channel
            
            
            channel_images_t c0 = m_channel_images[0];
            channel_images_t c1 = m_channel_images[1];
            channel_images_t c2 = m_channel_images[2];
            
            for (auto ii = 0; ii < m_channel_images[0].size(); ii++)
            {
                tracks[0].second.emplace_back(computeIntensityStatisticsResults(c0[ii]));
                tracks[1].second.emplace_back(computeIntensityStatisticsResults(c1[ii]));
            }
            
            auto sp =  instance().sm();
            sp->load_images (c2);
            std::packaged_task<bool()> task([sp](){ return sp->operator()(0, 0);}); // wrap the function
            std::future<bool>  future_ss = task.get_future();  // get a future
            std::thread(std::move(task)).detach(); // launch on a thread
            future_ss.wait();
            entropiesToTracks(sp, tracks[2]);
            
            return tracks;
        }
        
        const std::vector<Rectf>& rois () const { return m_rois; }
        
        
    private:
        typedef std::vector<roiWindow<P8U>> channel_images_t;
        smProducerRef m_sm;
        channel_images_t m_images;
        std::vector<channel_images_t> m_channel_images;
        std::vector<Rectf> m_rois;
        Rectf m_all;
    };
    
     vector_of_trackD1s_t    get_mean_luminance_and_aci (const std::shared_ptr<qTimeFrameCache>& frames, const std::vector<std::string>& names,
                                               bool test_data = false)
    {
        return algo_processor().instance().run(frames, names, test_data);
    }
    
}


/////////////  lifContext Implementation  ////////////////


lifContext::lifContext(WindowRef& ww, const boost::filesystem::path& dp)
: sequencedImageContext(ww), mPath (dp)
{
    m_valid = false;
    m_type = Type::lif_file_viewer;
    
    if (mPath.string().empty())
        mPath = getOpenFilePath();
    
    m_valid = ! mPath.string().empty() && exists(mPath);
    
    if (is_valid())
    {
        mWindow->setTitle( mPath.filename().string() );
        mWindow->setSize(960, 540);
        mFont = Font( "Menlo", 12 );
        mSize = vec2( getWindowWidth(), getWindowHeight() / 12);
    }
    
    setup ();
}




void lifContext::looping (bool what)
{
    m_is_looping = what;
    if (m_is_looping)
        mUIParams.setOptions( "mode", "label=`Looping`" );
}


bool lifContext::looping ()
{
    return m_is_looping;
}

void lifContext::play ()
{
    if (! have_movie() || m_is_playing ) return;
    m_is_playing = true;
    mUIParams.setOptions( "mode", "label=`At Playing`" );
}

void lifContext::pause ()
{
    if (! have_movie() || ! m_is_playing ) return;
    m_is_playing = false;
    mUIParams.setOptions( "mode", "label=`At Pause`" );
}

void lifContext::play_pause_button ()
{
    if (! have_movie () ) return;
    if (m_is_playing)
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

void lifContext::edit_no_edit_button ()
{
    if (! have_movie () )
        return;
    
    // Flip
    setManualEditMode(!getManualEditMode());
    
    // If we are in Edit mode. Stop and Go to Start
    if (getManualEditMode())
    {
        mUIParams.setOptions( "mode", "label=`Edit`" );
        if (looping())
        {
            mUIParams.setOptions( "mode", "label=`Stopping`" );
            looping(false);
            seekToStart();
        }
    }
    else
        mUIParams.setOptions( "mode", "label=`Browse`" );
    
}

bool lifContext::have_movie ()
{
    bool have = m_lifRef && m_current_serie_ref >= 0 && mFrameSet && vl.isSet();
    //  if (! have )
    //     mUIParams.setOptions( "mode", "label=`Nothing Loaded`" );
    return have;
}

void lifContext::seekToEnd ()
{
    seekToFrame (getNumFrames() - 1);
    mUIParams.setOptions( "mode", "label=`@ End`" );
}

void lifContext::seekToStart ()
{
    seekToFrame(0);
    mUIParams.setOptions( "mode", "label=`@ Start`" );
}

int lifContext::getNumFrames ()
{
    return m_frameCount;
}

int lifContext::getCurrentFrame ()
{
    return m_seek_position;
}

time_spec_t lifContext::getCurrentTime ()
{
    if (m_seek_position >= 0 && m_seek_position < m_serie.timeSpecs.size())
        return m_serie.timeSpecs[m_seek_position];
    else return -1.0;
}



void lifContext::seekToFrame (int mark)
{
    m_seek_position = mark;
    mTimeMarker.from_count (m_seek_position);
    m_marker_signal.emit(mTimeMarker);
}


void lifContext::onMarked ( marker_info& t)
{
    seekToFrame(t.current_frame());
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
    mUIParams = params::InterfaceGl( "Lif Player ", toPixels( ivec2( 200, 300 )));
    mUIParams.setPosition(getWindowSize() / 3);
    
    
    // Load the validated movie file
    loadLifFile ();
    
    clear_movie_params();
    
    if( m_valid )
    {
       	m_type = Type::qtime_viewer;
        mUIParams.addSeparator();
        
        m_series_names.clear ();
        for (auto ss = 0; ss < m_series_book.size(); ss++)
            m_series_names.push_back (m_series_book[ss].name);
        
        m_perform_names.clear ();
        m_perform_names.push_back("Manual Cell End Tracing");
        
        // Add an enum (list) selector.
        
        m_selected_serie_index = 0;
        m_current_serie_ref = std::shared_ptr<lifIO::LifSerie>();
        
        mUIParams.addParam( "Series ", m_series_names, &m_selected_serie_index )
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
        
        
        mUIParams.addSeparator();
        {
            const std::function<void (int)> setter = std::bind(&lifContext::seekToFrame, this, std::placeholders::_1);
            const std::function<int ()> getter = std::bind(&lifContext::getCurrentFrame, this);
            mUIParams.addParam ("Current Time Step", setter, getter);
        }
        
        mUIParams.addSeparator();
        mUIParams.addButton("Play / Pause ", bind( &lifContext::play_pause_button, this ) );
        mUIParams.addSeparator();
        mUIParams.addButton(" Loop ", bind( &lifContext::loop_no_loop_button, this ) );
        mUIParams.addSeparator();
        
        mUIParams.addSeparator();
        mUIParams.addButton(" Edit ", bind( &lifContext::edit_no_edit_button, this ) );
        mUIParams.addSeparator();
        mUIParams.addText( "mode", "label=`Browse`" );
        
        
        //        {
        //            const std::function<void (float)> setter = std::bind(&lifContext::setZoom, this, std::placeholders::_1);
        //            const std::function<float (void)> getter = std::bind(&lifContext::getZoom, this);
        //            mUIParams.addParam( "Zoom", setter, getter);
        //        }
        
        
    }
}

void lifContext::clear_movie_params ()
{
    m_seek_position = 0;
    m_is_playing = false;
    m_is_looping = false;
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
    if ( ! (m_lifRef || ! m_current_serie_ref) )
        return;
    
    try {
        
        // Create the frameset and assign the channel names
        // Fetch the media info
        mFrameSet = qTimeFrameCache::create (*m_current_serie_ref);
        mFrameSet->channel_names (m_series_book[m_selected_serie_index].channel_names);
        mMediaInfo = mFrameSet->media_info();
        
        vl.init (app::getWindow(), mFrameSet->media_info());
        
        if (m_valid)
        {
            getWindow()->setTitle( mPath.filename().string() );
            
            ci_console() <<  m_series_book.size() << "  Series  " << std::endl;
            
            const tiny_media_info tm = mFrameSet->media_info ();
            getWindow()->getApp()->setFrameRate(tm.getFramerate() / 5.0);
            
            mScreenSize = tm.getSize();
            m_frameCount = tm.getNumFrames ();
            mTimeMarker = marker_info (tm.getNumFrames (), tm.getDuration());
            
            
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
                
                for (Graph1DRef gr : m_tracks)
                {
                    m_marker_signal.connect(std::bind(&graph1D::set_marker_position, gr, std::placeholders::_1));
                }
                
                mTimeLineSlider.mBounds = vl.display_timeline_rect();
                mTimeLineSlider.mTitle = "Time Line";
                m_marker_signal.connect(std::bind(&tinyUi::TimeLineSlider::set_marker_position, mTimeLineSlider, std::placeholders::_1));
                mWidgets.push_back( &mTimeLineSlider );
                
                getWindow()->getSignalMouseDrag().connect( [this] ( MouseEvent &event ) { processDrag( event.getPos() ); } );
                
            }
            
            // Launch Average Luminance Computation
            m_async_luminance_tracks = std::async(std::launch::async, get_mean_luminance_and_aci,
                                                  mFrameSet, m_serie.channel_names, false);
        }
    }
    catch( const std::exception &ex ) {
        console() << ex.what() << endl;
        return;
    }
}

void lifContext::processDrag( ivec2 pos )
{
    if( mTimeLineSlider.hitTest( pos ) ) {
        mTimeMarker.from_norm(mTimeLineSlider.mValueScaled);
        seekToFrame(mTimeMarker.current_frame());
    }
    
}



void lifContext::mouseMove( MouseEvent event )
{
    mMouseInImage = false;
    mMouseInGraphs  = -1;
    
    if (! have_movie () ) return;
    
    mMouseInImage = get_image_display_rect().contains(event.getPos());
    if (mMouseInImage)
    {
        mMouseInImagePosition = event.getPos();
        update_instant_image_mouse ();
    }
    
    if (vl.display_timeline_rect().contains(event.getPos()))
    {
        std::vector<float> dds (m_track_rects.size());
        for (auto pp = 0; pp < m_track_rects.size(); pp++) dds[pp] = m_track_rects[pp].distanceSquared(event.getPos());
        
        auto min_iter = std::min_element(dds.begin(),dds.end());
        mMouseInGraphs = min_iter - dds.begin();
    }
    
}


void lifContext::mouseDrag( MouseEvent event )
{
    for (Graph1DRef graphRef : m_tracks)
        graphRef->mouseDrag( event );
    
    if (getManualEditMode() && mMouseInImage && channelIndex() == 2)
    {
        mLengthPoints.second = event.getPos();
    }
}


void lifContext::mouseDown( MouseEvent event )
{
    for (Graph1DRef graphRef : m_tracks )
    {
        graphRef->mouseDown( event );
        graphRef->get_marker_position(mTimeMarker);
        signalMarker.emit(mTimeMarker);
    }
    
    // If we are in the Visible Channel
    if (getManualEditMode() && mMouseInImage && channelIndex() == 2)
    {
        mLengthPoints.first = event.getPos();
        mLengthPoints.second = mLengthPoints.first;
    }
}


void lifContext::mouseUp( MouseEvent event )
{
    for (Graph1DRef graphRef : m_tracks)
        graphRef->mouseUp( event );
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
        
    }
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
    return vl.display_frame_rect();
}

bool lifContext::is_valid () { return m_valid && is_context_type(guiContext::lif_file_viewer); }

void lifContext::resize ()
{
    if (! have_movie () ) return;
    
    vl.update_window_size(getWindowSize ());
    mSize = vec2( getWindowWidth(), getWindowHeight() / 12);
    vl.plot_rects(m_track_rects);
    for (int cc = 0; cc < m_tracks.size(); cc++)
    {
        m_tracks[cc]->setRect (m_track_rects[cc]);
    }
    
    mTimeLineSlider.mBounds = vl.display_timeline_rect();
    
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
    
    if (m_is_playing ) seekToFrame (getCurrentFrame() + 1);
    
}

void lifContext::update_instant_image_mouse ()
{
    uint32_t m_instance_channel;
    auto mouseInChannelPos = mMouseInImagePosition;
    auto image_pos = vl.display2image(mMouseInImagePosition);
    // LIF 3 channel organization. Channel height is 1/3 of image height
    // Channel Index is pos.y / channel_height,
    // In channel x is pos.x, In channel y is pos.y % channel_height
    uint32_t channel_height = mMediaInfo.getChannelSize().y;
    
    m_instance_channel = (int) image_pos.y / (int) channel_height;
    image_pos.y = ((int) image_pos.y) % channel_height;
    m_instant_mouse_image_pos = image_pos;
}

gl::TextureRef lifContext::pixelInfoTexture ()
{
    if (! mMouseInImage) return gl::TextureRef ();
    TextLayout lout;
    
    std::string pos = " c: " + toString(channelIndex()) +  "[" + to_string(mMouseInImagePosition.x) + "," + to_string(mMouseInImagePosition.y) + "] i [" +
    to_string(imagePos().x) + "," + to_string(imagePos().y) + "]";
    
    
    
    lout.clear( ColorA::gray( 0.2f, 0.5f ) );
    lout.setFont( Font( "Menlo", 14 ) );
    lout.setColor( ColorA(0.8,0.2,0.1,1.0) );
    lout.setLeadingOffset( 3 );
    lout.addRightLine( pos );
    
    
    return gl::Texture::create( lout.render( true ));
    
}


void lifContext::draw_info ()
{
    if (! m_lifRef) return;
    
    std::string seri_str = m_serie.info();
    gl::setMatricesWindow( getWindowSize() );
    
    gl::ScopedBlendAlpha blend_;
    
    {
        gl::ScopedColor (getManualEditMode() ? ColorA( 0.25f, 0.5f, 1, 1 ) : ColorA::gray(1.0));
        gl::drawStrokedRect(get_image_display_rect(), 3.0f);
    }
    {
        gl::ScopedColor (ColorA::gray(0.0));
        gl::drawStrokedRect(vl.display_timeline_rect(), 3.0f);
    }
    {
        gl::ScopedColor (ColorA::gray(0.1));
        gl::drawStrokedRect(vl.display_plots_rect(), 3.0f);
    }
    
    
    TextLayout layoutR;
    
    layoutR.clear( ColorA::gray( 0.2f, 0.5f ) );
    layoutR.setFont( Font( "Arial", 18 ) );
    layoutR.setColor( Color::white() );
    layoutR.setLeadingOffset( 3 );
    layoutR.addRightLine( seri_str  );
    
    
    auto texR = gl::Texture::create( layoutR.render( true ) );
    Rectf tbox = texR->getBounds();
    tbox = tbox.getCenteredFit(getWindowBounds(), true);
    tbox.offset(vec2(0,getWindowHeight() - tbox.getHeight() - tbox.getY1()));
    gl::draw( texR, tbox);
    
    tinyUi::drawWidgets(mWidgets);
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
        
        auto pt = pixelInfoTexture ();
        if (pt)
        {
            gl::draw(pt, dr.scaled(0.2));
        }
        
        if (getManualEditMode())
        {
            gl::ScopedColor (ColorA( 0.25f, 0.5f, 1, 1 ));
            gl::drawLine(mLengthPoints.first, mLengthPoints.second);
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
    
    mUIParams.draw();
    
    
}

#if 0

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
