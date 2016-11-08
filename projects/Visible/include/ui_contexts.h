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


class uContext;

/*
 *  Need data producer interface.
 *  It requires a setup for setting length and scale & callback providing it
 *     The need here is for a time stamped string of std::tuple or a variant
 *
 *
 *
 *
 *
 */

class viewCentral : internal_singleton<viewCentral>
{
	std::vector<std::shared_ptr<uContext> > mContexts;
	
};


class uContext
{
public:
	

    typedef void (sig_cb_marker) (uint32_t);
	
    uContext (ci::app::WindowRef window) : mWindow (window)
    {
       mSelected = false;
       mWindow->setUserData(this);
       int m_unique_id = getNumWindows();
       mWindow->getSignalClose().connect([m_unique_id,this] { std::cout << "You closed window #" << m_unique_id << std::endl; });
       m_uniqueId = m_unique_id;
        
    }

	Signal <void(  uint32_t )> signalMarker;
	
    virtual ~uContext ()
    {
        std::cout << "uContext Base Dtor is called " << std::endl;
    }
    virtual const std::string & name() const = 0;
    virtual void name (const std::string& ) = 0;
    
    virtual void draw () = 0;
    virtual void update () = 0;
    //    virtual void resize () = 0;
    virtual void setup () = 0;
    virtual bool is_valid () = 0;
    
    // u implementation does nothing
    virtual void mouseDown( MouseEvent event ) {}
    virtual void mouseMove( MouseEvent event ) {}
  	virtual void mouseUp( MouseEvent event ) {}
    virtual void mouseDrag( MouseEvent event ) {}
    
	virtual void keyDown( KeyEvent event ) {}
	void normalize_point (vec2& pos, const ivec2& size)
	{
		pos.x *= (getWindowWidth () ) / size.x;
		pos.y *= (getWindowHeight () ) / size.y;
	}
	
    enum Type {
        matrix_viewer = 0,
        qtime_viewer = matrix_viewer+1,
        clip_viewer = qtime_viewer+1,
        viewer_types = clip_viewer+1
    };
    
//	virtual void onMarked (  uint32_t );

	Type context_type () const { return mType; }
	bool is_context_type (Type t) const { return mType == t; }
	
  protected:
    int m_uniqueId;
    bool m_valid;
    bool			mSelected;
    ci::app::WindowRef				mWindow;
    ci::signals::ScopedConnection	mCbMouseDown, mCbMouseDrag;
    ci::signals::ScopedConnection	mCbMouseMove, mCbMouseUp;
    ci::signals::ScopedConnection	mCbKeyDown;
	
	bool					mShouldQuit;
	std::shared_ptr<std::thread>		mThread;
	gl::TextureRef			mTexture, mLastTexture;
	ConcurrentCircularBuffer<gl::TextureRef>	*mImages;
	
	double					mLastTime;
	
	Type mType;

};





class matContext : public uContext
{
public:
    matContext(ci::app::WindowRef window) : uContext(window)
    {
		mType = Type::matrix_viewer;
		
       mCbMouseDown = mWindow->getSignalMouseDown().connect( std::bind( &matContext::mouseDown, this, std::placeholders::_1 ) );
       mCbMouseDrag = mWindow->getSignalMouseDrag().connect( std::bind( &matContext::mouseDrag, this, std::placeholders::_1 ) );
       mCbKeyDown = mWindow->getSignalKeyDown().connect( std::bind( &matContext::keyDown, this, std::placeholders::_1 ) );
       mWindow->setTitle (matContext::caption ());
        setup ();
    }
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
    virtual void onMarked ( uint32_t);
	
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
    movContext(ci::app::WindowRef window) : uContext(window)
    {
        mCbMouseDown = mWindow->getSignalMouseDown().connect( std::bind( &movContext::mouseDown, this, std::placeholders::_1 ) );
        mCbMouseDrag = mWindow->getSignalMouseDrag().connect( std::bind( &movContext::mouseDrag, this, std::placeholders::_1 ) );
        mCbMouseUp = mWindow->getSignalMouseUp().connect( std::bind( &movContext::mouseUp, this, std::placeholders::_1 ) );
        mCbMouseMove = mWindow->getSignalMouseMove().connect( std::bind( &movContext::mouseMove, this, std::placeholders::_1 ) );
        mCbKeyDown = mWindow->getSignalKeyDown().connect( std::bind( &movContext::keyDown, this, std::placeholders::_1 ) );
        
        mWindow->setTitle (movContext::caption ());
        setup ();
    }
    
    static const std::string& caption () { static std::string cp ("Qtime Viewer # "); return cp; }
    virtual const std::string & name() const { return mName; }
    virtual void name (const std::string& name) { mName = name; }
    virtual void draw ();
    virtual void setup ();
    virtual bool is_valid ();
    virtual void update ();
    void draw_window ();

	virtual void onMarked ( uint32_t);
    

    virtual void mouseDown( MouseEvent event );
    virtual void mouseMove( MouseEvent event );
  	virtual void mouseUp( MouseEvent event );
    virtual void mouseDrag( MouseEvent event );
    virtual void keyDown( KeyEvent event );

    const params::InterfaceGl& ui_params ()
    {
        return mMovieParams;
    }
    
    

    int getIndex ();
    
    void setIndex (int mark);
	
	void loadImagesThreadFn( gl::ContextRef sharedGlContext );	
    
private:
    void handleTime( vec2 pos );
    void loadMovieFile( const boost::filesystem::path &moviePath );
    bool have_movie () { return m_movie_valid; }
    void seek( size_t xPos );
    void clear_movie_params ();
    vec2 texture_to_display_zoom ();
    vec2 mScreenSize;
    gl::TextureRef mImage;
    ci::qtime::MovieSurfaceRef m_movie;
    bool m_movie_valid;
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

    
    static size_t Normal2Index (const Rectf& box, const size_t& pos, const size_t& wave)
    {
        size_t xScaled = (pos * wave) / box.getWidth();
		xScaled = svl::math<size_t>::clamp( xScaled, 0, wave );
        return xScaled;
    }
    
};


class clipContext : public uContext
{
public:
	

    clipContext(ci::app::WindowRef window) : uContext(window)
    {
        mCbMouseDown = mWindow->getSignalMouseDown().connect( std::bind( &clipContext::mouseDown, this, std::placeholders::_1 ) );
        mCbMouseDrag = mWindow->getSignalMouseDrag().connect( std::bind( &clipContext::mouseDrag, this, std::placeholders::_1 ) );
        mCbMouseUp = mWindow->getSignalMouseUp().connect( std::bind( &clipContext::mouseUp, this, std::placeholders::_1 ) );
        mCbMouseDrag = mWindow->getSignalMouseMove().connect( std::bind( &clipContext::mouseMove, this, std::placeholders::_1 ) );
        mCbKeyDown = mWindow->getSignalKeyDown().connect( std::bind( &clipContext::keyDown, this, std::placeholders::_1 ) );
        
        mWindow->setTitle (clipContext::caption ());
        setup ();
    }
    
    clipContext(const std::string& name);
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
	
	virtual void onMarked ( uint32_t);
	
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
