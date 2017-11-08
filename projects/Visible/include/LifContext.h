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
    std::shared_ptr<lif_processor> m_lifProcRef;
    
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
	
    void signal_content_loaded ();
    void signal_sm1d_available (int&);
    void signal_sm1dmed_available (int&,int&);
 
    void signal_frame_loaded (int& findex, double& timestamp);
 
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
