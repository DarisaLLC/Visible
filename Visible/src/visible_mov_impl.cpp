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
#include "movContext.h"
#include "timed_types.h"
#include "cinder_xchg.hpp"
#include "visible_layout.hpp"
#include "vision/opencv_utils.hpp"
#include "vision/histo.h"
#include "core/stl_utils.hpp"
#include "sm_producer.h"
#include "ssmt.hpp"
#include "core/signaler.h"
#include "logger/logger.hpp"
#include "cinder/Log.h"
#include "CinderImGui.h"
#include <boost/any.hpp>
#include "ImGuiExtensions.h"
#include "Resources.h"
#include "cinder_opencv.h"
#include "imguivariouscontrols.h"



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
std::map<unsigned int, unsigned int> numbered_half_colors = {{0, 0x330000FF },{1, 0x3300FF00},{2, 0x33FF0000} };

struct ScopedWindowWithFlag : public ci::Noncopyable {
    ScopedWindowWithFlag( const char* label, bool* p_open = nullptr, int windowflags = 0 );
    ~ScopedWindowWithFlag();
};

ScopedWindowWithFlag::ScopedWindowWithFlag( const char* label, bool* p_open, int window_flags )
{
    ImGui::Begin( label, p_open, window_flags );
}
ScopedWindowWithFlag::~ScopedWindowWithFlag()
{
    ImGui::End();
}

bool getPosSizeFromWindow(const char* last_window, ImVec2& pos, ImVec2& size){
    ImGuiWindow* window = ImGui::FindWindowByName(last_window);
    if (window == nullptr) return false;
    pos.x = window->Pos.x; pos.y = window->Pos.y + window->Size.y;
    size.x = window->Size.x; size.y = window->Size.y;
    return true;
}

class scopedPause : private Noncopyable {
public:
    scopedPause (const movContextRef& ref) :weakContext(ref) {
        if (!weakContext){
            if (weakContext->isPlaying())
                weakContext->play_pause_button();
        }
    }
    ~scopedPause (){
        if (!weakContext ){
            if (!weakContext->isPlaying()){
                weakContext->play_pause_button();
            }
        }
    }
private:
    std::shared_ptr<movContext>weakContext;
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

movContext::movContext(ci::app::WindowRef& ww, const cvVideoPlayer::ref& grabber, const fs::path& cache_path,  const fs::path& movie_file) : sequencedImageContext(ww), m_grabber_ref(grabber), mPath(movie_file),  mUserStorageDirPath (cache_path) {
    mContentFileName = mPath.stem().string();
    m_type = guiContext::Type::cv_video_viewer;
    // Create an invisible folder for storage
    mCurrentCachePath = mUserStorageDirPath;
    bool folder_exists = fs::exists(mCurrentCachePath);
    std::string msg = folder_exists ? " exists " : " does not exist ";
    msg = " folder for " + m_grabber_ref->name() + msg;
    vlogger::instance().console()->info(msg);
    m_playback_speed = 1;
    m_show_display_and_controls = false;
    m_show_results = false;
    m_show_playback = false;
    m_is_loading = false;
    m_content_loaded = false;
    
    m_valid = m_grabber_ref->isLoaded();
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
    ssmt_processor::params params = ssmt_processor::params (ssmt_processor::params::ContentType::bgra);
    m_movProcRef = std::make_shared<ssmt_processor> (mCurrentCachePath, params);
    
    // Support lifProcessor::content_loaded
    std::function<void (int64_t&)> content_loaded_cb = boost::bind (&movContext::signal_content_loaded, shared_from_above(), _1);
    boost::signals2::connection ml_connection = m_movProcRef->registerCallback(content_loaded_cb);
    
    // Support lifProcessor::flu results available
    std::function<void ()> intensity_over_time_ready_cb = boost::bind (&movContext::signal_intensity_over_time_ready, shared_from_above());
    boost::signals2::connection flu_connection = m_movProcRef->registerCallback(intensity_over_time_ready_cb);
    
    // Support lifProcessor::initial ss results available
    std::function<void (std::vector<float> &, const input_channel_selector_t&)> root_pci_ready_cb = boost::bind (&movContext::signal_root_pci_ready, shared_from_above(), _1, _2);
    boost::signals2::connection nl_connection = m_movProcRef->registerCallback(root_pci_ready_cb);
    
    // Support lifProcessor::median level set ss results available
    std::function<void (const input_channel_selector_t&)> root_pci_med_reg_ready_cb = boost::bind (&movContext::signal_root_pci_med_reg_ready, shared_from_above(), _1);
    boost::signals2::connection ol_connection = m_movProcRef->registerCallback(root_pci_med_reg_ready_cb);
    
    
}

void movContext::setup()
{
    ci::app::WindowRef ww = get_windowRef();
    
    m_showGUI = true;
    m_showLog = true;
    m_showHelp = true;
    
    srand( 133 );
    setup_signals();
    assert(is_valid());
    ww->setTitle( m_grabber_ref->name());
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


void movContext::signal_content_loaded (int64_t& loaded_frame_count )
{
    std::string msg = to_string(mMediaInfo.count) + " Samples in Media  " + to_string(loaded_frame_count) + " Loaded";
    vlogger::instance().console()->info(msg);
    m_is_loading = false;
    m_content_loaded.store(true, std::memory_order_release);
    
}



void movContext::signal_root_pci_ready (std::vector<float> & signal, const input_channel_selector_t& dummy)
{
    auto tracksRef = m_root_pci_trackWeakRef.lock();
     if ( tracksRef && !tracksRef->at(0).second.empty()){
         m_main_seq.m_time_data.load(tracksRef->at(0), named_colors["PCI"], 2);
     }
     
    stringstream ss;
    ss << svl::toString(dummy.region()) << " root self-similarity available ";
    if (tracksRef){
        ss <<  " Valid ";
        ss << tracksRef->at(0).second.size();
    }else ss << " Null " << std::endl;
    vlogger::instance().console()->info(ss.str());
}

void movContext::signal_root_pci_med_reg_ready (const input_channel_selector_t& dummy2)
{
    //@note this is also checked in update. Not sure if this is necessary
    auto tracksRef = m_root_pci_trackWeakRef.lock();
    if ( tracksRef && !tracksRef->at(0).second.empty()){
        m_main_seq.m_time_data.load(tracksRef->at(0), named_colors["PCI"], 2);
    }
    stringstream ss;
    ss << svl::toString(dummy2.region()) << " median regularized root self-similarity available ";
    vlogger::instance().console()->info(ss.str());
}


void movContext::signal_intensity_over_time_ready ()
{
    vlogger::instance().console()->info(" Flu Stats Available ");
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
    bool have = m_grabber_ref && mFrameSet;
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
    return time_spec_t (m_grabber_ref->getCurrentFrameTime());
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
    
    if (! have_sequence () ) return;
    
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
    if( have_sequence () ) {
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
    if ( ! is_valid() || ! m_grabber_ref )
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
        mFrameSet = seqFrameContainer::create (m_grabber_ref);
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
        
        m_layout->init ( mFrameSet->media_info());
        
        m_content_loaded.store(false, std::memory_order_release);
        auto load_thread = std::thread(&ssmt_processor::load_channels_from_video,m_movProcRef.get(),mFrameSet);
        load_thread.detach();
        m_is_loading = true;
        
        /*
         * Fetch length and channel names
         */
      //  mFrameSet->channel_names (m_serie.channel_names());
        reset_entire_clip(mFrameSet->count());
        m_minFrame = 0;
        m_maxFrame =  mFrameSet->count() - 1;
        
        // Initialize The Main Sequencer
        m_main_seq.mFrameMin = 0;
        m_main_seq.mFrameMax = (int) mFrameSet->count() - 1;
        m_main_seq.mSequencerItemTypeNames = mFrameSet->channel_names();
        m_main_seq.items.push_back(timeLineSequence::timeline_item{ 0, 0, (int) mFrameSet->count(), true});
        
        m_title = mContentFileName;
        
        auto ww = get_windowRef();
        ww->setTitle(m_title );
        ww->getApp()->setFrameRate(mMediaInfo.getFramerate());
        
     
        
        mFrameSize = mMediaInfo.getSize();
        
        mSurface = Surface8u::create (int32_t(mFrameSize.x), int32_t(mFrameSize.y), true);
        mFbo = gl::Fbo::create(mSurface->getWidth(), mSurface->getHeight());
        m_show_playback = true;
    }
    catch( const std::exception &ex ) {
        console() << ex.what() << endl;
        m_show_playback = false;
        return;
    }
}


void movContext::fraction_reporter(float f){
    m_fraction_done = f;
    std::cout << (int)(f * 100) << std::endl;
}

void movContext::process_async (){
    
    // @note: ID_LAB  specific. @todo general LIF / TIFF support
    progress_fn_t pf = std::bind(&movContext::fraction_reporter, this, std::placeholders::_1);
    switch(channel_count()){
        case 3:
        {
            input_channel_selector_t in (-1,0);
            m_root_pci_tracks_asyn = std::async(std::launch::async, &ssmt_processor::run_selfsimilarity_on_selected_input, m_movProcRef.get(), in, pf);
            break;
        }
        case 2:
        {
            input_channel_selector_t in (-1,1);
            m_root_pci_tracks_asyn = std::async(std::launch::async, &ssmt_processor::run_selfsimilarity_on_selected_input, m_movProcRef.get(), in, pf);
            break;
        }
        case 1:
        {
            input_channel_selector_t in (-1,0);
            m_root_pci_tracks_asyn = std::async(std::launch::async, &ssmt_processor::run_selfsimilarity_on_selected_input, m_movProcRef.get(), in, pf);
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


void movContext::add_canvas (){
    
    //@note: In ImGui, AddImage is called with uv parameters to specify a vertical flip.
    auto showImage = [&](const char *windowName,bool *open, const gl::Texture2dRef texture){
        if (open && *open)
        {
            //  ImGuiIO& io = ImGui::GetIO();
            // ImGui::SetNextWindowBgAlpha(0.4f); // Transparent background
            ImDrawList* draw_list = ImGui::GetWindowDrawList();
            if (ImGui::Begin(windowName, open)) //, io.ConfigResizeWindowsFromEdges))
            {
                ImVec2 pos = ImGui::GetCursorScreenPos(); // actual position
                draw_list->AddImage(  reinterpret_cast<ImTextureID> (texture->getId()), pos,
                                    ImVec2(ImGui::GetContentRegionAvail().x + pos.x, ImGui::GetContentRegionAvail().y  + pos.y),
                                    ImVec2(0,1), ImVec2(1,0));
                ImVec2 canvas_pos = ImGui::GetCursorScreenPos();            // ImDrawList API uses screen coordinates!
                ImVec2 canvas_size = ImGui::GetContentRegionAvail();        // Resize canvas to what's available
                ImRect regionRect(canvas_pos, canvas_pos + canvas_size);
                // Update image/display coordinate transformer
                m_layout->update_display_rect(canvas_pos, canvas_pos + canvas_size);
            }
            ImGui::End();
        }
    };
    
    //@note: assumes, next image plus any on image graphics have already been added
    // offscreen via FBO
    if(m_show_playback){
        ImVec2 size;
        ImVec2 pos (300,18);
        getPosSizeFromWindow(mContentFileName.c_str(), pos, size);
        size.x = mFrameSize.x+mFrameSize.x/2;
        size.y = mFrameSize.y+mFrameSize.y/2;
        auto ww = get_windowRef();
        
        ScopedWindowWithFlag utilities(wDisplay, nullptr, ImGuiWindowFlags_None );
        ImGui::SetNextWindowPos(pos);
        ImGui::SetNextWindowSize(size);
        ImGui::SameLine();
        showImage(wDisplay, &m_show_playback, mFbo->getColorTexture());
    }
}

void movContext::add_result_sequencer ()
{
    
    // let's create the sequencer
     static int selectedEntry = -1;
     static int64 firstFrame = 0;
     static bool expanded = true;
     ImVec2 pos, size;
     assert(getPosSizeFromWindow(wDisplay, pos, size));
     m_results_browser_display = Rectf(glm::vec2(pos.x,pos.y),glm::vec2(size.x,size.y));
     ImGui::SetNextWindowPos(pos);
     ImGui::SetNextWindowSize(size);

     static bool results_open;
     if(ImGui::Begin(wResult, &results_open, ImGuiWindowFlags_None)){
         Sequencer(&m_main_seq, &m_seek_position, &expanded, &selectedEntry, &firstFrame, ImSequencer::SEQUENCER_EDIT_NONE );
     }
     ImGui::End();
   
}


void movContext::add_navigation(){
    
    
    if(m_show_playback){
        
        ImGuiWindow* window = ImGui::FindWindowByName(wResult);
        assert(window != nullptr);
        ImVec2 pos (window->Pos.x, window->Pos.y + window->Size.y );
        ImVec2 size (window->Size.x, 100);
        
        ScopedWindowWithFlag utilities(wNavigator, nullptr, ImGuiWindowFlags_AlwaysAutoResize);
        m_navigator_display = Rectf(glm::vec2(pos.x,pos.y),glm::vec2(size.x,size.y));
        ImGui::SetNextWindowPos(pos);
        ImGui::SetNextWindowSize(size);
        ImGui::SameLine();
        ImGui::BeginGroup();
        {
            ImGui::PushItemWidth(40);
            ImGui::PushID(200);
            ImGui::InputInt("", &m_main_seq.mFrameMin, 0, 0);
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
            ImGui::InputInt("",  &m_main_seq.mFrameMax, 0, 0);
            ImGui::PopID();
            ImGui::SameLine();
            if (ImGui::Button(m_playback_speed == 10 ? " 1 " : " 10x "))
            {
                auto current = m_playback_speed;
                m_playback_speed = current == 1 ? 10 : 1;
            }
            ImGui::SameLine();
            
            auto dt = getCurrentTime().secs() - getStartTime().secs();
            ImGui::SameLine(0,0);ImGui::Text("% 8d\t%4.4f Seconds", getCurrentFrame(), dt);
            
        }
        ImGui::EndGroup();
    }
}





void  movContext::DrawGUI(){
    add_canvas();
    add_result_sequencer();
    add_navigation();

}

const Rectf& movContext::get_image_display_rect ()
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

//@todo implement
void movContext::resize ()
{
    //    if (! have_sequence () || ! mSurface ) return;
    //    if (! m_layout->isSet()) return;
    //    auto ww = get_windowRef();
    //    auto ws = ww->getSize();
    //    m_layout->update_window_size(ws);
    //    mSize = vec2(ws[0], ws[1] / 12);
    //    //  mMainTimeLineSlider.setBounds (m_layout->display_timeline_rect());
    //
    
}

bool movContext::haveTracks()
{
    return ! m_intensity_trackWeakRef.expired() && ! m_root_pci_trackWeakRef.expired();
}

void movContext::update ()
{
    if (! have_sequence() ) return;
    
    if ( is_ready (m_intensity_tracks_aync) )
        m_intensity_trackWeakRef = m_intensity_tracks_aync.get();
    
    
    auto flu_cnt = channel_count() - 1;
    // Update Fluorescence results if ready
    if (flu_cnt > 0 && ! m_intensity_trackWeakRef.expired())
    {
        // Number of Flu runs is channel count - 1
        auto tracksRef = m_intensity_trackWeakRef.lock();
        for (auto cc = 0; cc < flu_cnt; cc++){
            m_main_seq.m_time_data.load(tracksRef->at(cc), named_colors[tracksRef->at(cc).first], cc);
        }
    }
    
    
    // Update PCI result if ready
    if ( is_ready (m_root_pci_tracks_asyn)){
        m_root_pci_trackWeakRef = m_root_pci_tracks_asyn.get();
    }
    
    if ( ! m_root_pci_trackWeakRef.expired())
    {
        auto tracksRef = m_root_pci_trackWeakRef.lock();
        if(tracksRef && tracksRef->size() > 0 && ! tracksRef->at(0).second.empty())
            m_main_seq.m_time_data.load(tracksRef->at(0), named_colors["PCI"], channel_count()-1);
    }
    
    // Fetch Next Frame
    if (have_sequence ()){
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
    const auto names =m_grabber_ref->getChannelNames();
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

void movContext::draw ()
{
    if( have_sequence()  && mSurface )
    {
        if(m_showGUI)
            DrawGUI();
    }
}


#pragma GCC diagnostic pop
