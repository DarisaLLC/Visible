//
//  visible_lif_impl.cpp
//  Visible
//
//  Created by Arman Garakani on 5/13/16.
//
//

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdelete-non-virtual-dtor"
#pragma GCC diagnostic ignored "-Wunused-private-field"
#pragma GCC diagnostic ignored "-Wshorten-64-to-32"
#pragma GCC diagnostic ignored "-Wcomma"

#include "opencv2/stitching.hpp"
#include <stdio.h>
#include "guiContext.h"
#include "LifContext.h"
#include "cinder/app/App.h"
#include "cinder/gl/gl.h"
#include "cinder/Timeline.h"
#include "cinder/Timer.h"
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
#include "async_tracks.h"
#include "cinder_xchg.hpp"
#include "visible_layout.hpp"
#include "vision/opencv_utils.hpp"
#include "vision/histo.h"
#include "core/stl_utils.hpp"
#include "sm_producer.h"
#include "algo_Lif.hpp"
#include "core/signaler.h"
#include "contraction.hpp"
#include "logger.hpp"
#include "cinder/Log.h"
#include "CinderImGui.h"
#include "gui_handler.hpp"
#include "gui_base.hpp"
#include <boost/any.hpp>
#include "ImGuiExtensions.h"
#include "Resources.h"
#include "cinder_opencv.h"



using namespace ci;
using namespace ci::app;
using namespace std;
using namespace svl;


#define ONCE(x) { static int __once = 1; if (__once) {x;} __once = 0; }

namespace anonymous{
    std::map<std::string, unsigned int> named_colors = {{"Red", 0xFF0000FF }, {"red",0xFF0000FF },{"Green", 0xFF00FF00},
        {"green", 0xFF00FF00},{ "PCI", 0xFFFF0000}, { "pci", 0xFFFF0000}};
    
    
}



                    /************************
                     *
                     *  Setup & Load File
                     *
                     ************************/



    /////////////  lifContext Implementation  ////////////////
// @ todo setup default repository
// default median cover pct is 5% (0.05)

lifContext::lifContext(ci::app::WindowRef& ww, const lif_serie_data& sd) :sequencedImageContext(ww), m_serie(sd) {
    m_type = guiContext::Type::lif_file_viewer;
    m_valid = false;
        m_valid = sd.index() >= 0;
        if (m_valid){
            m_layout = std::make_shared<layoutManager>  ( ivec2 (10, 30) );
            if (auto lifRef = m_serie.readerWeakRef().lock()){
                m_cur_lif_serie_ref = std::shared_ptr<lifIO::LifSerie>(&lifRef->getSerie(sd.index()), stl_utils::null_deleter());
                setup();
                ww->getRenderer()->makeCurrentContext(true);
            }
    }
}


ci::app::WindowRef&  lifContext::get_windowRef(){
    return shared_from_above()->mWindow;
}

void lifContext::setup_signals(){

    // Create a Processing Object to attach signals to
    m_lifProcRef = std::make_shared<lif_serie_processor> ();
    
    // Support lifProcessor::content_loaded
    std::function<void (int64_t&)> content_loaded_cb = boost::bind (&lifContext::signal_content_loaded, shared_from_above(), _1);
    boost::signals2::connection ml_connection = m_lifProcRef->registerCallback(content_loaded_cb);
    
    // Support lifProcessor::flu results available
    std::function<void ()> flu_stats_available_cb = boost::bind (&lifContext::signal_flu_stats_available, shared_from_above());
    boost::signals2::connection flu_connection = m_lifProcRef->registerCallback(flu_stats_available_cb);
    
    // Support lifProcessor::initial ss results available
    std::function<void (int&)> sm1d_available_cb = boost::bind (&lifContext::signal_sm1d_available, shared_from_above(), _1);
    boost::signals2::connection nl_connection = m_lifProcRef->registerCallback(sm1d_available_cb);
    
    // Support lifProcessor::median level set ss results available
    std::function<void (int&,int&)> sm1dmed_available_cb = boost::bind (&lifContext::signal_sm1dmed_available, shared_from_above(), _1, _2);
    boost::signals2::connection ol_connection = m_lifProcRef->registerCallback(sm1dmed_available_cb);
    
    // Support lifProcessor::contraction results available
    std::function<void (lif_serie_processor::contractionContainer_t&)> contraction_available_cb = boost::bind (&lifContext::signal_contraction_available, shared_from_above(), _1);
    boost::signals2::connection contraction_connection = m_lifProcRef->registerCallback(contraction_available_cb);
    
    // Support lifProcessor::volume_var_available
    std::function<void ()> volume_var_available_cb = boost::bind (&lifContext::signal_volume_var_available, shared_from_above());
    boost::signals2::connection volume_var_connection = m_lifProcRef->registerCallback(volume_var_available_cb);
    
}

void lifContext::setup_params () {
    
    mUIParams = params::InterfaceGl( " Control ", toPixels( ivec2(getWindowWidth(), 100 )));
    mUIParams.setOptions("", "movable=false sizeable=false");
    mUIParams.setOptions( "TW_HELP", "visible=false" );
    mUIParams.setPosition(vec2(0,getWindowHeight()-250));
    
    
    mUIParams.addSeparator();
    m_perform_names.clear ();
    m_perform_names.push_back("Manual Cell End Tracing");
    
    {
        const std::function<void (int)> setter = std::bind(&lifContext::seekToFrame, this, std::placeholders::_1);
        const std::function<int ()> getter = std::bind(&lifContext::getCurrentFrame, this);
        mUIParams.addParam ("Current Time Step", setter, getter);
    }
    
    mUIParams.addParam( "Looping", &m_is_looping ).keyIncr( "l" );
    mUIParams.addSeparator();
    mUIParams.addParam("Play / Pause ", &m_is_playing).keyIncr(" ");
    mUIParams.addSeparator();
    mUIParams.addParam("Show Pixel Probe ", &m_show_probe).keyIncr("s");
    mUIParams.addSeparator();
    mUIParams.addButton("Edit ", bind( &lifContext::edit_no_edit_button, this ) );
    mUIParams.addSeparator();
    mUIParams.addText( "mode", "label=`Browse`" );
    mUIParams.addSeparator();
    {
        // Add a param with no target, but instead provide setter and getter functions.
        const std::function<void(uint32_t)> setter    = std::bind(&lifContext::setMedianCutOff, this, std::placeholders::_1 );
        const std::function<uint32_t()> getter    = std::bind( &lifContext::getMedianCutOff, this);
        
        // Attach a callback that is fired after a target is updated.
        mUIParams.addParam( "CutOff Pct", setter, getter );
    }
    {
        // Add a param with no target, but instead provide setter and getter functions.
        const std::function<void(uint32_t)> setter    = std::bind(&lifContext::setCellLength, this, std::placeholders::_1 );
        const std::function<uint32_t()> getter    = std::bind( &lifContext::getCellLength, this);
        
        // Attach a callback that is fired after a target is updated.
        mUIParams.addParam( "Cell Length ", setter, getter );
    }
    
}


void lifContext::setup()
{
    ci::app::WindowRef ww = get_windowRef();
    gl::enableVerticalSync();
    ui::initialize(ui::Options()
                   .itemSpacing(vec2(6, 6)) //Spacing between widgets/lines
                   .itemInnerSpacing(vec2(10, 4)) //Spacing between elements of a composed widget
                   .color(ImGuiCol_Button, ImVec4(0.86f, 0.93f, 0.89f, 0.39f)) //Darken the close button
                   .color(ImGuiCol_Border, ImVec4(0.86f, 0.93f, 0.89f, 0.39f))
                   .window(ww)
                   );

    m_showGUI = true;
    m_showLog = true;
    m_showHelp = true;
    
    srand( 133 );
    setup_signals();
    assert(is_valid());
    ww->setTitle( m_serie.name());
    ww->setSize(1280, 768);
    mFont = Font( "Menlo", 18 );
    auto ws = ww->getSize();
    mSize = vec2( ws[0], ws[1] / 12);
    m_contraction_names = m_contraction_none;
    clear_playback_params();
    setup_params ();
    loadCurrentSerie();
    shared_from_above()->update();
    
    ww->getSignalMouseDrag().connect( [this] ( MouseEvent &event ) { processDrag( event.getPos() ); } );
}



/************************
 *
 *
 *  Call Backs
 *
 ************************/

void lifContext::signal_sm1d_available (int& dummy)
{
    vlogger::instance().console()->info("self-similarity available: ");
}

void lifContext::signal_sm1dmed_available (int& dummy, int& dummy2)
{
    
    if (haveTracks())
    {
        auto tracksRef = m_pci_trackWeakRef.lock();
        if (!tracksRef->at(0).second.empty()){
         //   m_plots[channel_count()-1]->load(tracksRef->at(0));
            mySequence.m_editable_plot_data.load(tracksRef->at(0), anonymous::named_colors["PCI"], 2);
        }
    }
    vlogger::instance().console()->info("self-similarity available: ");
}

void lifContext::signal_content_loaded (int64_t& loaded_frame_count )
{
    std::string msg = to_string(mMediaInfo.count) + " Samples in Media  " + to_string(loaded_frame_count) + " Loaded";
    vlogger::instance().console()->info(msg);
    process_async();
}
void lifContext::signal_flu_stats_available ()
{
    update();
    vlogger::instance().console()->info(" Flu Stats Available ");

}

void lifContext::signal_contraction_available (lif_serie_processor::contractionContainer_t& contras)
{
    // Pauses playback if playing and restore it at scope's end
    scopedPause sp(std::shared_ptr<lifContext>(shared_from_above(), stl_utils::null_deleter () ));
    
    if (contras.empty()) return;
    
    // TimeLine Markers
    mMainTimeLineSlider.clear_timepoint_markers();
    
    // @note: We are handling one contraction for now.
    m_contractions = contras;
    reset_entire_clip(mMediaInfo.count);
    uint32_t index = 0;
    string name = " C " + svl::toString(index) + " ";
    m_contraction_names.push_back(name);
    m_clips.emplace_back(m_contractions[0].contraction_start.first,
                         m_contractions[0].relaxation_end.first,
                         m_contractions[0].contraction_peak.first);

    tinyUi::timepoint_marker_t tm;
    tm.first = m_contractions[0].contraction_peak.first/ ((float)mMediaInfo.count);
    tm.second = ColorA (0.9, 0.3, 0.1, 0.75);
    mMainTimeLineSlider.add_timepoint_marker(tm);
    
    update_contraction_selection();
    
}


void lifContext::signal_volume_var_available()
{
    vlogger::instance().console()->info(" Volume Variance are available ");
    if (! m_lifProcRef){
        vlogger::instance().console()->error("Lif Processor Object does not exist ");
        return;
    }
    
    // Create a texcture for display
  //  const cv::Mat& dvar = m_lifProcRef->display_volume_variances();
    Surface8uRef sur = Surface8u::create( cinder::fromOcv(m_lifProcRef->display_volume_variances()) );
    m_var_texture = gl::Texture::create(*sur);
    
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

const clip& lifContext::get_current_clip () const {
    return m_clips.at(m_current_clip_index);
}

void lifContext::reset_entire_clip (const size_t& frame_count) const
{
    // Stop Playback ?
    m_clips.clear();
    m_clips.emplace_back(frame_count);
    m_contraction_names.resize(1);
    m_contraction_names[0] = " Entire ";
    set_current_clip_index(0);
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
    m_show_probe = false;
    m_is_editing = false;
    m_zoom.x = m_zoom.y = 1.0f;
}

bool lifContext::have_lif_serie ()
{
    bool have =   m_serie.readerWeakRef().lock() && m_cur_lif_serie_ref  && mFrameSet && m_layout->isSet();
    return have;
}


bool lifContext::is_valid () const { return m_valid && is_context_type(guiContext::lif_file_viewer); }




/************************
 *
 *  UI Conrol Functions
 *
 ************************/

void lifContext::loop_no_loop_button ()
{
    if (! have_lif_serie()) return;
    if (looping())
        looping(false);
    else
        looping(true);
}

void lifContext::looping (bool what)
{
    m_is_looping = what;
//    static std::vector<std::string> loopstrings {"label=` Looping`", "label=` Not Looping`"};
//    mUIParams.setOptions( "mode", m_is_looping ? loopstrings[0] : loopstrings[1]);
}


bool lifContext::looping ()
{
    return m_is_looping;
}

void lifContext::play ()
{
    if (! have_lif_serie() || m_is_playing ) return;
    m_is_playing = true;
    mUIParams.setOptions( "mode", "label=` Playing`" );
}

void lifContext::pause ()
{
    if (! have_lif_serie() || ! m_is_playing ) return;
    m_is_playing = false;
    mUIParams.setOptions( "mode", "label=` Pause`" );
}

// For use with RAII scoped pause pattern
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
            nm = Side_t::major;
            break;
    }
    return nm;
    
}

void lifContext::edit_no_edit_button ()
{
    if (! have_lif_serie () ) return;
    m_is_editing = ! m_is_editing;
    if (! m_is_editing) return;
    
    looping(false);

    // If we are in Edit mode. Stop and Go to the end as detected
    setManualEditMode(getManualNextEditMode());
    const std::string& ename = mEditNames[getManualEditMode()];
    const clip& clip = m_clips[get_current_clip_index()];
    switch(getManualEditMode()){
        case Side_t::minor:
            seekToFrame(clip.first());
            break;
        case Side_t::major:
            seekToFrame(clip.last());
            break;
        default:
            break;
    }
    mUIParams.setOptions( "mode", ename);
    if (mContainer.getNumChildren())
        mContainer.removeChildren();
}


void lifContext::update_contraction_selection()
{
    mUIParams.removeParam("Select ");
    static int selected_index = 0;
    mUIParams.addParam( "Select ", m_contraction_names, &selected_index )
    .updateFn( [this]
              {
                  if ( selected_index >= 0 && selected_index < m_contraction_names.size() )
                  {
                      set_current_clip_index(selected_index);
                      const clip& clip = m_clips[get_current_clip_index()];
                      std::string msg = " Current Clip " + m_contraction_names[get_current_clip_index()] + " " + clip.to_string();
                      vlogger::instance().console()->info(msg);
                  }
              });
}

/************************
 *
 *  Seek Processing
 *
 ************************/

// @ todo: indicate mode differently
void lifContext::seekToEnd ()
{
    seekToFrame (get_current_clip().last());
    assert(! m_clips.empty());
}

void lifContext::seekToStart ()
{
    
    seekToFrame(get_current_clip().first());
    assert(! m_clips.empty());
}

int lifContext::getNumFrames ()
{
    return mMediaInfo.count;
}

int lifContext::getCurrentFrame ()
{
    return int(m_seek_position);
}

time_spec_t lifContext::getCurrentTime ()
{
    if (m_seek_position >= 0 && m_seek_position < m_serie.timeSpecs().size())
        return m_serie.timeSpecs()[m_seek_position];
    else return -1.0;
}



void lifContext::seekToFrame (int mark)
{
    const clip& curr = get_current_clip();

    if (mark < curr.first()) mark = curr.first();
    if (mark > curr.last())
        mark = looping() ? curr.first() : curr.last();
    
    m_seek_position = mark;
    mTimeMarker.from_count (m_seek_position);
    m_marker_signal.emit(mTimeMarker);
    mAuxTimeMarker.from_count (m_seek_position);
    m_aux_marker_signal.emit(mTimeMarker);
}


void lifContext::onMarked ( marker_info& t)
{
    seekToFrame((int) t.current_frame());
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
            seekToFrame((int) mTimeMarker.current_frame());
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
    update_with_mouse_position (event);
}

void lifContext::update_with_mouse_position ( MouseEvent event )
{
    Rectf pb(mUIParams.getPosition(), mUIParams.getSize());
    m_is_in_params = pb.contains(event.getPos());
    
    mMouseInImage = false;
    mMouseInGraphs  = -1;
    
    if (! have_lif_serie () ) return;
    
    mMouseInImage = get_image_display_rect().contains(event.getPos());
    if (mMouseInImage)
    {
        mMouseInImagePosition = event.getPos();
        update_instant_image_mouse ();
    }
    
    if (m_layout->display_plots_rect().contains(event.getPos()))
    {
        std::vector<float> dds (m_layout->plot_rects().size());
        for (auto pp = 0; pp < m_layout->plot_rects().size(); pp++) dds[pp] = m_layout->plot_rects()[pp].distanceSquared(event.getPos());
        
        auto min_iter = std::min_element(dds.begin(),dds.end());
        mMouseInGraphs = int(min_iter - dds.begin());
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
    for (graph1d::ref graphRef : m_plots)
        graphRef->mouseDrag( event );
    
    if (isEditing () && mMouseInImage && channelIndex() == 2)
    {
        sides_length_t& which = mCellEnds[getManualEditMode()];
        which.second = event.getPos();
    }
}


void lifContext::mouseDown( MouseEvent event )
{
    for (graph1d::ref graphRef : m_plots )
    {
        graphRef->mouseDown( event );
        graphRef->get_marker_position(mTimeMarker);
        graphRef->get_marker_position(mAuxTimeMarker);
    }
    
    if (isEditing() && mMouseInImage && channelIndex() == 2)
    {
        sides_length_t& which = mCellEnds[getManualEditMode()];
        which.first = event.getPos();
    }
}


void lifContext::mouseUp( MouseEvent event )
{
    for (graph1d::ref graphRef : m_plots)
        graphRef->mouseUp( event );
}




void lifContext::keyDown( KeyEvent event )
{
     ci::app::WindowRef ww = get_windowRef();
    
    if( event.getChar() == 'f' ) {
        setFullScreen( ! isFullScreen() );
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
            m_is_playing = true;
        }
        
    }
}

/************************
 *
 *  MedianCutOff Set/Get
 *
 ************************/

void lifContext::setMedianCutOff (int32_t newco)
{
    if (! m_lifProcRef) return;
    // Get a shared_ptr from weak and check if it had not expired
    auto spt = m_lifProcRef->contractionWeakRef().lock();
    if (! spt ) return;
    uint32_t tmp = newco % 100; // pct
    uint32_t current (spt->get_median_levelset_pct () * 100);
    if (tmp == current) return;
    spt->set_median_levelset_pct (tmp / 100.0f);
    m_lifProcRef->update();
}

int32_t lifContext::getMedianCutOff () const
{
    if (! m_cur_lif_serie_ref) return 0;
    
    // Get a shared_ptr from weak and check if it had not expired
    auto spt = m_lifProcRef->contractionWeakRef().lock();
    if (spt)
    {
        uint32_t current (spt->get_median_levelset_pct () * 100);
        return current;
    }
    return 0;
}

void lifContext::setCellLength (uint32_t newco){
    if (! m_lifProcRef) return;
    // Get a shared_ptr from weak and check if it had not expired
    auto spt = m_lifProcRef->contractionWeakRef().lock();
    if (! spt ) return;
    m_cell_length = newco;
}
uint32_t lifContext::getCellLength () const{
    if (! m_cur_lif_serie_ref) return 0;
    
    // Get a shared_ptr from weak and check if it had not expired
    auto spt = m_lifProcRef->contractionWeakRef().lock();
    if (spt)
    {
        return m_cell_length;
    }
    return 0;
}



/************************
 *
 *  Load Serie & Setup Widgets
 *
 ************************/

// Set window size according to layout
//  Channel             Data
//  1
//  2
//  3

void lifContext::loadCurrentSerie ()
{
    if ( ! is_valid() || ! m_cur_lif_serie_ref)
        return;
    
    try {
        m_clips.clear();
        mWidgets.clear ();
        setCellLength(0);
        pause();
        
        // Create the frameset and assign the channel names
        // Fetch the media info
        mFrameSet = seqFrameContainer::create (*m_cur_lif_serie_ref);
        
        if (! mFrameSet || ! mFrameSet->isValid())
        {
            vlogger::instance().console()->debug("Serie had 1 or no frames ");
            return;
        }
        mMediaInfo = mFrameSet->media_info();
        mChannelCount = (uint32_t) mMediaInfo.getNumChannels();
        assert(mChannelCount > 0 && mChannelCount < 4);
        // @todo output media info to console log
     //   vlogger::instance().console()->info(" ", mMediaInfo);
        m_layout->init (getWindowSize() , mFrameSet->media_info(), channel_count());
        
        // Start Loading Images on a different thread
        // Loading also produces voxel images.
        cv::Mat result;

        // note launch mode is std::launch::async
        auto future_res = std::async(std::launch::async, &lif_serie_processor::load, m_lifProcRef.get(), mFrameSet, m_serie.channel_names());
        mFrameSet->channel_names (m_serie.channel_names());
        reset_entire_clip(mFrameSet->count());
        m_minFrame = 0;
        m_maxFrame =  mFrameSet->count() - 1;
        
        // Initialize Sequencer
        mySequence.mFrameMin = 0;
        mySequence.mFrameMax = (int) mFrameSet->count();
        mySequence.mSequencerItemTypeNames = {"Time Line", "Length", "Force"};
        mySequence.myItems.push_back(timeLineSequence::timeline_item{ 0, 0, (int) mFrameSet->count(), true});
        
        m_title = m_serie.name() + " @ " + mPath.filename().string();
        
        auto ww = get_windowRef();
        ww->setTitle(m_title );
        ww->getApp()->setFrameRate(mMediaInfo.getFramerate());
        
        
        mScreenSize = mMediaInfo.getSize();
        
        mSurface = Surface8u::create (int32_t(mScreenSize.x), int32_t(mScreenSize.y), true);
        
        mTimeMarker = marker_info (mMediaInfo.getNumFrames (),mMediaInfo.getDuration());
        mAuxTimeMarker = marker_info (mMediaInfo.getNumFrames (),mMediaInfo.getDuration());
   
    }
    catch( const std::exception &ex ) {
        console() << ex.what() << endl;
        return;
    }
}

void lifContext::process_async (){
    
    switch(channel_count()){
        case 3:
        {
            // note launch mode is std::launch::async
            m_async_luminance_tracks = std::async(std::launch::async,&lif_serie_processor::run_flu_statistics,
                                                  m_lifProcRef.get(), std::vector<int> ({0,1}) );
             // Using scott meyer's wrapper that uses launch mode is std::launch::async
            m_async_pci_tracks = stl_utils::reallyAsync(&lif_serie_processor::run_pci, m_lifProcRef.get(), 2);
       
            break;
        }
        case 1:
        {
            m_async_pci_tracks = std::async(std::launch::async, &lif_serie_processor::run_pci,
                                            m_lifProcRef.get(), 0);
            break;
        }
    }
}


/************************
 *
 *  Update & Draw
 *
 ************************/

void  lifContext::draw_sequencer (){
    
 
    
}


void lifContext::add_result_sequencer ()
{
    // let's create the sequencer
    static int selectedEntry = -1;
    static int64 firstFrame = 0;
    static bool expanded = true;
    
    Rectf dr = get_image_display_rect();
    auto tr = dr.getUpperRight();
    ImGui::SetNextWindowPos(ImVec2(tr.x+10, tr.y));
    ImGui::SetNextWindowSize(dr.getSize());
    
    ImGui::Begin(" Results ");
    
    ImGui::PushItemWidth(130);
    ImGui::InputInt("Frame Min", &mySequence.mFrameMin);
    ImGui::SameLine();
    ImGui::InputInt64("Frame ", &m_seek_position);
    ImGui::SameLine();
    ImGui::InputInt("Frame Max", &mySequence.mFrameMax);
    ImGui::PopItemWidth();
    
    
#if 0
    mySequence.myItems.push_back(timeLineSequence::timeline_item{ 0,
        (int) m_contractions[0].contraction_start.first,
        (int) m_contractions[0].relaxation_end.first , true});
#endif
    
    Sequencer(&mySequence, &m_seek_position, &expanded, &selectedEntry, &firstFrame, ImSequencer::SEQUENCER_EDIT_NONE );
    // add a UI to edit that particular item
    if (selectedEntry != -1)
    {
        const timeLineSequence::timeline_item &item = mySequence.myItems[selectedEntry];
        ImGui::Text("I am a %s, please edit me", mySequence.mSequencerItemTypeNames[item.mType].c_str());
        // switch (type) ....
    }
    
    ImGui::End();
    
    
}

void lifContext::add_timeline(){
   
    int gScreenWidth = getWindowWidth();
    Rectf dr = get_image_display_rect();
    auto tr = dr.getUpperRight();
    auto pos = ImVec2(tr.x+10+dr.getWidth()+10, tr.y);
    ImGui::SetNextWindowPos(pos);
    ImVec2 size (gScreenWidth-30-pos.x, 100);
    ImGui::SetNextWindowSize(size);
    
    if (ImGui::Begin("Timeline"))
    {
        ImGui::PushItemWidth(40);
        ImGui::PushID(200);
        ImGui::InputInt("", &mySequence.mFrameMin, 0, 0);
        ImGui::PopID();
        ImGui::SameLine();
        if (ImGui::Button("|<"))
            seekToStart();
        ImGui::SameLine();
        if (ImGui::Button("<"))
            seekToFrame (getCurrentFrame() - 1);
        ImGui::SameLine();
        if (ImGui::Button(">"))
            seekToFrame (getCurrentFrame() + 1);
        ImGui::SameLine();
        if (ImGui::Button(">|"))
            seekToEnd();
        ImGui::SameLine();
        if (ImGui::Button(m_is_playing ? "Stop" : "Play"))
        {
            play_pause_button();
        }
        
        if(mNoLoop == nullptr){
            mNoLoop = gl::Texture::create( loadImage( loadResource( IMAGE_PNLOOP  )));
        }
        if (mLoop == nullptr){
            mLoop = gl::Texture::create( loadImage( loadResource( IMAGE_PLOOP )));
        }
        
        unsigned int playNoLoopTextureId = mNoLoop->getId();//  evaluation.GetTexture("Stock/PlayNoLoop.png");
        unsigned int playLoopTextureId = mLoop->getId(); //evaluation.GetTexture("Stock/PlayLoop.png");
        
        ImGui::SameLine();
        if (ImGui::ImageButton((ImTextureID)(uint64_t)(m_is_looping ? playLoopTextureId : playNoLoopTextureId), ImVec2(16.f, 16.f)))
            loop_no_loop_button();
        
        ImGui::SameLine();
        ImGui::PushID(202);
        ImGui::InputInt("",  &mySequence.mFrameMax, 0, 0);
        ImGui::PopID();
        
        if(!m_pci_trackWeakRef.expired()){
            int default_median_cover_pct = getMedianCutOff();
            ImGui::SliderInt("Median Cover Percent", &default_median_cover_pct, 0, 20);
            setMedianCutOff(default_median_cover_pct);
        }
        
        //   int a = m_current_clip_index;
        //   ImGui::SliderInt(" Contraction ", &a, 0, m_contraction_names.size());
        
        // @todo improve this logic. The moment we are able to set the median cover, set it to the default
        // of 5 percent
        // @todo move defaults in general setting
     
    }
    ImGui::End();

}


void lifContext::add_motion_profile (){
    
    int gScreenWidth = getWindowWidth();
    Rectf dr = get_image_display_rect();
    auto tr = dr.getUpperRight();
    auto pos = ImVec2(tr.x+10+dr.getWidth()+10, tr.y + 100 + 10);
    ImGui::SetNextWindowPos(pos);
    ImVec2 size (gScreenWidth-30-pos.x, 128);
    ImGui::SetNextWindowSize(size);
    
    if (ImGui::Begin("Motion Profile"))
    {
        if(m_var_texture){
            ImVec2 sz(m_var_texture->getWidth(),m_var_texture->getHeight());
            ImGui::Image( (void*)(intptr_t) m_var_texture->getId(), sz, ImVec2(0, 1), ImVec2(1, 0), ImVec4(1,1,1,1),ImVec4(255,255,255,0));
        }
        
    }
    ImGui::End();
    
}

void  lifContext::SetupGUIVariables(){

        ImGuiStyle &st = ImGui::GetStyle();
        st.FrameBorderSize = 1.0f;
        st.FramePadding = ImVec2(4.0f,2.0f);
        st.ItemSpacing = ImVec2(8.0f,2.0f);
        st.WindowBorderSize = 1.0f;
        //   st.TabBorderSize = 1.0f;
        st.WindowRounding = 1.0f;
        st.ChildRounding = 1.0f;
        st.FrameRounding = 1.0f;
        st.ScrollbarRounding = 1.0f;
        st.GrabRounding = 1.0f;
        //  st.TabRounding = 1.0f;
        
        // Setup style
        ImVec4* colors = ImGui::GetStyle().Colors;
        colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 0.95f);
        colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
        colors[ImGuiCol_WindowBg] = ImVec4(0.13f, 0.12f, 0.12f, 1.00f);
        colors[ImGuiCol_ChildBg] = ImVec4(1.00f, 1.00f, 1.00f, 0.00f);
        colors[ImGuiCol_PopupBg] = ImVec4(0.05f, 0.05f, 0.05f, 0.94f);
        colors[ImGuiCol_Border] = ImVec4(0.53f, 0.53f, 0.53f, 0.46f);
        colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_FrameBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.85f);
        colors[ImGuiCol_FrameBgHovered] = ImVec4(0.22f, 0.22f, 0.22f, 0.40f);
        colors[ImGuiCol_FrameBgActive] = ImVec4(0.16f, 0.16f, 0.16f, 0.53f);
        colors[ImGuiCol_TitleBg] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
        colors[ImGuiCol_TitleBgActive] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
        colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
        colors[ImGuiCol_MenuBarBg] = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
        colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
        colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.48f, 0.48f, 0.48f, 1.00f);
        colors[ImGuiCol_CheckMark] = ImVec4(0.79f, 0.79f, 0.79f, 1.00f);
        colors[ImGuiCol_SliderGrab] = ImVec4(0.48f, 0.47f, 0.47f, 0.91f);
        colors[ImGuiCol_SliderGrabActive] = ImVec4(0.56f, 0.55f, 0.55f, 0.62f);
        colors[ImGuiCol_Button] = ImVec4(0.50f, 0.50f, 0.50f, 0.63f);
        colors[ImGuiCol_ButtonHovered] = ImVec4(0.67f, 0.67f, 0.68f, 0.63f);
        colors[ImGuiCol_ButtonActive] = ImVec4(0.26f, 0.26f, 0.26f, 0.63f);
        colors[ImGuiCol_Header] = ImVec4(0.54f, 0.54f, 0.54f, 0.58f);
        colors[ImGuiCol_HeaderHovered] = ImVec4(0.64f, 0.65f, 0.65f, 0.80f);
        colors[ImGuiCol_HeaderActive] = ImVec4(0.25f, 0.25f, 0.25f, 0.80f);
        colors[ImGuiCol_Separator] = ImVec4(0.58f, 0.58f, 0.58f, 0.50f);
        colors[ImGuiCol_SeparatorHovered] = ImVec4(0.81f, 0.81f, 0.81f, 0.64f);
        colors[ImGuiCol_SeparatorActive] = ImVec4(0.81f, 0.81f, 0.81f, 0.64f);
        colors[ImGuiCol_ResizeGrip] = ImVec4(0.87f, 0.87f, 0.87f, 0.53f);
        colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.87f, 0.87f, 0.87f, 0.74f);
        colors[ImGuiCol_ResizeGripActive] = ImVec4(0.87f, 0.87f, 0.87f, 0.74f);
        //   colors[ImGuiCol_Tab] = ImVec4(0.01f, 0.01f, 0.01f, 0.86f);
        //   colors[ImGuiCol_TabHovered] = ImVec4(0.29f, 0.29f, 0.79f, 1.00f);
        //   colors[ImGuiCol_TabActive] = ImVec4(0.31f, 0.31f, 0.91f, 1.00f);
        //   colors[ImGuiCol_TabUnfocused] = ImVec4(0.02f, 0.02f, 0.02f, 1.00f);
        //   colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.19f, 0.19f, 0.19f, 1.00f);
        //  colors[ImGuiCol_DockingPreview] = ImVec4(0.38f, 0.48f, 0.60f, 1.00f);
        //  colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
        colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
        colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.68f, 0.68f, 0.68f, 1.00f);
        colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.77f, 0.33f, 1.00f);
        colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.87f, 0.55f, 0.08f, 1.00f);
        colors[ImGuiCol_TextSelectedBg] = ImVec4(0.47f, 0.60f, 0.76f, 0.47f);
        colors[ImGuiCol_DragDropTarget] = ImVec4(0.58f, 0.58f, 0.58f, 0.90f);
        colors[ImGuiCol_NavHighlight] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
        colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
        colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
        colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);

}


void  lifContext::DrawGUI(){
    
    add_timeline();
    add_result_sequencer();
    add_motion_profile ();
}

Rectf lifContext::get_image_display_rect ()
{
    return m_layout->display_frame_rect();
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
    if (! m_layout->isSet()) return;
    auto ww = get_windowRef();
    auto ws = ww->getSize();
    m_layout->update_window_size(ws);
    mSize = vec2(ws[0], ws[1] / 12);
  //  mMainTimeLineSlider.setBounds (m_layout->display_timeline_rect());


}

bool lifContext::haveTracks()
{
    return ! m_trackWeakRef.expired() && ! m_pci_trackWeakRef.expired();
}

void lifContext::update ()
{
    ci::app::WindowRef ww = get_windowRef();
    
    ww->getRenderer()->makeCurrentContext(true);
    if (! have_lif_serie() ) return;
    mContainer.update();
    auto ws = ww->getSize();
    m_layout->update_window_size(ws);
    
    // If Plots are ready, set them up
    // It is ready only for new data
    //@todo switch to using weak_ptr all together
    if ( is_ready (m_async_luminance_tracks) )
        m_trackWeakRef = m_async_luminance_tracks.get();
    
    if ( is_ready (m_async_pci_tracks))
        m_pci_trackWeakRef = m_async_pci_tracks.get();

    // Update Fluorescence results if ready
    if (! m_trackWeakRef.expired())
    {
        assert(channel_count() >= 3);
        auto tracksRef = m_trackWeakRef.lock();
        mySequence.m_editable_plot_data.load(tracksRef->at(0), anonymous::named_colors[tracksRef->at(0).first], 0);
        mySequence.m_editable_plot_data.load(tracksRef->at(1), anonymous::named_colors[tracksRef->at(1).first], 1);
    }

    // Update PCI result if ready
    if ( ! m_pci_trackWeakRef.expired())
    {
        auto tracksRef = m_pci_trackWeakRef.lock();
        mySequence.m_editable_plot_data.load(tracksRef->at(0), anonymous::named_colors["PCI"], 2);
    }

    // Fetch Next Frame
    if (have_lif_serie ()){
        mSurface = mFrameSet->getFrame(getCurrentFrame());
        mCurrentIndexTime = mFrameSet->currentIndexTime();
        if (mCurrentIndexTime.first != m_seek_position){
            vlogger::instance().console()->info(tostr(mCurrentIndexTime.first - m_seek_position));
        }
    }
    
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
    auto image_pos = m_layout->display2image(mMouseInImagePosition);
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
    const auto names = m_serie.channel_names();
    auto channel_name = (m_instant_channel < names.size()) ? names[m_instant_channel] : " ";
    
    // LIF has 3 Channels.
    uint32_t channel_height = mMediaInfo.getChannelSize().y;
    std::string pos = " " + svl::toString(((int)m_instant_pixel_Color.g)) + " @ [ " +
    to_string(imagePos().x) + "," + to_string(imagePos().y % channel_height) + "]";
    vec2                textSize(250, 100);
    TextBox tbox = TextBox().alignment( TextBox::LEFT).font( mFont ).size( ivec2( textSize.x, TextBox::GROW ) ).text( pos );
    tbox.setFont( Font( "Times New Roman", 34 ) );
    if (channel_name == "Red")
        tbox.setColor( ColorA(0.8,0.2,0.1,1.0) );
    else if (channel_name == "Green")
        tbox.setColor( ColorA(0.2,0.8,0.1,1.0) );
    else
        tbox.setColor( ColorA(0.0,0.0,0.0,1.0) );
    tbox.setBackgroundColor( ColorA( 0.3, 0.3, 0.3, 0.3 ) );
    ivec2 sz = tbox.measure(); sz.x = sz.y;
    return  gl::Texture2d::create( tbox.render() );
}


void lifContext::draw_info ()
{
   
    auto ww = get_windowRef();
    auto ws = ww->getSize();
    gl::setMatricesWindow( ws );
    
    gl::ScopedBlendAlpha blend_;
    
    {
        gl::ScopedColor (getManualEditMode() ? ColorA( 0.25f, 0.5f, 1, 1 ) : ColorA::gray(1.0));
        gl::drawStrokedRect(get_image_display_rect(), 3.0f);
    }

    {
        gl::ScopedColor (ColorA::gray(0.1));
        gl::drawStrokedRect(m_layout->display_plots_rect(), 3.0f);
    }
    
    if (m_show_probe)
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
        Rectf textrect (0.0, ws[1] - mTextTexture->getHeight(), ws[0],ws[1]);
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
        
        switch(channel_count())
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
        
        
        if (isEditing() && mCellEnds.size() == 2)
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
        

        mContainer.draw();
        draw_info ();
      //  mUIParams.draw();
      
        if(m_showGUI)
            DrawGUI();
        
    }
    
    
}

#pragma GCC diagnostic pop
