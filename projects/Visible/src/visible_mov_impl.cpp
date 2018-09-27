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

#include "opencv2/stitching.hpp"
#include <stdio.h>
#include "guiContext.h"
#include "MovContext.h"
#include "cinder/ip/Flip.h"
#include "opencv2/highgui.hpp"
#include "opencv_utils.hpp"


#include <strstream>
#include <future>
#include <mutex>
#include "async_producer.h"
#include "cinder_xchg.hpp"
#include "visible_layout.hpp"

#include "gradient.h"
#include "vision/opencv_utils.hpp"
#include "vision/histo.h"
#include "core/stl_utils.hpp"
#include "sm_producer.h"
#include "hockey_etc_cocoa_wrappers.h"
#include "core/signaler.h"

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
    

    
    static layoutManager vl (ivec2 (10, 10));
    
    
 
    
}

template boost::signals2::connection mov_processor::registerCallback(const std::function<mov_processor::mov_cb_content_loaded>&);
template boost::signals2::connection mov_processor::registerCallback(const std::function<mov_processor::mov_cb_frame_loaded>&);
template boost::signals2::connection mov_processor::registerCallback(const std::function<mov_processor::mov_cb_sm1d_available>&);
template boost::signals2::connection mov_processor::registerCallback(const std::function<mov_processor::mov_cb_sm1dmed_available>&);



/////////////  movContext Implementation  ////////////////

movContext::movContext(WindowRef& ww, const boost::filesystem::path& dp)
: sequencedImageContext(ww), mPath (dp)
{
    m_valid = false;
    m_type = Type::qtime_viewer;
    
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
    
    m_movProcRef = std::make_shared<mov_processor> ();
    
    std::function<void ()> content_loaded_cb = std::bind (&movContext::signal_content_loaded, this);
    boost::signals2::connection ml_connection = m_movProcRef->registerCallback(content_loaded_cb);
    std::function<void (int&)> sm1d_available_cb = boost::bind (&movContext::signal_sm1d_available, this, _1);
    boost::signals2::connection nl_connection = m_movProcRef->registerCallback(sm1d_available_cb);
    std::function<void (int&,int&)> sm1dmed_available_cb = boost::bind (&movContext::signal_sm1dmed_available, this, _1, _2);
    boost::signals2::connection ol_connection = m_movProcRef->registerCallback(sm1dmed_available_cb);
    
    setup ();
}



const  boost::filesystem::path movContext::source_path () const
{
    return mPath;
}



void movContext::looping (bool what)
{
    m_movie->setLoop(what);
    m_looping = what;
}


bool movContext::looping ()
{
    return m_looping;
}

void movContext::play ()
{
    if (! m_movie->isPlaying() )
    {
        m_movie->seekFrame(getCurrentFrame());
        m_movie->play ();
    }
}

void movContext::pause ()
{
    if (m_movie->isPlaying())
        m_movie->pause();
}

void movContext::play_pause_button ()
{
    if (! have_movie () ) return;
    if (m_movie->isPlaying())
        pause ();
    else
        play ();
}

void movContext::loop_no_loop_button ()
{
    if (! have_movie ()) return;
    if (looping())
        looping(false);
    else
        looping(true);
}
bool movContext::have_movie ()
{
    return m_movie != nullptr && m_valid && vl.isSet();
}

void movContext::seekToEnd ()
{
    m_movie->seekFrame(m_movie->getNumFrames()-1);
}

void movContext::seekToStart ()
{
    m_movie->seekFrame(0);
}

int movContext::getNumFrames ()
{
    return m_movie->getNumFrames();
}

time_spec_t movContext::getCurrentTime ()
{
    return m_movie->getElapsedSeconds();
}

int movContext::getCurrentFrame ()
{
    return m_movie->getElapsedFrames();
}

void movContext::seekToFrame (int mark)
{
    m_movie->seekFrame (mark);
    mTimeMarker.from_count(mark);
    m_marker_signal.emit(mTimeMarker);
    
}


void movContext::processDrag( ivec2 pos )
{
    if( mMainTimeLineSlider.hitTest( pos ) ) {
        mTimeMarker.from_norm(mMainTimeLineSlider.valueScaled());
        seekToFrame((int)mTimeMarker.current_frame());
    }
    
}


void movContext::onMarked ( marker_info& t)
{
    seekToFrame(int(t.current_frame()));
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

void movContext::signal_sm1d_available (int& dummy)
{
    static int i = 0;
    std::cout << "sm1d available: " << ++i << std::endl;
}

void movContext::signal_sm1dmed_available (int& dummy, int& dummy2)
{
    static int ii = 0;
    std::cout << "sm1dmed available: " << ++ii << std::endl;
}

void movContext::signal_content_loaded ()
{
    //    movie_loaded = true;
    std::cout << "SM Results Ready " << std::endl;
}
void movContext::signal_frame_loaded (int& findex, double& timestamp)
{
    //    frame_indices.push_back (findex);
    //    frame_times.push_back (timestamp);
    //     std::cout << frame_indices.size() << std::endl;
}

void movContext::setup()
{
    clear_movie_params();

    // Load the validated movie file
    loadMovieFile ();
    

    
    if( m_valid )
    {
       	m_type = Type::qtime_viewer;
        
        
        mButton_title_index = 0;
        string max = to_string( m_movie->getDuration() );
        {
            const std::function<void (int)> setter = std::bind(&movContext::seekToFrame, this, std::placeholders::_1);
            const std::function<int ()> getter = std::bind(&movContext::getCurrentFrame, this);
            mUIParams.addParam ("Current Time Step", setter, getter);
        }
        
        mUIParams.addSeparator();
        mUIParams.addButton("Play / Pause ", bind( &movContext::play_pause_button, this ) );
        mUIParams.addSeparator();
        mUIParams.addButton(" Loop ", bind( &movContext::loop_no_loop_button, this ) );
        
        //        {
        //            const std::function<void (float)> setter = std::bind(&movContext::setZoom, this, std::placeholders::_1);
        //            const std::function<float (void)> getter = std::bind(&movContext::getZoom, this);
        //            mUIParams.addParam( "Zoom", setter, getter);
        //        }
        
    }
}

void movContext::clear_movie_params ()
{
    if (m_movie)
    {
        m_movie->setLoop(false);
        m_movie->stop();
    }
    m_zoom.x = m_zoom.y = 1.0f;
}



void movContext::loadMovieFile()
{
    std::vector<std::string> names = {  "blue", "green", "red"};
    if ( ! mPath.empty () )
    {
        ci_console () << mPath.string ();
        
        m_movie = ocvPlayerRef ( new OcvVideoPlayer );
        if (m_movie->load (mPath.string())) {
//            CI_LOG_V( m_movie->getFilePath() << " loaded successfully: " );
//            
//            CI_LOG_V( " > Codec: "        << m_movie->getCodec() );
//            CI_LOG_V( " > Duration: "    << m_movie->getDuration() );
//            CI_LOG_V( " > FPS: "        << m_movie->getFrameRate() );
//            CI_LOG_V( " > Num frames: "    << m_movie->getNumFrames() );
//            CI_LOG_V( " > Size: "        << m_movie->getSize() );
            
            m_valid = m_movie->isLoaded();

            mFrameSet = seqFrameContainer::create (m_movie);
            mFrameSet->channel_names(names);
            mMediaInfo = mFrameSet->media_info();
            
          //  getWindow()->getApp()->setFrameRate(30.0);
            
            vl.init (movContext::startup_display_size(), mFrameSet->media_info());
            
            if (m_valid)
            {
                getWindow()->setTitle( mPath.filename().string() );
                mUIParams = params::InterfaceGl( "Movie Player", vec2( 260, 260 ) );
                
                // TBD: wrap these into media_info
                mScreenSize = vec2(std::fabs(m_movie->getSize().x), std::fabs(m_movie->getSize().y));
                mSurface = Surface8u::create (int32_t(mScreenSize.x), int32_t(mScreenSize.y), true);
                m_frameCount = m_movie->getNumFrames ();
                mTimeMarker = marker_info (m_movie->getNumFrames (), m_movie->getDuration());
                
                vl.normSinglePlotSize (vec2 (0.25, 0.1));
                
                ivec2 window_size (vl.desired_window_size());
                setWindowSize(window_size);
                
                // Setup Plot area
                {
                    std::lock_guard<std::mutex> lock(m_track_mutex);
                    
                    m_plots.resize (0);
                    
                    for (graph1d::ref gr : m_plots)
                    {
                        m_marker_signal.connect(std::bind(&graph1d::set_marker_position, gr, std::placeholders::_1));
                    }
                    
                    for (int cc = 0; cc < names.size() ; cc++)
                    {
                        m_plots.push_back( graph1d::ref (new graph1d (names[cc], vl.plot_rects() [cc])));
                    }
                
                    
                    for (graph1d::ref gr : m_plots)
                    {
                        m_marker_signal.connect(std::bind(&graph1d::set_marker_position, gr, std::placeholders::_1));
                    }
                    
                    mMainTimeLineSlider.setBounds (vl.display_timeline_rect());
                    mMainTimeLineSlider.setTitle ("Time Line");
                    m_marker_signal.connect(std::bind(&tinyUi::TimeLineSlider::set_marker_position, mMainTimeLineSlider, std::placeholders::_1));
                    mWidgets.push_back( &mMainTimeLineSlider );
                    
                    getWindow()->getSignalMouseDrag().connect( [this] ( MouseEvent &event ) { processDrag( event.getPos() ); } );
                    
                    play ();
                }

                
                // Launch Average Luminance Computation
               // m_async_luminance_tracks = std::async(std::launch::async, &mov_processor::run, m_movProcRef.get(),
                  //                                    mFrameSet, names, false);
                
                ci_console() << "Dimensions:" <<m_movie->getSize() << std::endl;
                ci_console() << "Duration:  " <<m_movie->getDuration() << " seconds" << std::endl;
                ci_console() << "Frames:    " <<m_movie->getNumFrames() << std::endl;
                ci_console() << "Framerate: " <<m_movie->getFrameRate() << std::endl;
                
            } else {
                ci_console() << "Unable to load movie" << std::endl;
                return;
            }
        }
    }
}


void movContext::mouseMove( MouseEvent event )
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
        mMouseInGraphs = int(min_iter - dds.begin());
    }
    
}


void movContext::mouseDrag( MouseEvent event )
{
    mMouseIsDragging = true;
    for (graph1d::ref graphRef : m_plots)
        graphRef->mouseDrag( event );
}


void movContext::mouseDown( MouseEvent event )
{
    mMouseIsDown = true;
    for (graph1d::ref graphRef : m_plots )
    {
        graphRef->mouseDown( event );
        graphRef->get_marker_position(mTimeMarker);
        signalMarker.emit(mTimeMarker);
    }
}


void movContext::mouseUp( MouseEvent event )
{
    mMouseIsDown = false;
    mMouseIsDragging = false;
    for (graph1d::ref graphRef : m_plots)
        graphRef->mouseUp( event );
}




void movContext::keyDown( KeyEvent event )
{
    if( event.getChar() == 'f' ) {
        setFullScreen( ! isFullScreen() );
    }
    else if( event.getChar() == 'o' ) {
        loadMovieFile();
    }
    
    // these keys only make sense if there is an active movie
    if( m_movie ) {
        if( event.getCode() == KeyEvent::KEY_LEFT ) {
            auto cf = getCurrentFrame() - 1;
            m_movie->seekFrame(cf < 0 ? 0 : cf >= getNumFrames() ? getNumFrames() - 1 : cf);
        }
        if( event.getCode() == KeyEvent::KEY_RIGHT ) {
            auto cf = getCurrentFrame() + 1;
            m_movie->seekFrame(cf < 0 ? 0 : cf >= getNumFrames() ? getNumFrames() - 1 : cf);
        }
        
        if( event.getChar() == ' ' ) {
            play_pause_button();
        }
        
    }
}

void movContext::update_instant_image_mouse ()
{
    auto image_pos = vl.display2image(mMouseInImagePosition);
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
  
    
    std::string pos =  "[ " +
    svl::toString(((int)m_instant_pixel_Color.r)) + " , " + svl::toString(((int)m_instant_pixel_Color.g)) +
    " , " + svl::toString(((int)m_instant_pixel_Color.b)) + " @ " + svl::toString(imagePos().x) + "," + svl::toString(imagePos().y) + "]";
    vec2                mSize(250, 100);
    TextBox tbox = TextBox().alignment( TextBox::LEFT).font( mFont ).size( ivec2( mSize.x, TextBox::GROW ) ).text( pos );
    tbox.setFont( Font( "Times New Roman", 24 ) );
    tbox.setColor( ColorA(0.0,0.0,0.0,1.0) );
    tbox.setBackgroundColor( ColorA( 0.3, 0.3, 0.3, 0.3 ) );
    __attribute__((unused)) ivec2 sz = tbox.measure();
    return  gl::Texture2d::create( tbox.render() );
}




bool movContext::is_valid () const { return m_valid && is_context_type(guiContext::qtime_viewer); }

void movContext::resize ()
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
void movContext::update ()
{
    if (! have_movie () ) return;
    mContainer.update();
    vl.update_window_size(getWindowSize ());
 
#if 0
    if ( is_ready (m_async_luminance_tracks))
    {
        auto tracksRef = m_async_luminance_tracks.get();
        assert (tracksRef->size() == m_plots.size ());
 //       m_plots[0]->setup(tracksRef->at(0));
//        m_plots[1]->setup(tracksRef->at(1));
//        m_plots[2]->setup(tracksRef->at(2));
//        if (!tracksRef->at(2).second.empty())
//            m_plots[2]->setup(tracksRef->at(2), graph1D::mapping_option::type_limits);
    }
#endif
    if (! have_movie () ) return;
    
    if ( m_movie->update() ) {
        
        time_spec_t new_time = m_movie->getElapsedSeconds();
        new_time += m_movie->getDuration() * m_movie->getFrameRate() /    getWindow()->getApp()->getFrameRate();
        if (mFrameSet->checkFrame(new_time))
            mSurface = mFrameSet->getFrame(new_time);
        else
        {
            mSurface = m_movie->createSurface();
            mFrameSet->loadFrame(mSurface, new_time);
        }
    }
    
}

void movContext::draw ()
{
    
    if( have_movie()  &&  mSurface )
    {
    
    Rectf dr = get_image_display_rect();
    mImage = gl::Texture::create(*mSurface);
    mImage->setMagFilter(GL_NEAREST_MIPMAP_NEAREST);
    gl::draw (mImage, dr);
    draw_info ();
    for(graph1d::ref gg : m_plots)
        gg->draw ();
    }
    mUIParams.draw();
}



////////   Info Log

void  movContext::update_log (const std::string& message)
{
    
    std::strstream msg;
    msg << " Looping: " << (looping() ? " On" : "Off");
    msg << " Playing: " << (m_movie->isPlaying() ? "Yes" : " No");
    msg << " | - F  " << setw(8) << getCurrentFrame () << std::endl;
    mLog = msg.str() + message;
    
    TextBox tbox = TextBox().alignment( TextBox::RIGHT).font( mFont ).size( mSize ).text( mLog );
    tbox.setColor( Color( 1.0f, 0.65f, 0.35f ) );
    tbox.setBackgroundColor( ColorA( 0.3f, 0.3f, 0.3f, 0.4f )  );
    tbox.measure();
    mTextTexture = gl::Texture2d::create( tbox.render() );
}

Rectf movContext::get_image_display_rect ()
{
    return vl.display_frame_rect();
    
}


void movContext::draw_info ()
{
    if (! have_movie () ) return;
    
    
    std::string seri_str = mPath.filename().string();
    
    gl::setMatricesWindow( getWindowSize() );
    
    gl::ScopedBlendAlpha blend_;
    
    {
        gl::ScopedColor (ColorA::gray(1.0));
        gl::drawStrokedRect(get_image_display_rect(), 3.0f);
    }
    {
        gl::ScopedColor (ColorA::gray(0.0));
        gl::drawStrokedRect(vl.display_timeline_rect(), 3.0f);
    }
    {
        gl::ScopedColor (ColorA::gray(0.75));
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


#pragma GCC diagnostic pop
