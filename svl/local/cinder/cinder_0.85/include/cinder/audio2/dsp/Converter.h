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

#include "cinder/audio2/Buffer.h"

#include <memory>

namespace cinder { namespace audio2 { namespace dsp {

class Converter {
public:
	//! If \a destSampleRate is 0, it is set to match \a sourceSampleRate. If \a destNumChannels is 0, it is set to match \a sourceNumChannels.
	static std::unique_ptr<Converter> create( size_t sourceSampleRate, size_t destSampleRate, size_t sourceNumChannels, size_t destNumChannels, size_t sourceMaxFramesPerBlock );

	virtual ~Converter() {}

	//! Converts up to getSourceMaxFramesPerBlock() frames of audio data from \a sourceBuffer into \a destBuffer. Returns a \a std::pair<num source frames used, num dest frames written>
	//! \note destBuffer must be large enough to complete the conversion, which is calculated as: \code minNumDestFrames = min( sourceBuffer->getNumFrames, getSourceMaxFramesPerBlock() ) * getDestSampleRate() * getSourceSampleRate() \endcode
	virtual std::pair<size_t, size_t> convert( const Buffer *sourceBuffer, Buffer *destBuffer ) = 0;

	//! Clears the state of the converter, discarding / flushing accumulated samples. Optional for implementations.
	virtual void clear()	{}

	size_t getSourceSampleRate()		const		{ return mSourceSampleRate; }
	size_t getDestSampleRate()			const		{ return mDestSampleRate; }
	size_t getSourceNumChannels()		const		{ return mSourceNumChannels; }
	size_t getDestNumChannels()			const		{ return mDestNumChannels; }
	size_t getSourceMaxFramesPerBlock()	const		{ return mSourceMaxFramesPerBlock; }
	size_t getDestMaxFramesPerBlock()	const		{ return mDestMaxFramesPerBlock; }

protected:
	Converter( size_t sourceSampleRate, size_t destSampleRate, size_t sourceNumChannels, size_t destNumChannels, size_t sourceMaxFramesPerBlock );

	size_t mSourceSampleRate, mDestSampleRate, mSourceNumChannels, mDestNumChannels, mSourceMaxFramesPerBlock, mDestMaxFramesPerBlock;
};

//! Mixes \a numFrames frames of \a sourceBuffer to \a destBuffer's layout, replacing its content. Channel up or down mixing is applied if necessary.
void mixBuffers( const Buffer *sourceBuffer, Buffer *destBuffer, size_t numFrames );
//! Mixes \a sourceBuffer to \a destBuffer's layout, replacing its content. Channel up or down mixing is applied if necessary. Unequal frame counts are permitted (the minimum size will be used).
inline void mixBuffers( const Buffer *sourceBuffer, Buffer *destBuffer )	{ mixBuffers( sourceBuffer, destBuffer, std::min( sourceBuffer->getNumFrames(), destBuffer->getNumFrames() ) ); }

//! Sums \a numFrames frames of \a sourceBuffer into \a destBuffer. Channel up or down mixing is applied if necessary.
void sumBuffers( const Buffer *sourceBuffer, Buffer *destBuffer, size_t numFrames );
//! Sums \a sourceBuffer into \a destBuffer. Channel up or down mixing is applied if necessary. Unequal frame counts are permitted (the minimum size will be used).
inline void sumBuffers( const Buffer *sourceBuffer, Buffer *destBuffer )	{ sumBuffers( sourceBuffer, destBuffer, std::min( sourceBuffer->getNumFrames(), destBuffer->getNumFrames() ) ); }

template <typename SourceT, typename DestT>
void convert( const SourceT *sourceArray, DestT *destArray, size_t length )
{
	for( size_t i = 0; i < length; i++ )
		destArray[i] = static_cast<DestT>( sourceArray[i] );
}

template <typename SourceT, typename DestT>
void convertBuffers( const BufferT<SourceT> *sourceBuffer, BufferT<DestT> *destBuffer )
{
	size_t numFrames = std::min( sourceBuffer->getNumFrames(), destBuffer->getNumFrames() );
	size_t numChannels = std::min( sourceBuffer->getNumChannels(), destBuffer->getNumChannels() );

	for( size_t ch = 0; ch < numChannels; ch++ )
		convert( sourceBuffer->getChannel( ch ), destBuffer->getChannel( ch ), numFrames );
}

template<typename T>
void interleave( const T *nonInterleavedSourceArray, T *interleavedDestArray, size_t numFramesPerChannel, size_t numChannels, size_t numCopyFrames )
{
	for( size_t ch = 0; ch < numChannels; ch++ ) {
		size_t x = ch;
		const T *sourceChannel = &nonInterleavedSourceArray[ch * numFramesPerChannel];
		for( size_t i = 0; i < numCopyFrames; i++ ) {
			interleavedDestArray[x] = sourceChannel[i];
			x += numChannels;
		}
	}
}

template<typename FloatT>
void interleave( const FloatT *nonInterleavedFloatSourceArray, int16_t *interleavedInt16DestArray, size_t numFramesPerChannel, size_t numChannels, size_t numCopyFrames )
{
	const FloatT intNormalizer = 32768;

	for( size_t ch = 0; ch < numChannels; ch++ ) {
		size_t x = ch;
		const FloatT *sourceChannel = &nonInterleavedFloatSourceArray[ch * numFramesPerChannel];
		for( size_t i = 0; i < numCopyFrames; i++ ) {
			interleavedInt16DestArray[x] = int16_t( sourceChannel[i] * intNormalizer );
			x += numChannels;
		}
	}
}

template<typename T>
void deinterleave( const T *interleavedSourceArray, T *nonInterleavedDestArray, size_t numFramesPerChannel, size_t numChannels, size_t numCopyFrames )
{
	for( size_t ch = 0; ch < numChannels; ch++ ) {
		size_t x = ch;
		T *destChannel = &nonInterleavedDestArray[ch * numFramesPerChannel];
		for( size_t i = 0; i < numCopyFrames; i++ ) {
			destChannel[i] = interleavedSourceArray[x];
			x += numChannels;
		}
	}
}

template<typename FloatT>
void deinterleave( const int16_t *interleavedInt16SourceArray, FloatT *nonInterleavedFloatDestArray, size_t numFramesPerChannel, size_t numChannels, size_t numCopyFrames )
{
	const FloatT floatNormalizer = (FloatT)3.0517578125e-05;	// 1.0 / 32768.0

	for( size_t ch = 0; ch < numChannels; ch++ ) {
		size_t x = ch;
		FloatT *destChannel = &nonInterleavedFloatDestArray[ch * numFramesPerChannel];
		for( size_t i = 0; i < numCopyFrames; i++ ) {
			destChannel[i] = (FloatT)interleavedInt16SourceArray[x] * floatNormalizer;
			x += numChannels;
		}
	}
}

// TODO: consider removing Buffer suffix on these overloads, it is clear from the args (convertBuffer too).
template<typename T>
void interleaveBuffer( const BufferT<T> *nonInterleavedSource, BufferInterleavedT<T> *interleavedDest )
{
	CI_ASSERT( interleavedDest->getNumChannels() == nonInterleavedSource->getNumChannels() );
	CI_ASSERT( interleavedDest->getSize() <= nonInterleavedSource->getSize() );

	interleave( nonInterleavedSource->getData(), interleavedDest->getData(), interleavedDest->getNumFrames(), interleavedDest->getNumChannels(), interleavedDest->getNumFrames() );
}

template<typename T>
void deinterleaveBuffer( const BufferInterleavedT<T> *interleavedSource, BufferT<T> *nonInterleavedDest )
{
	CI_ASSERT( interleavedSource->getNumChannels() == nonInterleavedDest->getNumChannels() );
	CI_ASSERT( nonInterleavedDest->getSize() <= interleavedSource->getSize() );

	deinterleave( interleavedSource->getData(), nonInterleavedDest->getData(), nonInterleavedDest->getNumFrames(), nonInterleavedDest->getNumChannels(), nonInterleavedDest->getNumFrames() );
}

template<typename T>
void interleaveStereoBuffer( const BufferT<T> *nonInterleavedSource, BufferInterleavedT<T> *interleavedDest )
{
	CI_ASSERT( interleavedDest->getNumChannels() == 2 && nonInterleavedSource->getNumChannels() == 2 );
	CI_ASSERT( interleavedDest->getSize() <= nonInterleavedSource->getSize() );

	size_t numFrames = interleavedDest->getNumFrames();
	const T *left = nonInterleavedSource->getChannel( 0 );
	const T *right = nonInterleavedSource->getChannel( 1 );

	T *mixed = interleavedDest->getData();

	size_t i, j;
	for( i = 0, j = 0; i < numFrames; i++, j += 2 ) {
		mixed[j] = left[i];
		mixed[j + 1] = right[i];
	}
}

template<typename T>
void deinterleaveStereoBuffer( const BufferInterleavedT<T> *interleavedSource, BufferT<T> *nonInterleavedDest )
{
	CI_ASSERT( interleavedSource->getNumChannels() == 2 && nonInterleavedDest->getNumChannels() == 2 );
	CI_ASSERT( nonInterleavedDest->getSize() <= interleavedSource->getSize() );

	size_t numFrames = nonInterleavedDest->getNumFrames();
	T *left = nonInterleavedDest->getChannel( 0 );
	T *right = nonInterleavedDest->getChannel( 1 );
	const T *mixed = interleavedSource->getData();

	size_t i, j;
	for( i = 0, j = 0; i < numFrames; i++, j += 2 ) {
		left[i] = mixed[j];
		right[i] = mixed[j + 1];
	}
}

} } } // namespace cinder::audio2::dsp