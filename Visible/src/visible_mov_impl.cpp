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
#include <strstream>
#include <algorithm>
#include <future>
#include <mutex>
#include <stdio.h>
#include "guiContext.h"
#include "movContext.h"
#include "cinder/app/App.h"
#include "cinder/gl/gl.h"
#include "cinder/Timer.h"
#include "cinder/ImageIo.h"
#include "cinder/ip/Blend.h"
#include "opencv2/highgui.hpp"
#include "cinder/ip/Flip.h"

#include "timed_value_containers.h"
#include "cinder_xchg.hpp"
#include "visible_layout.hpp"
#include "vision/opencv_utils.hpp"
#include "core/stl_utils.hpp"
#include "lif_content.hpp"
#include "core/signaler.h"
#include "contraction.hpp"
#include "logger/logger.hpp"
#include "cinder/Log.h"
#include "CinderImGui.h"
#include "ImGuiExtensions.h" // for 64bit count support
#include "Resources.h"
#include "cinder_opencv.h"
#include "imguivariouscontrols.h"
#include <boost/range/irange.hpp>
//#include "imgui_plot.h"
#include "imgui_visible_widgets.hpp"

using namespace ci;
using namespace ci::app;
using namespace std;
using namespace svl;

// #define SHORTTERM_ON

#define ONCE(x) { static int __once = 1; if (__once) {x;} __once = 0; }

namespace {
    std::map<std::string, unsigned int> named_colors = {{"Red", 0xFF0000FF }, {"red",0xFF0000FF },{"Green", 0xFF00FF00},
        {"green", 0xFF00FF00},{ "PCI", 0xFFFF0000}, { "pci", 0xFFFF0000}, { "Short", 0xFFD66D3E}, { "short", 0xFFFFFF3E},
        { "Synth", 0xFFFB4551}, { "synth", 0xFFFB4551},{ "Length", 0xFFFFFF3E}, { "length", 0xFFFFFF3E},
        { "Force", 0xFFFB4551}, { "force", 0xFFFB4551}, { "Elongation", 0xFF00FF00}, { "elongation", 0xFF00FF00}
    };
    
    
}

#define wDisplay "Display"
#define wResult "Result"
#define wCells  "Cells"
#define wContractions  "Contractions"
#define wNavigator   "Navigator"
#define wShape "Shape"



                    /************************
                     *
                     *  Setup & Load File
                     *
                     ************************/



    /////////////  movContext Implementation  ////////////////
// @ todo setup default repository
// default median cover pct is 5% (0.05)

movContext::movContext(ci::app::WindowRef& ww, const cvVideoPlayer::ref& sd, const fs::path& cache_path) :sequencedImageContext(ww), m_sequence_player_ref(sd), m_voxel_view_available(false), mUserStorageDirPath (cache_path) {
    m_type = guiContext::Type::cv_video_viewer;
    m_show_contractions = false;
    // Create an invisible folder for storage
    mCurrentCachePath = mUserStorageDirPath / sd->name();
    bool folder_exists = fs::exists(mCurrentCachePath);
    std::string msg = folder_exists ? " exists " : " does not exist ";
    msg = " folder for " + sd->name() + msg;
    vlogger::instance().console()->info(msg);
  //  m_idlab_defaults.median_level_set_cutoff_fraction = 15;
    m_playback_speed = 1;
    m_input_selector = input_channel_selector_t(-1,0);
    m_selector_last = -1;
    m_show_display_and_controls = false;
    m_show_results = false;
    m_show_playback = false;
    
    m_valid = sd->isLoaded();
    if (m_valid){
        m_layout = std::make_shared<imageDisplayMapper>  ();
        setup();
        ww->getRenderer()->makeCurrentContext(true);
    }
}


ci::app::WindowRef&  movContext::get_windowRef(){
    return shared_from_above()->mWindow;
}

void movContext::setup_signals(){

    // Create a Processing Object to attach signals to
    ssmt_processor::params bgra_params(ssmt_processor::params::ContentType::bgra);
    m_movProcRef = std::make_shared<ssmt_processor> (mCurrentCachePath, bgra_params );
    
    // Support lifProcessor::content_loaded
    std::function<void (int64_t&)> content_loaded_cb = boost::bind (&movContext::signal_content_loaded, shared_from_above(), _1);
    boost::signals2::connection ml_connection = m_movProcRef->registerCallback(content_loaded_cb);
    
    // Support lifProcessor::initial ss results available
    std::function<void (std::vector<float> &, const input_channel_selector_t&)> sm1d_ready_cb = boost::bind (&movContext::signal_sm1d_ready, shared_from_above(), _1, _2);
    boost::signals2::connection nl_connection = m_movProcRef->registerCallback(sm1d_ready_cb);
    
    // Support lifProcessor::median level set ss results available
    std::function<void (const input_channel_selector_t&)> sm1dmed_ready_cb = boost::bind (&movContext::signal_sm1dmed_ready, shared_from_above(), _1);
    boost::signals2::connection ol_connection = m_movProcRef->registerCallback(sm1dmed_ready_cb);
    
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
    ww->setSize(1536, 1024);
    mFont = Font( "Menlo", 18 );
    auto ws = ww->getSize();
    mSize = vec2( ws[0], ws[1] / 12);
  //  m_contraction_names = m_contraction_none;
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

void movContext::signal_sm1d_ready (std::vector<float> & signal, const input_channel_selector_t& dummy)
{
    stringstream ss;
    ss << dummy.region() << " self-similarity available ";
    vlogger::instance().console()->info(ss.str());
}

void movContext::signal_sm1dmed_ready (const input_channel_selector_t& dummy2)
{
   //@note this is also checked in update. Not sure if this is necessary
//   if (haveTracks())
//    {
//        auto tracksRef = m_longterm_pci_trackWeakRef.lock();
//        if ( tracksRef && !tracksRef->at(0).second.empty()){
//            m_result_seq.m_time_data.load(tracksRef->at(0), named_colors["PCI"], 2);
//        }
//    }
    vlogger::instance().console()->info("self-similarity available: ");
}

void movContext::signal_content_loaded (int64_t& loaded_frame_count )
{
    std::string msg = to_string(mMediaInfo.count) + " Samples in Media  " + to_string(loaded_frame_count) + " Loaded";
    vlogger::instance().console()->info(msg);
    process_async();
}

void movContext::glscreen_normalize (const sides_length_t& src, const Rectf& gdr, sides_length_t& dst){
    
    dst.first.x = (src.first.x*gdr.getWidth()) / mMediaInfo.getWidth();
    dst.second.x = (src.second.x*gdr.getWidth()) / mMediaInfo.getWidth();
    dst.first.y = (src.first.y*gdr.getHeight()) / mMediaInfo.getHeight();
    dst.second.y = (src.second.y*gdr.getHeight()) / mMediaInfo.getHeight();
}
                                     
void movContext::signal_segmented_view_ready (cv::Mat& image, cv::Mat& label)
{
    vlogger::instance().console()->info(" Voxel View Available  ");
    if (! m_movProcRef){
        vlogger::instance().console()->error("Lif Processor Object does not exist ");
        return;
    }
    m_segmented_image = image.clone();
    m_voxel_view_available = true;
    
    if (! m_segmented_texture ){
        m_segmented_surface = Surface8u::create(cinder::fromOcv(m_segmented_image));
    //    auto cell_ends =  m_movProcRef->cell_ends ();
     //   m_cell_ends = cell_ends;
    }
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
 //   m_contraction_names.resize(1);
//    m_contraction_names[0] = " Entire ";
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


bool movContext::is_valid () const {
    return m_valid && is_context_type(guiContext::cv_video_viewer); }




/************************
 *
 *  UI Conrol Functions
 *
 ************************/

void movContext::loop_no_loop_button ()
{
    if (! is_valid()) return;
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
    if (! is_valid() || m_is_playing ) return;
    m_is_playing = true;
}

void movContext::pause ()
{
    if (! is_valid() || ! m_is_playing ) return;
    m_is_playing = false;
}

// For use with RAII scoped pause pattern
void movContext::play_pause_button ()
{
    if (! is_valid () ) return;
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
    if (m_seek_position >= 0 && m_seek_position < getNumFrames())
        return m_sequence_player_ref->getElapsedSeconds();
    else return -1.0;
}

time_spec_t movContext::getStartTime (){
    return time_spec_t ();
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
    
    if (! is_valid() ) return;
    
    mMouseInImage = get_image_display_rect().contains(event.getPos());
    if (mMouseInImage)
    {
        mMouseInImagePosition = event.getPos();
        update_instant_image_mouse ();
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
    if( is_valid ()) {
        if( event.getCode() == KeyEvent::KEY_LEFT ) {
            pause();
            seekToFrame (getCurrentFrame() - m_playback_speed);
            if (mMouseInImage)
                update_instant_image_mouse ();
        }
        else if( event.getCode() == KeyEvent::KEY_RIGHT ) {
            pause ();
            seekToFrame (getCurrentFrame() + m_playback_speed);
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

void movContext::setMedianCutOff (int32_t newco)
{
    if (! m_movProcRef) return;
    // Get a shared_ptr from weak and check if it had not expired
    auto spt = m_movProcRef->entireContractionWeakRef().lock();
    if (! spt ) return;
    uint32_t tmp = newco % 100; // pct
    uint32_t current (spt->get_median_levelset_pct () * 100);
    if (tmp == current) return;
    spt->set_median_levelset_pct (tmp / 100.0f);
    m_movProcRef->update(m_input_selector);

}

int32_t movContext::getMedianCutOff () const
{
    // Get a shared_ptr from weak and check if it had not expired
    auto spt = m_movProcRef->entireContractionWeakRef().lock();
    if (spt)
    {
        uint32_t current (spt->get_median_levelset_pct () * 100);
        return current;
    }
    return 0;
    
//    if (! m_movProcRef || ! m_geometry_available) return 0;
//    int select_last (m_movProcRef->moving_bodies().size());
//    assert(select_last > 0);
//    select_last--;
//    const auto& mb = m_movProcRef->moving_bodies();
//    auto spt = mb[select_last]->locator();
//    if (! spt ) return 0;
//    m_selector_last = select_last;
//    uint32_t current (spt->get_median_levelset_pct () * 100);
//    return current;

}
//
//void movContext::setCellLength (uint32_t newco){
//    if (! m_movProcRef) return;
//    // Get a shared_ptr from weak and check if it had not expired
//    auto spt = m_movProcRef->contractionWeakRef().lock();
//    if (! spt ) return;
//    m_cell_length = newco;
//}
//uint32_t movContext::getCellLength () const{
//    if (! m_cur_lif_serie_ref) return 0;
//
//    // Get a shared_ptr from weak and check if it had not expired
//    auto spt = m_movProcRef->contractionWeakRef().lock();
//    if (spt)
//    {
//        return m_cell_length;
//    }
//    return 0;
//}
//


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
    if ( ! is_valid() || ! m_sequence_player_ref)
        return;
    
    try {
        /*
         * Clear and pause if we are playing back
         */
        m_clips.clear();
        m_instant_channel_display_rects.clear();
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
        m_instant_channel_display_rects.resize(mChannelCount);
        
        if (!(mChannelCount > 0 && mChannelCount <= 4)){
            vlogger::instance().console()->debug("Expected 1 or 2 or 3 channels ");
            return;
        }

        m_layout->init ( mFrameSet->media_info());

        /*
         * Create the frameset and assign the channel namesStart Loading Images on async on a different thread
         * Loading also produces voxel images.
         */
        m_plot_names.clear();
        m_plot_names = m_sequence_player_ref->channel_names();
        m_plot_names.push_back("MisRegister");
//        cv::Mat result;
        std::vector<std::thread> threads(1);
        threads[0] = std::thread(&ssmt_processor::load, m_movProcRef.get(), mFrameSet, m_sequence_player_ref->channel_names(), m_plot_names);
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
        m_result_seq.mSequencerItemTypeNames = {"All", "Contractions", "Length"};
        m_result_seq.items.push_back(timeLineSequence::timeline_item{ 0, 0, (int) mFrameSet->count(), true});
        
        m_title = m_sequence_player_ref->name() + " @ " + mPath.filename().string();
        
        auto ww = get_windowRef();
        ww->setTitle(m_title );
        ww->getApp()->setFrameRate(mMediaInfo.getFramerate());
        
        
        mScreenSize = mMediaInfo.getSize();
        
        mSurface = Surface8u::create (int32_t(mScreenSize.x), int32_t(mScreenSize.y), true);
        mFbo = gl::Fbo::create(mSurface->getWidth(), mSurface->getHeight());
        m_show_playback = true;
    }
    catch( const std::exception &ex ) {
        console() << ex.what() << endl;
        m_show_playback = false;
        return;
    }
}

void movContext::process_async (){
    
    // @note: ID_LAB  specific. @todo general LIF / TIFF support
    switch(channel_count()){
        case 3:
        {
            // note launch mode is std::launch::async
            m_fluorescense_tracks_aync = std::async(std::launch::async,&ssmt_processor::run_flu_statistics,
                                                  m_movProcRef.get(), std::vector<int> ({0,1}) );
            input_channel_selector_t in (-1,2);
            m_contraction_pci_tracks_asyn = std::async(std::launch::async, &ssmt_processor::run_contraction_pci_on_selected_input, m_movProcRef.get(), in);
            break;
        }
        case 2:
        {
            // note launch mode is std::launch::async
         //   m_fluorescense_tracks_aync = std::async(std::launch::async,&ssmt_processor::run_flu_statistics,
          //                                          m_movProcRef.get(), std::vector<int> ({0}) );
         //   input_channel_selector_t in (-1,1);
          //  m_contraction_pci_tracks_asyn = std::async(std::launch::async, &ssmt_processor::run_contraction_pci_on_selected_input, m_movProcRef.get(), in);
            break;
        }
        case 1:
        {
            input_channel_selector_t in (-1,0);
            m_contraction_pci_tracks_asyn = std::async(std::launch::async, &ssmt_processor::run_contraction_pci_on_selected_input, m_movProcRef.get(), in);
            break;
        }
    }
}


/************************
 *
 *  Update & Draw
 *
 ************************/


const Rectf& movContext::get_image_display_rect ()
{
    return m_display_rect;
}

void  movContext::update_channel_display_rects (){
    int cn = channel_count();
    assert(m_instant_channel_display_rects.size() == channel_count());
    vec2 channel_size (get_image_display_rect().getWidth(), get_image_display_rect().getHeight()/cn);
    vec2 itl = get_image_display_rect().getUpperLeft();
    for (int cc = 1; cc <= cn; cc++){
        Rectf& rect = m_instant_channel_display_rects[cc-1];
        rect = Rectf(itl, itl+channel_size);
        itl += ivec2(0, channel_size.y);
    }
}

const Rectf& movContext::get_channel_display_rect (const int channel_number_zero_based){
    return m_instant_channel_display_rects[channel_number_zero_based];
}


void movContext::add_canvas (){
   
    //@note: In ImGui, AddImage is called with uv parameters to specify a vertical flip.
    auto showImage = [&](const char *windowName,bool *open, const gl::Texture2dRef texture){
        if (open && *open)
        {
            ImGuiIO& io = ImGui::GetIO();
            ImGui::SetNextWindowBgAlpha(0.4f); // Transparent background
            ImDrawList* draw_list = ImGui::GetWindowDrawList();
            if (ImGui::Begin(windowName, open, io.ConfigWindowsResizeFromEdges))
            {
                ImVec2 pos = ImGui::GetCursorScreenPos(); // actual position
                draw_list->AddImage(  reinterpret_cast<ImTextureID> (texture->getId()), pos,
                                    ImVec2(ImGui::GetContentRegionAvail().x + pos.x, ImGui::GetContentRegionAvail().y  + pos.y),
                                    ivec2(0,1), ivec2(1,0));
                ImVec2 canvas_pos = ImGui::GetCursorScreenPos();            // ImDrawList API uses screen coordinates!
                ImVec2 canvas_size = ImGui::GetContentRegionAvail();        // Resize canvas to what's available
                ImRect regionRect(canvas_pos, canvas_pos + canvas_size);
                // Update image/display coordinate transformer
                m_layout->update_display_rect(canvas_pos, canvas_pos + canvas_size);
                // Draw Divider between channels
                ImVec2 midv = m_layout->image2display(ivec2(0,mMediaInfo.channel_size.height));
                ImVec2 midh = m_layout->image2display(ivec2(mMediaInfo.channel_size.width, mMediaInfo.channel_size.height));
                draw_list->AddLine(midv, midh, IM_COL32(0,0,128,128), 3.0f);
            }
            ImGui::End();
        }
    };
    
    //@note: assumes, next image plus any on image graphics have already been added
    // offscreen via FBO
    if(m_show_playback){
        ci::vec2 pos (0,0);
        ci::vec2 size (getWindowWidth()/2.0, getWindowHeight()/2.0f - 20.0);
        ui::ScopedWindow utilities(wDisplay);
        ImGui::SetNextWindowPos(pos);
        ImGui::SetNextWindowSize(size);
        ImGui::SameLine();
        showImage(wDisplay, &m_show_playback, mFbo->getColorTexture());
    }
    
    
}

void movContext::add_navigation(){
    
    if(m_show_playback){
        
        ImGuiWindow* window = ImGui::FindWindowByName(wDisplay);
        assert(window != nullptr);
        ImVec2 pos (0 , window->Pos.y + window->Size.y );
        ImVec2 size (window->Size.x, 100);
        
        ui::ScopedWindow utilities(wNavigator, ImGuiWindowFlags_NoResize);
        m_navigator_display = Rectf(pos,size);
        ImGui::SetNextWindowPos(pos);
        ImGui::SetNextWindowSize(size);
        ImGui::SameLine();
        ImGui::BeginGroup();
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
                seekToFrame (getCurrentFrame() - m_playback_speed);
            ImGui::SameLine();
            if (ImGui::Button(">"))
                seekToFrame (getCurrentFrame() + m_playback_speed);
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
            ImGui::SameLine();
            if (ImGui::Button(m_playback_speed == 10 ? " 1 " : " 10x "))
            {
                auto current = m_playback_speed;
                m_playback_speed = current == 1 ? 10 : 1;
            }
            ImGui::SameLine();
            
            auto dt = getCurrentTime().secs() - getStartTime().secs();
            ui::SameLine(0,0); ui::Text("% 8d\t%4.4f Seconds", getCurrentFrame(), dt);
            ImGui::SameLine();
            if (ImGui::Button(m_median_set_at_default ? "Default" : "Not Set"))
            {
                m_median_set_at_default = ! m_median_set_at_default;
            }
            
        }
        ImGui::EndGroup();
    }
}
/*
 * Result Window, to below the Display
 * Size width: remainder to the edge of app window in x ( minus a pad )
 * Size height: app window height / 2
 */

void movContext::add_result_sequencer (){
 
    // let's create the sequencer
    static int selectedEntry = -1;
    static int64 firstFrame = 0;
    static bool expanded = true;
    
    ImGuiWindow* window = ImGui::FindWindowByName(wDisplay);
    assert(window != nullptr);
    ImVec2 pos (window->Pos.x , window->Pos.y + window->Size.y);
    ImVec2 size (getWindowWidth()/2, getWindowHeight()/2);
    m_results_browser_display = Rectf(pos,size);
    ImGui::SetNextWindowPos(pos);
    ImGui::SetNextWindowSize(size);

    static bool results_open;
    if(ImGui::Begin(wResult, &results_open, ImGuiWindowFlags_AlwaysAutoResize)){
        ImGui::PushItemWidth(130);
        ImGui::InputInt("Frame Min", &m_result_seq.mFrameMin);
        ImGui::SameLine();
        ImGui::InputInt64("Frame ", &m_seek_position);
        ImGui::SameLine();
        ImGui::InputInt("Frame Max", &m_result_seq.mFrameMax);
        ImGui::PopItemWidth();
        

        Sequencer(&m_result_seq, &m_seek_position, &expanded, &selectedEntry, &firstFrame, ImSequencer::SEQUENCER_EDIT_NONE );

        
        // add a UI to edit that particular item
        if (selectedEntry != -1)
        {
            const timeLineSequence::timeline_item &item = m_result_seq.items[selectedEntry];
            ImGui::Text("I am a %s, please edit me", m_result_seq.mSequencerItemTypeNames[item.mType].c_str());
            // switch (type) ....
        }
    }
    ImGui::End();
                               
}



/*
 * Segmentation image is reduced from full resolution. It is exanpanded by displaying it in full res here
 */
void movContext::add_motion_profile (){
    
    if (! m_voxel_view_available ) return;
    if (! m_segmented_texture && m_segmented_surface){
        // Create a texcture for display
        Surface8uRef sur = Surface8u::create(cinder::fromOcv(m_segmented_image));
        auto texFormat = gl::Texture2d::Format().loadTopDown();
        m_segmented_texture = gl::Texture::create(*m_segmented_surface, texFormat);
    }
    
    ImGuiWindow* window = ImGui::FindWindowByName(wContractions);
    assert(window != nullptr);
    
    ImVec2  sz (m_segmented_texture->getWidth(),m_segmented_texture->getHeight());
    ImVec2  frame (mMediaInfo.channel_size.width, mMediaInfo.channel_size.height);
    ImVec2 pos (window->Pos.x, window->Pos.y + window->Size.y);
    m_motion_profile_display = Rectf(pos,frame);
    ImGui::SetNextWindowPos(pos);
    ImGui::SetNextWindowContentSize(frame);
    
    if (ImGui::Begin(wShape, nullptr, ImGuiWindowFlags_NoScrollbar  ))
    {
        if(m_segmented_texture){
            static ImVec2 zoom_center;
            // First Child
            ImGui::BeginChild(" ", frame, true,  ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar );
        //    ImVec2 pp = ImGui::GetCursorScreenPos();
       //     ImVec2 p (pp.x + mt.center.x, pp.y  + mt.center.y+128);
            ImGui::Image( (void*)(intptr_t) m_segmented_texture->getId(), frame);
            ImGui::EndChild();
            
            // Second Child
      
        }
    }
    ImGui::End();
}


void  movContext::SetupGUIVariables(){

  //      ImGuiStyle* st = &ImGui::GetStyle();
  //      ImGui::StyleColorsLightGreen(st);

}


void  movContext::DrawGUI(){
    
    add_canvas();
    add_navigation();
    add_result_sequencer();
 //   add_regions(&m_show_cells);
  //  add_contractions(&m_show_contractions);
    add_motion_profile ();
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
    if (! is_valid ()|| ! mSurface ) return;
}

bool movContext::haveTracks()
{
    return ! m_basic_trackWeakRef.expired() && ! m_longterm_pci_trackWeakRef.expired();
}


void movContext::update ()
{
    
    if (! is_valid () || ! mFrameSet || ! mFrameSet->isValid()) return;

    // Update PCI result if ready
    if ( is_ready (m_contraction_pci_tracks_asyn)){
        m_longterm_pci_trackWeakRef = m_contraction_pci_tracks_asyn.get();
    }
    
    if ( ! m_longterm_pci_trackWeakRef.expired())
    {
#ifdef SHORTTERM_ON
        if (m_movProcRef->shortterm_pci().at(0).second.empty())
                m_movProcRef->shortterm_pci(1);
#endif
        auto tracksRef = m_longterm_pci_trackWeakRef.lock();
        if(tracksRef && tracksRef->size() > 0 && ! tracksRef->at(0).second.empty())
            m_result_seq.m_time_data.load(tracksRef->at(0), named_colors["PCI"], channel_count()-1);

#ifdef SHORTTERM_ON
        // Update shortterm PCI result if ready
        if ( ! m_movProcRef->shortterm_pci().at(0).second.empty() )
        {
            auto tracksRef = m_movProcRef->shortterm_pci();
            m_result_seq.m_time_data.load(tracksRef.at(0), named_colors["Short"], 3);
        }
#endif
        
    }

    
    // Fetch Next Frame
    if (is_valid ()){
        mSurface = mFrameSet->getFrame(getCurrentFrame());
        mCurrentIndexTime = mFrameSet->currentIndexTime();
        if (mCurrentIndexTime.first != m_seek_position){
            vlogger::instance().console()->info(tostr(mCurrentIndexTime.first - m_seek_position));
        }
        // Update Fbo with texture.
        {
            lock_guard<mutex> scopedLock(m_update_mutex);
            if(mSurface){
                mFbo->getColorTexture()->update(*mSurface);
                renderToFbo(mSurface, mFbo);
            }
        }
    }
    
    if (m_is_playing )
    {
        update_instant_image_mouse ();
        seekToFrame (getCurrentFrame() + m_playback_speed);
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
    const auto names = m_sequence_player_ref->channel_names();
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
void  movContext::renderToFbo (const SurfaceRef&, gl::FboRef& fbo ){
    // this will restore the old framebuffer binding when we leave this function
    // on non-OpenGL ES platforms, you can just call mFbo->unbindFramebuffer() at the end of the function
    // but this will restore the "screen" FBO on OpenGL ES, and does the right thing on both platforms
    gl::ScopedFramebuffer fbScp( fbo );
    
}

void movContext::draw (){
    if( is_valid ()  && mSurface ){
        
        if(m_showGUI)
            DrawGUI();
    }
}
    //                {
    //                    cinder::gl::ScopedColor col (ColorA( 1, 0.1, 0.1, 0.8f ) );
    //                    gl::drawSolidCircle(length.first, 3.0);
    //                    gl::drawSolidCircle(length.second, 3.0);
    //                    gl::translate (gdr.getUpperLeft() + length.first);
    //                    vec2 ctr (0,0);
    //                    gl::drawLine(ctr, length.second - length.first);
    //
    //                }
    //                {
    //                    cinder::gl::ScopedColor col (ColorA( 0.1, 1.0, 0.1, 0.8f ) );
    //                    gl::drawLine(width.first, width.second);
    //                    gl::drawSolidCircle(width.first, 3.0);
    //                    gl::drawSolidCircle(width.second, 3.0);
    //                }
    
#pragma GCC diagnostic pop
