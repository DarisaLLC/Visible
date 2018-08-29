//
//  visible_lif_impl.cpp
//  Visible
//
//  Created by Arman Garakani on 5/13/16.
//
//


#include <stdio.h>
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
#include "contraction.hpp"
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
    static layoutManager sLayoutMgr ( ivec2 (10, 10));
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
    
    m_lifProcRef = std::make_shared<lif_processor> ();
    
    if (is_valid())
    {
        mWindow->setTitle( mPath.filename().string() );
        mWindow->setSize(960, 540);
        mFont = Font( "Menlo", 18 );
        mSize = vec2( getWindowWidth(), getWindowHeight() / 12);
    }
    
    
    std::function<void (int64_t&)> content_loaded_cb = boost::bind (&lifContext::signal_content_loaded, this, _1);
    boost::signals2::connection ml_connection = m_lifProcRef->registerCallback(content_loaded_cb);
    
    std::function<void ()> flu_stats_available_cb = std::bind (&lifContext::signal_flu_stats_available, this);
    boost::signals2::connection flu_connection = m_lifProcRef->registerCallback(flu_stats_available_cb);
    
    std::function<void (int&)> sm1d_available_cb = boost::bind (&lifContext::signal_sm1d_available, this, _1);
    boost::signals2::connection nl_connection = m_lifProcRef->registerCallback(sm1d_available_cb);
    std::function<void (int&,int&)> sm1dmed_available_cb = boost::bind (&lifContext::signal_sm1dmed_available, this, _1, _2);
    boost::signals2::connection ol_connection = m_lifProcRef->registerCallback(sm1dmed_available_cb);
    
    std::function<void (lif_processor::contractionContainer_t&)> contraction_available_cb = boost::bind (&lifContext::signal_contraction_available, this, _1);
    boost::signals2::connection contraction_connection = m_lifProcRef->registerCallback(contraction_available_cb);
    
    
    setup ();
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

void lifContext::signal_content_loaded (int64_t& processed_frame_count )
{
    std::cout << m_frameCount << " Samples in Media  " << std::endl;
    std::cout << processed_frame_count << " Images Processed  " << std::endl;
}
void lifContext::signal_flu_stats_available ()
{
    std::cout << "Flu Stats Available " << std::endl;
}

void lifContext::signal_contraction_available (lif_processor::contractionContainer_t& contras)
{
    std::lock_guard<std::mutex> guard(m_clip_mutex);
    
    // TimeLine Markers
    mMainTimeLineSlider.clear_timepoint_markers();
    m_contractions = contras;
    std::cout << m_contractions.size() << std::endl;
    
    m_contraction_names.clear();
    m_contraction_names.push_back("Entire");
    
    if (contras.empty()) return;
    
    // @note: We are handling one contraction for now.
    uint32_t index = 0;
    m_clips.resize(1 + m_contractions.size());
    auto clipItr = m_clips.begin();
    clipItr++;
    string name = " C " + toString(index) + " ";
    m_contraction_names.push_back(name);
    
    clipItr->begin = m_contractions[0].contraction_start.first;
    clipItr->end = m_contractions[0].relaxation_end.first;
    clipItr->anchor = m_contractions[0].contraction_peak.first;
    tinyUi::timepoint_marker_t tm;
    tm.first =clipItr->anchor / ((float)m_frameCount);
    tm.second = ColorA (0.9, 0.3, 0.1, 0.75);
    mMainTimeLineSlider.add_timepoint_marker(tm);
    std::cout << "Contration Time Point Added  " << tm.first << std::endl;
    
}



void lifContext::signal_frame_loaded (int& findex, double& timestamp)
{
    //    frame_indices.push_back (findex);
    //    frame_times.push_back (timestamp);
    //     std::cout << frame_indices.size() << std::endl;
}

                    /************************
                     *
                     *  CLIP Processing
                     *
                     ************************/

int lifContext::get_current_clip_index () const
{
    std::lock_guard<std::mutex> guard(m_clip_mutex);
    return m_current_clip_index;
}

void lifContext::set_current_clip_index (int cindex) const
{
    std::lock_guard<std::mutex> guard(m_clip_mutex);
    m_current_clip_index = cindex;
}

void lifContext::reset_entire_clip (const size_t& frame_count) const
{
    std::lock_guard<std::mutex> guard(m_clip_mutex);
    m_clips.resize(1);
    m_clips[0].end = frame_count - 2;
    m_contraction_names.resize(1);
    m_contraction_names[0] = " Entire ";
    m_current_clip_index = 0;
}

void lifContext::reset_clips () const
{
    std::lock_guard<std::mutex> guard(m_clip_mutex);
    m_clips.resize(1);
    m_clips[0].end = 0;
    m_contraction_names.resize(1);
    m_contraction_names[0] = " Entire ";
    m_current_clip_index = 0;
}


                /************************
                 *
                 *  Validation, Clear & Log
                 *
                 ************************/

void lifContext::clear_playback_params ()
{
    m_seek_position = 0;
    m_is_playing = false;
    m_is_looping = false;
    m_zoom.x = m_zoom.y = 1.0f;
}

bool lifContext::have_lif_serie ()
{
    bool have = m_lifRef && m_current_serie_ref >= 0 && mFrameSet && sLayoutMgr.isSet();
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
    if (! have_lif_serie() || m_is_playing ) return;
    m_is_playing = true;
    mUIParams.setOptions( "mode", "label=`At Playing`" );
}

void lifContext::pause ()
{
    if (! have_lif_serie() || ! m_is_playing ) return;
    m_is_playing = false;
    mUIParams.setOptions( "mode", "label=`At Pause`" );
}

void lifContext::play_pause_button ()
{
    if (! have_lif_serie () ) return;
    if (m_is_playing)
        pause ();
    else
        play ();
}


            /************************
             *
             *  Manual Edit Support ( Width and Height setting )
             *
             ************************/

lifContext::Side_t lifContext::getManualNextEditMode ()
{
    Side_t nm = getManualEditMode();
    switch(nm)
    {
        case Side_t::major:
            nm = Side_t::minor;
            break;
        case Side_t::minor:
            nm = Side_t::notset;
            break;
        case Side_t::notset:
            nm = Side_t::major;
            break;
    }
    return nm;
    
}

void lifContext::edit_no_edit_button ()
{
    if (! have_lif_serie () )
        return;
    
    // Increment
    setManualEditMode(getManualNextEditMode());
    
    const std::string& ename = mEditNames[getManualEditMode()];
    // If we are in Edit mode. Stop and Go to Start
    if (getManualEditMode() != notset)
    {
        mUIParams.setOptions( "mode", ename);
        //        if (looping())
        //        {
        //            mUIParams.setOptions( "mode", "label=`Stopping`" );
        //            looping(false);
        //            seekToStart();
        //        }
        if (mContainer.getNumChildren())
            mContainer.removeChildren();
    }
    else
        mUIParams.setOptions( "mode", ename);
    
}


void lifContext::analyze_analyzing_button()
{
    std::lock_guard<std::mutex> guard(m_clip_mutex);
    if (! have_lif_serie () )
        return;
    
    // Flip
    setAnalyzeMode(!getAnalyzeMode());
    // If we are in Edit mode. Stop and Go to Start
    if (getAnalyzeMode())
    {
        mUIParams.setOptions( "state", "label=`Processing`" );
        if (looping())
        {
            looping(false);
            seekToStart();
        }
        
        mUIParams.removeParam("Select ");
        static int selected_index = 0;
        mUIParams.addParam( "Select ", m_contraction_names, &selected_index )
        .updateFn( [this]
                  {
                      if ( selected_index >= 0 && selected_index < m_contraction_names.size() )
                      {
                          set_current_clip_index(selected_index);
                          std::cout << m_contraction_names[get_current_clip_index()] << std::endl;
                          const lifContext::clip& clip = m_clips[get_current_clip_index()];
                          std::cout << clip << std::endl;
                      }
                  });
        
    }
    else
        mUIParams.setOptions( "state", "label=`Ready`" );
}

                /************************
                 *
                 *  Seek Processing
                 *
                 ************************/

// @ todo: indicate mode differently
void lifContext::seekToEnd ()
{
    seekToFrame (m_clips[get_current_clip_index()].end);
    assert(! m_clips.empty());
    
    //mUIParams.setOptions( "mode", "label=`@ End`" );
}

void lifContext::seekToStart ()
{
    
    seekToFrame(m_clips[get_current_clip_index()].begin);
    assert(! m_clips.empty());
    
    //mUIParams.setOptions( "mode", "label=`@ Start`" );
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
    std::lock_guard<std::mutex> guard(m_clip_mutex);
    
    if (mark < m_clips[m_current_clip_index].begin || mark > m_clips[m_current_clip_index].end)
        mark = m_clips[m_current_clip_index].begin;
    
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
    
    if (! have_lif_serie () ) return;
    
    mMouseInImage = get_image_display_rect().contains(event.getPos());
    if (mMouseInImage)
    {
        mMouseInImagePosition = event.getPos();
        update_instant_image_mouse ();
    }
    
    if (sLayoutMgr.display_plots_rect().contains(event.getPos()))
    {
        std::vector<float> dds (sLayoutMgr.plot_rects().size());
        for (auto pp = 0; pp < sLayoutMgr.plot_rects().size(); pp++) dds[pp] = sLayoutMgr.plot_rects()[pp].distanceSquared(event.getPos());
        
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
    
    if (getManualEditMode() != notset && mMouseInImage && channelIndex() == 2)
    {
        sides_length_t& which = mCellEnds[getManualEditMode()];
        which.second = event.getPos();
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
    
    if (getManualEditMode() != notset && mMouseInImage && channelIndex() == 2)
    {
        sides_length_t& which = mCellEnds[getManualEditMode()];
        which.first = event.getPos();
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
    if( have_lif_serie () ) {
        if( event.getCode() == KeyEvent::KEY_LEFT ) {
            pause();
            seekToFrame (getCurrentFrame() - 1);
            if (mMouseInImage)
                update_instant_image_mouse ();
        }
        else if( event.getCode() == KeyEvent::KEY_RIGHT ) {
            pause ();
            seekToFrame (getCurrentFrame() + 1);
            if (mMouseInImage)
                update_instant_image_mouse ();
        }
        else if( event.getChar() == ' ' ) {
            play_pause_button();
        }
        
    }
}

                /************************
                 *
                 *  MedianCutOff Set/Get
                 *
                 ************************/

void lifContext::setMedianCutOff (uint32_t newco)
{
    // Get a shared_ptr from weak and check if it had not expired
    auto spt = m_lifProcRef->contractionWeakRef().lock();
    if (! spt ) return;
    uint32_t tmp = newco % 100; // pct
    uint32_t current (spt->get_median_levelset_pct () * 100);
    if (tmp == current) return;
    spt->set_median_levelset_pct (tmp / 100.0f);
    m_lifProcRef->update();
}

uint32_t lifContext::getMedianCutOff () const
{
    // Get a shared_ptr from weak and check if it had not expired
    auto spt = m_lifProcRef->contractionWeakRef().lock();
    if (spt)
    {
        uint32_t current (spt->get_median_levelset_pct () * 100);
        return current;
    }
    return 0;
}


                /************************
                 *
                 *  Setup & Load File
                 *
                 ************************/

void  lifContext::get_series_info (const std::shared_ptr<lifIO::LifReader>& lifer)
{
    m_series_book.clear ();
    for (unsigned ss = 0; ss < lifer->getNbSeries(); ss++)
    {
        series_info si;
        si.name = lifer->getSerie(ss).getName();
        si.timesteps = lifer->getSerie(ss).getNbTimeSteps();
        si.pixelsInOneTimestep = lifer->getSerie(ss).getNbPixelsInOneTimeStep();
        si.dimensions = lifer->getSerie(ss).getSpatialDimensions();
        si.channelCount = lifer->getSerie(ss).getChannels().size();
        si.channels.clear ();
        for (lifIO::ChannelData cda : lifer->getSerie(ss).getChannels())
        {
            si.channel_names.push_back(cda.getName());
            si.channels.emplace_back(cda);
        }
        
        // Get timestamps in to time_spec_t and store it in info
        si.timeSpecs.resize (lifer->getSerie(ss).getTimestamps().size());
        
        // Adjust sizes based on the number of bytes
        std::transform(lifer->getSerie(ss).getTimestamps().begin(), lifer->getSerie(ss).getTimestamps().end(),
                       si.timeSpecs.begin(), [](lifIO::LifSerie::timestamp_t ts) { return time_spec_t ( ts / 10000.0); });
        
        si.length_in_seconds = lifer->getSerie(ss).total_duration ();
        
        m_series_book.emplace_back (si);
    }
}

void lifContext::setup()
{
    srand( 133 );
    
    mUIParams = params::InterfaceGl( "Lif Player ", toPixels( ivec2( 300, 400 )));
    mUIParams.setPosition(getWindowSize() / 3);
    m_contraction_names = m_contraction_none;
    
    m_prev_selected_index = -1;
    loadLifFile ();
    
    clear_playback_params();
    
    if( is_valid() )
    {
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
                 *  Load Serie & Setup Widgets
                 *
                 ************************/

void lifContext::add_widgets (const int channel_count)
{
    std::lock_guard<std::mutex> lock(m_track_mutex);
    
    assert (  sLayoutMgr.plot_rects().size() >= channel_count);
    
    m_plots.resize (0);
    
    for (int cc = 0; cc < channel_count; cc++)
    {
        m_plots.push_back( Graph1DRef (new graph1D (m_current_serie_ref->getChannels()[cc].getName(),
                                                    sLayoutMgr.plot_rects() [cc])));
    }
    
    for (Graph1DRef gr : m_plots)
    {
        gr->backgroundColor = ColorA( 0.3, 0.3, 0.3, 0.3 );
        gr->strokeColor = ColorA(0.2,0.8,0.1,1.0);
        m_marker_signal.connect(std::bind(&graph1D::set_marker_position, gr, std::placeholders::_1));
    }
    if (m_plots.size() == 3)
    {
        m_plots[1]->strokeColor = ColorA(0.8,0.2,0.1,1.0);
        m_plots[2]->strokeColor = ColorA(0.0,0.0,0.0,1.0);
    }
    
    std::cout << sLayoutMgr.display_timeline_rect() << std::endl;
    auto rect = sLayoutMgr.display_timeline_rect();
    mMainTimeLineSlider = tinyUi::TimeLineSlider(rect);
    mMainTimeLineSlider.clear_timepoint_markers();
    mMainTimeLineSlider.setTitle ("Time Line");
    m_marker_signal.connect(std::bind(&tinyUi::TimeLineSlider::set_marker_position, mMainTimeLineSlider, std::placeholders::_1));
    mWidgets.push_back( &mMainTimeLineSlider );
    
    
    getWindow()->getSignalMouseDrag().connect( [this] ( MouseEvent &event ) { processDrag( event.getPos() ); } );
    
}


// Set window size according to layout
//  Channel             Data
//  1
//  2
//  3

void lifContext::loadCurrentSerie ()
{
    if ( ! is_valid() || ! (m_lifRef || ! m_current_serie_ref) )
        return;

    try {
        
        reset_clips();
        mWidgets.clear ();
        setMedianCutOff(0);
        pause();
        
        // Create the frameset and assign the channel names
        // Fetch the media info
        mFrameSet = qTimeFrameCache::create (*m_current_serie_ref);
        if (! mFrameSet || ! mFrameSet->isValid())
        {
            ci_console() << "Serie had 1 or no frames " << std::endl;
            return;
        }
        mMediaInfo = mFrameSet->media_info();
        mMediaInfo.output(std::cout);
        sLayoutMgr.init (lifContext::startup_display_size() , mFrameSet->media_info(), mMediaInfo.getNumChannels());
        
        // Start Loading Images on a different thread
        auto future_res = std::async(std::launch::async, &lif_processor::load, m_lifProcRef.get(), mFrameSet, m_serie.channel_names);
        
        mFrameSet->channel_names (m_series_book[m_cur_selected_index].channel_names);
    
        reset_entire_clip(getNumFrames());

    
        auto title = m_series_names[m_cur_selected_index] + " @ " + mPath.filename().string();
        getWindow()->setTitle( title );
        const tiny_media_info tm = mFrameSet->media_info ();
        getWindow()->getApp()->setFrameRate(tm.getFramerate() * 2);
        
        
        mScreenSize = tm.getSize();
        m_frameCount = tm.getNumFrames ();
        mSurface = Surface8u::create (int32_t(mScreenSize.x), int32_t(mScreenSize.y), true);
        
        mTimeMarker = marker_info (tm.getNumFrames (), tm.getDuration());
        mAuxTimeMarker = marker_info (tm.getNumFrames (), tm.getDuration());
        
        ivec2 window_size (sLayoutMgr.desired_window_size());
        setWindowSize(window_size);
        int channel_count = (int) tm.getNumChannels();
        
        std::cout << "Lif frames " << future_res.get() << std::endl;
        
        add_widgets(channel_count);
        seekToStart();
        play();
        
        m_async_luminance_tracks = std::async(std::launch::async,&lif_processor::run_flu_statistics,m_lifProcRef.get());
        m_async_pci_tracks = std::async(std::launch::async, &lif_processor::run_pci, m_lifProcRef.get());
        auto res = m_lifProcRef->run_volume_sum_sumsq_count ();
        res.PrintTo(res,&std::cout);
        
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
    sLayoutMgr.update_window_size(getWindowSize ());
    return sLayoutMgr.display_frame_rect();
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
    if (! have_lif_serie () || ! mSurface ) return;
    if (! sLayoutMgr.isSet()) return;
    
    sLayoutMgr.update_window_size(getWindowSize ());
    mSize = vec2( getWindowWidth(), getWindowHeight() / 12);
    mMainTimeLineSlider.setBounds (sLayoutMgr.display_timeline_rect());
    if (m_plots.empty()) return;
    
    for (int cc = 0; cc < sLayoutMgr.plot_rects().size(); cc++)
    {
        m_plots[cc]->setRect (sLayoutMgr.plot_rects()[cc]);
    }
    

}

bool lifContext::haveTracks()
{
    return ! m_trackWeakRef.expired() && ! m_pci_trackWeakRef.expired();
}

void lifContext::update ()
{
    if (! have_lif_serie() ) return;
    
    mContainer.update();
    sLayoutMgr.update_window_size(getWindowSize ());
    int cc = m_current_serie_ref->getChannels().size();
    
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
        if (cc > 1)
            m_plots[1]->setup(tracksRef->at(1));
    }
    if ( ! m_pci_trackWeakRef.expired())
    {
        auto tracksRef = m_pci_trackWeakRef.lock();
        if (!tracksRef->at(0).second.empty())
            m_plots[cc-1]->setup(tracksRef->at(0));
    }
    
    if (getCurrentFrame() > getNumFrames())
    {
        if (! looping () ) seekToEnd();
        else
            seekToStart();
    }
    
    if (have_lif_serie ())
        mSurface = mFrameSet->getFrame(getCurrentFrame());
    
    if (m_is_playing )
    {
        update_instant_image_mouse ();
        seekToFrame (getCurrentFrame() + 1);
    }

    // Update text texture with most recent text
    auto num_str = to_string( int( getCurrentFrame ()));
    std::string frame_str = m_is_playing ? string( "Playing: " + num_str) : string ("Paused: " + num_str);
    update_log (frame_str);
}

//@note:
// LIF 3 channel organization. Channel height is 1/3 of image height
// Channel Index is pos.y / channel_height,
// In channel x is pos.x, In channel y is pos.y % channel_height
void lifContext::update_instant_image_mouse ()
{
    auto image_pos = sLayoutMgr.display2image(mMouseInImagePosition);
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
        gl::drawStrokedRect(sLayoutMgr.display_timeline_rect(), 3.0f);
    }
    {
        gl::ScopedColor (ColorA::gray(0.1));
        gl::drawStrokedRect(sLayoutMgr.display_plots_rect(), 3.0f);
    }
    
    if (getManualEditMode() == notset)
    {
        auto texR = pixelInfoTexture ();
        if (texR)
        {
            Rectf tbox = texR->getBounds();
            tbox.offset(mMouseInImagePosition);
            gl::draw( texR, tbox);
        }
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
    
    if( have_lif_serie()  && mSurface )
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
        
        
        if (getManualEditMode() != notset && mCellEnds.size() == 2)
        {
            const sides_length_t& length = mCellEnds[0];
            const sides_length_t& width = mCellEnds[1];
            cinder::gl::ScopedLineWidth( 10.0f );
            {
                cinder::gl::ScopedColor col (ColorA( 1, 0.1, 0.1, 0.8f ) );
                gl::drawLine(length.first, length.second);
                gl::drawSolidCircle(length.first, 3.0);
                gl::drawSolidCircle(length.second, 3.0);
            }
            {
                cinder::gl::ScopedColor col (ColorA( 0.1, 1.0, 0.1, 0.8f ) );
                gl::drawLine(width.first, width.second);
                gl::drawSolidCircle(width.first, 3.0);
                gl::drawSolidCircle(width.second, 3.0);
            }
        }
        
        
        if (m_serie.channelCount)
        {
            for (int cc = 0; cc < m_plots.size(); cc++)
            {
                m_plots[cc]->setRect (sLayoutMgr.plot_rects()[cc]);
                m_plots[cc]->draw();
            }
        }
        
        mContainer.draw();
        draw_info ();
        
        
    }
    
    mUIParams.draw();
    
    
}

