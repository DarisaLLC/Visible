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
#include <sstream>

#include "core/timestamp.h"
#include "seq_frame_container.hpp"

#include "core/stl_utils.hpp"
#include "timed_types.h"
#include "timeMarker.h"
#include "visible_types.h"
#include "DisplayObjectContainer.h"
#include "gui_base.hpp"



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
		cv_video_viewer = null_viewer+1,
		image_dir_viewer = cv_video_viewer+1,
		lif_file_viewer = image_dir_viewer+1,
		viewer_types = lif_file_viewer+1,
	};
	
	// Signal time point mark to all
	// receive time point mark from all
	typedef marker_info marker_info_t;
    typedef void (sig_cb_marker) (marker_info_t&);
	Signal <void(marker_info_t&)> signalMarker;
	
	
	guiContext (ci::app::WindowRef& ww);
	virtual ~guiContext ();
	
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
};

class mainContext : public guiContext
{
public:
	mainContext(ci::app::WindowRef& ww, const boost::filesystem::path& pp = boost::filesystem::path ());
	void resize () {}
	void draw () {}
	void update () {}
	void setup () {}
};



///////////////////   Visual Browsing Contexts

class sequencedImageContext : public guiContext, public gui_base
{
public:
	
	typedef	 signals::Signal<void( marker_info_t & )>		MarkerSignalInfo_t;
	
	sequencedImageContext(ci::app::WindowRef& ww)
	: guiContext (ww)
	{}
	
	virtual void seekToStart () = 0;
	virtual void seekToEnd () = 0;
	virtual void seekToFrame (int) = 0;
	virtual int getCurrentFrame () = 0;
	virtual time_spec_t getCurrentTime () = 0;
	virtual time_spec_t getStartTime () = 0;
	virtual int getNumFrames () = 0;
	
	virtual void draw_info () = 0;
	virtual void update_log (const std::string& meg = "") = 0;
	virtual bool looping () = 0;
	virtual void looping (bool what) = 0;
	virtual void processDrag( ivec2 pos ) = 0;
	virtual bool have_tracks () const { return m_have_tracks; }
	
	virtual const Rectf& get_image_display_rect () = 0;
	virtual void update_channel_display_rects () = 0;

	// App default startup params
	static ivec2 startup_display_size () { return ivec2( 848, 564 ); }
	
protected:

	std::mutex m_track_mutex;
	
	arrayOfNamedTracks_t m_luminance_tracks;
	vecOfNamedTrack_t m_shortterm_tracks;
	
	async_vecOfNamedTrack_t m_fluorescense_tracks_aync;
	async_vecOfNamedTrack_t m_contraction_pci_tracks_asyn;
	async_vecOfNamedTrack_t m_longterm_pci_tracks_async;
	async_vecOfNamedTrack_t m_roi_longterm_pci_tracks_async;
	
	bool m_have_tracks;
	bool m_show_display_and_controls, m_show_results;
	
	std::vector<size_t> m_spatial_dims;
	std::vector<std::string> m_perform_names;
	int  m_selected_perform_index;

	
	ivec2 m_instant_mouse_image_pos;
	uint32_t m_instant_channel;
	ColorA8u m_instant_pixel_Color;
	UInt8 m_instant_channel_pixel;
	float m_instant_pixel_normalized;
	float m_instant_pixel_Luminance;
	vec2 m_instant_image_display_scale;
	std::vector<Rectf> m_instant_channel_display_rects;
	Rectf m_display_rect;
	
	gl::TextureRef		mTextTexture;
	vec2				mSize;
	Font				mFont;
	std::string			mLog;
	index_time_t    mCurrentIndexTime;
	
	gl::FboRef mFbo, mFbo2;
	gl::GlslProgRef mShader;
	
};




#endif
