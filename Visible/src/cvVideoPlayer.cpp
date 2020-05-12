
#include "cvVideoPlayer.h"
#include "cinder_cv/cinder_opencv.h"
using namespace ci;
using namespace std;

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
        dref->mChannelNames = {"Blue", "Green", "Red", "Alpha"};
		dref->mLoaded = true;
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
