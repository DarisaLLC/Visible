#ifndef __LIFContext___h
#define __LIFContext___h


#include <OpenImageIO/imagebuf.h>
#include <OpenImageIO/imagecache.h>
#include <OpenImageIO/imageio.h>

#include "guiContext.h"
#include "ssmt.hpp"
//#include "lif_content.hpp"
#include "clipManager.hpp"
#include "visible_layout.hpp"
#include <atomic>
#include "OnImagePlotUtils.h"
#include "imGuiCustom/imgui_plot.h"
#include <map>

using namespace boost;
using namespace boost::filesystem;
using namespace ci;
using namespace ci::app;
using namespace ci::signals;

using namespace std;
namespace bfs = boost::filesystem;
using namespace OIIO;

typedef std::shared_ptr<class visibleContext> visibleContextRef;



class visibleContext : public sequencedImageContext
{
public:
    
    enum pipeline {
        cardiac = 0,
        temporalEntropy = 1,
        spatioTemporalSegmentation = 2,
        temporalIntensity = 3,
        count = temporalIntensity + 1
    };
    
    using pipeline = visibleContext::pipeline;

  
    
    
    
    // From a oiio ImageBuf
    visibleContext(ci::app::WindowRef& ww,
               const std::shared_ptr<ImageBuf>& input,
               const mediaSpec& mspec,
               const bfs::path&,
               const bfs::path&,
				   const float magnification = 10.0f,
				   const float displayFPS = 1.0f,
                   const pipeline which_pipeline = pipeline::cardiac);
    
    std::shared_ptr<visibleContext> shared_from_above(){
        return std::dynamic_pointer_cast<visibleContext>(shared_from_this ());
    }

	// namedTimeSeries_t<float> is std::map<std::string, std::vector<float>> namedTimeSeries_t;
	template<class T>
	using timeDataDict_t = std::map<std::string, std::vector<T>>;
	
	
	static const std::string& caption () { static std::string cp ("Lif Viewer # "); return cp; }
	virtual void draw () override;
	virtual void setup () override;
	virtual bool is_valid () const override;
	virtual void update () override;
	virtual void resize () override;
	void draw_window ();

	
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
    virtual time_spec_t getStartTime () override;
	virtual int getNumFrames () override;
    
    
	virtual void processDrag (ivec2 pos) override;
    
    bool haveTracks();
	
	const ivec2& imagePos () const { return m_instant_mouse_image_pos; }
	const uint32_t& channelIndex () const { return m_instant_channel; }
	
	void setZoom (vec2);
	vec2 getZoom ();
	bool setMedianCutOff (float newco) const;
	float getMedianCutOff () const;
	
	void play_pause_button ();
	void loop_no_loop_button ();
    void edit_no_edit_button ();
    void update_sequencer ();
	
    const uint32_t& channel_count () const { return mChannelCount; }
    
    // Navigation
    void play ();
    void pause ();
    bool isPlaying () const { return m_is_playing; }
    bool isEditing () const { return m_is_editing; }
    
    // Supporting gui_base
    void DrawGUI();
    bool setup_panels (int image_width, int image_height, int vp_width, int vp_height);
    
    // Async Processing
    virtual void process_async () override;
	
	// Access to data asynchronically updated
	const timeDataDict_t<float>& timedDataDict_F () const;
     
    // Status
    bool isLoaded() const { return m_content_loaded; }
    
    // Processing Choices @todo add an actual design !!
	const pipeline& operation () const {  std::lock_guard<std::mutex> guard(m_clip_mutex); return m_operation; }
	void operation (pipeline p) const {  std::lock_guard<std::mutex> guard(m_clip_mutex); m_operation = p; }
	bool isCardiacPipeline () const {   return operation() == pipeline::cardiac; }
	bool isVisualEntropyPipeline () const {  return operation() == pipeline::temporalEntropy; }
	bool isSpatioTemporalPipeline () const {  return operation() == pipeline::spatioTemporalSegmentation; }
	bool isTemporalIntensityPipeline () const {  return operation() == pipeline::temporalIntensity; }
	void magnification (const float& mmag) const { m_magnification = mmag; }
	float magnification () const { return m_magnification; }
    
private:
	timeDataDict_t<float> m_timeFloatDict;
	
		// utility structure for realtime plot
	struct RollingBuffer {
		float Span;
		ImVector<ImVec2> Data;
		RollingBuffer() {
			Span = 10.0f;
			Data.reserve(2000);
		}
		void AddPoint(float x, float y) {
			float xmod = fmodf(x, Span);
			if (!Data.empty() && xmod < Data.back().x)
				Data.shrink(0);
			Data.push_back(ImVec2(xmod, y));
		}
	};
	
	mutable float m_magnification;
	mutable float m_displayFPS;
	
    mutable pipeline m_operation;
    
    void renderToFbo (const SurfaceRef&, gl::FboRef& );
    void setup_signals ();
    ci::app::WindowRef& get_windowRef();
    
    // Normalize for image rendering
    void glscreen_normalize (const sides_length_t& , const Rectf& display_rect,  sides_length_t&);
    

    std::shared_ptr<ssmt_processor> m_ssmtRef;
	void loadCurrentMedia ();
	bool have_content ();

    
    ImageSpec m_oiio_spec;
    boost::filesystem::path mPath;
    std::string mContentFileName;
    std::vector<std::string> m_plot_names;

    
    // Callbacks
    void signal_content_loaded (int64_t&);
    void signal_intensity_over_time_ready ();
    void signal_root_pci_ready (std::vector<float> &, const input_section_selector_t&);
    void signal_root_mls_ready (std::vector<float> &, const input_section_selector_t&, uint32_t& body_id);
    void signal_contraction_ready (contractionLocator::contractionContainer_t&,const input_section_selector_t&);
    void signal_frame_loaded (int& findex, double& timestamp);
    void signal_regions_ready (int, const input_section_selector_t&);
    void signal_segmented_view_ready (cv::Mat&, cv::Mat&);
    void fraction_reporter(float);
    
    // Availability
    std::atomic<bool> m_voxel_view_available;
    input_section_selector_t  m_input_selector;
    mutable std::atomic<int> m_selector_last;

    // Clip Processing
    int get_current_clip_index () const;
    void set_current_clip_index (int cindex) const;
    void reset_entire_clip (const size_t& frame_count) const;
    const clip& get_current_clip () const;
    mutable volatile int m_current_clip_index;
    mutable std::vector<clip> m_clips;
    mutable std::mutex m_clip_mutex;
    mutable std::mutex m_update_mutex;
    
    // Frame Cache and frame store
    std::shared_ptr<ImageBuf>  mImageCache;
    SurfaceRef  mSurface;
  
    
    // Variance Image & Texture
    cv::Mat m_segmented_image;
    ci::gl::TextureRef m_segmented_texture;
    SurfaceRef m_segmented_surface;
    


    // Selection -1 means none selected
    bool change_current_cell ();
    mutable std::atomic<int> m_selected_cell;
    mutable std::atomic<int> m_selected_contraction;
    
    // Contraction
    contractionLocator::contractionContainer_t m_contractions;
    std::vector<std::string> m_contraction_none = {" Entire "};
    mutable std::vector<std::string> m_contraction_names;
    float m_major_cell_length, m_minor_cell_length;
    std::vector<cv::Point2f> m_mid_points;
    void draw_contraction_plots(const contractionLocator::contraction_t&);
    bool save_contraction_plots(const contractionLocator::contraction_t&);

    // Directory of Cells / Contractions and SS
    std::unordered_map<cell_id_t,contractionLocator::contractionContainer_t>m_cell2contractions_map;
    
    
    // Folder for Per user result / content caching
    boost::filesystem::path mUserStorageDirPath;
    boost::filesystem::path mCurrentCachePath;
    ustring mContentNameU;
  
        
    // Content Info
//    tiny_media_info      mMediaInfo;
    mutable uint32_t  mChannelCount;
    int64_t m_minFrame;
    int64_t m_maxFrame;
    
    // Normalized to screen
    
    
    // Instant information
    float m_fraction_done;
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

    bool mMouseInImage; // if in Image, mMouseInGraph is -1
    ivec2 mMouseInImagePosition;
    std::atomic<bool> m_content_loaded;
    
    std::vector<sides_length_t> m_cell_ends = {sides_length_t (), sides_length_t()};
    std::vector<sides_length_t> m_normalized_cell_ends = {sides_length_t (), sides_length_t()};
    
    gl::TextureRef mImage;
    uint32_t m_cutoff_pct;

    // Time / Screen / Layout Info
    timeIndexConverter m_tic;
    mediaSpec m_mspec;
    
    vec2 mFrameSize;
//    gl::TextureRef pixelInfoTexture ();
    std::string m_title;
    
    // Update GUI
    void clear_playback_params ();
    void update_instant_image_mouse ();
    void update_with_mouse_position ( MouseEvent event );
    const Rectf& get_image_display_rect () override;
    void update_channel_display_rects () override;
    
    vec2 texture_to_display_zoom ();
    void add_main_info ();
    void add_canvas ();
    void add_result_sequencer ();
    void add_navigation ();
    void add_motion_profile ();
    void add_contractions (bool* p_open);

    // Navigation
    void update_log (const std::string& meg = "") override;
    bool looping () override;
    void looping (bool what) override;

    
    // Layout Manager
    std::shared_ptr<imageDisplayMapper> m_imageDisplayMapper;
  
    // UI flags
    bool m_showLog, m_showGUI, m_showHelp;
    bool m_show_contractions, m_show_cells, m_show_playback;
    bool m_median_set_at_default;
    
    

    
    // UI instant sub-window rects
    Rectf m_results_browser_display;
    Rectf m_motion_profile_display;
    Rectf m_navigator_display;
    int8_t m_playback_speed;

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
    scopedPause (const visibleContextRef& ref) : weakLif(ref) {
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
    std::shared_ptr<visibleContext> weakLif;
};

#endif
