#ifndef __LIFContext___h
#define __LIFContext___h

#include "guiContext.h"
#include "algo_Lif.hpp"
#include "clipManager.hpp"
#include "visible_layout.hpp"
#include <atomic>

#include "imGuiCustom/ImSequencer.h"
#include "imGuiCustom/visibleSequencer.h"

using namespace tinyUi;

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
    
    using contractionContainer_t = lif_serie_processor::contractionContainer_t;
    
  
    enum Side_t : uint32_t
    {
        major = 0,
        minor = 1,
    };

    typedef std::pair<vec2,vec2> sides_length_t;
    
    // From a lif_serie_data
    lifContext(ci::app::WindowRef& ww, const lif_serie_data& );
    
    // shared_from_this() through base class
    lifContext* shared_from_above () {
        return static_cast<lifContext*>(((guiContext*)this)->shared_from_this().get());
    }

    
	Signal <void(bool)> signalManualEditMode;
	
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
	
	const params::InterfaceGl& ui_params ()
	{
		return mUIParams;
	}
	
	const ivec2& imagePos () const { return m_instant_mouse_image_pos; }
	const uint32_t& channelIndex () const { return m_instant_channel; }
	
	void setManualEditMode (Side_t b)  { mManualEditMode = b; }
	Side_t getManualEditMode ()  { return mManualEditMode; }
    Side_t getManualNextEditMode ();
	
	void setAnalyzeMode (bool b)  { mAnalyze = b; }
	bool getAnalyzeMode ()  { return mAnalyze; }
	
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
	
	void receivedEvent ( InteractiveObjectEvent event );
	
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
    
    // LIF Support
    std::shared_ptr<lif_serie_processor> m_lifProcRef;
	void loadCurrentSerie ();
	bool have_lif_serie ();
    std::shared_ptr<lifIO::LifSerie> m_cur_lif_serie_ref;
    lif_serie_data m_serie;
    boost::filesystem::path mPath;
    
    // Callbacks
    void signal_content_loaded (int64_t&);
    void signal_flu_stats_available ();
    void signal_sm1d_available (int&);
    void signal_sm1dmed_available (int&,int&);
    void signal_contraction_available (contractionContainer_t&);
    void signal_frame_loaded (int& findex, double& timestamp);
    void signal_volume_var_available ();
    
    // Availability
    std::atomic<bool> m_volume_var_available;

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
  
    // Variance Image Texture
    ci::gl::TextureRef m_var_texture;
    
    // Tracks of frame associated results
    std::weak_ptr<vecOfNamedTrack_t> m_trackWeakRef;
    std::weak_ptr<vecOfNamedTrack_t> m_pci_trackWeakRef;

    // Contraction
    lif_serie_processor::contractionContainer_t m_contractions;
    std::vector<std::string> m_contraction_none = {" Entire "};
    mutable std::vector<std::string> m_contraction_names;
    int32_t m_cell_length;
    
    
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
    sides_length_t mLengthPoints;
    std::vector<sides_length_t> mCellEnds = {sides_length_t (), sides_length_t()};
    gl::TextureRef mImage;
    uint32_t m_cutoff_pct;
    bool mAnalyze;
    Side_t mManualEditMode;
    
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
    void add_timeline ();
    void add_motion_profile ();
    
    // Navigation
    void update_log (const std::string& meg = "") override;
    bool looping () override;
    void looping (bool what) override;
	void seek( size_t xPos );

	
    // UI Params Menu
    params::InterfaceGl         mUIParams;
    std::vector<std::string> mEditNames = {"label=`Cell Length`", "label=`Cell Width`", "label=`None`"};
    std::vector<std::string>  mPlayOrPause = {"Play", "Pause"};
    std::vector<std::string>  mProcessOrProcessing = {"Process", "Processing"};
	int mButton_title_index;
	std::string mButton_title;
    
    // Layout Manager
    std::shared_ptr<layoutManager> m_layout;
  
    // UI flags
    bool m_showLog, m_showGUI, m_showHelp;
    
    // imGui
    timeLineSequence mySequence;
    void draw_sequencer ();
    
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
