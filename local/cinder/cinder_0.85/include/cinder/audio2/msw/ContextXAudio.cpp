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

#include "cinder/audio2/msw/ContextXAudio.h"

#if defined( CINDER_AUDIO_XAUDIO2 )

#include "cinder/audio2/msw/DeviceManagerWasapi.h"
#include "cinder/audio2/dsp/Dsp.h"
#include "cinder/audio2/dsp/Converter.h"
#include "cinder/audio2/Utilities.h"
#include "cinder/audio2/Exception.h"
#include "cinder/audio2/CinderAssert.h"
#include "cinder/audio2/Debug.h"

#include "cinder/Utilities.h"
#include "cinder/msw/CinderMsw.h"

using namespace std;

namespace cinder { namespace audio2 { namespace msw {

namespace {

const string DEFAULT_XAUDIO2_DEVICE_KEY = "xaudio2-default";
const size_t DEFAULT_XAUDIO2_FRAMES_PER_BLOCK = 512;

} // anonymous namespace

struct VoiceCallbackImpl : public ::IXAudio2VoiceCallback {
	VoiceCallbackImpl( LineOutXAudio *lineOut  ) : mLineOut( lineOut ) {}

	void renderIfNecessary()
	{
		::XAUDIO2_VOICE_STATE state;
#if defined( CINDER_XAUDIO_2_8 )
		mLineOut->mSourceVoice->GetState( &state, XAUDIO2_VOICE_NOSAMPLESPLAYED );
#else
		mLineOut->mSourceVoice->GetState( &state );
#endif
		if( state.BuffersQueued == 0 ) // This could be increased to 1 to decrease chances of underuns
			mLineOut->submitNextBuffer();
	}

	void _stdcall OnBufferEnd( void *pBufferContext )
	{
		// called when a buffer has been consumed, so check if we need to submit another
		renderIfNecessary();
	}

	void _stdcall OnVoiceProcessingPassStart( UINT32 SamplesRequired )
	{
		// called after Node::start(), and there after
		renderIfNecessary();
	}

	void _stdcall OnStreamEnd() {}
	void _stdcall OnVoiceProcessingPassEnd() {}
	void _stdcall OnBufferStart( void *pBufferContext ) {}
	void _stdcall OnLoopEnd( void *pBufferContext ) {}
	void _stdcall OnVoiceError( void *pBufferContext, HRESULT Error )
	{
		CI_LOG_E( "error: " << Error );
	}

	LineOutXAudio*			mLineOut; // weak pointer to parent
};

struct EngineCallbackImpl : public IXAudio2EngineCallback {

	void _stdcall OnProcessingPassStart() {}
	void _stdcall OnProcessingPassEnd ()  {}

	void _stdcall OnCriticalError( HRESULT Error )
	{
		CI_LOG_E( "error: " << Error );
	}
};

// ----------------------------------------------------------------------------------------------------
// MARK: - LineOutXAudio
// ----------------------------------------------------------------------------------------------------

LineOutXAudio::LineOutXAudio( DeviceRef device, const Format &format )
: LineOut( device, format ), mVoiceCallback( new VoiceCallbackImpl( this ) ), mSourceVoice( nullptr ),  mFilterEnabled( true )
{
}

LineOutXAudio::~LineOutXAudio()
{
}

void LineOutXAudio::initialize()
{
	CI_ASSERT_MSG( getNumChannels() <= 2, "number of channels greater than 2 is not supported." );

	setupProcessWithSumming();

	auto internalBuffer = getInternalBuffer();
	size_t numSamples = internalBuffer->getSize();

	memset( &mXAudioBuffer, 0, sizeof( mXAudioBuffer ) );
	mXAudioBuffer.AudioBytes = numSamples * sizeof( float );
	if( getNumChannels() == 2 ) {
		// setup stereo, XAudio2 requires interleaved samples so point at interleaved buffer
		mBufferInterleaved = BufferInterleaved( internalBuffer->getNumFrames(), internalBuffer->getNumChannels() );
		mXAudioBuffer.pAudioData = reinterpret_cast<BYTE *>( mBufferInterleaved.getData() );
	}
	else {
		// setup mono
		mXAudioBuffer.pAudioData = reinterpret_cast<BYTE *>( internalBuffer->getData() );
	}

	initSourceVoice();
}

void LineOutXAudio::uninitialize()
{
	if( mSourceVoice ) {
		mSourceVoice->DestroyVoice();
		mSourceVoice = nullptr;
	}
}

void LineOutXAudio::initSourceVoice()
{
	CI_ASSERT( ! mSourceVoice );

	auto context = dynamic_pointer_cast<ContextXAudio>( getContext() );

	auto wfx = msw::interleavedFloatWaveFormat( getSampleRate(), getNumChannels() );

	IXAudio2 *xaudio = context->getXAudio();
	UINT32 flags = ( mFilterEnabled ? XAUDIO2_VOICE_USEFILTER : 0 );
	HRESULT hr = xaudio->CreateSourceVoice( &mSourceVoice, wfx.get(), flags, XAUDIO2_DEFAULT_FREQ_RATIO, mVoiceCallback.get() );
	CI_ASSERT( hr == S_OK );

	CI_LOG_V( "complete" );
}

void LineOutXAudio::enableProcessing()
{
	HRESULT hr = mSourceVoice->Start();
	CI_ASSERT( hr ==S_OK );
}

void LineOutXAudio::disableProcessing()
{
	HRESULT hr = mSourceVoice->Stop();
	CI_ASSERT( hr == S_OK );
}

void LineOutXAudio::submitNextBuffer()
{
	auto ctx = getContext();
	if( ! ctx )
		return;

	lock_guard<mutex> lock( ctx->getMutex() );

	// verify context still exists, since its destructor may have been holding the lock
	ctx = getContext();
	if( ! ctx )
		return;

	auto internalBuffer = getInternalBuffer();

	internalBuffer->zero();
	pullInputs( internalBuffer );

	if( checkNotClipping() )
		internalBuffer->zero();

	if( getNumChannels() == 2 )
		dsp::interleaveStereoBuffer( internalBuffer, &mBufferInterleaved );

	HRESULT hr = mSourceVoice->SubmitSourceBuffer( &mXAudioBuffer );
	CI_ASSERT( hr == S_OK );

	postProcess();
}

// ----------------------------------------------------------------------------------------------------
// MARK: - ContextXAudio
// ----------------------------------------------------------------------------------------------------

ContextXAudio::ContextXAudio()
	: mXAudio( nullptr ), mMasteringVoice( nullptr ), mEngineCallback( new EngineCallbackImpl )
{
	initXAudio2();
	initMasteringVoice();

	// mXAudio is started at creation time, so stop it here as mEnabled = false
	mXAudio->StopEngine();
}

ContextXAudio::~ContextXAudio()
{
	CI_ASSERT_MSG( mMasteringVoice, "Expected to have a valid mastering voice" );

	// first ensure the IXAudio2SourceVoice destroyed by uninitializing the LineOutXAudio
	uninitializeNode( mOutput );

	mMasteringVoice->DestroyVoice();
	mXAudio->Release();
}

LineOutRef ContextXAudio::createLineOut( const DeviceRef &device, const Node::Format &format )
{
	return makeNode( new LineOutXAudio( device, format ) );
}

LineInRef ContextXAudio::createLineIn( const DeviceRef &device, const Node::Format &format )
{
	CI_LOG_W( "No LineIn available with XAudio2 path.");
	return LineInRef();
}

void ContextXAudio::setOutput( const NodeOutputRef &output )
{
	CI_LOG_W( "NodeOutput is not user-settable with XAUdio2 path." );
}

shared_ptr<LineOutXAudio> ContextXAudio::getLineOutXAudio() const
{
	auto result = dynamic_pointer_cast<LineOutXAudio>( mOutput );
	CI_ASSERT( result );

	return result;
}

void ContextXAudio::enable()
{
	HRESULT hr = mXAudio->StartEngine();
	CI_ASSERT( hr == S_OK );

	Context::enable();
}

void ContextXAudio::disable()
{
	mXAudio->StopEngine();

	Context::disable();
}

void ContextXAudio::initXAudio2()
{
#if defined( CINDER_XAUDIO_2_7 )
	CI_LOG_V( "CINDER_XAUDIO_2_7" );
	UINT32 flags = XAUDIO2_DEBUG_ENGINE;

	//! CoInitializeEx is only required by XAudio2.7
	// TODO: using the initializeCom() creates a thread-local shared object that may go down before ~Context()
	// - so temp soln. is to not use it, long term is to have it pass back ref counted object.
	//ci::msw::initializeCom(); 
	::CoInitializeEx( NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE );

#else
	CI_LOG_V( "CINDER_XAUDIO_2_8" );
	UINT32 flags = 0;
#endif

	HRESULT hr = ::XAudio2Create( &mXAudio, flags, XAUDIO2_DEFAULT_PROCESSOR );
	CI_ASSERT( hr == S_OK );
	hr = mXAudio->RegisterForCallbacks( mEngineCallback.get() );
	CI_ASSERT( hr == S_OK );

#if defined( CINDER_XAUDIO_2_8 )
	::XAUDIO2_DEBUG_CONFIGURATION debugConfig = {0};
	//debugConfig.TraceMask = XAUDIO2_LOG_ERRORS;
	//debugConfig.BreakMask = XAUDIO2_LOG_ERRORS;
	debugConfig.TraceMask = XAUDIO2_LOG_WARNINGS;
	debugConfig.BreakMask = XAUDIO2_LOG_WARNINGS;
	debugConfig.LogFunctionName = true;
	mXAudio->SetDebugConfiguration( &debugConfig );
#endif
}

void ContextXAudio::initMasteringVoice()
{
	// create the IXAudio2MasteringVoice that represents the default hardware device.
	HRESULT hr = mXAudio->CreateMasteringVoice( &mMasteringVoice );
	CI_ASSERT( hr == S_OK );
	CI_ASSERT( mMasteringVoice );
}

// ----------------------------------------------------------------------------------------------------
// MARK: - DeviceMaangerXAudio
// ----------------------------------------------------------------------------------------------------

DeviceManagerXAudio::DeviceManagerXAudio()
	: mDeviceDetailsRetrieved( false )
{

}

DeviceRef DeviceManagerXAudio::getDefaultOutput()
{
	if( ! mDefaultDevice )
		mDefaultDevice = addDevice( DEFAULT_XAUDIO2_DEVICE_KEY );

	return mDefaultDevice;
}

DeviceRef DeviceManagerXAudio::getDefaultInput()
{
	CI_LOG_W( "XAudio2 does not support hardware audio input." );
	return DeviceRef();
}

const std::vector<DeviceRef>& DeviceManagerXAudio::getDevices()
{
	if( mDevices.empty() )
		getDefaultDevice();

	return mDevices;
}

std::string DeviceManagerXAudio::getName( const DeviceRef &device )
{
	return DEFAULT_XAUDIO2_DEVICE_KEY;
}

size_t DeviceManagerXAudio::getNumInputChannels( const DeviceRef &device )
{
	return 0;
}

size_t DeviceManagerXAudio::getNumOutputChannels( const DeviceRef &device )
{
	if( ! mDeviceDetailsRetrieved )
		retrieveDeviceDetails();

	return mVoiceDetails.InputSampleRate;
}

size_t DeviceManagerXAudio::getSampleRate( const DeviceRef &device )
{
	if( ! mDeviceDetailsRetrieved )
		retrieveDeviceDetails();

	return mVoiceDetails.InputSampleRate;
}

size_t DeviceManagerXAudio::getFramesPerBlock( const DeviceRef &device )
{
	return DEFAULT_XAUDIO2_FRAMES_PER_BLOCK;
}

void DeviceManagerXAudio::setSampleRate( const DeviceRef &device, size_t sampleRate )
{
	CI_LOG_W( "setting samplerate not supported, instead use the OS control panel" );
}

void DeviceManagerXAudio::setFramesPerBlock( const DeviceRef &device, size_t framesPerBlock )
{
	CI_LOG_W( "setting frames per block not supported" );
}

const DeviceRef& DeviceManagerXAudio::getDefaultDevice()
{
	if( ! mDefaultDevice )
		mDefaultDevice = addDevice( DEFAULT_XAUDIO2_DEVICE_KEY );

	return mDefaultDevice;
}

void DeviceManagerXAudio::retrieveDeviceDetails()
{
	auto ctxXaudio = dynamic_cast<ContextXAudio *>( master() );
	CI_ASSERT_MSG( ctxXaudio, "DeviceManagerXAudio must be used in conjunction with ContextXaudio" );

	ctxXaudio->getMasteringVoice()->GetVoiceDetails( &mVoiceDetails );
}

} } } // namespace cinder::audio2::msw

#endif // defined( CINDER_AUDIO_XAUDIO2 )