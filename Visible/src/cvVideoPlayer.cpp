
#include "cvVideoPlayer.h"
#include "cinder_opencv.h"
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

const ci::ivec2& cvVideoPlayer::size () const { return mSize; }
const double& cvVideoPlayer::format () const { return mFormat; }


cvVideoPlayer& cvVideoPlayer::operator=( const cvVideoPlayer& rhs )
{
	mCapture		= rhs.mCapture;
	mCodec			= rhs.mCodec;
	mDuration		= rhs.mDuration;
	mElapsedFrames	= rhs.mElapsedFrames;
	mElapsedSeconds	= rhs.mElapsedSeconds;
	mFilePath		= rhs.mFilePath;
	mFrameDuration	= rhs.mFrameDuration;
	mFrameRate		= rhs.mFrameRate;
	mGrabTime		= rhs.mGrabTime;
	mLoaded			= rhs.mLoaded;
	mLoop			= rhs.mLoop;
	mPlaying		= rhs.mPlaying;
	mNumFrames		= rhs.mNumFrames;
	mCurrentRatio	= rhs.mCurrentRatio;
	mSize			= rhs.mSize;
	mSpeed			= rhs.mSpeed;
    mName           = rhs.mName;
    mChannelNames   = rhs.mChannelNames;
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
		int32_t cc		= static_cast<int32_t>( dref->mCapture->get( CV_CAP_PROP_FOURCC ) );
		char codec[]	= {
			(char)(		cc & 0X000000FF ), 
			(char)( (	cc & 0X0000FF00 ) >> 8 ),
			(char)( (	cc & 0X00FF0000 ) >> 16 ),
			(char)( (	cc & 0XFF000000 ) >> 24 ),
			0 };
		dref->mCodec			= string( codec );

		dref->mFilePath	= filepath;
		dref->mFrameRate	= dref->mCapture->get( CV_CAP_PROP_FPS );
		dref->mNumFrames	= (uint32_t)dref->mCapture->get( CV_CAP_PROP_FRAME_COUNT );
		dref->mSize.x		= (int32_t)dref->mCapture->get( CV_CAP_PROP_FRAME_WIDTH );
		dref->mSize.y		= (int32_t)dref->mCapture->get( CV_CAP_PROP_FRAME_HEIGHT );
        dref->mChannelNames = {"Blue", "Green", "Red", "Alpha"};

        dref->mLoaded = dref->mFrameRate > 0.0;
		if (dref->mLoaded ) {
			dref->mDuration		= (double) dref->mNumFrames / dref->mFrameRate;
			dref->mFrameDuration	= 1.0 / dref->mFrameRate;
		}

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

void cvVideoPlayer::seek( double seconds )
{
	if ( mCapture != nullptr && mLoaded ) {
		double millis	= math<double>::clamp( seconds, 0.0, mDuration ) * 1000.0;
		mCapture->set( CV_CAP_PROP_POS_MSEC, millis );
		mGrabTime		= chrono::high_resolution_clock::now();
		mCurrentRatio		= millis / mDuration;
		mElapsedFrames	= (uint32_t)( mCurrentRatio * (double)mNumFrames );
		mElapsedSeconds	= millis * 0.001;
	}
}

void cvVideoPlayer::seekFrame( uint32_t frameNum )
{
	seek( (double)frameNum * mFrameDuration );
}

void cvVideoPlayer::seekTime( float ratio )
{
	seek( (double)ratio * mDuration );
}

void cvVideoPlayer::stop()
{
	pause();
	seek( 0.0 );
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
		auto now				= chrono::high_resolution_clock::now();
		double d				= chrono::duration_cast<chrono::duration<double>>( now - mGrabTime ).count();
		double nextFrame		= mCapture->get( CV_CAP_PROP_POS_FRAMES );
		bool loop = mLoop && (uint32_t)nextFrame == mNumFrames - 1;
		if ( d >= mFrameDuration / mSpeed && mCapture->grab() ) {
			mElapsedFrames		= (uint32_t)nextFrame;
			mElapsedSeconds		= mCapture->get( CV_CAP_PROP_POS_MSEC ) * 0.001;
			mGrabTime			= now;
			mCurrentRatio			= mCapture->get( CV_CAP_PROP_POS_AVI_RATIO );
			if ( loop ) {
				seek( 0.0 );
			}
			return true;
		}
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

uint32_t cvVideoPlayer::getElapsedFrames() const
{
	return mElapsedFrames;
}
 
double cvVideoPlayer::getElapsedSeconds() const
{
	return mElapsedSeconds;
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

double cvVideoPlayer::getPosition() const
{
	return mCurrentRatio;
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
