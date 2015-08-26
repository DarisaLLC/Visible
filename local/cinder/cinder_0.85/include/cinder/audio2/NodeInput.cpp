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

#include "cinder/audio2/NodeInput.h"
#include "cinder/audio2/Context.h"

using namespace ci;
using namespace std;

namespace cinder { namespace audio2 {

// ----------------------------------------------------------------------------------------------------
// MARK: - NodeInput
// ----------------------------------------------------------------------------------------------------

NodeInput::NodeInput( const Format &format ) : Node( format )
{
	// NodeInput's don't have inputs, so make the default match outputs
	if( getChannelMode() == ChannelMode::MATCHES_INPUT )
		setChannelMode( ChannelMode::MATCHES_OUTPUT );
}

NodeInput::~NodeInput()
{
}

void NodeInput::connectInput( const NodeRef &input, size_t bus )
{
	CI_ASSERT_MSG( 0, "NodeInput does not support inputs" );
}

// ----------------------------------------------------------------------------------------------------
// MARK: - LineIn
// ----------------------------------------------------------------------------------------------------

LineIn::LineIn( const DeviceRef &device, const Format &format )
: NodeInput( format ), mDevice( device ), mLastOverrun( 0 ), mLastUnderrun( 0 )
{
	size_t deviceNumChannels = mDevice->getNumInputChannels();

	// If number of channels hasn't been specified, default to 2.
	if( getChannelMode() != ChannelMode::SPECIFIED ) {
		setChannelMode( ChannelMode::SPECIFIED );
		setNumChannels( std::min( deviceNumChannels, (size_t)2 ) );
	}

	// TODO: this doesn't always mean a failing cause, need Device::supportsNumInputChannels( mNumChannels ) to be sure
	//	- on iOS, the RemoteIO audio unit can have 2 input channels, while the AVAudioSession reports only 1 input channel.
//	if( deviceNumChannels < mNumChannels )
//		throw AudioFormatExc( string( "Device can not accommodate " ) + to_string( deviceNumChannels ) + " output channels." );
}

LineIn::~LineIn()
{
}

uint64_t LineIn::getLastUnderrun() const
{
	uint64_t result = mLastUnderrun;
	mLastUnderrun = 0;
	return result;
}

uint64_t LineIn::getLastOverrun() const
{
	uint64_t result = mLastOverrun;
	mLastOverrun = 0;
	return result;
}

void LineIn::markUnderrun()
{
	auto ctx = getContext();
	CI_ASSERT( ctx );

	mLastUnderrun = ctx->getNumProcessedFrames();
}

void LineIn::markOverrun()
{

	auto ctx = getContext();
	CI_ASSERT( ctx );

	mLastOverrun = getContext()->getNumProcessedFrames();
}

// ----------------------------------------------------------------------------------------------------
// MARK: - CallbackProcessor
// ----------------------------------------------------------------------------------------------------

void CallbackProcessor::process( Buffer *buffer )
{
	if( mCallbackFn )
		mCallbackFn( buffer, getSampleRate() );
}

} } // namespace cinder::audio2