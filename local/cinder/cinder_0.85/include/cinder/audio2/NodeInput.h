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
#include "cinder/audio2/Param.h"
#include "cinder/audio2/Device.h"

namespace cinder { namespace audio2 {

typedef std::shared_ptr<class NodeInput>				NodeInputRef;
typedef std::shared_ptr<class LineIn>					LineInRef;
typedef std::shared_ptr<class CallbackProcessor>		CallbackProcessorRef;

//! \brief NodeInput is the base class for Node's that produce audio. It cannot have any inputs.
//!
//! In general, you must call start() before a NodeInput will process audio. \see Node::Format::autoEnable()
class NodeInput : public Node {
  public:
	virtual ~NodeInput();

  protected:
	NodeInput( const Format &format );
  private:
	// NodeInput's cannot have any sources, overridden to assert this method isn't called
	void connectInput( const NodeRef &input, size_t bus ) override;
};

//! \brief Interface representing a Node that communicates with a hardware input device. This is typically a microphone or a 'line-in' on an audio interface.
//!
//! If number of channels hasn't been specified via Node::Format, defaults to `min( 2, getDevice()->getNumInputChannels() )`.
class LineIn : public NodeInput {
  public:
	virtual ~LineIn();

	//! Returns the associated \a Device.
	const DeviceRef& getDevice() const		{ return mDevice; }

	//! Returns the frame of the last buffer underrun or 0 if none since the last time this method was called.
	uint64_t getLastUnderrun() const;
	//! Returns the frame of the last buffer overrun or 0 if none since the last time this method was called.
	uint64_t getLastOverrun() const;

  protected:
	LineIn( const DeviceRef &device, const Format &format );

	//! Should be called by platform-specific implementations that detect an under-run.
	void markUnderrun();
	//! Should be called by platform-specific implementations that detect an over-run.
	void markOverrun();

  private:
	DeviceRef						mDevice;
	mutable std::atomic<uint64_t>	mLastOverrun, mLastUnderrun;
};

//! Callback used to allow simple audio processing without subclassing a Node. First parameter is the Buffer to which to write samples, second parameter is the samplerate.
typedef std::function<void( Buffer *, size_t )> CallbackProcessorFn;

//! NodeInput that processes audio with a std::function callback. \see CallbackProcessorFn
class CallbackProcessor : public NodeInput {
  public:
	CallbackProcessor( const CallbackProcessorFn &callbackFn, const Format &format = Format() ) : NodeInput( format ), mCallbackFn( callbackFn ) {}
	virtual ~CallbackProcessor() {}

  protected:
	void process( Buffer *buffer ) override;

  private:
	CallbackProcessorFn mCallbackFn;
};

} } // namespace cinder::audio2