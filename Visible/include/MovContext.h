#ifndef __movContext___h
#define __movContext___h

#include "guiContext.h"
#include "clipManager.hpp"
#include "visible_layout.hpp"
#include <atomic>
#include "OnImagePlotUtils.h"
#include "imGuiCustom/ImSequencer.h"
#include "imGuiCustom/visibleSequencer.h"
#include "cvVideoPlayer.h"
#include "algo_Mov.hpp"


using namespace boost;
using namespace boost::filesystem;
using namespace ci;
using namespace ci::app;
using namespace ci::signals;

using namespace params;

using namespace std;
namespace fs = boost::filesystem;

typedef std::shared_ptr<class movContext> movContextRef;


/* Context specific items
 input channels   processing channels display channels. 
	example: LIF three channels custom for IDLab:		3 input [1], [2], [3].  Each, Axxx or xxxA 
	example: LIF 1 channel custom for IDLab:		1 input [1].  Each, Axxx or xxxA 

	example: mov 3 channels:				3 inputs [1,2,3] * [1,2,3]T, Axxx, xxxA
	example: directory of images:			3 inputs [1,2,3] * [1,2,3]T, Axxx, xxxA

 base implementation processing:
		ingest and display in Axxx or xxxA

 	image_sequence is struct similar to lif serie


*/


template<typename T, typename C>
class image_sequence_data
{
public:
    image_sequence_data ();
    std::shared_ptr<T>& create (const T& m_lifRef);
	
    int index () const { return m_index; }
    const std::string name () const { return m_name; }
   
    float seconds () const { return m_length_in_seconds; }
    uint32_t timesteps () const { return m_timesteps; }
    uint32_t channelCount () const { return m_channelCount; }
    const std::vector<size_t>& dimensions () const { return m_dimensions; }

    const std::vector<C>& channels () const { return m_channels; }
	
    const std::vector<std::string>& channel_names () const { return m_channel_names; }
    const std::vector<time_spec_t>& timeSpecs () const { return  m_timeSpecs; }
	
    const std::weak_ptr<T>& readerWeakRef () const;
	
    const cv::Mat& poster () const { return m_poster; }
    const std::string& custom_identifier () const { return m_contentType; }
 

private:
    mutable int32_t m_index;
    mutable std::string m_name;
    uint32_t    m_timesteps;
    uint32_t    m_pixelsInOneTimestep;
    uint32_t    m_channelCount;
    std::vector<size_t> m_dimensions;
    std::vector<C> m_channels;
    std::vector<std::string> m_channel_names;
    std::vector<time_spec_t> m_timeSpecs;
    cv::Mat m_poster;
    mutable std::weak_ref m_lifWeakRef;
    
    mutable float  m_length_in_seconds;
    mutable std::string m_contentType; // "" denostes canonical LIF
    
    
    friend std::ostream& operator<< (std::ostream& out, const image_sequence_data& se)
    {
        out << "Serie:    " << se.name() << std::endl;
        out << "Channels: " << se.channelCount() << std::endl;
        out << "TimeSteps  " << se.timesteps() << std::endl;
        out << "Dimensions:" << se.dimensions()[0]  << " x " << se.dimensions()[1] << std::endl;
        out << "Time Length:" << se.seconds()    << std::endl;
        return out;
    }
    
    std::string info ()
    {
        std::ostringstream osr;
        osr << *this << std::endl;
        return osr.str();
    }
    
};


class movContext : public sequencedImageContext
{

public:
    
  
    
    // From a image_sequence_data
    movContext(ci::app::WindowRef& ww, const cvVideoPlayer&, const fs::path&  );
    
    // shared_from_this() through base class
    movContext* shared_from_above () {
        return static_cast<movContext*>(((guiContext*)this)->shared_from_this().get());
    }

    
	static const std::string& caption () { static std::string cp ("Media Viewer # "); return cp; }
	virtual void draw () override;
	virtual void setup () override;
	virtual bool is_valid () const override;
	virtual void update () override;
	virtual void resize () override;
	void draw_window ();
	void draw_info () override;
	
	virtual void onMarked (marker_info_t&) ;
	virtual void mouseDown( MouseEvent event ) override;
	virtual void mouseMove( MouseEvent event ) override;
	virtual void mouseUp( MouseEvent event ) override;
	virtual void mouseDrag( MouseEvent event ) override;
	virtual void mouseWheel( MouseEvent event ) ;
	virtual void keyDown( KeyEvent event ) override;
	
	virtual void seekToStart () override;
	virtual void seekToEnd () override;
	virtual void seekToFrame (int) override;
	virtual int getCurrentFrame () override;
	virtual time_spec_t getCurrentTime () override;
	virtual int getNumFrames () override;
	virtual void processDrag (ivec2 pos) override;
    
    bool haveTracks();
	
	const ivec2& imagePos () const { return m_instant_mouse_image_pos; }
	const uint32_t& channelIndex () const { return m_instant_channel; }
	
	void setZoom (vec2);
	vec2 getZoom ();
	void setMedianCutOff (int32_t newco);
	int32_t getMedianCutOff () const;
    void setCellLength (uint32_t newco);
    uint32_t getCellLength () const;


	
	void play_pause_button ();
	void loop_no_loop_button ();
    void edit_no_edit_button ();
	
	const tiny_media_info& media () const { return mMediaInfo; }
    const uint32_t& channel_count () const { return mChannelCount; }
    
    // Navigation
    void play ();
    void pause ();
    bool isPlaying () const { return m_is_playing; }
    bool isEditing () const { return m_is_editing; }
    
    // Supporting gui_base
    void SetupGUIVariables() override;
    void DrawGUI() override;
    void QuitApp();
    
private:

    void setup_signals ();
    void setup_params ();
    ci::app::WindowRef& get_windowRef();
    
    // mov Support
    std::shared_ptr<sequence_processor> m_movProcRef;
	void load_current_sequence ();
	bool have_sequence ();
    std::shared_ptr<image_sequence> m_cur_image_sequence_ref;
    image_sequence_data m_serie;
    boost::filesystem::path mPath;
    std::vector<std::string> m_plot_names;
    
    
    // Callbacks
    void signal_content_loaded (int64_t&);
    void signal_sm1d_available (int&);
    void signal_sm1dmed_available (int&,int&);
    void signal_frame_loaded (int& findex, double& timestamp);
    
    // Availability
    std::atomic<bool> m_geometry_available;

    // Clip Processing
    int get_current_clip_index () const;
    void set_current_clip_index (int cindex) const;
    void reset_entire_clip (const size_t& frame_count) const;
    const clip& get_current_clip () const;
    mutable volatile int m_current_clip_index;
    mutable std::vector<clip> m_clips;
    mutable std::mutex m_clip_mutex;
    
    // Async Processing
    void process_async ();
    
    // Frame Cache and frame store
    std::shared_ptr<seqFrameContainer> mFrameSet;
    SurfaceRef  mSurface;
  
    
    // Variance Image & Texture
    cv::Mat m_segmented_image;
    ci::gl::TextureRef m_segmented_texture;
    
    // Tracks of frame associated results
    std::weak_ptr<vecOfNamedTrack_t> m_basic_trackWeakRef;

    // Folder for Per user result / content caching
    boost::filesystem::path mUserStorageDirPath;
    boost::filesystem::path mCurrentCachePath;
    
    // Content Info
    tiny_media_info      mMediaInfo;
    mutable uint32_t  mChannelCount;
    int64_t m_minFrame;
    int64_t m_maxFrame;
    
    // Instant information
    int64_t m_seek_position;
    bool m_is_playing, m_is_looping;
    bool m_is_editing, m_show_probe;
    bool m_is_in_params;
    vec2 m_zoom;
    vec2 mMousePos;
    bool mMouseIsDown;
    bool mMouseIsMoving;
    bool mMouseIsDragging;
    bool mMetaDown;
    int mMouseInGraphs; // -1 if not, 0 1 2
    bool mMouseInImage; // if in Image, mMouseInGraph is -1
    ivec2 mMouseInImagePosition;
    uint32_t m_cutoff_pct;	

    gl::TextureRef mImage;

    // Screen Info
    vec2 mScreenSize;
    gl::TextureRef pixelInfoTexture ();
    std::string m_title;
    
    // Update GUI
    void clear_playback_params ();
    void update_instant_image_mouse ();
    void update_with_mouse_position ( MouseEvent event );
    Rectf get_image_display_rect () override;
    vec2 texture_to_display_zoom ();
    void add_result_sequencer ();
    void add_navigation ();

    // Navigation
    void update_log (const std::string& meg = "") override;
    bool looping () override;
    void looping (bool what) override;
	void seek( size_t xPos );

	
    // Layout Manager
    std::shared_ptr<layoutManager> m_layout;
  
    // UI flags
    bool m_showLog, m_showGUI, m_showHelp;
    bool m_show_contractions, m_show_playback;
    bool m_median_set_at_default;
    
    
    // imGui
    timeLineSequence m_result_seq;
    void draw_sequencer ();
    
    // UI instant sub-window rects
    Rectf m_results_browser_display;
    Rectf m_navigator_display;
    

    OnImagePlot m_tsPlotter;
    
    // Resource Icons
    ci::gl::TextureRef    mNoLoop, mLoop;
    
	static size_t Normal2Index (const Rectf& box, const size_t& pos, const size_t& wave)
	{
		size_t xScaled = (pos * wave) / box.getWidth();
		xScaled = svl::math<size_t>::clamp( xScaled, 0, wave );
		return xScaled;
	}
	

};

class scopedPause : private Noncopyable {
public:
    scopedPause (const movContextRef& ref) :weakContext(ref) {
        if (!weakContext){
            if (weakLif->isPlaying())
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

#endif
