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
#include "CinderOpenCV.h"

#include <memory>
#include <utility>
#include <boost/integer_traits.hpp>
// use int64_t instead of long long for better portability
#include <boost/filesystem.hpp>
#include <boost/cstdint.hpp>

#include "app_utils.hpp"
#include "timestamp.h"
#include "qtime_frame_cache.hpp"

#include "graph1d.hpp"
#include "roiWindow.h"
#include "AccordionItem.h"
#include "otherIO/lifFile.hpp"
#include "core/stl_utils.hpp"
#include "async_producer.h"
#include "directoryPlayer.h"
#include "qtimeAvfLink.h"
#include "timeMarker.h"
#include "TinyUi.h"
#include "OcvVideo.h"
#include <sstream>

#include "DisplayObjectContainer.h"
#include "Square.h"
#include "Circle.h"

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
    virtual bool is_valid () const ;

    
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


class imageDirContext : public guiContext
{
public:
	// From just a name, use the browse to folder dialog to get the file
	// From a name and a path
	imageDirContext(ci::app::WindowRef& ww, const boost::filesystem::path& pp = boost::filesystem::path () );
	
	static const std::string& caption () { static std::string cp ("Image Dir Viewer "); return cp; }
	void loadImageDirectory ( const filesystem::path& directory);
	
	virtual void draw ();
	virtual void resize ();
	virtual void setup ();
	virtual bool is_valid ();
	virtual void update ();
	virtual void mouseMove( MouseEvent event );
	
private:
	int				mTotalItems;
	std::pair<int,int> mLarge;
	
	float			mItemExpandedWidth;
	float			mItemRelaxedWidth;
	float			mItemHeight;
	
	csv::matf_t mdat;
	bool m_normalize;
	size_t m_frames, m_file_frames, m_read_pos;
	size_t m_rows, m_columns;
	
	list<AccordionItem>				mItems;
	std::vector<gl::TextureRef>			mTextures;
	std::vector<SurfaceRef>			mSurfaces;
	list<AccordionItem>::iterator	mCurrentSelection;

	boost::filesystem::path mFolderPath;
	std::vector<boost::filesystem::path> mImageFiles;
	std::vector<std::string> mSupportedExtensions;

	
	
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
};


class timelineContext : public guiContext
{
public:
	
	typedef	 signals::Signal<void( marker_info_t & )>		MarkerSignalInfo_t;
	MarkerSignalInfo_t&	getMarkerSignal () { return m_marker_signal; }
	
	// Create one
	timelineContext(const Rectf&); //ci::app::WindowRef& ww, const boost::filesystem::path& pp = boost::filesystem::path ());

	
	static const std::string& caption () { static std::string cp ("Timeline Browser "); return cp; }
	
	virtual void draw ();
	virtual void setup ();
	virtual bool is_valid ();
	virtual void update ();
	virtual void resize ();
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
	
	//	static DataSourcePathRef create (const std::string& fqfn)
	//	{
	//		return  DataSourcePath::create (boost::filesystem::path  (fqfn));
	//	}
	
	params::InterfaceGl         mClipParams;
	Graph1DRef mGraph1D;
	boost::filesystem::path mPath;
	csv::matf_t mdat;
	int m_column_select;
	bool m_normalize;
	size_t m_frames, m_file_frames, m_read_pos;
	size_t m_rows, m_columns;
	MarkerSignalInfo_t m_marker_signal;
};


class clipContext : public guiContext
{
public:
	
	// From just a name, use the open file dialog to get the file
	// From a name and a path
	clipContext( ci::app::WindowRef& ww, const boost::filesystem::path& pp = boost::filesystem::path ());
	
	static const std::string& caption () { static std::string cp ("Result Clip Viewer # "); return cp; }
	virtual void draw ();
	virtual void setup ();
	virtual bool is_valid ();
	virtual void update ();
	virtual void resize ();
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
	
//	static DataSourcePathRef create (const std::string& fqfn)
//	{
//		return  DataSourcePath::create (boost::filesystem::path  (fqfn));
//	}
	
	void select_column (size_t col) { m_column_select = col; }
	
	params::InterfaceGl         mClipParams;
	Graph1DRef mGraph1D;
	boost::filesystem::path mPath;
	csv::matf_t mdat;
	int m_column_select;
	bool m_normalize;
	size_t m_frames, m_file_frames, m_read_pos;
	size_t m_rows, m_columns;
	
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

	
protected:
	TimeLineSlider					mTimeLineSlider;
	TimeLineSlider					mAuxTimeLineSlider;
	vector<Widget *>	mWidgets;

	
	
	std::vector<Graph1DRef> m_plots;
	
	std::mutex m_track_mutex;
	
	vector_of_trackD1s_t m_luminance_tracks;
	async_tracksD1_t m_async_luminance_tracks;
	promised_tracks_t m_promised_tracks;
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
	

	
	
};


///////////////////

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
    virtual bool is_valid ();
    virtual void update ();
	virtual void resize ();	
	void draw_window ();

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

	virtual void draw_info ();
	virtual void update_log (const std::string& meg = "");
	virtual bool looping ();
	virtual void looping (bool what);
	virtual Rectf get_image_display_rect ();
	virtual void processDrag (ivec2 pos);
	
    const params::InterfaceGl& ui_params ()
    {
        return mUIParams;
    }
    
	void setShowMotionCenter (bool b)  { mShowMotionCenter = b; }
	bool getShowMotionCenter ()  { return mShowMotionCenter; }
	
	void setShowMotionBubble (bool b)  { mShowMotionBubble = b; }
	bool getShowMotionBubble ()  { return mShowMotionBubble; }
	
	void setZoom (vec2);
	vec2 getZoom ();

	void play_pause_button ();
	void loop_no_loop_button ();
	bool isMouseDown () { return mMouseIsDown; }
	bool isMouseIsMoving () {return mMouseIsMoving;}
	bool isMouseIsDragging () {return mMouseIsDragging;}
	
	
private:
	const ivec2& imagePos () const { return m_instant_mouse_image_pos; }
	const uint32_t& channelIndex () const { return m_instant_channel; }
	
	gl::TextureRef pixelInfoTexture ();
	void update_instant_image_mouse ();	
    void loadMovieFile();
	bool have_movie ();
	void play ();
	void pause ();
	
    void seek( size_t xPos );
    void clear_movie_params ();
    vec2 texture_to_display_zoom ();
	
	mutable boost::filesystem::path mPath;

	bool m_looping;
    vec2 mScreenSize;
    gl::TextureRef mImage;
    ocvPlayerRef m_movie;
    size_t m_frameCount;
    params::InterfaceGl         mUIParams;
    vec2 m_zoom;
	vec2		mMousePos;
	std::shared_ptr<qTimeFrameCache> mFrameSet;
	SurfaceRef  mSurface;
	std::vector<time_spec_t> mTimeHist;
	vec2 mCom;
	vec2 m_prev_com;
	int64_t m_index;
	bool movie_error_do_flip;
	tiny_media_info      mMediaInfo;	
	
    bool mMouseIsDown;
    bool mMouseIsMoving;
    bool mMouseIsDragging;
    bool mMetaDown;
	
	int mMouseInGraphs; // -1 if not, 0 1 2
	bool mMouseInImage; // if in Image, mMouseInGraph is -1
	ivec2 mMouseInImagePosition;
	
	
	bool mShowMotionCenter, mShowMotionBubble;
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


class movDirContext : public guiContext
{
public:
	// From a name and a path and an optional format for anonymous file names
	movDirContext(ci::app::WindowRef& ww, const boost::filesystem::path& pp = boost::filesystem::path ());
	
	void set_dir_info (const std::vector<std::string>& extension = { ".jpg", ".png", ".JPG", ".jpeg"},
					   double fps=29.97, const std::string anonymous_format = "01234567890abcdefghijklmnopqrstuvwxy");
	
	const  boost::filesystem::path source_path () const;

	virtual bool is_valid ()
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


class lifContext : public sequencedImageContext
{
public:
	
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
	
	const params::InterfaceGl& ui_params ()
	{
		return mUIParams;
	}
	
	const ivec2& imagePos () const { return m_instant_mouse_image_pos; }
	const uint32_t& channelIndex () const { return m_instant_channel; }
	
	void setManualEditMode (bool b)  { mManualEditMode = b; }
	bool getManualEditMode ()  { return mManualEditMode; }
	
	void setShowMotionBubble (bool b)  { mShowMotionBubble = b; }
	bool getShowMotionBubble ()  { return mShowMotionBubble; }
	
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
	gl::TextureRef pixelInfoTexture ();
	void loadLifFile();
	void loadCurrentSerie ();
	bool have_movie ();
	void play ();
	void pause ();
	void update_log (const std::string& meg = "");
	bool looping ();
	void looping (bool what);
	
	Rectf get_image_display_rect ();
	

	void get_series_info (const std::shared_ptr<lifIO::LifReader>& lifer)
	{
		m_series_book.clear ();
		for (unsigned ss = 0; ss < lifer->getNbSeries(); ss++)
		{
			series_info si;
			si.name = lifer->getSerie(ss).getName();
			si.timesteps = lifer->getSerie(ss).getNbTimeSteps();
			si.pixelsInOneTimestep = lifer->getSerie(ss).getNbPixelsInOneTimeStep();
			si.dimensions = lifer->getSerie(ss).getSpatialDimensions();
			si.channelCount = lifer->getSerie(ss).getChannels().size();
			si.channels.clear ();
			for (lifIO::ChannelData cda : lifer->getSerie(ss).getChannels())
			{
				si.channel_names.push_back(cda.getName());
				si.channels.emplace_back(cda);
			}
			
			// Get timestamps in to time_spec_t and store it in info
			si.timeSpecs.resize (lifer->getSerie(ss).getTimestamps().size());
			
			// Adjust sizes based on the number of bytes
			std::transform(lifer->getSerie(ss).getTimestamps().begin(), lifer->getSerie(ss).getTimestamps().end(),
						   si.timeSpecs.begin(), [](lifIO::LifSerie::timestamp_t ts) { return time_spec_t ( ts / 10000.0); });
			
			si.length_in_seconds = lifer->getSerie(ss).total_duration ();
			
			std::cout << si << std::endl;
			
			m_series_book.emplace_back (si);
		}
	}
	
	
	std::shared_ptr<lifIO::LifReader> m_lifRef;
	std::vector<series_info> m_series_book;
    std::vector<std::string> m_series_names;
	std::shared_ptr<lifIO::LifSerie> m_current_serie_ref;
	int  m_selected_serie_index;
	
	void seek( size_t xPos );
	void clear_movie_params ();
	vec2 texture_to_display_zoom ();
	void update_instant_image_mouse ();
	
	std::pair<vec2,vec2> mLengthPoints;
	vec2 mScreenSize;
	gl::TextureRef mImage;
	series_info m_serie;
	
	size_t m_frameCount;
	params::InterfaceGl         mUIParams;
	tiny_media_info      mMediaInfo;
	
	int64_t m_seek_position;

	bool m_is_playing, m_is_looping;
	uint32_t m_cutoff_pct;
	int mAuxTimeSliderIndex;
	
	
	vec2 m_zoom;
	boost::filesystem::path mPath;
	vec2		mMousePos;
	std::shared_ptr<qTimeFrameCache> mFrameSet;
	SurfaceRef  mSurface;
	
	bool mMouseIsDown;
	bool mMouseIsMoving;
	bool mMouseIsDragging;
	bool mMetaDown;
	
	int mMouseInGraphs; // -1 if not, 0 1 2
	bool mMouseInImage; // if in Image, mMouseInGraph is -1
	bool mMouseInTimeLine;
	bool mAuxMouseInTimeLine;
	
	ivec2 mMouseInImagePosition;
	
	bool mManualEditMode, mShowMotionBubble;
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
