#ifndef __MOVContext___h
#define __MOVContext___h

#include "algo_mov.hpp"

using namespace tinyUi;

using namespace boost;
using namespace boost::filesystem;
using namespace ci;
using namespace ci::app;
using namespace ci::signals;

using namespace params;

using namespace std;
namespace fs = boost::filesystem;




class movContext : public sequencedImageContext
{
public:
	

	// From just a name, use the open file dialog to get the file
	// From a name and a path
	movContext(ci::app::WindowRef& ww, const boost::filesystem::path& pp = boost::filesystem::path () );
	
    Signal <void(bool)> signalShowMotioncenter;
	
    
    const  boost::filesystem::path source_path () const;
    void  source_path (const boost::filesystem::path& source_path) const;
    
	static const std::string& caption () { static std::string cp ("Movie Viewer # "); return cp; }
	virtual void draw ();
	virtual void setup ();
	virtual bool is_valid () const;
	virtual void update ();
	virtual void resize ();
	void draw_window ();
	void draw_info ();
	
	virtual void onMarked (marker_info_t&);
	virtual void mouseDown( MouseEvent event );
	virtual void mouseMove( MouseEvent event );
	virtual void mouseUp( MouseEvent event );
	virtual void mouseDrag( MouseEvent event );

	virtual void keyDown( KeyEvent event );
	
	virtual void seekToStart ();
	virtual void seekToEnd ();
	virtual void seekToFrame (int);
	virtual int getCurrentFrame ();
	virtual time_spec_t getCurrentTime ();
	virtual int getNumFrames ();
	virtual void processDrag (ivec2 pos);
	
	const params::InterfaceGl& ui_params ()
	{
		return mUIParams;
	}
	
	const ivec2& imagePos () const { return m_instant_mouse_image_pos; }
	const uint32_t& channelIndex () const { return m_instant_channel; }
	
	void setShowMotionCenter (bool b)  { mShowMotionCenter = b; }
	bool getShowMotionCenter ()  { return mShowMotionCenter; }
	
	void setZoom (vec2);
	vec2 getZoom ();
	void setMedianCutOff (uint32_t newco);
	uint32_t getMedianCutOff () const;

	
	void play_pause_button ();
	void loop_no_loop_button ();
	void edit_no_edit_button ();
	
	void receivedEvent ( InteractiveObjectEvent event );
	
	const tiny_media_info& media () const { return mMediaInfo; }
	
private:
    std::shared_ptr<mov_processor> m_movProcRef;
    
	gl::TextureRef pixelInfoTexture ();
	void loadMovieFile();
	void loadCurrentSerie ();
	bool have_movie ();
	void play ();
	void pause ();
	void update_log (const std::string& meg = "");
	bool looping ();
	void looping (bool what);
	
	Rectf get_image_display_rect ();
	
    void signal_content_loaded ();
    void signal_sm1d_available (int&);
    void signal_sm1dmed_available (int&,int&);
 
    void signal_frame_loaded (int& findex, double& timestamp);
 

	
	void seek( size_t xPos );
	void clear_movie_params ();
	vec2 texture_to_display_zoom ();
	void update_instant_image_mouse ();
	
    ocvPlayerRef m_movie;
	vec2 mScreenSize;
	gl::TextureRef mImage;
	
	size_t m_frameCount;
	params::InterfaceGl         mUIParams;
	tiny_media_info      mMediaInfo;
	
	int64_t m_seek_position;

	bool m_is_playing, m_is_looping;
	uint32_t m_cutoff_pct;

	
	
	vec2 m_zoom;
	boost::filesystem::path mPath;
	vec2		mMousePos;
	std::shared_ptr<seqFrameContainer> mFrameSet;
	SurfaceRef  mSurface;
	
	bool mMouseIsDown;
	bool mMouseIsMoving;
	bool mMouseIsDragging;
	bool mMetaDown;
    bool m_looping;
    
	int mMouseInGraphs; // -1 if not, 0 1 2
	bool mMouseInImage; // if in Image, mMouseInGraph is -1
	bool mMouseInTimeLine;
	bool mAuxMouseInTimeLine;
	
	ivec2 mMouseInImagePosition;
	
	bool mManualEditMode, mShowMotionCenter;
	std::vector<std::string>  mPlayOrPause = {"Play", "Pause"};
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
