
#ifndef _OCV_VIDEO_
#define _OCV_VIDEO_

#pragma once
#include "opencv2/opencv.hpp"
#include "cinder/Cinder.h"
#include "cinder/ImageIo.h"
#include "cinder/Filesystem.h"
#include "cinder/Vector.h"
#include <exception>
#include <chrono>
#include <string>


class cvVideoPlayer
{
public:
    
    typedef std::shared_ptr<cvVideoPlayer> ref;
    typedef std::weak_ptr<cvVideoPlayer> weak_ref;
    
    typedef std::shared_ptr<cv::VideoCapture>                VideoCaptureRef;
    typedef std::chrono::time_point<std::chrono::high_resolution_clock>    time_point_t;

    ref create(const ci::fs::path& filepath);
    
	cvVideoPlayer();
	cvVideoPlayer( const cvVideoPlayer& rhs );
	~cvVideoPlayer();

    
	cvVideoPlayer&			operator=( const cvVideoPlayer& rhs );

	cvVideoPlayer&			loop( bool enabled = true );
	cvVideoPlayer&			pause( bool resume = false );
	cvVideoPlayer&			speed( float v );

	ci::Surface8uRef		createSurface();
	bool					load( const ci::fs::path& filepath );
	void					play();
	void					seek( double seconds );
	void					seekFrame( uint32_t frameNum );
	void					seekPosition( float ratio );
	void					stop();
	void					unload();
	bool					update();

	const std::string&		getCodec() const;
	double					getDuration() const;
	uint32_t				getElapsedFrames() const;	
	double					getElapsedSeconds() const;	
	const ci::fs::path& 	getFilePath() const;
	double					getFrameRate() const;
	uint32_t				getNumFrames() const;
	double					getPosition() const;
	const ci::ivec2&		getSize() const;
	float					getSpeed() const;
	bool					isLoaded() const;
	bool					isLooped() const;
	bool					isPlaying() const;

	void					setLoop( bool enabled );
	void					setPause( bool resume );
	void					setSpeed( float v );
protected:
	VideoCaptureRef			mCapture		= nullptr;
	std::string				mCodec;
	double					mDuration		= 0.0;
	uint32_t				mElapsedFrames	= 0;
	double					mElapsedSeconds	= 0.0;
	ci::fs::path 			mFilePath;
	double					mFrameDuration	= 0.0;
	double					mFrameRate		= 0.0;
	time_point_t				mGrabTime;
	bool					mLoaded			= false;
	bool					mLoop			= true;
	bool					mPlaying		= false;
	uint32_t				mNumFrames		= 0;
	double					mPosition		= 0.0;
	ci::ivec2				mSize;
	float					mSpeed			= 1.0f;
    std::string                  mName;
};


#endif

