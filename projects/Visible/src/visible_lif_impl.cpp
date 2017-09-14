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
    
    
    class algo_processor : public internal_singleton <algo_processor>
    {
    public:
        algo_processor ()
        {
            m_sm = std::shared_ptr<sm_producer> ( new sm_producer () );
        }

        const smProducerRef sm () const { return m_sm; }
        std::shared_ptr<sm_filter> smFilterRef () { return m_smfilterRef; }
        
        
    private:

        
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

        // Note tracks contained timed data. 
        void entropiesToTracks (trackD1_t& track)
        {
            m_medianLevel.resize(m_smfilterRef->size());
            m_smfilterRef->median_levelset_similarities(m_medianLevel);
            auto bee = m_smfilterRef->entropies().begin();
            auto mee = m_medianLevel.begin();
            for (auto ss = 0; bee != m_smfilterRef->entropies().end() && ss < frame_count(); ss++, bee++, mee++)
            {
                index_time_t ti;
                ti.first = ss;
                timed_double_t res;
                res.first = ti;
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
          std::shared_ptr<vector_of_trackD1s_t>  run (const std::shared_ptr<qTimeFrameCache>& frames, const std::vector<std::string>& names,
                        bool test_data = false)
        {
            
            load_channels_from_images(frames);
            
            m_tracksRef = std::make_shared<vector_of_trackD1s_t> ();
            
            m_tracksRef->resize (names.size ());
            for (auto tt = 0; tt < names.size(); tt++)
                m_tracksRef->at(tt).first = names[tt];
            
            // Run Histogram on channels 0 and 1
            // Filling up tracks 0 and 1
            // Now Do Aci on the 3rd channel
            channel_images_t c0 = m_channel_images[0];
            channel_images_t c1 = m_channel_images[1];
            channel_images_t c2 = m_channel_images[2];
            
            for (auto ii = 0; ii < m_channel_images[0].size(); ii++)
            {
                m_tracksRef->at(0).second.emplace_back(computeIntensityStatisticsResults(c0[ii]));
                m_tracksRef->at(1).second.emplace_back(computeIntensityStatisticsResults(c1[ii]));
            }
            
            auto sp =  instance().sm();
            sp->load_images (c2);
            std::packaged_task<bool()> task([sp](){ return sp->operator()(0, 0);}); // wrap the function
            std::future<bool>  future_ss = task.get_future();  // get a future
            std::thread(std::move(task)).detach(); // launch on a thread
            future_ss.wait();
            auto entropies = sp->shannonProjection ();
            auto smat = sp->similarityMatrix();
            m_smfilterRef = std::make_shared<sm_filter> (entropies, smat);
            update();
            return m_tracksRef;
        }
        
        const std::vector<Rectf>& rois () const { return m_rois; }
        const trackD1_t& similarity_track () const { return m_tracksRef->at(2); }
        
        void update ()
        {
            if(m_tracksRef && !m_tracksRef->empty() && m_smfilterRef && m_smfilterRef->isValid())
                entropiesToTracks(m_tracksRef->at(2));
        }

    private:
        typedef std::vector<roiWindow<P8U>> channel_images_t;
        smProducerRef m_sm;
        channel_images_t m_images;
        std::vector<channel_images_t> m_channel_images;
        std::vector<Rectf> m_rois;
        Rectf m_all;
        std::shared_ptr<sm_filter> m_smfilterRef;
        std::deque<double> m_medianLevel;
        std::shared_ptr<vector_of_trackD1s_t>  m_tracksRef;
    };
    
    std::shared_ptr<vector_of_trackD1s_t>   get_mean_luminance_and_aci (const std::shared_ptr<qTimeFrameCache>& frames, const std::vector<std::string>& names,
                                               bool test_data = false)
    {
        return algo_processor().get_mutable_instance().run(frames, names, test_data);
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
    auto sp =  algo_processor::get_mutable_instance().smFilterRef();
    uint32_t current (sp->get_median_levelset_pct () * 100);
    if (tmp == current) return;
    sp->set_median_levelset_pct (tmp / 100.0f);
    algo_processor::get_mutable_instance().update();
}

uint32_t lifContext::getMedianCutOff () const
{
    auto sp =  algo_processor::get_mutable_instance().smFilterRef();
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
                
                m_plots.resize (0);
                
                for (int cc = 0; cc < channel_count; cc++)
                {
                    m_plots.push_back( Graph1DRef (new graph1D (m_current_serie_ref->getChannels()[cc].getName(),
                                                                 m_track_rects [cc])));
                }
                
                for (Graph1DRef gr : m_plots)
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
        std::vector<float> dds (m_track_rects.size());
        for (auto pp = 0; pp < m_track_rects.size(); pp++) dds[pp] = m_track_rects[pp].distanceSquared(event.getPos());
    
        auto min_iter = std::min_element(dds.begin(),dds.end());
        mMouseInGraphs = min_iter - dds.begin();
    }
    
    mMouseInTimeLine = vl.display_timeline_rect().contains(event.getPos());
    
    if (mMouseInTimeLine && ! mCellEnds.empty())
    {
        auto cf = getCurrentFrame();
        if (cf == mCellEnds[0].first.first)
        {
            Square *s = new Square();
            s->setRegPoint(rph::DisplayObject2D::RegistrationPoint::CENTERCENTER);
            s->setColor(Color(Rand::randFloat(),Rand::randFloat(),Rand::randFloat()));
            s->setSize(Rand::randInt(50,100),Rand::randInt(50,100));
            s->setPos( event.getPos() );
            s->fadeOutAndDie();
            mContainer.addChild(s);
        }
        else if (cf == mCellEnds[0].second.first)
        {
            Circle *c = new Circle();
            c->setRegPoint(rph::DisplayObject2D::RegistrationPoint::CENTERCENTER);
            c->setColor(Color(Rand::randFloat(),Rand::randFloat(),Rand::randFloat()));
            c->setScale(Rand::randInt(50,100));
            c->setPos( event.getPos() );
            c->fadeOutAndDie();
            mContainer.addChild(c);

        }
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
    }
    
    // If we are in the Visible Channel
    if (getManualEditMode() && mMouseInTimeLine )
    {
        if( mTimeLineSlider.hitTest( event.getPos() ) ) {
            mTimeMarker.from_norm(mTimeLineSlider.mValueScaled);
            auto currentFrame = mTimeMarker.current_frame();
            index_time_t tt (currentFrame, 0.0);
            if (! mCellEnds.empty())
            {
                if (currentFrame > mCellEnds[0].first.first)
                    mCellEnds[0].second = tt;
                else if (currentFrame < mCellEnds[0].first.first)
                {
                    mCellEnds[0].second  = mCellEnds[0].first;
                    mCellEnds[0].first = tt;
                }
            }
            else // (mCellEnds.empty())
            {
                length_time_t lt (tt,tt);
                mCellEnds.push_back(lt);
            }
        }
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
    vl.plot_rects(m_track_rects);
    for (int cc = 0; cc < m_plots.size(); cc++)
    {
        m_plots[cc]->setRect (m_track_rects[cc]);
    }
    
    mTimeLineSlider.mBounds = vl.display_timeline_rect();
    
}
void lifContext::update ()
{
  mContainer.update();
    
  if ( is_ready (m_async_luminance_tracks))
    {
        auto tracksRef = m_async_luminance_tracks.get();
        assert (tracksRef->size() == m_plots.size ());
        for (int cc = 0; cc < tracksRef->size(); cc++)
        {
            m_plots[cc]->setup(tracksRef->at(cc));
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
    
    std::string pos = channel_name + "[ " +
    toString(((int)m_instant_pixel_Color.g)) + " ]" +
    "[" + to_string(mMouseInImagePosition.x) +
    "," + to_string(mMouseInImagePosition.y) +
    "] i [" +
    to_string(imagePos().x) + "," + to_string(imagePos().y) + "]";
    
    lout.clear( ColorA::gray( 0.2f, 0.5f ) );
    lout.setFont( Font( "Menlo", 10 ) );
    if (channel_name == "Red")
        lout.setColor( ColorA(0.8,0.2,0.1,1.0) );
    else if (channel_name == "Green")
        lout.setColor( ColorA(0.2,0.8,0.1,1.0) );
    else
         lout.setColor( ColorA(0.8,0.8,0.8,1.0) );
    
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
    
    
    auto texR = pixelInfoTexture ();
    if (texR)
    {
        Rectf tbox = texR->getBounds();
        tbox = tbox.getCenteredFit(getWindowBounds(), true);
        tbox.scale(vec2(0.5, 0.67));
        tbox.offset(vec2(getWindowWidth() - tbox.getWidth(),getWindowHeight() - tbox.getHeight() - tbox.getY1()));
        
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
        
        draw_info ();
        
        if (m_serie.channelCount)
        {
            for (int cc = 0; cc < m_plots.size(); cc++)
            {
                m_plots[cc]->draw();
            }
        }
        
        mContainer.draw();        
    }
    
    mUIParams.draw();
    
    
}

