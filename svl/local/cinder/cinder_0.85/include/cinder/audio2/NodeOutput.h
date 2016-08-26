/*
 Copyright (c) 2014, The Cinder Project

 This code is intended to be used with the Cinder C++ library, http://libcinder.org

 Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this list of conditions and
	the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
	the following disclaimer in the documentation and/or other materials provided with the distribution.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 POSSIBILITY OF SUCH DAMAGE.
*/

#pragma once

#include "cinder/audio2/Node.h"
#include "cinder/audio2/Device.h"

namespace cinder { namespace audio2 {

typedef std::shared_ptr<class NodeOutput>		NodeOutputRef;
typedef std::shared_ptr<class LineOut>			LineOutRef;

//! Base class for Node's that consume an audio signal, for example speakers. It cannot have any outputs.
class NodeOutput : public Node {
  public:
	virtual ~NodeOutput() {}

	virtual size_t getOutputSampleRate()				= 0;
	virtual size_t getOutputFramesPerBlock()			= 0;

	//! Enables clip detection, so that values over \a threshold will be interpreted as a clip (enabled by default).
	//! \note if a clip is detected, the internal buffer will be silenced in order to prevent speaker / ear damage.
	void enableClipDetection( bool enable = true, float threshold = 2 );
	//! Returns whether clip detection is enabled or not.
	bool isClipDetectionEnabled() const		{ return mClipDetectionEnabled; }
	//! Returns the frame of the last buffer clip or 0 if none since the last time this method was called.
	uint64_t getLastClip();
	//! Returns the total number of frames that have already been processed in the dsp loop.
	uint64_t getNumProcessedFrames() const		{ return mNumProcessedFrames; }

  protected:
	NodeOutput( const Format &format = Format() );

	//! Implementations should call this to detect if the internal audio buffer is clipping. Always returns false if clip detection is disabled.
	bool checkNotClipping();
	//! Implementations should call this at the end of each rendering block.
	void postProcess();

	std::atomic<uint64_t>		mNumProcessedFrames, mLastClip;
	bool						mClipDetectionEnabled;
	float						mClipThreshold;

  private:
	// NodeOutput does not have outputs, overridden to assert this method isn't called
	void connect( const NodeRef &output, size_t outputBus, size_t inputBus ) override;

	void incrementFrameCount();
};

//! \brief Interface representing a Node that communicates with a hardware output device. This is typically speakers or a 'line-out' on an audio interface.
//!
//! If number of channels hasn't been specified via Node::Format, defaults to `min( 2, getDevice()->getNumOutputChannels() )`.
class LineOut : public NodeOutput {
  public:
	virtual ~LineOut() {}

	const DeviceRef& getDevice() const		{ return mDevice; }

	size_t getOutputSampleRate() override			{ return getDevice()->getSampleRate(); }
	size_t getOutputFramesPerBlock() override		{ return getDevice()->getFramesPerBlock(); }

	virtual void deviceParamsWillChange();
	virtual void deviceParamsDidChange();

  protected:
	LineOut( const DeviceRef &device, const Format &format = Format() );

	DeviceRef mDevice;

	bool	mWasEnabledBeforeParamsChange;

	signals::scoped_connection mWillChangeConn, mDidChangeConn;
};

} } // namespace cinder::audio2