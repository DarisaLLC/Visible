#ifndef __LIFContext___h
#define __LIFContext___h

#include "guiContext.h"
#include "ssmt.hpp"
#include "lif_content.hpp"
#include "clipManager.hpp"
#include "visible_layout.hpp"
#include <atomic>
#include "OnImagePlotUtils.h"
#include "imGuiCustom/ImSequencer.h"
#include "imGuiCustom/visibleSequencer.h"



using namespace boost;
using namespace boost::filesystem;
using namespace ci;
using namespace ci::app;
using namespace ci::signals;

using namespace params;

using namespace std;
namespace fs = boost::filesystem;

typedef std::shared_ptr<class lifContext> lifContextRef;



class lifContext : public sequencedImageContext
{
public:
    
    using contractionContainer_t = ssmt_processor::contractionContainer_t;
    
  
    enum Side_t : uint32_t
    {
        major = 0,
        minor = 1,
    };

  
    
    // From a lif_serie_data
    lifContext(ci::app::WindowRef& ww, const lif_serie_data&, const fs::path&  );
    
    // shared_from_this() through base class
    lifContext* shared_from_above () {
        return static_cast<lifContext*>(((guiContext*)this)->shared_from_this().get());
    }

    

	
	static const std::string& caption () { static std::string cp ("Lif Viewer # "); return cp; }
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
    void update_contraction_selection ();
	
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
    
    // Normalize for image rendering
    void glscreen_normalize (const sides_length_t& , const Rectf& display_rect,  sides_length_t&);
    
    // LIF Support
    std::shared_ptr<ssmt_processor> m_lifProcRef;
	void loadCurrentSerie ();
	bool have_lif_serie ();
    std::shared_ptr<lifIO::LifSerie> m_cur_lif_serie_ref;
    lif_serie_data m_serie;
    boost::filesystem::path mPath;
    std::vector<std::string> m_plot_names;
    idlab_cardiac_defaults m_idlab_defaults;
    const Rectf& get_channel_display_rect (const int channel_number);
    
    // Callbacks
    void signal_content_loaded (int64_t&);
    void signal_flu_stats_ready ();
    void signal_sm1d_ready (input_channel_selector_t&);
    void signal_sm1dmed_ready (input_channel_selector_t&);
    void signal_contraction_ready (contractionContainer_t&);
    void signal_frame_loaded (int& findex, double& timestamp);
    void signal_geometry_ready (int, const input_channel_selector_t&);
    void signal_segmented_view_ready (cv::Mat&, cv::Mat&);
    
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
    SurfaceRef m_segmented_surface;
    
    // Tracks of frame associated results
    std::weak_ptr<vecOfNamedTrack_t> m_flurescence_trackWeakRef;
    std::weak_ptr<vecOfNamedTrack_t> m_contraction_pci_trackWeakRef;

    // Contraction
    ssmt_processor::contractionContainer_t m_contractions;
    std::vector<std::string> m_contraction_none = {" Entire "};
    mutable std::vector<std::string> m_contraction_names;
    float m_major_cell_length, m_minor_cell_length;
    std::vector<cv::Point2f> m_mid_points;
    
    
    // Folder for Per user result / content caching
    boost::filesystem::path mUserStorageDirPath;
    boost::filesystem::path mCurrentSerieCachePath;
    
  
        
    // Content Info
    tiny_media_info      mMediaInfo;
    mutable uint32_t  mChannelCount;
    int64_t m_minFrame;
    int64_t m_maxFrame;
    
    // Normalized to screen
    
    
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

    std::vector<sides_length_t> m_cell_ends = {sides_length_t (), sides_length_t()};
    std::vector<sides_length_t> m_normalized_cell_ends = {sides_length_t (), sides_length_t()};
    
    gl::TextureRef mImage;
    uint32_t m_cutoff_pct;


    
    // Screen Info
    vec2 mScreenSize;
    gl::TextureRef pixelInfoTexture ();
    std::string m_title;
    
    // Update GUI
    void clear_playback_params ();
    void update_instant_image_mouse ();
    void update_with_mouse_position ( MouseEvent event );
    const Rectf& get_image_display_rect () override;
    void update_channel_display_rects () override;
    
    vec2 texture_to_display_zoom ();
    void add_result_sequencer ();
    void add_navigation ();
    void add_motion_profile ();
    void add_contractions (bool* p_open);
    // Navigation
    void update_log (const std::string& meg = "") override;
    bool looping () override;
    void looping (bool what) override;

    
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
    Rectf m_motion_profile_display;
    Rectf m_navigator_display;
    int8_t m_playback_speed;

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
    scopedPause (const lifContextRef& ref) : weakLif(ref) {
        if (! weakLif){
            if (weakLif->isPlaying())
                weakLif->play_pause_button();
        }
    }
    ~scopedPause (){
        if (! weakLif ){
            if (! weakLif->isPlaying()){
                weakLif->play_pause_button();
            }
        }
    }
private:
    std::shared_ptr<lifContext> weakLif;
};

#endif
