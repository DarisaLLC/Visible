#ifndef __guiContext___h
#define __guiContext___h

#include "cinder/app/App.h"
#include "cinder/gl/Vbo.h"
#include "cinder/gl/gl.h"
#include "cinder/Rect.h"
#include "cinder/qtime/QuickTimeGl.h"
#include "cinder/CameraUi.h"
#include "cinder/params/Params.h"
#include "cinder/Timeline.h"
#include "cinder/ImageIo.h"
#include "cinder/Thread.h"
#include "cinder/Signals.h"
#include "cinder/Cinder.h"
#include "cinder/Surface.h"
#include "cinder/Text.h"
#include "cinder_cv/cinder_opencv.h"

#include <memory>
#include <utility>
#include <boost/integer_traits.hpp>
// use int64_t instead of long long for better portability
#include <boost/filesystem.hpp>
#include <boost/cstdint.hpp>

#include "timestamp.h"
#include "seq_frame_container.hpp"

#include "graph1d.hpp"
#include "roiWindow.h"
#include "otherIO/lifFile.hpp"
#include "core/stl_utils.hpp"
#include "async_producer.h"
#include "directoryPlayer.h"
#include "hockey_etc_cocoa_wrappers.h"
#include "timeMarker.h"
#include "TinyUi.h"
#include "OcvVideo.h"
#include <sstream>

#include "DisplayObjectContainer.h"
//#include "Square.h"
//#include "Circle.h"

using namespace tinyUi;

using namespace boost;
using namespace boost::filesystem;
using namespace ci;
using namespace ci::app;
using namespace ci::signals;

using namespace params;

using namespace std;
namespace fs = boost::filesystem;





class guiContext : public std::enable_shared_from_this<guiContext>
{
public:
	enum Type {
		null_viewer = 0,
		matrix_viewer = null_viewer+1,
		qtime_viewer = matrix_viewer+1,
		clip_viewer = qtime_viewer+1,
		image_dir_viewer = clip_viewer+1,
		lif_file_viewer = image_dir_viewer+1,
		movie_dir_viewer = lif_file_viewer+1,
		timeline_browser = movie_dir_viewer+1,
		viewer_types = timeline_browser+1,
	};
	
	// Signal time point mark to all
	// receive time point mark from all
	typedef marker_info marker_info_t;
    typedef void (sig_cb_marker) (marker_info_t&);
	Signal <void(marker_info_t&)> signalMarker;
	
	
	guiContext (ci::app::WindowRef& ww);
	virtual ~guiContext ();
	
	std::shared_ptr<guiContext> getRef();
	

	
	virtual const std::string & getName() const ;
	virtual void setName (const std::string& name);
	virtual guiContext::Type context_type () const;
	virtual bool is_context_type (const guiContext::Type t) const;
	

	virtual void resize () = 0;
	virtual void draw () = 0;
	virtual void update () = 0;
	virtual void setup () = 0;
    virtual bool is_valid () const = 0;

    
    // u implementation does nothing
	virtual void mouseDown( MouseEvent event );
	virtual void mouseMove( MouseEvent event );
	virtual void mouseUp( MouseEvent event );
	virtual void mouseDrag( MouseEvent event );
	
	virtual void keyDown( KeyEvent event );
	virtual void normalize_point (vec2& pos, const ivec2& size);

	virtual bool keepAspect () {return true; }
	
  protected:
	Type m_type;
    size_t m_uniqueId;
    bool m_valid;
    ci::app::WindowRef				mWindow;
	std::string mName;
	marker_info mTimeMarker;
	marker_info mAuxTimeMarker;
	
};

typedef std::shared_ptr<guiContext> guiContextRef;

class mainContext : public guiContext
{
public:
	mainContext(ci::app::WindowRef& ww, const boost::filesystem::path& pp = boost::filesystem::path ());
	void resize () {}
	void draw () {}
	void update () {}
	void setup () {}
};






class matContext : public guiContext
{
public:
	// From just a name, use the open file dialog to get the file
	// From a name and a path
	matContext(ci::app::WindowRef& ww, const boost::filesystem::path& pp = boost::filesystem::path ());
    static const std::string& caption () { static std::string cp ("Smm Viewer # "); return cp; }

	virtual void resize ();
    virtual void draw ();
    virtual void setup ();
    virtual void update ();
    virtual void mouseDrag( MouseEvent event );
    virtual void mouseDown( MouseEvent event );
    void draw_window ();
    virtual void onMarked (marker_info_t&);
	
private:

    void internal_setupmat_from_file (const boost::filesystem::path &);
    params::InterfaceGl         mMatParams;
    
    gl::VboMeshRef mPointCloud;
    gl::BatchRef	mPointsBatch;

    
    CameraPersp		mCam;
    CameraUi		mCamUi;
    
    boost::filesystem::path mPath;
};



///////////////////   Visual Browsing Contexts

class sequencedImageContext : public guiContext
{
public:
	
	typedef	 signals::Signal<void( marker_info_t & )>		MarkerSignalInfo_t;
	MarkerSignalInfo_t&	getMarkerSignal () { return m_marker_signal; }
	MarkerSignalInfo_t&	getAuxMarkerSignal () { return m_aux_marker_signal; }
	
	sequencedImageContext(ci::app::WindowRef& ww)
	: guiContext (ww)
	{}
	
	virtual void seekToStart () = 0;
	virtual void seekToEnd () = 0;
	virtual void seekToFrame (int) = 0;
	virtual int getCurrentFrame () = 0;
	virtual time_spec_t getCurrentTime () = 0;
	virtual int getNumFrames () = 0;
	
	virtual void draw_info () = 0;
	virtual void update_log (const std::string& meg = "") = 0;
	virtual bool looping () = 0;
	virtual void looping (bool what) = 0;
	virtual Rectf get_image_display_rect () = 0;
	virtual void processDrag( ivec2 pos ) = 0;
	
	MarkerSignalInfo_t& markerSignal () const { return m_marker_signal; }
	
	virtual bool have_tracks () const { return m_have_tracks; }

	// App default startup params
	static ivec2 startup_display_size () { return ivec2( 848, 564 ); }
	
protected:
	int mMainTimeLineSliderIndex;
	TimeLineSlider                    mMainTimeLineSlider;
	vector<Widget *>	mWidgets;
	vector<bool> mMouseInWidgets;

	
	
	std::vector<graph1d::ref> m_plots;
	std::mutex m_track_mutex;
	
	vectorOfVectorOfnamedTrackOfdouble_t m_luminance_tracks;
	async_vectorOfnamedTrackOfdouble_t m_async_luminance_tracks;
	vectorOfVectorOfnamedTrackOfmat_t m_oflow_tracks;
	async_vectorOfnamedTrackOfmat_t m_async_oflow_tracks;
	vectorOfVectorOfnamedTrackOfdouble_t m_pci_tracks;
	async_vectorOfnamedTrackOfdouble_t m_async_pci_tracks;
	
	
	bool m_have_tracks;
	
	std::vector<size_t> m_spatial_dims;
	std::vector<std::string> m_perform_names;
	int  m_selected_perform_index;

	mutable MarkerSignalInfo_t m_marker_signal;
	mutable MarkerSignalInfo_t m_aux_marker_signal;
	
	ivec2 m_instant_mouse_image_pos;
	uint32_t m_instant_channel;
	ColorA8u m_instant_pixel_Color;
	UInt8 m_instant_channel_pixel;
	float m_instant_pixel_normalized;
	float m_instant_pixel_Luminance;
	
	
	gl::TextureRef		mTextTexture;
	vec2				mSize;
	Font				mFont;
	std::string			mLog;
	rph::DisplayObjectContainer mContainer;
	index_time_t    mCurrentIndexTime;
	
	
	
};


class movDirContext : public guiContext
{
public:
	// From a name and a path and an optional format for anonymous file names
	movDirContext(ci::app::WindowRef& ww, const boost::filesystem::path& pp = boost::filesystem::path ());
	
	void set_dir_info (const std::vector<std::string>& extension = { ".jpg", ".png", ".JPG", ".jpeg"},
					   double fps=29.97, const std::string anonymous_format = "01234567890abcdefghijklmnopqrstuvwxy");
	
	const  boost::filesystem::path source_path () const;

	virtual bool is_valid () const
	{
		return m_valid && is_context_type(guiContext::movie_dir_viewer);
	}
	
	static const std::string& caption () { static std::string cp ("Image Dir Viewer "); return cp; }
	
	virtual void draw ();
	virtual void resize ();
	virtual void setup ();
	virtual void update ();
	virtual void mouseMove( MouseEvent event );
	virtual void onMarked (marker_info_t&);
	virtual void mouseDown( MouseEvent event );
	virtual void mouseUp( MouseEvent event );
	virtual void mouseDrag( MouseEvent event );
	virtual void keyDown( KeyEvent event );
	
	
	
	void setZoom (vec2);
	vec2 getZoom ();
		void draw_info ();
	
	void                            seekToFrame( const size_t frame );
	void                            seekToStart();
	void                            seekToEnd();
	size_t                          getCurrentFrame() const;
	size_t                          getNumFrames() const;
	double                          getCurrentTime() const;
	double                          getDuration() const;
	time_spec_t						getCurrentTime ();
	
	
private:
	
	mutable boost::filesystem::path mPath;
	
	params::InterfaceGl         mUIParams;	
	bool mMoviePlay;
	bool mMovieLoop;
	void play_pause_button ();
	void loop_no_loop_button ();	
	bool have_movie () const;
	void play ();
	void pause ();
	void clear_movie_params ();
	void loadMovieFile();
	
	void update_log (const std::string& meg = "");
	Rectf get_image_display_rect ();
	
	
	mutable directoryPlayerRef m_Dm;
	std::vector<std::string> m_extension;
	std::string m_anonymous_format;
	double m_fps;
	
	vec2		mMousePos;
	vec2 m_zoom;
	vec2 mScreenSize;
	bool mMouseIsDown;
	bool mMouseIsMoving;
	bool mMouseIsDragging;
	bool mMetaDown;
	
	int mMouseInGraphs; // -1 if not, 0 1 2
	bool mMouseInImage; // if in Image, mMouseInGraph is -1
	
	gl::TextureRef		mTextTexture;
	vec2				mSize;
	Font				mFont;
	std::string			mLog;
	
	
};




#endif
