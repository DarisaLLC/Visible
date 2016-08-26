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

#include "cinder/audio2/NodeEffect.h"
#include "cinder/audio2/dsp/Biquad.h"

#include <vector>

// TODO: add api for setting biquad with arbitrary set of coefficients, ala pd's [biquad~]

namespace cinder { namespace audio2 {

typedef std::shared_ptr<class FilterLowPass>		FilterLowPassRef;
typedef std::shared_ptr<class FilterHighPass>		FilterHighPassRef;
typedef std::shared_ptr<class FilterBandPass>		FilterBandPassRef;

//! Base class for filter nodes that use Biquad
class FilterBiquad : public NodeEffect {
  public:
	enum class Mode { LOWPASS, HIGHPASS, BANDPASS, LOWSHELF, HIGHSHELF, PEAKING, ALLPASS, NOTCH, CUSTOM };

	FilterBiquad( Mode mode = Mode::LOWPASS, const Format &format = Format() ) : NodeEffect( format ), mMode( mode ), mCoeffsDirty( true ), mFreq( 200.0f ), mQ( 1.0f ), mGain( 0.0f ) {}
	virtual ~FilterBiquad() {}

	void setMode( Mode mode )	{ mMode = mode; mCoeffsDirty = true; }
	Mode getMode() const		{ return mMode; }

	void setFreq( float freq )	{ mFreq = freq; mCoeffsDirty = true; }
	float getFreq() const		{ return mFreq; }

	void setQ( float q )		{ mQ = q; mCoeffsDirty = true; }
	float getQ() const			{ return mQ; }

	void setGain( float gain )	{ mGain = gain; mCoeffsDirty = true; }
	float getGain() const		{ return mGain; }

  protected:
	void initialize()				override;
	void uninitialize()				override;
	void process( Buffer *buffer )	override;

	void updateBiquadParams();

	std::vector<dsp::Biquad> mBiquads;
	std::atomic<bool> mCoeffsDirty;
	BufferT<double> mBufferd;
	size_t mNiquist;

	Mode mMode;
	float mFreq, mQ, mGain;
};

class FilterLowPass : public FilterBiquad {
  public:
	FilterLowPass( const Format &format = Format() ) : FilterBiquad( Mode::LOWPASS, format ) {}
	virtual ~FilterLowPass() {}

	void setCutoffFreq( float freq )			{ setFreq( freq ); }
	void setResonance( float resonance )		{ setQ( resonance ); }

	float getCutoffFreq() const	{ return mFreq; }
	float getResonance() const	{ return mQ; }
};

class FilterHighPass : public FilterBiquad {
public:
	FilterHighPass( const Format &format = Format() ) : FilterBiquad( Mode::HIGHPASS, format ) {}
	virtual ~FilterHighPass() {}

	void setCutoffFreq( float freq )			{ setFreq( freq ); }
	void setResonance( float resonance )		{ setQ( resonance ); }

	float getCutoffFreq() const	{ return mFreq; }
	float getResonance() const	{ return mQ; }
};

class FilterBandPass : public FilterBiquad {
public:
	FilterBandPass( const Format &format = Format() ) : FilterBiquad( Mode::BANDPASS, format ) {}
	virtual ~FilterBandPass() {}

	void setCutoffFreq( float freq )	{ setFreq( freq ); }
	void setWidth( float width )		{ setQ( width ); }

	float getCutoffFreq() const		{ return mFreq; }
	float getWidth() const			{ return mQ; }
};


} } // namespace cinder::audio2