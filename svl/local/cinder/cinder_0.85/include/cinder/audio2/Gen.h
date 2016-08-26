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

#include "cinder/audio2/NodeInput.h"
#include "cinder/audio2/dsp/WaveTable.h"

namespace cinder { namespace audio2 {

typedef std::shared_ptr<class Gen>						GenRef;
typedef std::shared_ptr<class GenOscillator>			GenOscillatorRef;
typedef std::shared_ptr<class GenPulse>					GenPulseRef;

//! Base class for NodeInput's that generate audio samples. Gen's are always mono channel.
class Gen : public NodeInput {
  public:
	Gen( const Format &format = Format() );
	Gen( float freq, const Format &format = Format() );

	void setFreq( float freq )		{ mFreq.setValue( freq ); }
	float getFreq() const			{ return mFreq.getValue(); }

	Param* getParamFreq()			{ return &mFreq; }

  protected:
	void initialize() override;
	void initImpl();

	float mSamplePeriod;

	Param mFreq;
	float mPhase;
};

//! Noise generator. \note freq param is ignored
class GenNoise : public Gen {
  public:
	GenNoise( const Format &format = Format() ) : Gen( format ) {}

  protected:
	void process( Buffer *buffer ) override;
};

//! Phase generator, i.e. ramping waveform that runs from 0 to 1.
class GenPhasor : public Gen {
  public:
	GenPhasor( const Format &format = Format() ) : Gen( format ) {}
	GenPhasor( float freq, const Format &format = Format() ) : Gen( freq, format ) {}

  protected:
	void process( Buffer *buffer ) override;
};

//! Sine waveform generator.
class GenSine : public Gen {
  public:
	GenSine( const Format &format = Format() ) : Gen( format )	{}
	GenSine( float freq, const Format &format = Format() ) : Gen( freq, format ) {}

  protected:
	void process( Buffer *buffer ) override;
};

//! Triangle waveform generator.
class GenTriangle : public Gen {
  public:
	GenTriangle( const Format &format = Format() );
	GenTriangle( float freq, const Format &format = Format() );

	void setUpSlope( float up )			{ mUpSlope = up; }
	void setDownSlope( float down )		{ mDownSlope = down; }

	float getUpSlope() const		{ return mUpSlope; }
	float getDownSlope() const		{ return mDownSlope; }

  protected:
	void process( Buffer *buffer ) override;

  private:
	std::atomic<float> mUpSlope, mDownSlope;
};

//! Basic table-lookup oscillator. \note aliasing will occur at higher frequencies, in this case refer to GenOscillator which is more robust.
class GenTable : public Gen {
  public:
	GenTable( const Format &format = Format() )	: Gen( format )		{}
	GenTable( float freq, const Format &format = Format() ) : Gen( freq, format )	{}

	void setWaveTable( const dsp::WaveTableRef &waveTable )	{ mWaveTable = waveTable; }
	const dsp::WaveTableRef&	getWaveTable()	{ return mWaveTable; }

  protected:
	void initialize()				override;
	void process( Buffer *buffer )	override;

	dsp::WaveTableRef	mWaveTable;
};

//! General purpose, band-limited oscillator using wavetable lookup.
class GenOscillator : public Gen {
  public:

	struct Format : public Node::Format {
		Format() : mWaveformType( WaveformType::SINE )	{}

		Format&		waveform( WaveformType type )	{ mWaveformType = type; return *this; }

		const WaveformType& getWaveform() const	{ return mWaveformType; }

      private:
		WaveformType mWaveformType;
	};

	GenOscillator( const Format &format = Format() );
	GenOscillator( float freq, const Format &format = Format() );


	void setWaveform( WaveformType type );

	void setWaveTable( const dsp::WaveTable2dRef &waveTable )	{ mWaveTable = waveTable; }
	const dsp::WaveTable2dRef getWaveTable() const				{ return mWaveTable; }

	WaveformType	getWaveForm() const			{ return mWaveformType; }
	size_t			getTableSize() const		{ return mWaveTable->getTableSize(); }

  protected:
	void initialize() override;
	void process( Buffer *buffer ) override;


	dsp::WaveTable2dRef		mWaveTable;
	WaveformType			mWaveformType;
};

//! Pulse waveform generator with variable pulse width. Based on wavetable lookup of two band-limited sawtooth waveforms, subtracted from each other.
class GenPulse : public Gen {
  public:
	GenPulse( const Format &format = Format() );
	GenPulse( float freq, const Format &format = Format() );

	//! Set the pulse width (aka 'duty cycle'). Expected range is between [0:1] (default = 0.5, creating a square wave).
	void			setWidth( float width )	{ mWidth.setValue( width ); }
	//! Get the current pulse width. \see setWidth()
	float			getWidth() const		{ return mWidth.getValue(); }
	//! Returns the Param associated with the width (aka 'duty cycle').  Expected range is between [0:1].
	Param* getParamWidth()			{ return &mWidth; }

  protected:
	void initialize() override;
	void process( Buffer *buffer ) override;

	dsp::WaveTable2dRef		mWaveTable;
	BufferDynamic			mBuffer2;
	Param					mWidth;
};

} } // namespace cinder::audio2