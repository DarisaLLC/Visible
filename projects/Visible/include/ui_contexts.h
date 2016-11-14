#ifndef __ui_contexts___h
#define __ui_contexts___h

#include "cinder/app/App.h"
#include "cinder/gl/Vbo.h"
#include "cinder/gl/gl.h"
#include "cinder/Rect.h"
#include "cinder/qtime/QuicktimeGl.h"
#include "cinder/CameraUi.h"
#include "cinder/params/Params.h"
#include "cinder/Timeline.h"
#include "cinder/ImageIo.h"
#include "cinder/Thread.h"
#include "cinder/ConcurrentCircularBuffer.h"
#include "cinder/Signals.h"
#include "cinder/Cinder.h"
#include "iclip.hpp"
#include "app_utils.hpp"
#include "timestamp.h"
#include "qtime_frame_cache.hpp"
#include "CinderOpenCV.h"
#include "roiWindow.h"
#include <memory>
#include <utility>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include "cinder/Timeline.h"
#include "AccordionItem.h"

#include <boost/integer_traits.hpp>
// use int64_t instead of long long for better portability
#include <boost/filesystem.hpp>
#include <boost/cstdint.hpp>
#include "core/singleton.hpp"

#include "qtimeAvfLink.h"

using namespace boost;
using namespace boost::filesystem;
using namespace ci;
using namespace ci::app;
using namespace ci::signals;

using namespace params;

using namespace std;
namespace fs = boost::filesystem;

class uContext : public std::enable_shared_from_this<uContext>
{
public:
	
	typedef std::shared_ptr<uContext> uContextRef;
	
	typedef marker_info marker_info_t;
    typedef void (sig_cb_marker) (marker_info_t&);
	
	uContext () : m_valid (false), m_type (null_viewer)
	{
//		m_parent = getRef ();
	}
	
	uContext (const uContextRef& parent)
		: m_valid (false), m_type (null_viewer)
	{
//		if (! parent) m_parent = getRef ();
	}
	

	uContextRef getRef()
	{
		return shared_from_this();
	}
	
	ci::app::WindowRef	getWindowRef () const { return mWindow; }
	void setWindowRef (ci::app::WindowRef& ww) 
	{
		mWindow = ww;
		mWindow->setUserData(this);
	}
	
	
	uContextRef	getContextRef () const;
	void setContextRef (uContextRef& ) const;

	Signal <void(marker_info_t&)> signalMarker;
	
    virtual ~uContext ()
    {
        std::cout << "uContext Base Dtor is called " << std::endl;
    }
    virtual const std::string & name() const;
    virtual void name (const std::string& );
    
    virtual void draw ();
    virtual void update ();
    //    virtual void resize () = 0;
    virtual void setup ();
    virtual bool is_valid ();
    
    // u implementation does nothing
    virtual void mouseDown( MouseEvent event ) {}
    virtual void mouseMove( MouseEvent event ) {}
  	virtual void mouseUp( MouseEvent event ) {}
    virtual void mouseDrag( MouseEvent event ) {}
    
	virtual void keyDown( KeyEvent event ) {}
	void normalize_point (vec2& pos, const ivec2& size)
	{
		pos.x = pos.x / size.x;
		pos.y = pos.y / size.y;
	}
	
    enum Type {
		null_viewer = 0,
        matrix_viewer = null_viewer+1,
        qtime_viewer = matrix_viewer+1,
        clip_viewer = qtime_viewer+1,
		image_dir_viewer = clip_viewer+1,
        viewer_types = clip_viewer+1
    };
	

	Type context_type () const { return m_type; }
	bool is_context_type (const Type t) const { return m_type == t; }
	
  protected:
	Type m_type;
    size_t m_uniqueId;
    bool m_valid;
    mutable ci::app::WindowRef				mWindow;
	mutable uContextRef	                    m_parent;
    ci::signals::ScopedConnection	mCbMouseDown, mCbMouseDrag;
    ci::signals::ScopedConnection	mCbMouseMove, mCbMouseUp;
    ci::signals::ScopedConnection	mCbKeyDown;

	
//	std::shared_ptr<std::thread>		mThread;
//	gl::TextureRef			mTexture, mLastTexture;
//	ConcurrentCircularBuffer<gl::TextureRef>	*mImages;
	

};

class imageDirContext : public uContext
{
public:
	// From just a name, use the browse to folder dialog to get the file
	// From a name and a path
	imageDirContext(const uContextRef& parent = uContextRef () , const boost::filesystem::path& pp = boost::filesystem::path () );

	static const std::string& caption () { static std::string cp ("Image Dir Viewer "); return cp; }
	void loadImageDirectory ( const filesystem::path& directory);
	
	virtual const std::string & name() const { return mName; }
	virtual void name (const std::string& name) { mName = name; }
	virtual void draw ();
	virtual void setup ();
	virtual bool is_valid ();
	virtual void update ();
	virtual void mouseMove( MouseEvent event );
	
private:
	int				mTotalItems;
	
	float			mItemExpandedWidth;
	float			mItemRelaxedWidth;
	float			mItemHeight;
	
	list<AccordionItem>				mItems;
	list<AccordionItem>::iterator	mCurrentSelection;

	boost::filesystem::path mFolderPath;
	std::vector<boost::filesystem::path> mImageFiles;
	std::vector<std::string> mSupportedExtensions;
	std::string mName;
	
	
};




class matContext : public uContext
{
public:
	// From just a name, use the open file dialog to get the file
	// From a name and a path
	matContext(const uContextRef& parent = uContextRef (), const boost::filesystem::path& pp = boost::filesystem::path ());
	
    static const std::string& caption () { static std::string cp ("Smm Viewer # "); return cp; }

    virtual const std::string & name() const { return mName; }
    virtual void name (const std::string& name) { mName = name; }
    virtual void draw ();
    virtual void setup ();
    virtual bool is_valid ();
    virtual void update ();
    virtual void mouseDrag( MouseEvent event );
    virtual void mouseDown( MouseEvent event );
    void draw_window ();
    virtual void onMarked (marker_info_t&);
	
private:

    void internal_setupmat_from_file (const boost::filesystem::path &);
    void internal_setupmat_from_mat (const csv::matf_t &);
    params::InterfaceGl         mMatParams;
    
    gl::VboMeshRef mPointCloud;
    gl::BatchRef	mPointsBatch;

    
    CameraPersp		mCam;
    CameraUi		mCamUi;
    
    boost::filesystem::path mPath;
    std::string mName;
	

};


class movContext : public uContext
{
public:
	// From just a name, use the open file dialog to get the file
	// From a name and a path
	movContext(const uContextRef& parent = uContextRef (), const boost::filesystem::path& pp = boost::filesystem::path () );
	
	Signal <void(bool)> signalShowMotioncenter;
	
    static const std::string& caption () { static std::string cp ("Qtime Viewer # "); return cp; }
    virtual const std::string & name() const { return mName; }
    virtual void name (const std::string& name) { mName = name; }
    virtual void draw ();
    virtual void setup ();
    virtual bool is_valid ();
    virtual void update ();
    void draw_window ();

	virtual void onMarked (marker_info_t&);
    virtual void mouseDown( MouseEvent event );
    virtual void mouseMove( MouseEvent event );
  	virtual void mouseUp( MouseEvent event );
    virtual void mouseDrag( MouseEvent event );
    virtual void keyDown( KeyEvent event );

    const params::InterfaceGl& ui_params ()
    {
        return mMovieParams;
    }
    
	void setShowMotionCenter (bool b)  { mShowMotionCenter = b; }
	bool getShowMotionCenter ()  { return mShowMotionCenter; }
	void setZoom (float);
	float getZoom ();

    int getIndex ();
    
    void setIndex (int mark);
	
	void play_pause_button ();
	
	void loadImagesThreadFn( gl::ContextRef sharedGlContext );
	
	
	// Add tracks
	void add_scalar_track_get_file () { add_scalar_track (); }
	
	void add_scalar_track (const boost::filesystem::path& file = boost::filesystem::path ());
	

	
private:
    void loadMovieFile();
	bool have_movie ();
	void play ();
	void pause ();
	
    void seek( size_t xPos );
    void clear_movie_params ();
    vec2 texture_to_display_zoom ();
    vec2 mScreenSize;
    gl::TextureRef mImage;
    ci::qtime::MovieSurfaceRef m_movie;
    size_t m_fc;
    std::string mName;
    params::InterfaceGl         mMovieParams;
    float mMoviePosition, mPrevMoviePosition;
    size_t mMovieIndexPosition, mPrevMovieIndexPosition;
    float mMovieRate, mPrevMovieRate;
    bool mMoviePlay, mPrevMoviePlay;
    bool mMovieLoop, mPrevMovieLoop;
    vec2 m_zoom;
    Rectf m_display_rect;
    float mMovieCZoom;
    boost::filesystem::path mPath;
	vec2		mMousePos;
	std::shared_ptr<qTimeFrameCache> mFrameSet;
	SurfaceRef  mSurface;
	std::vector<time_spec_t> mTimeHist;
	vec2 mCom;
	vec2 m_prev_com;
	cv::Mat mS;
	cv::Mat mSS;
	roiWindow<P8U> mModel;
	vec2 m_max_motion;
	int64_t m_index;
	bool movie_error_do_flip;
	
    bool mMouseIsDown;
    bool mMouseIsMoving;
    bool mMouseIsDragging;
    bool mMetaDown;
	
	bool mShowMotionCenter;
	std::vector<std::string>  mPlayOrPause = {"Play", "Pause"};
	int mButton_title_index;
	std::string mButton_title;
	
    static size_t Normal2Index (const Rectf& box, const size_t& pos, const size_t& wave)
    {
        size_t xScaled = (pos * wave) / box.getWidth();
		xScaled = svl::math<size_t>::clamp( xScaled, 0, wave );
        return xScaled;
    }
	
	
	std::list<std::shared_ptr<uContext> > m_tracks;
	
	
};


class clipContext : public uContext
{
public:
	
	// From just a name, use the open file dialog to get the file
	// From a name and a path
	clipContext(const uContextRef& parent = uContextRef (), const boost::filesystem::path& pp = boost::filesystem::path ());
	
    static const std::string& caption () { static std::string cp ("Result Clip Viewer # "); return cp; }
    virtual const std::string & name() const { return mName; }
    virtual void name (const std::string& name) { mName = name; }
    virtual void draw ();
    virtual void setup ();
    virtual bool is_valid ();
    virtual void update ();
    virtual void mouseDrag( MouseEvent event );
    virtual void mouseMove( MouseEvent event );
    virtual void mouseDown( MouseEvent event );
    virtual void mouseUp( MouseEvent event );
	void normalize (const bool do_normalize = false){m_normalize = do_normalize; }
	bool normalize_option () const { return m_normalize; }
	
	void draw_window ();
    void receivedEvent ( InteractiveObjectEvent event );
	void loadAll (const csv::matf_t& src);
	
	virtual void onMarked (marker_info_t&);
	
private:
  
    static DataSourcePathRef create (const std::string& fqfn)
    {
        return  DataSourcePath::create (boost::filesystem::path  (fqfn));
    }
    
    void select_column (size_t col) { m_column_select = col; }
    
    params::InterfaceGl         mClipParams;
    Graph1DRef mGraph1D;
    boost::filesystem::path mPath;
    std::string mName;
	csv::matf_t mdat;
    int m_column_select;
	bool m_normalize;
    size_t m_frames, m_file_frames, m_read_pos;
    size_t m_rows, m_columns;
	
};





#endif
