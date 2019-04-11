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
        {"green", 0xFF00FF00},{ "PCI", 0xFFFF0000}, { "pci", 0xFFFF0000}, { "Short", 0xFFD66D3E}, { "short", 0xFFD66D3E}};
    
    
}



                    /************************
                     *
                     *  Setup & Load File
                     *
                     ************************/



    /////////////  lifContext Implementation  ////////////////
// @ todo setup default repository
// default median cover pct is 5% (0.05)

lifContext::lifContext(ci::app::WindowRef& ww, const lif_serie_data& sd, const fs::path& cache_path) :sequencedImageContext(ww), m_serie(sd), m_geometry_available(false), mUserStorageDirPath (cache_path) {
    m_type = guiContext::Type::lif_file_viewer;
    m_show_contractions = false;
    // Create an invisible folder for storage
    mCurrentSerieCachePath = mUserStorageDirPath / m_serie.name();
    bool folder_exists = fs::exists(mCurrentSerieCachePath);
    std::string msg = folder_exists ? " exists " : " does not exist ";
    msg = " folder for " + m_serie.name() + msg;
    vlogger::instance().console()->info(msg);
    m_idlab_defaults.median_level_set_cutoff_fraction = 7;
    
    
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
    m_lifProcRef = std::make_shared<lif_serie_processor> (mCurrentSerieCachePath);
    
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
    
    // Support lifProcessor::geometry_available
    std::function<void ()> geometry_available_cb = boost::bind (&lifContext::signal_geometry_available, shared_from_above());
    boost::signals2::connection geometry_connection = m_lifProcRef->registerCallback(geometry_available_cb);
    
    // Support lifProcessor::temporal_image_available
    std::function<void (cv::Mat&)> ss_image_available_cb = boost::bind (&lifContext::signal_ss_image_available, shared_from_above(), _1);
    boost::signals2::connection ss_image_connection = m_lifProcRef->registerCallback(ss_image_available_cb);
    
}

void lifContext::setup_params () {
    
//    gl::enableVerticalSync();
 
    
}


void lifContext::setup()
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
    ww->setTitle( m_serie.name());
    ww->setSize(1280, 768);
    mFont = Font( "Menlo", 18 );
    auto ws = ww->getSize();
    mSize = vec2( ws[0], ws[1] / 12);
    m_contraction_names = m_contraction_none;
    clear_playback_params();
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
        auto tracksRef = m_contraction_pci_trackWeakRef.lock();
        if (!tracksRef->at(0).second.empty()){
         //   m_plots[channel_count()-1]->load(tracksRef->at(0));
            m_result_seq.m_editable_plot_data.load(tracksRef->at(0), named_colors["PCI"], 2);
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
    
    
    // @note: We are handling one contraction for now.
    m_contractions = contras;
    reset_entire_clip(mMediaInfo.count);
    uint32_t index = 0;
    string name = " C " + svl::toString(index) + " ";
    m_contraction_names.push_back(name);
    m_clips.emplace_back(m_contractions[0].contraction_start.first,
                         m_contractions[0].relaxation_end.first,
                         m_contractions[0].contraction_peak.first);

    update_contraction_selection();
    
}


void lifContext::signal_geometry_available()
{
    vlogger::instance().console()->info(" Volume Variance are available ");
    if (! m_lifProcRef){
        vlogger::instance().console()->error("Lif Processor Object does not exist ");
        return;
    }
  
}

void lifContext::signal_ss_image_available(cv::Mat& image)
{
    vlogger::instance().console()->info(" SS Image Available  ");
    if (! m_lifProcRef){
        vlogger::instance().console()->error("Lif Processor Object does not exist ");
        return;
    }
    m_segmented_image = image.clone();
    m_geometry_available = true;
    
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
}


bool lifContext::looping ()
{
    return m_is_looping;
}

void lifContext::play ()
{
    if (! have_lif_serie() || m_is_playing ) return;
    m_is_playing = true;
}

void lifContext::pause ()
{
    if (! have_lif_serie() || ! m_is_playing ) return;
    m_is_playing = false;
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



void lifContext::update_contraction_selection()
{
    m_show_contractions = true;
    m_result_seq.myItems.resize(1);
    m_result_seq.myItems.push_back(timeLineSequence::timeline_item{ 0,
        (int) m_contractions[0].contraction_start.first,
        (int) m_contractions[0].relaxation_end.first , true});


}

void lifContext::add_contractions (bool* p_open)
{
    Rectf dr = get_image_display_rect();
    auto pos = ImVec2(dr.getLowerLeft().x, 150 + getWindowHeight()/2.0);
    ImGui::SetNextWindowPos(pos);
    ImGui::SetNextWindowSize(ImVec2(dr.getWidth(), 340), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Contractions", p_open, ImGuiWindowFlags_MenuBar))
    {
        if (ImGui::BeginMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("Close")) *p_open = false;
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }
        
        // left
        static int selected = 0;
        ImGui::BeginChild("left pane", ImVec2(150, 0), true);
        for (int i = 0; i < m_contraction_names.size(); i++)
        {
            char label[128];
            sprintf(label, "Contraction %d", i);
            if (ImGui::Selectable(label, selected == i))
                selected = i;
        }
        ImGui::EndChild();
        ImGui::SameLine();
        
        // right
        ImGui::BeginGroup();
        ImGui::BeginChild("item view", ImVec2(0, -ImGui::GetFrameHeightWithSpacing())); // Leave room for 1 line below us
        ImGui::Text("Contraction: %d", selected);
        ImGui::Separator();
        if (ImGui::BeginTabBar("##Tabs", ImGuiTabBarFlags_None))
        {
            if (ImGui::BeginTabItem("Description"))
            {
                set_current_clip_index(selected);
                std::string msg = " Entire ";
                if (selected != 0){
                const contraction_analyzer::contraction_t& se = m_contractions[get_current_clip_index() - 1];
                auto ctr_info = " Contraction: [" + to_string(se.contraction_start.first) + "," + to_string(se.relaxation_end.first) + "]";
                msg = " Current Clip " + m_contraction_names[get_current_clip_index()] + " " + ctr_info;
                }
                ImGui::TextWrapped("%s", msg.c_str());
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Details"))
            {
                ImGui::Text("ID: 0123456789");
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
        ImGui::EndChild();
        if (ImGui::Button("Save")) {}
        ImGui::EndGroup();
    }
    ImGui::End();
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
//    for (Widget* wPtr : mWidgets)
//    {
//        if( wPtr->hitTest( pos ) ) {
//            mTimeMarker.from_norm(wPtr->valueScaled());
//            seekToFrame((int) mTimeMarker.current_frame());
//        }
//    }
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
    
  
}


void lifContext::mouseDrag( MouseEvent event )
{
  
}


void lifContext::mouseDown( MouseEvent event )
{
   
}


void lifContext::mouseUp( MouseEvent event )
{

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
        /*
         * Clear and pause if we are playing back
         */
        m_clips.clear();
        setCellLength(0);
        pause();
        
        /*
         * Create the frameset and assign the channel names
         * Fetch the media info
         */
        mFrameSet = seqFrameContainer::create (*m_cur_lif_serie_ref);
        if (! mFrameSet || ! mFrameSet->isValid())
        {
            vlogger::instance().console()->debug("Serie had 1 or no frames ");
            return;
        }
        mMediaInfo = mFrameSet->media_info();
        mChannelCount = (uint32_t) mMediaInfo.getNumChannels();
        if (!(mChannelCount > 0 && mChannelCount < 4)){
            vlogger::instance().console()->debug("Expected 1 or 2 or 3 channels ");
            return;
        }

        m_layout->init (getWindowSize() , mFrameSet->media_info(), channel_count());

        /*
         * Create the frameset and assign the channel namesStart Loading Images on async on a different thread
         * Loading also produces voxel images.
         */
        m_plot_names.clear();
        m_plot_names = m_serie.channel_names();
        m_plot_names.push_back("MisRegister");
        cv::Mat result;
        std::vector<std::thread> threads(1);
        threads[0] = std::thread(&lif_serie_processor::load, m_lifProcRef.get(), mFrameSet, m_serie.channel_names(), m_plot_names);
        std::for_each(threads.begin(), threads.end(), std::mem_fn(&std::thread::join));

        /*
         * Fetch length and channel names
         */
        mFrameSet->channel_names (m_serie.channel_names());
        reset_entire_clip(mFrameSet->count());
        m_minFrame = 0;
        m_maxFrame =  mFrameSet->count() - 1;
        
        // Initialize Sequencer
        m_result_seq.mFrameMin = 0;
        m_result_seq.mFrameMax = (int) mFrameSet->count() - 1;
        m_result_seq.mSequencerItemTypeNames = {"All", "Contraction", "Force"};
        m_result_seq.myItems.push_back(timeLineSequence::timeline_item{ 0, 0, (int) mFrameSet->count(), true});
        
        m_title = m_serie.name() + " @ " + mPath.filename().string();
        
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

void lifContext::process_async (){
    
    // @note: ID_LAB  specific. @todo general LIF / TIFF support
    switch(channel_count()){
        case 3:
        {
            // note launch mode is std::launch::async
            m_fluorescense_tracks_aync = std::async(std::launch::async,&lif_serie_processor::run_flu_statistics,
                                                  m_lifProcRef.get(), std::vector<int> ({0,1}) );
            m_contraction_pci_tracks_asyn = std::async(std::launch::async, &lif_serie_processor::run_contraction_pci_on_channel, m_lifProcRef.get(), 2);
            break;
        }
        case 1:
        {
            m_contraction_pci_tracks_asyn = std::async(std::launch::async, &lif_serie_processor::run_contraction_pci_on_channel,m_lifProcRef.get(), 0);
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

void lifContext::add_result_sequencer ()
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

void lifContext::add_navigation(){
   
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
        
        if(!m_contraction_pci_trackWeakRef.expired()){
            if(m_median_set_at_default){
                int default_median_cover_pct = m_idlab_defaults.median_level_set_cutoff_fraction;
                setMedianCutOff(default_median_cover_pct);
                ImGui::SliderInt("Median Cover Percent", &default_median_cover_pct, default_median_cover_pct, default_median_cover_pct);
            }
            else{
                int default_median_cover_pct = getMedianCutOff();
                ImGui::SliderInt("Median Cover Percent", &default_median_cover_pct, 0, 20);
                setMedianCutOff(default_median_cover_pct);
            }
        }
        ImGui::SameLine();
        if (ImGui::Button(m_median_set_at_default ? "Default" : "Not Set"))
        {
            m_median_set_at_default = ! m_median_set_at_default;
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
    
    const RotatedRect& mt = m_lifProcRef->motion_surface_bottom();
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

void  lifContext::SetupGUIVariables(){

  //      ImGuiStyle* st = &ImGui::GetStyle();
  //      ImGui::StyleColorsLightGreen(st);

}


void  lifContext::DrawGUI(){
    
    add_navigation();
    add_result_sequencer();
    add_motion_profile ();
    add_contractions(&m_show_contractions);
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
    return ! m_flurescence_trackWeakRef.expired() && ! m_contraction_pci_trackWeakRef.expired();
}

void lifContext::update ()
{
    ci::app::WindowRef ww = get_windowRef();
    

    
    ww->getRenderer()->makeCurrentContext(true);
    if (! have_lif_serie() ) return;
    auto ws = ww->getSize();
    m_layout->update_window_size(ws);
    
    // If Plots are ready, set them up It is ready only for new data
    // @todo replace with signal
    //@todo switch to using weak_ptr all together
    if ( is_ready (m_fluorescense_tracks_aync) )
        m_flurescence_trackWeakRef = m_fluorescense_tracks_aync.get();
    
    if ( is_ready (m_contraction_pci_tracks_asyn)){
        m_contraction_pci_trackWeakRef = m_contraction_pci_tracks_asyn.get();
    }
    

    // Update Fluorescence results if ready
    if (! m_flurescence_trackWeakRef.expired())
    {
        assert(channel_count() >= 3);
        auto tracksRef = m_flurescence_trackWeakRef.lock();
        m_result_seq.m_editable_plot_data.load(tracksRef->at(0), named_colors[tracksRef->at(0).first], 0);
        m_result_seq.m_editable_plot_data.load(tracksRef->at(1), named_colors[tracksRef->at(1).first], 1);
    }

    // Update PCI result if ready
    if ( ! m_contraction_pci_trackWeakRef.expired())
    {
#ifdef SHORTTERM_ON
     //  if (m_lifProcRef->shortterm_pci().at(0).second.empty())
      //      m_lifProcRef->shortterm_pci(1);
#endif
        auto tracksRef = m_contraction_pci_trackWeakRef.lock();
        m_result_seq.m_editable_plot_data.load(tracksRef->at(0), named_colors["PCI"], 2);


        // Update shortterm PCI result if ready
        if ( ! m_lifProcRef->shortterm_pci().at(0).second.empty() )
        {
            auto tracksRef = m_lifProcRef->shortterm_pci();
            m_result_seq.m_editable_plot_data.load(tracksRef.at(0), named_colors["Short"], 3);
        }
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


void lifContext::draw ()
{
    if( have_lif_serie()  && mSurface )
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
            const cv::RotatedRect& ellipse = m_lifProcRef->motion_surface();
            const fPair& ab = m_lifProcRef->ellipse_ab();
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
        

        if(m_showGUI)
            DrawGUI();
        
    }
    
    
}

#pragma GCC diagnostic pop
