#ifndef __LIFContext___h
#define __LIFContext___h

#include "algo_Lif.hpp"

using namespace tinyUi;

using namespace boost;
using namespace boost::filesystem;
using namespace ci;
using namespace ci::app;
using namespace ci::signals;

using namespace params;

using namespace std;
namespace fs = boost::filesystem;

class lifContext : public sequencedImageContext
{
public:
    
    using contractionContainer_t = lif_processor::contractionContainer_t;
    
	struct clip
    {
        size_t begin;
        size_t end;
        size_t anchor;
        
        friend std::ostream& operator<< (std::ostream& out, const lifContext::clip& se)
        {
            out << "Begin:    " << se.begin << std::endl;
            out << "End: " << se.end << std::endl;
            out << "Anchor  " << se.anchor << std::endl;
            return out;
        }
    };
    
    
    enum Side_t : uint32_t
    {
        major = 0,
        minor = 1,
        notset = 2,
        number_of_sides = notset
    };

	class series_info
	{
	public:
		std::string name;
		uint32_t    timesteps;
		uint32_t    pixelsInOneTimestep;
		uint32_t	channelCount;
		std::vector<size_t> dimensions;
		std::vector<lifIO::ChannelData> channels;
		std::vector<std::string> channel_names;
		std::vector<time_spec_t> timeSpecs;
		float                    length_in_seconds;
		
        
		friend std::ostream& operator<< (std::ostream& out, const series_info& se)
		{
			out << "Serie:    " << se.name << std::endl;
			out << "Channels: " << se.channelCount << std::endl;
			out << "TimeSteps  " << se.timesteps << std::endl;
			out << "Dimensions:" << se.dimensions[0]  << " x " << se.dimensions[1] << std::endl;
			out << "Time Length:" << se.length_in_seconds	<< std::endl;
			return out;
		}
		
		std::string info ()
		{
			std::ostringstream osr;
			osr << *this << std::endl;
			return osr.str();
		}
		
	};
	
 
    
    typedef std::pair<vec2,vec2> sides_length_t;
    
	// From just a name, use the open file dialog to get the file
	// From a name and a path
	lifContext(ci::app::WindowRef& ww, const boost::filesystem::path& pp = boost::filesystem::path () );
	
	Signal <void(bool)> signalManualEditMode;
	
	static const std::string& caption () { static std::string cp ("Lif Viewer # "); return cp; }
	virtual void draw ();
	virtual void setup ();
	virtual bool is_valid ();
	virtual void update ();
	virtual void resize ();
	void draw_window ();
	void draw_info ();
	
	virtual void onMarked (marker_info_t&);
	virtual void mouseDown( MouseEvent event );
	virtual void mouseMove( MouseEvent event );
	virtual void mouseUp( MouseEvent event );
	virtual void mouseDrag( MouseEvent event );
	virtual void mouseWheel( MouseEvent event );
	virtual void keyDown( KeyEvent event );
	
	virtual void seekToStart ();
	virtual void seekToEnd ();
	virtual void seekToFrame (int);
	virtual int getCurrentFrame ();
	virtual time_spec_t getCurrentTime ();
	virtual int getNumFrames ();
	virtual void processDrag (ivec2 pos);
    
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
	void setMedianCutOff (uint32_t newco);
	uint32_t getMedianCutOff () const;

	
	void play_pause_button ();
	void loop_no_loop_button ();
    void edit_no_edit_button ();
    void analyze_analyzing_button ();
	
	void receivedEvent ( InteractiveObjectEvent event );
	
	const tiny_media_info& media () const { return mMediaInfo; }
    const uint32_t& channel_count () const { return mChannelCount; }
    
    
private:
    // LIF Support
    std::shared_ptr<lif_processor> m_lifProcRef;
    std::shared_ptr<lifIO::LifReader> m_lifRef;
    std::vector<series_info> m_series_book;
    std::vector<std::string> m_series_names;
	void loadLifFile();
	void loadCurrentSerie ();
	bool have_lif_serie ();
    void get_series_info (const std::shared_ptr<lifIO::LifReader>& lifer);
    std::shared_ptr<lifIO::LifSerie> m_current_serie_ref;
    series_info m_serie;
    boost::filesystem::path mPath;
    
    // Callbacks
    void signal_content_loaded (int64_t&);
    void signal_flu_stats_available ();
    void signal_sm1d_available (int&);
    void signal_sm1dmed_available (int&,int&);
    void signal_contraction_available (contractionContainer_t&);
    void signal_frame_loaded (int& findex, double& timestamp);
    

    // Clip Processing
    int get_current_clip_index () const;
    void set_current_clip_index (int cindex) const;
    void reset_entire_clip (const size_t& frame_count) const;
    void reset_clips () const;
    int m_cur_selected_index;
    int m_prev_selected_index;
    mutable volatile int m_current_clip_index;
    mutable std::deque<clip> m_clips;
    mutable clip m_entire;
    mutable std::mutex m_clip_mutex;
    
    // Frame Cache and frame store
    std::shared_ptr<qTimeFrameCache> mFrameSet;
    SurfaceRef  mSurface;
  
    // Tracks of frame associated results
    std::weak_ptr<vectorOfnamedTrackOfdouble_t> m_trackWeakRef;
    std::weak_ptr<vectorOfnamedTrackOfdouble_t> m_pci_trackWeakRef;

    // Contraction
    lif_processor::contractionContainer_t m_contractions;
    std::vector<std::string> m_contraction_none = {" Entire "};
    mutable std::vector<std::string> m_contraction_names;
    
    // Content Info
    tiny_media_info      mMediaInfo;
    mutable uint32_t  mChannelCount;
    
    // Instant information
    int64_t m_seek_position;
    bool m_is_playing, m_is_looping;
    vec2 m_zoom;
    vec2        mMousePos;
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
 
    
    // Update GUI
    void clear_playback_params ();
    void update_instant_image_mouse ();
    Rectf get_image_display_rect ();
    vec2 texture_to_display_zoom ();
    void add_plots ();
    
    // Navigation
    void play ();
    void pause ();
    void update_log (const std::string& meg = "");
    bool looping ();
    void looping (bool what);
	void seek( size_t xPos );

	
    // UI Params Menu
    params::InterfaceGl         mUIParams;
    std::vector<std::string> mEditNames = {"label=`Cell Length`", "label=`Cell Width`", "label=`None`"};
    std::vector<std::string>  mPlayOrPause = {"Play", "Pause"};
    std::vector<std::string>  mProcessOrProcessing = {"Process", "Processing"};
	int mButton_title_index;
	std::string mButton_title;
    

  
    
	static size_t Normal2Index (const Rectf& box, const size_t& pos, const size_t& wave)
	{
		size_t xScaled = (pos * wave) / box.getWidth();
		xScaled = svl::math<size_t>::clamp( xScaled, 0, wave );
		return xScaled;
	}
	

};


#endif
