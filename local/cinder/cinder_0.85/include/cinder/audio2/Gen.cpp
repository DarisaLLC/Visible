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

#include "cinder/audio2/Gen.h"
#include "cinder/audio2/Context.h"
#include "cinder/audio2/dsp/Dsp.h"
#include "cinder/audio2/Utilities.h"
#include "cinder/audio2/Debug.h"
#include "cinder/Rand.h"

#define DEFAULT_TABLE_SIZE 4096
#define DEFAULT_BANDLIMITED_TABLES 40

using namespace std;

namespace cinder { namespace audio2 {

// ----------------------------------------------------------------------------------------------------
// MARK: - Gen
// ----------------------------------------------------------------------------------------------------

Gen::Gen( const Format &format )
	: NodeInput( format ), mFreq( this ), mPhase( 0 )
{
	initImpl();
}

Gen::Gen( float freq, const Format &format )
	: NodeInput( format ), mFreq( this, freq ), mPhase( 0 )
{
	initImpl();
}

void Gen::initImpl()
{
	setChannelMode( ChannelMode::SPECIFIED );
	setNumChannels( 1 );
}

void Gen::initialize()
{
	mSamplePeriod = 1.0f / (float)getSampleRate();
}

// ----------------------------------------------------------------------------------------------------
// MARK: - GenNoise
// ----------------------------------------------------------------------------------------------------

void GenNoise::process( Buffer *buffer )
{
	float *data = buffer->getData();
	size_t count = buffer->getSize();

	for( size_t i = 0; i < count; i++ )
		data[i] = ci::randFloat( -1.0f, 1.0f );
}

// ----------------------------------------------------------------------------------------------------
// MARK: - GenSine
// ----------------------------------------------------------------------------------------------------

void GenSine::process( Buffer *buffer )
{
	float *data = buffer->getData();
	const size_t count = buffer->getSize();
	const float samplePeriod = mSamplePeriod;
	float phase = mPhase;

	if( mFreq.eval() ) {
		const float *freqValues = mFreq.getValueArray();
		for( size_t i = 0; i < count; i++ ) {
			data[i] = math<float>::sin( phase * float( 2 * M_PI ) );
			phase = wrap( phase + freqValues[i] * samplePeriod );
		}
	}
	else {
		const float phaseIncr = mFreq.getValue() * samplePeriod;
		for( size_t i = 0; i < count; i++ ) {
			data[i] = math<float>::sin( phase * float( 2 * M_PI ) );
			phase = wrap( phase + phaseIncr );
		}
	}

	mPhase = phase;
}

// ----------------------------------------------------------------------------------------------------
// MARK: - GenPhasor
// ----------------------------------------------------------------------------------------------------

void GenPhasor::process( Buffer *buffer )
{
	float *data = buffer->getData();
	const size_t count = buffer->getSize();
	const float samplePeriod = mSamplePeriod;
	float phase = mPhase;

	if( mFreq.eval() ) {
		const float *freqValues = mFreq.getValueArray();
		for( size_t i = 0; i < count; i++ ) {
			data[i] = phase;
			phase = wrap( phase + freqValues[i] * samplePeriod );
		}
	}
	else {
		const float phaseIncr = mFreq.getValue() * samplePeriod;
		for( size_t i = 0; i < count; i++ ) {
			data[i] = phase;
			phase = wrap( phase + phaseIncr );
		}
	}

	mPhase = phase;
}

// ----------------------------------------------------------------------------------------------------
// MARK: - GenTriangle
// ----------------------------------------------------------------------------------------------------

GenTriangle::GenTriangle( const Format &format )
	: Gen( format ), mUpSlope( 1 ), mDownSlope( 1 )
{
}

GenTriangle::GenTriangle( float freq, const Format &format )
	: Gen( freq, format ), mUpSlope( 1 ), mDownSlope( 1 )
{
}

namespace {

inline float calcTriangleSignal( float phase, float upSlope, float downSlope )
{
	// if up slope = down slope = 1, signal ranges from 0 to 0.5. so normalize this from -1 to 1
	float signal = std::min( phase * upSlope, ( 1 - phase ) * downSlope );
	return signal * 4 - 1;
}

} // anonymous namespace

void GenTriangle::process( Buffer *buffer )
{
	float *data = buffer->getData();
	size_t count = buffer->getSize();
	const float samplePeriod = mSamplePeriod;
	float phase = mPhase;

	if( mFreq.eval() ) {
		const float *freqValues = mFreq.getValueArray();
		for( size_t i = 0; i < count; i++ )	{
			data[i] = calcTriangleSignal( phase, mUpSlope, mDownSlope );
			phase = wrap( phase + freqValues[i] * samplePeriod );
		}

	}
	else {
		const float phaseIncr = mFreq.getValue() * samplePeriod;
		for( size_t i = 0; i < count; i++ )	{
			data[i] = calcTriangleSignal( phase, mUpSlope, mDownSlope );
			phase = wrap( phase + phaseIncr );
		}
	}

	mPhase = phase;
}

// ----------------------------------------------------------------------------------------------------
// MARK: - GenTable
// ----------------------------------------------------------------------------------------------------

void GenTable::initialize()
{
	Gen::initialize();

	if( ! mWaveTable ) {
		mWaveTable.reset( new dsp::WaveTable( getSampleRate(), DEFAULT_TABLE_SIZE ) );
		mWaveTable->fillSine();
	}
}

void GenTable::process( Buffer *buffer )
{
	if( mFreq.eval() )
		mPhase = mWaveTable->lookup( buffer->getData(), buffer->getSize(), mPhase, mFreq.getValueArray() );
	else
		mPhase = mWaveTable->lookup( buffer->getData(), buffer->getSize(), mPhase, mFreq.getValue() );
}

// ----------------------------------------------------------------------------------------------------
// MARK: - GenOscillator
// ----------------------------------------------------------------------------------------------------

GenOscillator::GenOscillator( const Format &format )
	: Gen( format ), mWaveformType( format.getWaveform() )
{
}

GenOscillator::GenOscillator( float freq, const Format &format )
	: Gen( freq, format ), mWaveformType( format.getWaveform() )
{
}

void GenOscillator::initialize()
{
	Gen::initialize();

	size_t sampleRate = getSampleRate();
	bool needsFill = false;
	if( ! mWaveTable ) {
		mWaveTable.reset( new dsp::WaveTable2d( sampleRate, DEFAULT_TABLE_SIZE, DEFAULT_BANDLIMITED_TABLES ) );
		needsFill = true;
	}
	else if( sampleRate != mWaveTable->getSampleRate() )
		needsFill = true;

	if( needsFill )
		mWaveTable->fillBandlimited( mWaveformType );
}

void GenOscillator::setWaveform( WaveformType type )
{
	if( mWaveformType == type )
		return;

	// TODO: to prevent the entire graph from blocking, use our own lock and tryLock / fail when blocked in process()
	lock_guard<mutex> lock( getContext()->getMutex() );

	mWaveformType = type;
	mWaveTable->fillBandlimited( type );
}

void GenOscillator::process( Buffer *buffer )
{
	if( mFreq.eval() )
		mPhase = mWaveTable->lookupBandlimited( buffer->getData(), buffer->getSize(), mPhase, mFreq.getValueArray() );
	else
		mPhase = mWaveTable->lookupBandlimited( buffer->getData(), buffer->getSize(), mPhase, mFreq.getValue() );
}

// ----------------------------------------------------------------------------------------------------
// MARK: - GenPulse
// ----------------------------------------------------------------------------------------------------

GenPulse::GenPulse( const Format &format )
	: Gen( format ), mWidth( this, 0.5f )
{
}

GenPulse::GenPulse( float freq, const Format &format )
	: Gen( freq, format ), mWidth( this, 0.5f )
{
}

void GenPulse::initialize()
{
	Gen::initialize();

	mBuffer2.setNumFrames( getFramesPerBlock() );

	size_t sampleRate = getSampleRate();
	bool needsFill = false;
	if( ! mWaveTable ) {
		mWaveTable.reset( new dsp::WaveTable2d( sampleRate, DEFAULT_TABLE_SIZE, DEFAULT_BANDLIMITED_TABLES ) );
		needsFill = true;
	}
	else if( sampleRate != mWaveTable->getSampleRate() )
		needsFill = true;

	if( needsFill )
		mWaveTable->fillBandlimited( WaveformType::SAWTOOTH );
}

void GenPulse::process( Buffer *buffer )
{
	size_t numFrames = buffer->getNumFrames();
	const float samplePeriod = mSamplePeriod;
	float phase = mPhase;

	if( mWidth.eval() ) {
		float *data2 = mBuffer2.getData();
		const float *widthArray = mWidth.getValueArray();

		if( mFreq.eval() ) {
			const float *f0Array = mFreq.getValueArray();
			mPhase = mWaveTable->lookupBandlimited( buffer->getData(), numFrames, phase, f0Array );

			for( size_t i = 0; i < numFrames; i++ ) {
				float f0 = f0Array[i];
				float phaseIncr = f0 * samplePeriod;
				float phaseOffset = widthArray[i];
				float phase2 = wrap( phase + phaseOffset );
				float phaseCorrect = 1 - 2 * phaseOffset;
				data2[i] = mWaveTable->lookupBandlimited( phase2, f0 ) - phaseCorrect;
				phase = wrap( phase + phaseIncr );
			}
		} else {
			float f0 = mFreq.getValue();
			float phaseIncr = f0 * samplePeriod;
			mPhase = mWaveTable->lookupBandlimited( buffer->getData(), numFrames, phase, f0 );

			for( size_t i = 0; i < numFrames; i++ ) {
				float phaseOffset = widthArray[i];
				float phase2 = wrap( phase + phaseOffset );
				float phaseCorrect = 1 - 2 * phaseOffset;
				data2[i] = mWaveTable->lookupBandlimited( phase2, f0 ) - phaseCorrect;
				phase += phaseIncr;
			}
		}
	}
	else {
		float phaseOffset = mWidth.getValue();
		float phase2 = wrap( phase + phaseOffset );

		if( mFreq.eval() ) {
			mPhase = mWaveTable->lookupBandlimited( buffer->getData(), numFrames, phase, mFreq.getValueArray() );
			mWaveTable->lookupBandlimited( mBuffer2.getData(), numFrames, phase2, mFreq.getValueArray() );
		} else {
			float f0 = mFreq.getValue();
			mPhase = mWaveTable->lookupBandlimited( buffer->getData(), numFrames, phase, f0 );
			mWaveTable->lookupBandlimited( mBuffer2.getData(), numFrames, phase2, f0 );
		}

		float phaseCorrect = 1 - 2 * phaseOffset;
		dsp::add( buffer->getData(), phaseCorrect, buffer->getData(), buffer->getSize() );
	}

	dsp::sub( buffer->getData(), mBuffer2.getData(), buffer->getData(), buffer->getSize() );
}

} } // namespace cinder::audio2