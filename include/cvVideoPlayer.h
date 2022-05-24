
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
#include "cvCaptureProperties.hpp"
#include <map>

using namespace cvProperties;

class cvVideoPlayer
{
public:
    
    typedef std::shared_ptr<cvVideoPlayer> ref;
    typedef std::weak_ptr<cvVideoPlayer> weak_ref_t;
    
    typedef std::shared_ptr<cv::VideoCapture>                VideoCaptureRef;
    static ref create(const ci::fs::path& filepath);
    
	cvVideoPlayer();
	cvVideoPlayer( const cvVideoPlayer& rhs );
	~cvVideoPlayer();

    const std::string& name() const { return mName; }
    const ci::ivec2& size () const;
    const double& format () const;
    
	cvVideoPlayer&			operator=( const cvVideoPlayer& rhs );

	cvVideoPlayer&			loop( bool enabled = true );
	cvVideoPlayer&			pause( bool resume = false );
	cvVideoPlayer&			speed( float v );

	ci::Surface8uRef		createSurface();
    ci::Surface8uRef        createSurface(int idx);
    
	bool					load( const ci::fs::path& filepath );
	void					play();
	void					seek( double seconds );
	bool					seekToFrame( size_t frameNum );
    bool                    seekToRatio( double ratio );
    bool                    seekToTime( double seconds );
	void					stop();
	void					unload();
	bool					update();

	const std::string&		getCodec() const;
	double					getDuration() const;
    double                  getCurrentFrameTime() const;
    size_t                  getCurrentFrameCount() const;
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
    
    const std::map<int, Property>&       properties () const;
    const std::vector<std::string>&      getChannelNames();
    
protected:
	VideoCaptureRef			mCapture		= nullptr;

	std::string				mCodec;
	double					mDuration		= 0.0;
    double                  mFormat;
	ci::fs::path 			mFilePath;
	double					mFrameDuration	= 0.0;
	double					mFrameRate		= 0.0;
	bool					mLoaded			= false;
	bool					mLoop			= true;
	bool					mPlaying		= false;
	uint32_t				mNumFrames		= 0;
	double					mCurrentRatio		= 0.0;
	ci::ivec2				mSize;
	float					mSpeed			= 1.0f;
    std::string                  mName;
    std::vector<std::string>                  mChannelNames;
    std::map<int, Property>              mProperties;
};



#endif

