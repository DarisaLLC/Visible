//
//  visible_mov_impl.cpp
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
#include "movContext.h"
#include "cinder/app/App.h"
#include "cinder/gl/gl.h"
#include "cinder/Timeline.h"
#include "cinder/Timer.h"
#include "cinder/params/Params.h"
#include "cinder/ImageIo.h"
#include "cinder/ip/Blend.h"
#include "opencv2/highgui.hpp"
#include "cinder/ip/Flip.h"
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
#include "algo_Mov.hpp"
#include "core/signaler.h"
#include "logger.hpp"
#include "cinder/Log.h"
#include "CinderImGui.h"
#include "gui_handler.hpp"
#include "gui_base.hpp"
#include <boost/any.hpp>
#include "ImGuiExtensions.h"
#include "Resources.h"
#include "cinder_opencv.h"
#include "imguivariouscontrols.h"
#include "imgui_wrapper.h"


using namespace ci;
using namespace ci::app;
using namespace std;
using namespace svl;

#define SHORTTERM_ON

#define ONCE(x) { static int __once = 1; if (__once) {x;} __once = 0; }

namespace {
    std::map<std::string, unsigned int> named_colors = {{"Red", 0xFF0000FF }, {"red",0xFF0000FF },{"Green", 0xFF00FF00},
        {"green", 0xFF00FF00},{ "Long", 0xFFFF0000}, { "long", 0xFFFF0000}, { "Short", 0xFFD66D3E}, { "short", 0xFFD66D3E}};
 
    
}



/************************
 *
 *  Setup & Load File
 *
 ************************/



/////////////  movContext Implementation  ////////////////
// @ todo setup default repository
// default median cover pct is 5% (0.05)

movContext::movContext(ci::app::WindowRef& ww, const cvVideoPlayer::ref& sd, const fs::path& cache_path) :sequencedImageContext(ww), m_sequence_player_ref(sd), m_geometry_available(false), mUserStorageDirPath (cache_path) {
    m_type = guiContext::Type::cv_video_viewer;
    m_show_contractions = false;
    // Create an invisible folder for storage
    mCurrentCachePath = mUserStorageDirPath;
    bool folder_exists = fs::exists(mCurrentCachePath);
    std::string msg = folder_exists ? " exists " : " does not exist ";
    msg = " folder for " + m_sequence_player_ref->name() + msg;
   // vlogger::instance().console()->info(msg);
    
    
    m_valid = m_sequence_player_ref->isLoaded();
    if (m_valid){
        m_layout = std::make_shared<layoutManager>  ( ivec2 (10, 30) );
            setup();
            ww->getRenderer()->makeCurrentContext(true);
        }
}


ci::app::WindowRef&  movContext::get_windowRef(){
    return shared_from_above()->mWindow;
}

void movContext::setup_signals(){
    
    // Create a Processing Object to attach signals to
    m_movProcRef = std::make_shared<sequence_processor> (mCurrentCachePath);
    
    // Support lifProcessor::content_loaded
    std::function<void (int64_t&)> content_loaded_cb = boost::bind (&movContext::signal_content_loaded, shared_from_above(), _1);
    boost::signals2::connection ml_connection = m_movProcRef->registerCallback(content_loaded_cb);
    
    // Support movProcessor::initial ss results available
    std::function<void (int&)> sm1d_available_cb = boost::bind (&movContext::signal_sm1d_available, shared_from_above(), _1);
    boost::signals2::connection nl_connection = m_movProcRef->registerCallback(sm1d_available_cb);
    
 
    
}

void movContext::setup_params () {
    
    //    gl::enableVerticalSync();
    
    
}


void movContext::setup()
{
    ci::app::WindowRef ww = get_windowRef();
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
    ww->setTitle( m_sequence_player_ref->name());
    ww->setSize(1280, 768);
    mFont = Font( "Menlo", 18 );
    auto ws = ww->getSize();
    mSize = vec2( ws[0], ws[1] / 12);
    clear_playback_params();
    load_current_sequence();
    shared_from_above()->update();
    
    ww->getSignalMouseDrag().connect( [this] ( MouseEvent &event ) { processDrag( event.getPos() ); } );
}



/************************
 *
 *
 *  Call Backs
 *
 ************************/

void movContext::signal_sm1d_available (int& dummy)
{
    vlogger::instance().console()->info("self-similarity available: ");
}

void movContext::signal_content_loaded (int64_t& loaded_frame_count )
{
    std::string msg = to_string(mMediaInfo.count) + " Samples in Media  " + to_string(loaded_frame_count) + " Loaded";
    vlogger::instance().console()->info(msg);
    process_async();
}


void movContext::signal_frame_loaded (int& findex, double& timestamp)
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

int movContext::get_current_clip_index () const
{
    std::lock_guard<std::mutex> guard(m_clip_mutex);
    return m_current_clip_index;
}

void movContext::set_current_clip_index (int cindex) const
{
    std::lock_guard<std::mutex> guard(m_clip_mutex);
    m_current_clip_index = cindex;
}

const clip& movContext::get_current_clip () const {
    return m_clips.at(m_current_clip_index);
}

void movContext::reset_entire_clip (const size_t& frame_count) const
{
    // Stop Playback ?
    m_clips.clear();
    m_clips.emplace_back(frame_count);
    set_current_clip_index(0);
}


/************************
 *
 *  Validation, Clear & Log
 *
 ************************/

void movContext::clear_playback_params ()
{
    m_seek_position = 0;
    m_is_playing = false;
    m_is_looping = false;
    m_show_probe = false;
    m_is_editing = false;
    m_zoom.x = m_zoom.y = 1.0f;
}

bool movContext::have_sequence ()
{
    cvVideoPlayer::weak_ref wr(m_sequence_player_ref);
    bool have =   wr.lock() && m_sequence_player_ref && mFrameSet && m_layout->isSet();
    return have;
}


bool movContext::is_valid () const { return m_valid && is_context_type(guiContext::cv_video_viewer); }




/************************
 *
 *  UI Conrol Functions
 *
 ************************/

void movContext::loop_no_loop_button ()
{
    if (! have_sequence()) return;
    if (looping())
        looping(false);
    else
        looping(true);
}

void movContext::looping (bool what)
{
    m_is_looping = what;
}


bool movContext::looping ()
{
    return m_is_looping;
}

void movContext::play ()
{
    if (! have_sequence() || m_is_playing ) return;
    m_is_playing = true;
}

void movContext::pause ()
{
    if (! have_sequence() || ! m_is_playing ) return;
    m_is_playing = false;
}

// For use with RAII scoped pause pattern
void movContext::play_pause_button ()
{
    if (! have_sequence () ) return;
    if (m_is_playing)
        pause ();
    else
        play ();
}




/************************
 *
 *  Seek Processing
 *
 ************************/

// @ todo: indicate mode differently
void movContext::seekToEnd ()
{
    seekToFrame (get_current_clip().last());
    assert(! m_clips.empty());
}

void movContext::seekToStart ()
{
    
    seekToFrame(get_current_clip().first());
    assert(! m_clips.empty());
}

int movContext::getNumFrames ()
{
    return mMediaInfo.count;
}

int movContext::getCurrentFrame ()
{
    return int(m_seek_position);
}

time_spec_t movContext::getCurrentTime ()
{
    return time_spec_t (m_sequence_player_ref->getElapsedSeconds());
}



void movContext::seekToFrame (int mark)
{
    const clip& curr = get_current_clip();
    
    if (mark < curr.first()) mark = curr.first();
    if (mark > curr.last())
        mark = looping() ? curr.first() : curr.last();
    
    m_seek_position = mark;
}


void movContext::onMarked ( marker_info& t)
{
    seekToFrame((int) t.current_frame());
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


/************************
 *
 *  Navigation UI
 *
 ************************/

void movContext::processDrag( ivec2 pos )
{
    //    for (Widget* wPtr : mWidgets)
    //    {
    //        if( wPtr->hitTest( pos ) ) {
    //            mTimeMarker.from_norm(wPtr->valueScaled());
    //            seekToFrame((int) mTimeMarker.current_frame());
    //        }
    //    }
}

void  movContext::mouseWheel( MouseEvent event )
{
#if 0
    if( mMouseInTimeLine )
        mTimeMarker.from_norm(mTimeLineSlider.mValueScaled);
    seekToFrame(mTimeMarker.current_frame());
}

mPov.adjustDist( event.getWheelIncrement() * -5.0f );
#endif

}


void movContext::mouseMove( MouseEvent event )
{
    update_with_mouse_position (event);
}

void movContext::update_with_mouse_position ( MouseEvent event )
{
    mMouseInImage = false;
    mMouseInGraphs  = -1;
    
    if (! have_sequence () ) return;
    
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
    
    
}


void movContext::mouseDrag( MouseEvent event )
{
    
}


void movContext::mouseDown( MouseEvent event )
{
    
}


void movContext::mouseUp( MouseEvent event )
{
    
}




void movContext::keyDown( KeyEvent event )
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
    if( have_sequence () ) {
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
 *  Load Serie & Setup Widgets
 *
 ************************/

// Set window size according to layout
//  Channel             Data
//  1
//  2
//  3

void movContext::load_current_sequence ()
{
    if ( ! is_valid() || ! m_sequence_player_ref )
        return;
    
    try {
        /*
         * Clear and pause if we are playing back
         */
        m_clips.clear();
        pause();
        
        /*
         * Create the frameset and assign the channel names
         * Fetch the media info
         */
        mFrameSet = seqFrameContainer::create (m_sequence_player_ref);
        if (! mFrameSet || ! mFrameSet->isValid())
        {
            vlogger::instance().console()->debug("Serie had 1 or no frames ");
            return;
        }
        mMediaInfo = mFrameSet->media_info();
        mChannelCount = (uint32_t) mMediaInfo.getNumChannels();
        if (!(mChannelCount > 0 && mChannelCount < 5)){
            vlogger::instance().console()->debug("Expected 1 or 2 or 3 channels ");
            return;
        }
        
        m_layout->init (getWindowSize() , mFrameSet->media_info(), channel_count());
        
        /*
         * Create the frameset and assign the channel namesStart Loading Images on async on a different thread
         * Loading also produces voxel images.
         */
        m_plot_names.clear();
        m_plot_names =m_sequence_player_ref->channel_names();
        m_plot_names.push_back("MisRegister");
        cv::Mat result;
        std::vector<std::thread> threads(1);
        threads[0] = std::thread(&sequence_processor::load, m_movProcRef.get(), mFrameSet,m_sequence_player_ref->channel_names(), m_plot_names);
        std::for_each(threads.begin(), threads.end(), std::mem_fn(&std::thread::join));
        
        /*
         * Fetch length and channel names
         */
        mFrameSet->channel_names (m_sequence_player_ref->channel_names());
        reset_entire_clip(mFrameSet->count());
        m_minFrame = 0;
        m_maxFrame =  mFrameSet->count() - 1;
        
        // Initialize Sequencer
        m_result_seq.mFrameMin = 0;
        m_result_seq.mFrameMax = (int) mFrameSet->count() - 1;
        m_result_seq.mSequencerItemTypeNames = {"All", "Contraction", "Force"};
        m_result_seq.myItems.push_back(timeLineSequence::timeline_item{ 0, 0, (int) mFrameSet->count(), true});
        
        m_title =m_sequence_player_ref->name() + " @ " + mPath.filename().string();
        
        auto ww = get_windowRef();
        ww->setTitle(m_title );
        ww->getApp()->setFrameRate(mMediaInfo.getFramerate());
        
        
        mScreenSize = mMediaInfo.getSize();
        
        mSurface = Surface8u::create (int32_t(mScreenSize.x), int32_t(mScreenSize.y), true);
    }
    catch( const std::exception &ex ) {
        console() << ex.what() << endl;
        return;
    }
}

void movContext::process_window(int window_id){
    
}

void movContext::process_async (){
    
    switch(channel_count()){
        case 3:
        case 4:
        {
            // note launch mode is std::launch::async
            m_longterm_pci_tracks_async = std::async(std::launch::async, &sequence_processor::run_longterm_pci_on_channel, m_movProcRef.get(), 2);
            break;
        }
        case 1:
        {
            m_longterm_pci_tracks_async = std::async(std::launch::async, &sequence_processor::run_longterm_pci_on_channel,m_movProcRef.get(), 0);
            break;
        }
    }
}


/************************
 *
 *  Update & Draw
 *
 ************************/


/*
 * Result Window, to the right of image display + pad
 * Size width: remainder to the edge of app window in x ( minus a pad )
 * Size height: app window height / 2
 */

void movContext::add_result_sequencer ()
{
    // let's create the sequencer
    static int selectedEntry = -1;
    static int64 firstFrame = 0;
    static bool expanded = true;
    
    Rectf dr = get_image_display_rect();
    auto tr = dr.getUpperRight();
    auto pos = ImVec2(tr.x+10, tr.y);
    ImVec2 size (getWindowWidth()-30-pos.x, (3*getWindowHeight()) / 4);
    
    m_results_browser_display = Rectf(pos,size);
    ImGui::SetNextWindowPos(pos);
    ImGui::SetNextWindowSize(size);
    
    ImGui::Begin(" Results ", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    
    ImGui::PushItemWidth(130);
    ImGui::InputInt("Frame Min", &m_result_seq.mFrameMin);
    ImGui::SameLine();
    ImGui::InputInt64("Frame ", &m_seek_position);
    ImGui::SameLine();
    ImGui::InputInt("Frame Max", &m_result_seq.mFrameMax);
    ImGui::PopItemWidth();
    
    
    Sequencer(&m_result_seq, &m_seek_position, &expanded, &selectedEntry, &firstFrame, ImSequencer::SEQUENCER_EDIT_NONE );
    ImGui::End();
    
    //    // add a UI to edit that particular item
    //    if (selectedEntry != -1)
    //    {
    //        const timeLineSequence::timeline_item &item = mySequence.myItems[selectedEntry];
    //        ImGui::Text("I am a %s, please edit me", mySequence.mSequencerItemTypeNames[item.mType].c_str());
    //        // switch (type) ....
    //    }
    
    
    
    
}

void movContext::add_navigation(){
    
    Rectf dr = get_image_display_rect();
    auto pos = ImVec2(dr.getLowerLeft().x, dr.getLowerLeft().y + 30);
    ImVec2  size (dr.getWidth(), std::min(128.0, dr.getHeight()/2.0));
    m_navigator_display = Rectf(pos,size);
    ImGui::SetNextWindowPos(pos);
    ImGui::SetNextWindowSize(size);
    
    
    if (ImGui::Begin("Navigation", &m_show_playback, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::PushItemWidth(40);
        ImGui::PushID(200);
        ImGui::InputInt("", &m_result_seq.mFrameMin, 0, 0);
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
        ImGui::InputInt("",  &m_result_seq.mFrameMax, 0, 0);
        ImGui::PopID();
        
        
    }
    ImGui::End();
    
}


void movContext::add_motion_profile (){
    
    if (! m_geometry_available ) return;
    if (! m_segmented_texture ){
        // Create a texcture for display
        Surface8uRef sur = Surface8u::create(cinder::fromOcv(m_segmented_image));
        auto texFormat = gl::Texture2d::Format().loadTopDown();
        m_segmented_texture = gl::Texture::create(*sur, texFormat);
    }
    
    ImVec2  sz (m_segmented_texture->getWidth(),m_segmented_texture->getHeight());
    ImVec2  frame (m_segmented_texture->getWidth()*3.0f,m_segmented_texture->getHeight()*3.0f);
    Rectf dr = m_results_browser_display;
    auto pos = ImVec2(dr.getLowerLeft().x, dr.getLowerLeft().y + 30);
    m_motion_profile_display = Rectf(pos,frame);
    ImGui::SetNextWindowPos(pos);
    ImGui::SetNextWindowSize(frame);
    
    const RotatedRect& mt = m_movProcRef->motion_surface_bottom();
    cv::Point2f points[4];
    mt.points(&points[0]);
    
    if (ImGui::Begin("Motion Profile", nullptr,ImGuiWindowFlags_AlwaysAutoResize ))
    {
        if(m_segmented_texture){
            static ImVec2 zoom_center;
            // First Child
            ImGui::BeginChild(" ", frame, true,  ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar );
            ImVec2 pp = ImGui::GetCursorScreenPos();
            //     ImGui::ImageZoomAndPan( (void*)(intptr_t)  m_segmented_texture->getId(),sz,
            //                            m_segmented_texture->getAspectRatio(),NULL,&zoom,&zoom_center);
            ImVec2 p (pp.x + mt.center.x, pp.y  + mt.center.y);
            ImGui::Image( (void*)(intptr_t) m_segmented_texture->getId(), frame);
            ImGui::EndChild();
            
            // Second Child
            ImGui::BeginChild(" ", frame, true,  ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar );
            ImGui::GetWindowDrawList()->AddLine(ImVec2(p.x -5, p.y ), ImVec2(p.x + 5, p.y ), IM_COL32(255, 0, 0, 255), 3.0f);
            ImGui::GetWindowDrawList()->AddLine(ImVec2(p.x , p.y-5 ), ImVec2(p.x , p.y + 5), IM_COL32(255, 0, 0, 255), 3.0f);
            
            for (int ii = 0; ii < 4; ii++){
                cv::Point2f pt = points[ii];
                pt.x += (pp.x );
                pt.y += (pp.y );
                if (pt.x >=5 && pt.y >=5){
                    ImGui::GetWindowDrawList()->AddLine(ImVec2(pt.x -5, pt.y ), ImVec2(pt.x + 5, pt.y ), IM_COL32(255, 128, 0, 255), 3.0f);
                    ImGui::GetWindowDrawList()->AddLine(ImVec2(pt.x , pt.y-5 ), ImVec2(pt.x , pt.y + 5), IM_COL32(255, 128, 0, 255), 3.0f);
                }
            }
            ImGui::EndChild();
        }
    }
    ImGui::End();
}

void  movContext::SetupGUIVariables(){
    
    //      ImGuiStyle* st = &ImGui::GetStyle();
    //      ImGui::StyleColorsLightGreen(st);
    
}


void  movContext::DrawGUI(){
    
    add_navigation();
    add_result_sequencer();
    add_motion_profile ();
}

Rectf movContext::get_image_display_rect ()
{
    return m_layout->display_frame_rect();
}

void  movContext::update_log (const std::string& msg)
{
    if (msg.length() > 2)
        mLog = msg;
    TextBox tbox = TextBox().alignment( TextBox::RIGHT).font( mFont ).size( mSize ).text( mLog );
    tbox.setColor( Color( 1.0f, 0.65f, 0.35f ) );
    tbox.setBackgroundColor( ColorA( 0.3f, 0.3f, 0.3f, 0.4f )  );
    tbox.measure();
    mTextTexture = gl::Texture2d::create( tbox.render() );
}


void movContext::resize ()
{
    if (! have_sequence () || ! mSurface ) return;
    if (! m_layout->isSet()) return;
    auto ww = get_windowRef();
    auto ws = ww->getSize();
    m_layout->update_window_size(ws);
    mSize = vec2(ws[0], ws[1] / 12);
    //  mMainTimeLineSlider.setBounds (m_layout->display_timeline_rect());
    
    
}

bool movContext::haveTracks()
{
    return  ! m_longterm_pci_trackWeakRef.expired();
}

void movContext::update ()
{
    ci::app::WindowRef ww = get_windowRef();
    ww->getRenderer()->makeCurrentContext(true);
    if (! have_sequence() ) return;
    auto ws = ww->getSize();
    m_layout->update_window_size(ws);
    
    // If Plots are ready, set them up It is ready only for new data
    // @todo replace with signal
    //@todo switch to using weak_ptr all together
    if ( is_ready (m_longterm_pci_tracks_async)){
        m_longterm_pci_trackWeakRef = m_longterm_pci_tracks_async.get();
    }
    
    // Update PCI result if ready
    if ( ! m_longterm_pci_trackWeakRef.expired())
    {
#ifdef SHORTTERM_ON
        //  if (m_movProcRef->shortterm_pci().at(0).second.empty())
        //      m_movProcRef->shortterm_pci(1);
#endif
        auto tracksRef = m_longterm_pci_trackWeakRef.lock();
        m_result_seq.m_editable_plot_data.load(tracksRef->at(0), named_colors["Long"], 2);
        
        // Update shortterm PCI result if ready
        if ( ! m_movProcRef->shortterm_pci().at(0).second.empty() )
        {
            auto tracksRef = m_movProcRef->shortterm_pci();
            m_result_seq.m_editable_plot_data.load(tracksRef.at(0), named_colors["Short"], 3);
        }
    }
    
    // Fetch Next Frame
    if (have_sequence ()){
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
void movContext::update_instant_image_mouse ()
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

gl::TextureRef movContext::pixelInfoTexture ()
{
    if (! mMouseInImage) return gl::TextureRef ();
    TextLayout lout;
    const auto names =m_sequence_player_ref->channel_names();
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


void movContext::draw_info ()
{
    
    auto ww = get_windowRef();
    auto ws = ww->getSize();
    gl::setMatricesWindow( ws );
    
    
    gl::ScopedBlendAlpha blend_;
    
    
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
    
}


void movContext::draw ()
{
    if( have_sequence()  && mSurface )
    {
        Rectf dr = get_image_display_rect();
        ivec2 tl = dr.getUpperLeft();
        tl.y += (2*dr.getHeight())/3.0;
        Rectf gdr (tl, dr.getLowerRight());
        
        assert(dr.getWidth() > 0 && dr.getHeight() > 0);
        
        gl::ScopedBlendAlpha blend_;
        
        switch(channel_count())
        {
            case 1:
                mImage = gl::Texture::create(*mSurface);
                mImage->setMagFilter(GL_NEAREST_MIPMAP_NEAREST);
                gl::draw (mImage, dr);
                break;
            case 3:
            case 4:
                mImage = gl::Texture::create(*mSurface);
                mImage->setMagFilter(GL_NEAREST_MIPMAP_NEAREST);
                gl::draw (mImage, dr);
                break;
        }
        
        {
            //@todo change color to indicate presence in any display area
            //  gl::ScopedColor (getManualEditMode() ? ColorA( 0.25f, 0.5f, 1, 1 ) : ColorA::gray(1.0));
            gl::drawStrokedRect(get_image_display_rect(), 3.0f);
        }
        
        // std::vector<float> test = {0.0,0.1,0.2,1.0,0.2,0.1,0.0};
        // m_tsPlotter.setBounds(gdr);
        // m_tsPlotter.draw(test);
        
        if (m_geometry_available)
        {
            const cv::RotatedRect& ellipse = m_movProcRef->motion_surface();
            const fPair& ab = m_movProcRef->ellipse_ab();
            vec2 a_v ((ab.first * gdr.getWidth()) / 512, 0.0f );
            vec2 b_v (0.0f, (ab.second * gdr.getHeight()) / 128);
            
            cinder::gl::ScopedLineWidth( 10.0f );
            {
                cinder::gl::ScopedColor col (ColorA( 1.0, 0.1, 0.0, 0.8f ) );
                {
                    cinder::gl::ScopedModelMatrix _mdl;
                    uDegree da(ellipse.angle);
                    uRadian dra (da);
                    vec2 ctr ((ellipse.center.x * gdr.getWidth()) / 512, (ellipse.center.y * gdr.getHeight()) / 128 );
                    ctr = ctr + gdr.getUpperLeft();
                    gl::translate(ctr);
                    
                    gl::rotate(dra.Double());
                    ctr = vec2(0,0);
                    gl::drawSolidCircle(ctr, 3.0);
                    gl::drawLine(ctr-b_v, ctr+b_v);
                    gl::drawLine(ctr-a_v, ctr+a_v);
                    
                    gl::drawStrokedEllipse(ctr, a_v.x, b_v.y);
                }
            }
        }
        
    
        
        if(m_showGUI)
            DrawGUI();
        
    }
    
    
}

#pragma GCC diagnostic pop
