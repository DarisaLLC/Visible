
#include "cvVideoPlayer.h"
#include "cinder_cv/cinder_opencv.h"
#include <sstream>
#include <iostream>

using namespace ci;
using namespace std;
using namespace cvProperties;

cvVideoPlayer::cvVideoPlayer()
: mCodec( "" ), mFilePath( "" ), mSize( ivec2( 0 ) )
{
}

cvVideoPlayer::cvVideoPlayer( const cvVideoPlayer& rhs )
{
	*this = rhs;
}

cvVideoPlayer::~cvVideoPlayer()
{
	unload();
}

cvVideoPlayer& cvVideoPlayer::operator=( const cvVideoPlayer& rhs )
{
	mCapture		= rhs.mCapture;
	mCodec			= rhs.mCodec;
	mDuration		= rhs.mDuration;
	mFilePath		= rhs.mFilePath;
	mFrameDuration	= rhs.mFrameDuration;
	mFrameRate		= rhs.mFrameRate;
	mLoaded			= rhs.mLoaded;
	mLoop			= rhs.mLoop;
	mPlaying		= rhs.mPlaying;
	mNumFrames		= rhs.mNumFrames;
	mSize			= rhs.mSize;
	mSpeed			= rhs.mSpeed;
	return *this;
}

cvVideoPlayer& cvVideoPlayer::loop( bool enabled )
{
	setLoop( enabled );
	return *this;
}

cvVideoPlayer& cvVideoPlayer::pause( bool resume )
{
	setPause( resume );
	return *this;
}

cvVideoPlayer& cvVideoPlayer::speed( float v )
{
	setSpeed( v );
	return *this;
}

Surface8uRef cvVideoPlayer::createSurface()
{
	if ( mLoaded ) {
		cv::Mat frame;
        *mCapture >> frame;
		if ( ! frame.empty() )
			return Surface8u::create( fromOcv( frame ) );
	}
	return nullptr;
}

Surface8uRef cvVideoPlayer::createSurface(int idx)
{
    if ( mLoaded ) {
        cv::Mat frame;
        if ( mCapture->retrieve( frame , idx) ) {
            return Surface8u::create( fromOcv( frame ) );
        }
    }
    return nullptr;
}

const std::vector<std::string>& cvVideoPlayer::getChannelNames()  {
    if(! mChannelNames.empty()) return mChannelNames;
    static std::map<int, std::string> order_map = { {ci::SurfaceChannelOrder::RGBA,"RGBA"},{ci::SurfaceChannelOrder::RGB,"RGB"},{ci::SurfaceChannelOrder::BGRA,"BGRA"},{ci::SurfaceChannelOrder::BGR,"BGR"},{ci::SurfaceChannelOrder::ARGB,"ARGB"},{ci::SurfaceChannelOrder::ABGR,"ABGR"}  };

    stringstream msg;
    if(mLoaded){
        seekToFrame(0);
        Surface8uRef su8 = createSurface();
        seekToFrame(0);
        ci::SurfaceChannelOrder order = su8->getChannelOrder();
        int channel_count = order.hasAlpha() ? 4 : 3;
        if (order_map.find(order.getCode()) == order_map.end()){
            for (auto cc = 0; cc < channel_count; cc++)
                mChannelNames.push_back(std::to_string(cc));
        }else{
            auto order_string = order_map[order.getCode()];
            assert(channel_count == order_string.length());
            for (auto cc = 0; cc < channel_count; cc++)
                   mChannelNames.emplace_back(1, order_string[cc]);
        }
    }
    return mChannelNames;
}

/**
double dWidth = cap.get(CV_CAP_PROP_FRAME_WIDTH); //get the width of frames of the video
double dHeight = cap.get(CV_CAP_PROP_FRAME_HEIGHT); //get the height of frames of the video
double dPositionMS = cap.get(CV_CAP_PROP_POS_MSEC); //Current position of the video file in milliseconds
double dFrameIndex = cap.get(CV_CAP_PROP_POS_FRAMES); //0-based index of the frame to be decoded/captured next
double dPositionVideo = cap.get(CV_CAP_PROP_POS_AVI_RATIO); //Relative position of the video file: 0 - start of the film, 1 - end of the film
double dFPS = cap.get(CV_CAP_PROP_FPS); //Frame rate
double dCodec = cap.get(CV_CAP_PROP_FOURCC); //4-character code of codec
double dFrameCount = cap.get(CV_CAP_PROP_FRAME_COUNT); //Number of frames in the video file
double dFormat = cap.get(CV_CAP_PROP_FORMAT); //Format of the Mat objects returned by retrieve()
**/

cvVideoPlayer::ref cvVideoPlayer::create( const fs::path& filepath )
{
    ref dref = std::make_shared<cvVideoPlayer>();
    
    if(!exists(filepath)) return dref;
    fs::path filename = filepath.has_filename() ? filepath.filename() :
    filepath.has_stem() ? filepath.stem() : filepath;
    dref->mName = filename.string();
	if ( dref->mCapture == nullptr ) {
		dref->mCapture = VideoCaptureRef( new cv::VideoCapture() );
	}
	if ( dref->mCapture != nullptr && dref->mCapture->open( filepath.string() ) && dref->mCapture->isOpened() ) {
        int32_t cc		= static_cast<int32_t>( dref->mCapture->get( cv::CAP_PROP_FOURCC ) );
		char codec[]	= {
			(char)(		cc & 0X000000FF ), 
			(char)( (	cc & 0X0000FF00 ) >> 8 ),
			(char)( (	cc & 0X00FF0000 ) >> 16 ),
			(char)( (	cc & 0XFF000000 ) >> 24 ),
			0 };
		dref->mCodec			= string( codec );

		dref->mFilePath	= filepath;
        dref->mFrameRate	= dref->mCapture->get( cv::CAP_PROP_FPS );
        dref->mNumFrames	= (uint32_t)dref->mCapture->get( cv::CAP_PROP_FRAME_COUNT );
        dref->mSize.x		= (int32_t)dref->mCapture->get( cv::CAP_PROP_FRAME_WIDTH );
        dref->mSize.y		= (int32_t)dref->mCapture->get( cv::CAP_PROP_FRAME_HEIGHT );

		if ( dref->mFrameRate > 0.0 ) {
			dref->mDuration		= (double) dref->mNumFrames / dref->mFrameRate;
			dref->mFrameDuration	= 1.0 / dref->mFrameRate;
		}
		dref->mLoaded = true;
        dref->mProperties = cvProperties::Helper::GetAll(*dref->mCapture);
        dref->getChannelNames();
	}
    return dref;
}

void cvVideoPlayer::play()
{
	if ( mLoaded ) {
		stop();
		mPlaying	= true;
	}
}

bool cvVideoPlayer::seekToTime( double seconds )
{
    assert( mCapture != nullptr && mLoaded );
    double millis	= math<double>::clamp( seconds, 0.0, mDuration ) * 1000.0;
    return mCapture->set( cv::CAP_PROP_POS_MSEC, millis );
}

bool cvVideoPlayer::seekToFrame(size_t frameNum )
{
    assert( mCapture != nullptr && mLoaded );
    auto count = frameNum % mNumFrames;
	return mCapture->set(cv::CAP_PROP_POS_FRAMES, static_cast<double>(count));
}

bool cvVideoPlayer::seekToRatio( double ratio )
{
    assert( mCapture != nullptr && mLoaded );
    assert(ratio >= 0.0 && ratio <= 1.0);
    const size_t frameIdx = static_cast<size_t>(std::floor(ratio * mNumFrames));
    return seekToFrame(frameIdx);
}

void cvVideoPlayer::stop()
{
	pause();
	seekToTime( 0.0 );
}

void cvVideoPlayer::unload()
{
	stop();
	if ( mCapture != nullptr && mLoaded ) {
		mCapture->release();
		mCapture.reset();
		mCapture = nullptr;
	}
	mCodec			= "";
	mFilePath		= fs::path( "" );
	mFrameDuration	= 0.0;
	mFrameRate		= 0.0;
	mLoaded			= false;
	mNumFrames		= 0;
	mSize			= ivec2( 0 );
}

bool cvVideoPlayer::update()
{
	if ( mCapture != nullptr && mLoaded && mPlaying && mNumFrames > 0 && mDuration > 0.0 ) {
        auto current = getCurrentFrameCount();
		bool loop = mLoop && current == mNumFrames - 1;
        if ( loop ) return seekToTime( 0.0 );
        return seekToFrame(current+1);
	}
	return false;
}

const std::map<int, Property> &  cvVideoPlayer::properties () const { return mProperties; }

const string& cvVideoPlayer::getCodec() const
{
	return mCodec;
}

double cvVideoPlayer::getDuration() const
{
	return mDuration;
}

size_t cvVideoPlayer::getCurrentFrameCount() const
{
    return static_cast<size_t>(mCapture->get(cv::CAP_PROP_POS_FRAMES));
}
 
double cvVideoPlayer::getCurrentFrameTime() const
{
	return mCapture->get(cv::CAP_PROP_POS_MSEC);
}
 
const fs::path& cvVideoPlayer::getFilePath() const
{
	return mFilePath;
}

double cvVideoPlayer::getFrameRate() const
{
	return mFrameRate;
}

uint32_t cvVideoPlayer::getNumFrames() const
{
	return mNumFrames;
}

const ivec2& cvVideoPlayer::getSize() const
{
	return mSize;
}

float cvVideoPlayer::getSpeed() const
{
	return mSpeed;
}

bool cvVideoPlayer::isLoaded() const
{
	return mLoaded;
}

bool cvVideoPlayer::isLooped() const
{
	return mLoop;
}

bool cvVideoPlayer::isPlaying() const
{
	return mPlaying;
}

void cvVideoPlayer::setLoop( bool enabled )
{
	mLoop = enabled;
}

void cvVideoPlayer::setPause( bool resume )
{
	mPlaying = resume;
}

void cvVideoPlayer::setSpeed( float v )
{
	if ( v <= 0.0f ) {
		v = 1.0f;
	}
	mSpeed = v;
}


// ------------------------------ Opencv explicitation --------------------------------
   const std::vector<int> cvProperties::GenerateAllPropertiesId() {
       return std::vector<int> {
           cv::CAP_PROP_POS_MSEC,         // Current position of the video file in milliseconds.
           cv::CAP_PROP_POS_FRAMES,     // 0-based index of the frame to be decoded/captured next.
           cv::CAP_PROP_POS_AVI_RATIO,    // Relative position of the video file: 0 - start of the film, 1 - end of the film.
           cv::CAP_PROP_FRAME_WIDTH,     // Width of the frames in the video stream.
           cv::CAP_PROP_FRAME_HEIGHT,     // Height of the frames in the video stream.
           cv::CAP_PROP_FPS,             // Frame rate.
           cv::CAP_PROP_FOURCC,         // 4-character code of codec.
           cv::CAP_PROP_FRAME_COUNT,     // Number of frames in the video file.
           cv::CAP_PROP_FORMAT,         // Format of the Mat objects returned by retrieve() .
           cv::CAP_PROP_MODE,             // Backend-specific value indicating the current capture mode.
           cv::CAP_PROP_BRIGHTNESS,     // Brightness of the image (only for cameras).
           cv::CAP_PROP_CONTRAST,         // Contrast of the image (only for cameras).
           cv::CAP_PROP_SATURATION,     // Saturation of the image (only for cameras).
           cv::CAP_PROP_HUE,             // Hue of the image (only for cameras).
           cv::CAP_PROP_GAIN,             // Gain of the image (only for cameras).
           cv::CAP_PROP_EXPOSURE,         // Exposure (only for cameras).
           cv::CAP_PROP_CONVERT_RGB,     // Boolean flags indicating whether images should be converted to RGB.
           cv::CAP_PROP_WHITE_BALANCE_BLUE_U,
           cv::CAP_PROP_RECTIFICATION,    // Rectification flag for stereo cameras (note: only supported by DC1394 v 2.x backend currently)
           cv::CAP_PROP_MONOCHROME,
           cv::CAP_PROP_SHARPNESS,
           cv::CAP_PROP_AUTO_EXPOSURE,
           cv::CAP_PROP_GAMMA,
           cv::CAP_PROP_TEMPERATURE,
           cv::CAP_PROP_TRIGGER,
           cv::CAP_PROP_TRIGGER_DELAY,
           cv::CAP_PROP_WHITE_BALANCE_RED_V,
           cv::CAP_PROP_ZOOM,
           cv::CAP_PROP_FOCUS,
           cv::CAP_PROP_GUID,
           cv::CAP_PROP_ISO_SPEED,
           cv::CAP_PROP_BACKLIGHT,
           cv::CAP_PROP_PAN,
           cv::CAP_PROP_TILT,
           cv::CAP_PROP_ROLL,
           cv::CAP_PROP_IRIS,
           cv::CAP_PROP_SETTINGS,
           cv::CAP_PROP_BUFFERSIZE,
           cv::CAP_PROP_AUTOFOCUS,
           cv::CAP_PROP_CHANNEL
       };
   }
   const std::string cvProperties::GetPropertyName(int id) {
       switch(id) {
           case cv::CAP_PROP_POS_MSEC:         return "CAP_PROP_POS_MSEC";
           case cv::CAP_PROP_POS_FRAMES:         return "CAP_PROP_POS_FRAMES";
           case cv::CAP_PROP_POS_AVI_RATIO:     return "CAP_PROP_POS_AVI_RATIO";
           case cv::CAP_PROP_FRAME_WIDTH:         return "CAP_PROP_FRAME_WIDTH";
           case cv::CAP_PROP_FRAME_HEIGHT:     return "CAP_PROP_FRAME_HEIGHT";
           case cv::CAP_PROP_FPS:                 return "CAP_PROP_FPS";
           case cv::CAP_PROP_FOURCC:             return "CAP_PROP_FOURCC";
           case cv::CAP_PROP_FRAME_COUNT:         return "CAP_PROP_FRAME_COUNT";
           case cv::CAP_PROP_FORMAT:             return "CAP_PROP_FORMAT";
           case cv::CAP_PROP_MODE:             return "CAP_PROP_MODE";
           case cv::CAP_PROP_BRIGHTNESS:         return "CAP_PROP_BRIGHTNESS";
           case cv::CAP_PROP_CONTRAST:         return "CAP_PROP_CONTRAST";
           case cv::CAP_PROP_SATURATION:         return "CAP_PROP_SATURATION";
           case cv::CAP_PROP_HUE:                 return "CAP_PROP_HUE";
           case cv::CAP_PROP_GAIN:             return "CAP_PROP_GAIN";
           case cv::CAP_PROP_EXPOSURE:         return "CAP_PROP_EXPOSURE";
           case cv::CAP_PROP_CONVERT_RGB:         return "CAP_PROP_CONVERT_RGB";
           case cv::CAP_PROP_WHITE_BALANCE_BLUE_U: return "CAP_PROP_WHITE_BALANCE_BLUE_U";
           case cv::CAP_PROP_RECTIFICATION:     return "CAP_PROP_RECTIFICATION";
           case cv::CAP_PROP_MONOCHROME:        return "CAP_PROP_MONOCHROME";
           case cv::CAP_PROP_SHARPNESS:         return "CAP_PROP_SHARPNESS";
           case cv::CAP_PROP_AUTO_EXPOSURE:     return "CAP_PROP_AUTO_EXPOSURE";
           case cv::CAP_PROP_GAMMA:             return "CAP_PROP_GAMMA";
           case cv::CAP_PROP_TEMPERATURE:         return "CAP_PROP_TEMPERATURE";
           case cv::CAP_PROP_TRIGGER:             return "CAP_PROP_TRIGGER";
           case cv::CAP_PROP_TRIGGER_DELAY:     return "CAP_PROP_TRIGGER_DELAY";
           case cv::CAP_PROP_WHITE_BALANCE_RED_V: return "CAP_PROP_WHITE_BALANCE_RED_V";
           case cv::CAP_PROP_ZOOM:             return "CAP_PROP_ZOOM";
           case cv::CAP_PROP_FOCUS:             return "CAP_PROP_FOCUS";
           case cv::CAP_PROP_GUID:             return "CAP_PROP_GUID";
           case cv::CAP_PROP_ISO_SPEED:         return "CAP_PROP_ISO_SPEED";
           case cv::CAP_PROP_BACKLIGHT:         return "CAP_PROP_BACKLIGHT";
           case cv::CAP_PROP_PAN:                 return "CAP_PROP_PAN";
           case cv::CAP_PROP_TILT:             return "CAP_PROP_TILT";
           case cv::CAP_PROP_ROLL:             return "CAP_PROP_ROLL";
           case cv::CAP_PROP_IRIS:             return "CAP_PROP_IRIS";
           case cv::CAP_PROP_SETTINGS:         return "CAP_PROP_SETTINGS";
           case cv::CAP_PROP_BUFFERSIZE:         return "CAP_PROP_BUFFERSIZE";
           case cv::CAP_PROP_AUTOFOCUS:         return "CAP_PROP_AUTOFOCUS";
 
           default: return "";
       }
   }
   
Property Helper::Get(const cv::VideoCapture& cap, int property_id) {
         Property prop;
         prop.id     = property_id;
         prop.value     = cap.get(property_id);
         
         return prop;
     }
     bool Helper::Set(cv::VideoCapture& cap, const Property& prop) {
         return cap.set(prop.id, prop.value);
     }
     
     std::map<int, Property> Helper::GetAll(const cv::VideoCapture& cap) {
         std::map<int, Property> mappedProp;
         for(int idProp : GenerateAllPropertiesId())
             mappedProp[idProp] = Get(cap, idProp);
         
         return mappedProp;
     }
