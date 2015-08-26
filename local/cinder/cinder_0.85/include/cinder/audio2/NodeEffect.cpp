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

#include "cinder/audio2/NodeEffect.h"
#include "cinder/audio2/Debug.h"
#include "cinder/audio2/Utilities.h"

#include "cinder/CinderMath.h"

using namespace ci;
using namespace std;

namespace cinder { namespace audio2 {

// ----------------------------------------------------------------------------------------------------
// MARK: - NodeEffect
// ----------------------------------------------------------------------------------------------------

NodeEffect::NodeEffect( const Format &format )
: Node( format )
{
	if( boost::indeterminate( format.getAutoEnable() ) )
		setAutoEnabled();
}

// ----------------------------------------------------------------------------------------------------
// MARK: - Gain
// ----------------------------------------------------------------------------------------------------

Gain::Gain( const Format &format )
: NodeEffect( format ), mParam( this, 1 ), mMin( 0 ), mMax( 1 )
{
}

Gain::Gain( float initialValue, const Format &format )
: NodeEffect( format ), mParam( this, initialValue ), mMin( 0 ), mMax( 1 )
{
}

void Gain::process( Buffer *buffer )
{
	if( mParam.eval() ) {
		for( size_t ch = 0; ch < getNumChannels(); ch++ ) {
			float *channel = buffer->getChannel( ch );
			dsp::mul( channel, mParam.getValueArray(), channel, buffer->getNumFrames() );
		}
	}
	else
		dsp::mul( buffer->getData(), mParam.getValue(), buffer->getData(), buffer->getSize() );
}

// ----------------------------------------------------------------------------------------------------
// MARK: - Add
// ----------------------------------------------------------------------------------------------------

Add::Add( const Format &format )
	: Node( format ), mParam( this, 0 )
{
	if( boost::indeterminate( format.getAutoEnable() ) )
		setAutoEnabled();
}

Add::Add( float initialValue, const Format &format )
	: Node( format ), mParam( this, initialValue )
{
	if( boost::indeterminate( format.getAutoEnable() ) )
		setAutoEnabled();
}

void Add::process( Buffer *buffer )
{
	if( mParam.eval() ) {
		for( size_t ch = 0; ch < getNumChannels(); ch++ ) {
			float *channel = buffer->getChannel( ch );
			dsp::add( channel, mParam.getValueArray(), channel, buffer->getNumFrames() );
		}
	}
	else
		dsp::add( buffer->getData(), mParam.getValue(), buffer->getData(), buffer->getSize() );
}

// ----------------------------------------------------------------------------------------------------
// MARK: - Pan2d
// ----------------------------------------------------------------------------------------------------

Pan2d::Pan2d( const Format &format )
: NodeEffect( format ), mPos( 0.5f ), mMonoInputMode( true )
{
	setChannelMode( ChannelMode::SPECIFIED );
	setNumChannels( 2 );
}

/*
 * TODO: below is an attempt at an optimization for (possibly many) mono input -> stereo output
 8	- I didn't get it to mesh well enough with Node::configureConnections, during various edge cases, so it is on the back burner for now..
bool Pan2d::supportsInputNumChannels( size_t numChannels )
{
	if( numChannels == 1 ) {
		mMonoInputMode = true;
		mProcessInPlace = false;
		size_t framesPerBlock = getFramesPerBlock();

		// internal buffer is mono (which will be passed to inputs), while summing buffer is stereo
		if( mInternalBuffer.getNumChannels() != 1 || mInternalBuffer.getNumFrames() != framesPerBlock )
			mInternalBuffer = Buffer( framesPerBlock, 1 );
		if( mSummingBuffer.getNumChannels() != mNumChannels || mSummingBuffer.getNumFrames() != framesPerBlock )
			mSummingBuffer = Buffer( framesPerBlock, mNumChannels );
	}
	else
		mMonoInputMode = false;

	LOG_V << "mono input mode: " << boolalpha << mMonoInputMode << dec << endl;

	return ( numChannels <= 2 );
}

void Pan2d::pullInputs( Buffer *destBuffer )
{
	CI_ASSERT( getContext() );

	if( ! mMonoInputMode )
		Node::pullInputs( destBuffer );
	else {
		// inputs are summed to channel 0 only
		mInternalBuffer.zero();
		mSummingBuffer.zero();

		size_t numFrames = mSummingBuffer.getNumFrames();
		float *summingChannel0 = mSummingBuffer.getChannel( 0 );
		for( NodeRef &input : mInputs ) {
			if( ! input )
				continue;

			input->pullInputs( &mInternalBuffer );
			if( input->getProcessInPlace() )
				add( summingChannel0, mInternalBuffer.getChannel( 0 ), summingChannel0, numFrames );
			else
				add( summingChannel0, input->getInternalBuffer()->getChannel( 0 ), summingChannel0, numFrames );
		}

		if( mEnabled )
			process( &mSummingBuffer );

		// at this point, audio will be in both stereo channels

		// TODO: if possible, just copy summing buffer to output buffer
		// - this is on hold until further work towards avoiding both mInternalBuffer and mSummingBuffer
		// - at that point, it may be possible to avoid this mix as well, in some cases
		Converter::mixBuffers( &mSummingBuffer, destBuffer );
	}
}
*/

// equal power panning eq:
// left = cos(p) * signal, right = sin(p) * signal, where p is in radians from 0 to PI/2
// gives +3db when panned to center, which helps to remove the 'dead spot'
void Pan2d::process( Buffer *buffer )
{
	float pos = mPos;
	float *channel0 = buffer->getChannel( 0 );
	float *channel1 = buffer->getChannel( 1 );

	float posRadians = pos * float( M_PI / 2.0 );
	float leftGain = math<float>::cos( posRadians );
	float rightGain = math<float>::sin( posRadians );

	if( mMonoInputMode ) {
		dsp::mul( channel0, leftGain, channel0, buffer->getNumFrames() );
		dsp::mul( channel1, rightGain, channel1, buffer->getNumFrames() );
	}
	else {
		// suitable impl for stereo panning an already-stereo input...

		static const float kCenterGain = math<float>::cos( float( M_PI / 4.0 ) );
		size_t n = buffer->getNumFrames();

		if( pos < 0.5f ) {
			for( size_t i = 0; i < n; i++ ) {
				channel0[i] = channel0[i] * leftGain + channel1[i] * ( leftGain - kCenterGain );
				channel1[i] *= rightGain;
			}
		} else {
			for( size_t i = 0; i < n; i++ ) {
				channel1[i] = channel1[i] * rightGain + channel0[i] * ( rightGain - kCenterGain );
				channel0[i] *= leftGain;
			}
		}
	}
}

void Pan2d::setPos( float pos )
{
	mPos = math<float>::clamp( pos );
}

// ----------------------------------------------------------------------------------------------------
// MARK: - Delay
// ----------------------------------------------------------------------------------------------------

Delay::Delay( const Format &format )
	: NodeEffect( format ), mDelaySeconds( 0 ), mReadIndex( 0 ), mDelayFrames( 0 )
{
}

void Delay::setDelaySeconds( float seconds )
{
	mDelayFrames = lroundf( seconds * getSampleRate() );
	mDelaySeconds = seconds;

	size_t delayBufferFrames = 1 + max( mDelayFrames, getFramesPerBlock() );
	mDelayBuffer.setSize( delayBufferFrames, getNumChannels() );
	mReadIndex = 0;

	CI_LOG_V( "seconds: " << seconds << ", frames: " << mDelayFrames << ", delay buffer frames: " << delayBufferFrames );
}

void Delay::initialize()
{
	if( mDelayBuffer.getNumChannels() != getNumChannels() )
		mDelayBuffer.setNumChannels( getNumChannels() );
}

void Delay::process( Buffer *buffer )
{
	size_t numFrames = buffer->getNumFrames();
	size_t maxDelayFrames = mDelayBuffer.getNumFrames();
	size_t readIndex = mReadIndex;
	size_t delayIndex = readIndex + mDelayFrames;

	for( size_t ch = 0; ch < buffer->getNumChannels(); ch++ ) {
		float *inChannel = buffer->getChannel( ch );
		float *delayChannel = mDelayBuffer.getChannel( ch );

		for( size_t i = 0; i < numFrames; i++ ) {
			if( delayIndex >= maxDelayFrames )
				delayIndex -= maxDelayFrames;
			if( readIndex >= maxDelayFrames )
				readIndex -= maxDelayFrames;

			float sample = *inChannel;
			*inChannel++ = delayChannel[readIndex++];
			delayChannel[delayIndex++] = sample;
		}
	}

	mReadIndex = readIndex;
}

} } // namespace cinder::audio2