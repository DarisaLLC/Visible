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
#include "LifContext.h"
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
#include "algo_Lif.hpp"
#include "signaler.h"

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
        mFont = Font( "Menlo", 18 );
        mSize = vec2( getWindowWidth(), getWindowHeight() / 12);
    }
    
    m_lifProcRef = std::make_shared<lif_processor> ();
    
    std::function<void ()> content_loaded_cb = std::bind (&lifContext::signal_content_loaded, this);
    boost::signals2::connection ml_connection = m_lifProcRef->registerCallback(content_loaded_cb);
    
    std::function<void ()> flu_stats_available_cb = std::bind (&lifContext::signal_flu_stats_available, this);
    boost::signals2::connection flu_connection = m_lifProcRef->registerCallback(flu_stats_available_cb);
    
    
    
    
    std::function<void (int&)> sm1d_available_cb = boost::bind (&lifContext::signal_sm1d_available, this, _1);
    boost::signals2::connection nl_connection = m_lifProcRef->registerCallback(sm1d_available_cb);
    std::function<void (int&,int&)> sm1dmed_available_cb = boost::bind (&lifContext::signal_sm1dmed_available, this, _1, _2);
    boost::signals2::connection ol_connection = m_lifProcRef->registerCallback(sm1dmed_available_cb);
    
    setup ();
}

            /************************
             *
             *  Validation, Clear & Log
             *
             ************************/

void lifContext::clear_movie_params ()
{
    m_seek_position = 0;
    m_is_playing = false;
    m_is_looping = false;
    m_zoom.x = m_zoom.y = 1.0f;
}

bool lifContext::have_movie ()
{
    bool have = m_lifRef && m_current_serie_ref >= 0 && mFrameSet && vl.isSet();
    //  if (! have )
    //     mUIParams.setOptions( "mode", "label=`Nothing Loaded`" );
    return have;
}


bool lifContext::is_valid () { return m_valid && is_context_type(guiContext::lif_file_viewer); }



            /************************
             *
             *  UI Bind Functions
             *
             ************************/

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
        if (mContainer.getNumChildren())
            mContainer.removeChildren();
    }
    else
        mUIParams.setOptions( "mode", "label=`Browse`" );
    
}


void lifContext::analyze_analyzing_button()
{
    if (! have_movie () )
        return;
    
    // Flip
    setAnalyzeMode(!getAnalyzeMode());
#if 1
    // If we are in Edit mode. Stop and Go to Start
    if (getAnalyzeMode())
    {
        mUIParams.setOptions( "state", "label=`Processing`" );
        if (looping())
        {
            looping(false);
            seekToStart();
        }
    }
    else
        mUIParams.setOptions( "state", "label=`Ready`" );
#endif
}

        /************************
         *
         *  Seek Processing
         *
         ************************/

void lifContext::seekToEnd ()
{
    seekToFrame (m_clips[m_current_clip_index].end);
//    seekToFrame (getNumFrames() - 1);
    mUIParams.setOptions( "mode", "label=`@ End`" );
}

void lifContext::seekToStart ()
{
    seekToFrame(m_clips[m_current_clip_index].anchor);
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
    mAuxTimeMarker.from_count (m_seek_position);
    m_aux_marker_signal.emit(mTimeMarker);
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


            /************************
             *
             *  Navigation UI
             *
             ************************/

void lifContext::processDrag( ivec2 pos )
{
    for (Widget* wPtr : mWidgets)
    {
        if( wPtr->hitTest( pos ) ) {
            mTimeMarker.from_norm(wPtr->valueScaled());
            seekToFrame(mTimeMarker.current_frame());
        }
    }
}

void  lifContext::mouseWheel( MouseEvent event )
{
#if 0
    if( mMouseInTimeLine )
        mTimeMarker.from_norm(mTimeLineSlider.mValueScaled);
    seekToFrame(mTimeMarker.current_frame());
}

mPov.adjustDist( event.getWheelIncrement() * -5.0f );
#endif

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
    
    if (vl.display_plots_rect().contains(event.getPos()))
    {
        std::vector<float> dds (vl.plot_rects().size());
        for (auto pp = 0; pp < vl.plot_rects().size(); pp++) dds[pp] = vl.plot_rects()[pp].distanceSquared(event.getPos());
        
        auto min_iter = std::min_element(dds.begin(),dds.end());
        mMouseInGraphs = min_iter - dds.begin();
    }
    
    mMouseInWidgets.resize(0);
    for (Widget* wPtr : mWidgets)
    {
        bool inside = wPtr->contains(event.getPos());
        mMouseInWidgets.push_back(inside);
    }
}


void lifContext::mouseDrag( MouseEvent event )
{
    for (Graph1DRef graphRef : m_plots)
        graphRef->mouseDrag( event );
    
    if (getManualEditMode() && mMouseInImage && channelIndex() == 2)
    {
        mLengthPoints.second = event.getPos();
    }
}


void lifContext::mouseDown( MouseEvent event )
{
    for (Graph1DRef graphRef : m_plots )
    {
        graphRef->mouseDown( event );
        graphRef->get_marker_position(mTimeMarker);
        graphRef->get_marker_position(mAuxTimeMarker);
    }
    
}


void lifContext::mouseUp( MouseEvent event )
{
    for (Graph1DRef graphRef : m_plots)
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
    else if( event.getChar() == 'b' ) {
        getWindow()->setBorderless( ! getWindow()->isBorderless() );
    }
    else if( event.getChar() == 't' ) {
        getWindow()->setAlwaysOnTop( ! getWindow()->isAlwaysOnTop() );
    }
    
    
    // these keys only make sense if there is an active movie
    if( have_movie () ) {
        if( event.getCode() == KeyEvent::KEY_LEFT ) {
            pause();
            seekToFrame (getCurrentFrame() - 1);
        }
        else if( event.getCode() == KeyEvent::KEY_RIGHT ) {
            pause ();
            seekToFrame (getCurrentFrame() + 1);
        }
        else if( event.getChar() == ' ' ) {
            play_pause_button();
        }
        
    }
}


void lifContext::setMedianCutOff (uint32_t newco)
{
    uint32_t tmp = newco % 100; // pct
    auto sp =  m_lifProcRef->smFilterRef();
    if (! sp ) return;
    uint32_t current (sp->get_median_levelset_pct () * 100);
    if (tmp == current) return;
    sp->set_median_levelset_pct (tmp / 100.0f);
    m_lifProcRef->update();
}

uint32_t lifContext::getMedianCutOff () const
{
    auto sp =  m_lifProcRef->smFilterRef();
    if (sp)
    {
        uint32_t current (sp->get_median_levelset_pct () * 100);
        return current;
    }
    return 0;
}


            /************************
             *
             *  Setup & Load File
             *
             ************************/

void lifContext::setup()
{
    srand( 133 );
    mUIParams = params::InterfaceGl( "Lif Player ", toPixels( ivec2( 300, 400 )));
    mUIParams.setPosition(getWindowSize() / 3);
    m_contraction_names = m_contraction_none;
    
    m_prev_selected_index = -1;
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
        
        // If it is first time
        if (m_prev_selected_index < 0)
        {
            m_cur_selected_index = 0;
            m_current_serie_ref = std::shared_ptr<lifIO::LifSerie>();
        }
        
        mUIParams.addParam( "Series ", m_series_names, &m_cur_selected_index )
        //        .keyDecr( "[" )
        //        .keyIncr( "]" )
        .updateFn( [this]
                  {
                      bool exists = m_prev_selected_index  >= 0 && m_prev_selected_index == m_cur_selected_index;
                      if (! exists && m_cur_selected_index >= 0 && m_cur_selected_index < m_series_names.size() )
                      {
                          m_serie = m_series_book[m_cur_selected_index];
                          m_current_serie_ref = std::shared_ptr<lifIO::LifSerie>(&m_lifRef->getSerie(m_cur_selected_index), stl_utils::null_deleter());
                          loadCurrentSerie ();
                          m_prev_selected_index = m_cur_selected_index;
                      }
                  });
    
        mUIParams.addSeparator();
        mUIParams.addSeparator();
        
        {
            const std::function<void (int)> setter = std::bind(&lifContext::seekToFrame, this, std::placeholders::_1);
            const std::function<int ()> getter = std::bind(&lifContext::getCurrentFrame, this);
            mUIParams.addParam ("Current Time Step", setter, getter);
        }
        
        clip entire;
        entire.begin = 0;
        entire.end = getNumFrames()-2;
        entire.anchor = 0;
        m_clips.push_back(entire);
        
        mUIParams.addParam( "Select ", m_contraction_names, &m_current_clip_index )
        .updateFn( [this]
                  {
                      if (m_current_clip_index >= 0 && m_current_clip_index < m_contraction_names.size() )
                      {
                          std::cout << m_contraction_names[m_current_clip_index] << std::endl;
                      }
                  });
        
        mUIParams.addSeparator();
        mUIParams.addButton("Play / Pause ", bind( &lifContext::play_pause_button, this ) );
        mUIParams.addSeparator();
        mUIParams.addParam( " Loop ", &m_is_looping ).keyIncr( "l" );
        mUIParams.addSeparator();
        mUIParams.addButton(" Edit ", bind( &lifContext::edit_no_edit_button, this ) );
        mUIParams.addSeparator();
        mUIParams.addText( "mode", "label=`Browse`" );
        
        mUIParams.addSeparator();
        mUIParams.addButton(" Analyze ", bind( &lifContext::analyze_analyzing_button, this ) );
        mUIParams.addSeparator();
        mUIParams.addText( "state", "label=`Ready`" );
        mUIParams.addSeparator();
        
        {
            // Add a param with no target, but instead provide setter and getter functions.
            const std::function<void(uint32_t)> setter	= std::bind(&lifContext::setMedianCutOff, this, std::placeholders::_1 );
            const std::function<uint32_t()> getter	= std::bind( &lifContext::getMedianCutOff, this);
            
            // Attach a callback that is fired after a target is updated.
            mUIParams.addParam( "CutOff Pct", setter, getter );
        }
    }
}


void lifContext::loadLifFile ()
{
    if ( ! mPath.empty () )
    {
        ci_console () << mPath.string ();
        
        try {
            
            m_lifRef =  std::shared_ptr<lifIO::LifReader> (new lifIO::LifReader (mPath.string()));
            get_series_info (m_lifRef);
            ci_console() <<  std::endl << m_series_book.size() << "  Series  " << std::endl;
            
        }
        catch( ... ) {
            ci_console() << "Unable to load the movie." << std::endl;
            return;
        }
        
    }
}


                /************************
                 *
                 *
                 *  Call Backs
                 *
                 ************************/

void lifContext::signal_sm1d_available (int& dummy)
{
    static int i = 0;
    std::cout << "sm1d available: " << ++i << std::endl;
}

void lifContext::signal_sm1dmed_available (int& dummy, int& dummy2)
{
    static int ii = 0;
    
    if (haveTracks())
    {
        auto tracksRef = m_pci_trackWeakRef.lock();
        if (!tracksRef->at(0).second.empty())
            m_plots[2]->setup(tracksRef->at(0));
    }
    
    std::cout << "sm1dmed available: " << ++ii << std::endl;
}

void lifContext::signal_content_loaded ()
{
    std::cout << std::endl << " Images Loaded  " << std::endl;
}
void lifContext::signal_flu_stats_available ()
{
    std::cout << "Flu Stats Available " << std::endl;
}



void lifContext::signal_frame_loaded (int& findex, double& timestamp)
{
    //    frame_indices.push_back (findex);
    //    frame_times.push_back (timestamp);
    //     std::cout << frame_indices.size() << std::endl;
}


        /************************
         *
         *  Load Serie & Setup Widgets
         *
         ************************/

void lifContext::add_plot_widgets (const int channel_count)
{
    std::lock_guard<std::mutex> lock(m_track_mutex);
    
    assert (  vl.plot_rects().size() >= channel_count);
    
    m_plots.resize (0);
    
    for (int cc = 0; cc < channel_count; cc++)
    {
        m_plots.push_back( Graph1DRef (new graph1D (m_current_serie_ref->getChannels()[cc].getName(),
                                                    vl.plot_rects() [cc])));
    }
    m_plots[0]->strokeColor = ColorA(0.2,0.8,0.1,1.0);
    m_plots[1]->strokeColor = ColorA(0.8,0.2,0.1,1.0);
    m_plots[2]->strokeColor = ColorA(0.0,0.0,0.0,1.0);
    
    for (Graph1DRef gr : m_plots)
    {
        gr->backgroundColor = ColorA( 0.3, 0.3, 0.3, 0.3 );
        m_marker_signal.connect(std::bind(&graph1D::set_marker_position, gr, std::placeholders::_1));
    }
    
    mMainTimeLineSlider.setBounds (vl.display_timeline_rect());
    mMainTimeLineSlider.clear_timepoint_markers();
    mMainTimeLineSlider.setTitle ("Time Line");
    m_marker_signal.connect(std::bind(&tinyUi::TimeLineSlider::set_marker_position, mMainTimeLineSlider, std::placeholders::_1));
    mWidgets.push_back( &mMainTimeLineSlider );

    
    getWindow()->getSignalMouseDrag().connect( [this] ( MouseEvent &event ) { processDrag( event.getPos() ); } );
    
}


void lifContext::loadCurrentSerie ()
{
    
    if ( ! (m_lifRef || ! m_current_serie_ref) )
        return;
    
    try {
        
        mWidgets.clear ();
        
        // Create the frameset and assign the channel names
        // Fetch the media info
        mFrameSet = qTimeFrameCache::create (*m_current_serie_ref);
        if (! mFrameSet || ! mFrameSet->isValid())
        {
            ci_console() << "Serie had 1 or no frames " << std::endl;
            return;
        }
        
        mFrameSet->channel_names (m_series_book[m_cur_selected_index].channel_names);
        mMediaInfo = mFrameSet->media_info();
        vl.init (app::getWindow(), mFrameSet->media_info());
        mMediaInfo.output(std::cout);
        
        std::async(std::launch::async, &lif_processor::load, m_lifProcRef.get(), mFrameSet, m_serie.channel_names);
        
        if (m_valid)
        {
            auto title = m_series_names[m_cur_selected_index] + " @ " + mPath.filename().string();
            getWindow()->setTitle( title );
            const tiny_media_info tm = mFrameSet->media_info ();
            getWindow()->getApp()->setFrameRate(tm.getFramerate() * 2);

            
            mScreenSize = tm.getSize();
            m_frameCount = tm.getNumFrames ();
            mSurface = Surface8u::create (int32_t(mScreenSize.x), int32_t(mScreenSize.y), true);
            
            mTimeMarker = marker_info (tm.getNumFrames (), tm.getDuration());
            mAuxTimeMarker = marker_info (tm.getNumFrames (), tm.getDuration());
            
            // Set window size according to layout
            //  Channel             Data
            //  1
            //  2
            //  3
            
            ivec2 window_size (vl.desired_window_size());
            setWindowSize(window_size);
            int channel_count = (int) tm.getNumChannels();
            add_plot_widgets(channel_count);
            seekToStart();
            play();
            

            m_async_luminance_tracks = std::async(std::launch::async,
                                                  &lif_processor::run_flu_statistics,
                                                  m_lifProcRef.get());

            m_async_pci_tracks = std::async(std::launch::async, &lif_processor::run_pci, m_lifProcRef.get());
        }
    }
    catch( const std::exception &ex ) {
        console() << ex.what() << endl;
        return;
    }
}



            /************************
             *
             *  Update & Draw
             *
             ************************/

Rectf lifContext::get_image_display_rect ()
{
    vl.update_window_size(getWindowSize ());
    return vl.display_frame_rect();
}



void  lifContext::update_log (const std::string& msg)
{
    if (msg.length() > 2)
        mLog = msg;
    TextBox tbox = TextBox().alignment( TextBox::RIGHT).font( mFont ).size( mSize ).text( mLog );
    tbox.setColor( Color( 1.0f, 0.65f, 0.35f ) );
    tbox.setBackgroundColor( ColorA( 0.3f, 0.3f, 0.3f, 0.4f )  );
    tbox.measure();
    mTextTexture = gl::Texture2d::create( tbox.render() );
}


void lifContext::resize ()
{
    if (! have_movie () ) return;
    
    vl.update_window_size(getWindowSize ());
    mSize = vec2( getWindowWidth(), getWindowHeight() / 12);
    
    for (int cc = 0; cc < vl.plot_rects().size(); cc++)
    {
        m_plots[cc]->setRect (vl.plot_rects()[cc]);
    }
    
    mMainTimeLineSlider.setBounds (vl.display_timeline_rect());
}

bool lifContext::haveTracks()
{
    return ! m_trackWeakRef.expired() && ! m_pci_trackWeakRef.expired();
}

void lifContext::update ()
{
    mContainer.update();
    vl.update_window_size(getWindowSize ());
    
    // If Plots are ready, set them up
    // It is ready only for new data
    if ( is_ready (m_async_luminance_tracks) )
        m_trackWeakRef = m_async_luminance_tracks.get();
    if ( is_ready (m_async_pci_tracks))
        m_pci_trackWeakRef = m_async_pci_tracks.get();
    
    if (! m_trackWeakRef.expired())
    {
        auto tracksRef = m_trackWeakRef.lock();
        m_plots[0]->setup(tracksRef->at(0));
        m_plots[1]->setup(tracksRef->at(1));
    }
    if ( ! m_pci_trackWeakRef.expired())
    {
        auto tracksRef = m_pci_trackWeakRef.lock();
        if (!tracksRef->at(0).second.empty())
            m_plots[2]->setup(tracksRef->at(0));
    }
    
    if (getCurrentFrame() > getNumFrames())
    {
        if (! looping () ) seekToEnd();
        else
            seekToStart();
    }
    
    if (have_movie ())
        mSurface = mFrameSet->getFrame(getCurrentFrame());
    
    if (m_is_playing )
    {
        update_instant_image_mouse ();
        seekToFrame (getCurrentFrame() + 1);
    }
    // TBD: this is inefficient as it continuously clears and updates contraction peak and time markers
    // should be signal driven.

    mMainTimeLineSlider.clear_timepoint_markers();
    if (m_lifProcRef && m_lifProcRef->smFilterRef() && ! m_lifProcRef->smFilterRef()->low_peaks().empty())
    {
        for(auto lowp : m_lifProcRef->smFilterRef()->low_peaks())
        {
            tinyUi::timepoint_marker_t tm;
            tm.first = float(lowp.first) / m_lifProcRef->smFilterRef()->size();
            tm.second = ColorA (0.9, 0.3, 0.1, 0.75);
            mMainTimeLineSlider.add_timepoint_marker(tm);
        }
    }

    // Update text texture with most recent text
    auto num_str = to_string( int( getCurrentFrame ()));
    std::string frame_str = m_is_playing ? string( "Playing: " + num_str) : string ("Paused: " + num_str);
    update_log (frame_str);
}

void lifContext::update_instant_image_mouse ()
{
    auto image_pos = vl.display2image(mMouseInImagePosition);
    // LIF 3 channel organization. Channel height is 1/3 of image height
    // Channel Index is pos.y / channel_height,
    // In channel x is pos.x, In channel y is pos.y % channel_height
    uint32_t channel_height = mMediaInfo.getChannelSize().y;
    m_instant_channel = ((int) image_pos.y) / channel_height;
    m_instant_mouse_image_pos = image_pos;
    if (mSurface)
    {
        m_instant_pixel_Color = mSurface->getPixel(m_instant_mouse_image_pos);
    }
    
    
}

gl::TextureRef lifContext::pixelInfoTexture ()
{
    if (! mMouseInImage) return gl::TextureRef ();
    TextLayout lout;
    const auto names = m_series_book[m_cur_selected_index].channel_names;
    auto channel_name = (m_instant_channel < names.size()) ? names[m_instant_channel] : " ";
    
    // LIF has 3 Channels.
    uint32_t channel_height = mMediaInfo.getChannelSize().y;
    std::string pos = " " + toString(((int)m_instant_pixel_Color.g)) + " @ [ " +
        to_string(imagePos().x) + "," + to_string(imagePos().y % channel_height) + "]";
    vec2                mSize(250, 100);
    TextBox tbox = TextBox().alignment( TextBox::LEFT).font( mFont ).size( ivec2( mSize.x, TextBox::GROW ) ).text( pos );
    tbox.setFont( Font( "Times New Roman", 34 ) );
    if (channel_name == "Red")
        tbox.setColor( ColorA(0.8,0.2,0.1,1.0) );
    else if (channel_name == "Green")
        tbox.setColor( ColorA(0.2,0.8,0.1,1.0) );
    else
        tbox.setColor( ColorA(0.0,0.0,0.0,1.0) );
    tbox.setBackgroundColor( ColorA( 0.3, 0.3, 0.3, 0.3 ) );
    ivec2 sz = tbox.measure();
    return  gl::Texture2d::create( tbox.render() );
}


void lifContext::draw_info ()
{
    if (! m_lifRef) return;
    
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
    
    
    auto texR = pixelInfoTexture ();
    if (texR)
    {
        Rectf tbox = texR->getBounds();
        tbox.offset(mMouseInImagePosition);
        gl::draw( texR, tbox);
    }
    
    
    if (mTextTexture)
    {
        Rectf textrect (0.0, getWindowHeight() - mTextTexture->getHeight(), getWindowWidth(), getWindowHeight());
        gl::draw(mTextTexture, textrect);
    }
    
//    if (haveTracks())
    tinyUi::drawWidgets(mWidgets);
    
}


void lifContext::draw ()
{
    
    if( have_movie()  && mSurface )
    {
        Rectf dr = get_image_display_rect();
        assert(dr.getWidth() > 0 && dr.getHeight() > 0);
        
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
        
        
        if (getManualEditMode())
        {
            gl::ScopedColor (ColorA( 0.25f, 0.5f, 1, 1 ));
            gl::drawLine(mLengthPoints.first, mLengthPoints.second);
        }
        
        
        if (m_serie.channelCount)
        {
            for (int cc = 0; cc < m_plots.size(); cc++)
            {
                m_plots[cc]->setRect (vl.plot_rects()[cc]);
                m_plots[cc]->draw();
            }
        }
        
        mContainer.draw();
        draw_info ();
        
        
    }
    
    mUIParams.draw();
    
    
}

