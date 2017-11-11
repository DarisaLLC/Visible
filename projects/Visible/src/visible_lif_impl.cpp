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
        mFont = Font( "Menlo", 12 );
        mSize = vec2( getWindowWidth(), getWindowHeight() / 12);
    }
    
    m_lifProcRef = std::make_shared<lif_processor> ();
    
    std::function<void ()> content_loaded_cb = std::bind (&lifContext::signal_content_loaded, this);
    boost::signals2::connection ml_connection = m_lifProcRef->registerCallback(content_loaded_cb);
    std::function<void (int&)> sm1d_available_cb = boost::bind (&lifContext::signal_sm1d_available, this, _1);
    boost::signals2::connection nl_connection = m_lifProcRef->registerCallback(sm1d_available_cb);
    std::function<void (int&,int&)> sm1dmed_available_cb = boost::bind (&lifContext::signal_sm1dmed_available, this, _1, _2);
    boost::signals2::connection ol_connection = m_lifProcRef->registerCallback(sm1dmed_available_cb);
    
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
        if (mContainer.getNumChildren())
            mContainer.removeChildren();
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

void lifContext::setMedianCutOff (uint32_t newco)
{
    uint32_t tmp = newco % 100; // pct
    auto sp =  m_lifProcRef->smFilterRef();
    if (! sp ) return;
    uint32_t current (sp->get_median_levelset_pct () * 100);
    if (tmp == current) return;
    sp->set_median_levelset_pct (tmp / 100.0f);
    m_lifProcRef->update();
    auto tracksRef = m_lifProcRef->similarity_track();
    m_plots[2]->setup(tracksRef);
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
        
        {
            // Add a param with no target, but instead provide setter and getter functions.
            const std::function<void(uint32_t)> setter	= std::bind(&lifContext::setMedianCutOff, this, std::placeholders::_1 );
            const std::function<uint32_t()> getter	= std::bind( &lifContext::getMedianCutOff, this);
            
            // Attach a callback that is fired after a target is updated.
            mUIParams.addParam( "CutOff Pct", setter, getter );
        }
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

void lifContext::signal_sm1d_available (int& dummy)
{
    static int i = 0;
    std::cout << "sm1d available: " << ++i << std::endl;
}

void lifContext::signal_sm1dmed_available (int& dummy, int& dummy2)
{
    static int ii = 0;
    std::cout << "sm1dmed available: " << ++ii << std::endl;
}

void lifContext::signal_content_loaded ()
{
//    movie_loaded = true;
    std::cout << "SM Results Ready " << std::endl;
}
void lifContext::signal_frame_loaded (int& findex, double& timestamp)
{
//    frame_indices.push_back (findex);
//    frame_times.push_back (timestamp);
//     std::cout << frame_indices.size() << std::endl;
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
        
        mFrameSet->channel_names (m_series_book[m_selected_serie_index].channel_names);
        mMediaInfo = mFrameSet->media_info();
        
        vl.init (app::getWindow(), mFrameSet->media_info());
        
        if (m_valid)
        {
            getWindow()->setTitle( mPath.filename().string() );
            
            ci_console() <<  m_series_book.size() << "  Series  " << std::endl;
            
            const tiny_media_info tm = mFrameSet->media_info ();
            getWindow()->getApp()->setFrameRate(tm.getFramerate() * 2);
            
            mScreenSize = tm.getSize();
            m_frameCount = tm.getNumFrames ();
            mTimeMarker = marker_info (tm.getNumFrames (), tm.getDuration());
            mAuxTimeMarker = marker_info (tm.getNumFrames (), tm.getDuration());
            
            
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
                
                assert (  vl.plot_rects().size() >= channel_count);
                
                m_plots.resize (0);
                
                for (int cc = 0; cc < channel_count; cc++)
                {
                    m_plots.push_back( Graph1DRef (new graph1D (m_current_serie_ref->getChannels()[cc].getName(),
                                                                 vl.plot_rects() [cc])));
                }
                
                for (Graph1DRef gr : m_plots)
                {
                    m_marker_signal.connect(std::bind(&graph1D::set_marker_position, gr, std::placeholders::_1));
                }
                
                mTimeLineSlider.setBounds (vl.display_timeline_rect());
                mTimeLineSlider.clear_timepoint_markers();
                mTimeLineSlider.setTitle ("Time Line");
                m_marker_signal.connect(std::bind(&tinyUi::TimeLineSlider::set_marker_position, mTimeLineSlider, std::placeholders::_1));
                mWidgets.push_back( &mTimeLineSlider );
                

                mAuxTimeSliderIndex = vl.add_slider_rect ();
                mAuxTimeLineSlider.clear_timepoint_markers();
                mAuxTimeLineSlider.setTitle("Contraction Markers");
                mAuxTimeLineSlider.setBounds(vl.slider_rects()[mAuxTimeSliderIndex]);
                m_aux_marker_signal.connect(std::bind(&tinyUi::TimeLineSlider::set_marker_position, mAuxTimeLineSlider, std::placeholders::_1));
                mWidgets.push_back( &mAuxTimeLineSlider );
           
                getWindow()->getSignalMouseDrag().connect( [this] ( MouseEvent &event ) { processDrag( event.getPos() ); } );
                
            }
            
            // Launch Average Luminance Computation
            m_async_luminance_tracks = std::async(std::launch::async, &lif_processor::run, m_lifProcRef.get(),
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
        mTimeMarker.from_norm(mTimeLineSlider.valueScaled());
        seekToFrame(mTimeMarker.current_frame());
    }
    
    if( mAuxTimeLineSlider.hitTest( pos ) ) {
        mAuxTimeMarker.from_norm(mAuxTimeLineSlider.valueScaled());
        seekToFrame(mAuxTimeMarker.current_frame());
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
    
    mMouseInTimeLine =  mTimeLineSlider.contains(event.getPos());
    mAuxMouseInTimeLine =  mAuxTimeLineSlider.contains(event.getPos());
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
        else if( event.getChar() == 'l' ) {
            loop_no_loop_button();
        }
        
        else if( event.getChar() == ' ' ) {
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
    tbox.measure();
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

    for (int cc = 0; cc < vl.plot_rects().size(); cc++)
    {
        m_plots[cc]->setRect (vl.plot_rects()[cc]);
    }
    
    mTimeLineSlider.setBounds (vl.display_timeline_rect());
    mAuxTimeLineSlider.setBounds(vl.slider_rects()[mAuxTimeSliderIndex]);
}
void lifContext::update ()
{
  if (! have_movie () ) return;
  mContainer.update();
  vl.update_window_size(getWindowSize ());
    
  if ( is_ready (m_async_luminance_tracks))
    {
        auto tracksRef = m_async_luminance_tracks.get();
        assert (tracksRef->size() == m_plots.size ());
        m_plots[0]->setup(tracksRef->at(0));
        m_plots[1]->setup(tracksRef->at(1));
        if (!tracksRef->at(2).second.empty())
            m_plots[2]->setup(tracksRef->at(2), graph1D::mapping_option::type_limits);
    }
    

    
    if (getCurrentFrame() >= getNumFrames())
    {
        if (! looping () ) pause ();
        else
            seekToStart();
    }
    
    mSurface = mFrameSet->getFrame(getCurrentFrame());
    
    if (m_is_playing ) seekToFrame (getCurrentFrame() + 1);
    
    // TBD: this is inefficient as it continuously clears and updates contraction peak and time markers
    // should be signal driven.
    
    mAuxTimeLineSlider.clear_timepoint_markers();
    if (m_lifProcRef && m_lifProcRef->smFilterRef() && ! m_lifProcRef->smFilterRef()->low_peaks().empty())
    {
        for(auto lowp : m_lifProcRef->smFilterRef()->low_peaks())
        {
            tinyUi::timepoint_marker_t tm;
            tm.first = float(lowp.first) / m_lifProcRef->smFilterRef()->size();
            tm.second = ColorA (0.9, 0.3, 0.1, 0.75);
            mAuxTimeLineSlider.add_timepoint_marker(tm);
        }
    }
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
    const auto names = m_series_book[m_selected_serie_index].channel_names;
    std::string channel_name = " ";
    if(m_instant_channel < names.size())
        channel_name = names[m_instant_channel];
    
    // LIF has 3 Channels.
    uint32_t channel_height = mMediaInfo.getChannelSize().y;
    std::string pos = channel_name + "[ " +
    toString(((int)m_instant_pixel_Color.g)) + " @ " +
    to_string(imagePos().x) + "," + to_string(imagePos().y % channel_height) + "]";
    vec2                mSize(250, 100);
    TextBox tbox = TextBox().alignment( TextBox::LEFT).font( mFont ).size( ivec2( mSize.x, TextBox::GROW ) ).text( pos );
    tbox.setFont( Font( "Times New Roman", 24 ) );
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
    
    
    auto texR = pixelInfoTexture ();
    if (texR)
    {
        Rectf tbox = texR->getBounds();
        tbox.offset(mMouseInImagePosition);
        gl::draw( texR, tbox);
    }
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

