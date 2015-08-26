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

#if( _WIN32_WINNT >= _WIN32_WINNT_VISTA )

#include "cinder/audio2/msw/ContextWasapi.h"
#include "cinder/audio2/msw/DeviceManagerWasapi.h"
#include "cinder/audio2/msw/MswUtil.h"
#include "cinder/audio2/Context.h"
#include "cinder/audio2/dsp/Dsp.h"
#include "cinder/audio2/dsp/RingBuffer.h"
#include "cinder/audio2/dsp/Converter.h"
#include "cinder/audio2/Exception.h"
#include "cinder/audio2/CinderAssert.h"
#include "cinder/audio2/Debug.h"

#include <Audioclient.h>
#include <mmdeviceapi.h>
#include <avrt.h>
#pragma comment(lib, "avrt.lib")

namespace {

// TODO: should requestedDuration come from Device's frames per block?
// - seems like this is fixed at 10ms in shared mode. (896 samples @ stereo 44100 s/r) 
const size_t DEFAULT_AUDIOCLIENT_FRAMES = 1024;
//! When there are mismatched samplerates between LineOut and LineIn, the capture AudioClient's buffer needs to be larger.
const size_t CAPTURE_CONVERSION_PADDING_FACTOR = 2;

// converts to 100-nanoseconds
inline ::REFERENCE_TIME samplesToReferenceTime( size_t samples, size_t sampleRate )
{
	return (::REFERENCE_TIME)( (double)samples * 10000000.0 / (double)sampleRate );
}

} // anonymous namespace

using namespace std;

namespace cinder { namespace audio2 { namespace msw {

struct WasapiAudioClientImpl {
	WasapiAudioClientImpl();

	unique_ptr<::IAudioClient, ComReleaser>			mAudioClient;

	size_t	mNumFramesBuffered, mAudioClientNumFrames, mNumChannels;

  protected:
	void initAudioClient( const DeviceRef &device, size_t numChannels, HANDLE eventHandle );
};

struct WasapiRenderClientImpl : public WasapiAudioClientImpl {
	WasapiRenderClientImpl( LineOutWasapi *lineOut );
	~WasapiRenderClientImpl();

	void init();
	void uninit();

	unique_ptr<::IAudioRenderClient, ComReleaser>	mRenderClient;

	::HANDLE	mRenderSamplesReadyEvent, mRenderShouldQuitEvent;
	::HANDLE    mRenderThread;

	std::unique_ptr<dsp::RingBuffer>	mRingBuffer;

private:
	void initRenderClient();
	void runRenderThread();
	void renderAudio();
	void increaseThreadPriority();

	static DWORD __stdcall renderThreadEntryPoint( LPVOID Context );

	LineOutWasapi*	mLineOut; // weak pointer to parent
};

struct WasapiCaptureClientImpl : public WasapiAudioClientImpl {
	WasapiCaptureClientImpl( LineInWasapi *lineOut );

	void init();
	void uninit();
	void captureAudio();

	unique_ptr<::IAudioCaptureClient, ComReleaser>	mCaptureClient;

	vector<dsp::RingBuffer>				mRingBuffers;

  private:
	void initCapture();

	std::unique_ptr<dsp::Converter>		mConverter;
	BufferInterleaved					mInterleavedBuffer;
	BufferDynamic						mReadBuffer, mConvertedReadBuffer;
	size_t								mMaxReadFrames;

	LineInWasapi*		mLineIn; // weak pointer to parent
};

// ----------------------------------------------------------------------------------------------------
// MARK: - WasapiAudioClientImpl
// ----------------------------------------------------------------------------------------------------

WasapiAudioClientImpl::WasapiAudioClientImpl()
	: mAudioClientNumFrames( DEFAULT_AUDIOCLIENT_FRAMES ), mNumFramesBuffered( 0 ), mNumChannels( 0 )
{
}

void WasapiAudioClientImpl::initAudioClient( const DeviceRef &device, size_t numChannels, HANDLE eventHandle )
{
	CI_ASSERT( ! mAudioClient );

	DeviceManagerWasapi *manager = dynamic_cast<DeviceManagerWasapi *>( Context::deviceManager() );
	CI_ASSERT( manager );

	//auto device = mLineIn->getDevice();
	shared_ptr<::IMMDevice> immDevice = manager->getIMMDevice( device );

	::IAudioClient *audioClient;
	HRESULT hr = immDevice->Activate( __uuidof(::IAudioClient), CLSCTX_ALL, NULL, (void**)&audioClient );
	CI_ASSERT( hr == S_OK );
	mAudioClient = makeComUnique( audioClient );

	size_t sampleRate = device->getSampleRate();
	auto wfx = interleavedFloatWaveFormat( sampleRate, numChannels );
	::WAVEFORMATEX *closestMatch;
	hr = mAudioClient->IsFormatSupported( ::AUDCLNT_SHAREMODE_SHARED, wfx.get(), &closestMatch );
	if( hr == S_FALSE ) {
		CI_ASSERT_MSG( closestMatch, "expected closestMatch" );

		// If possible, update wfx to the closestMatch. Currently this can only be done if the channels are different.
		if( closestMatch->wFormatTag != wfx->wFormatTag )
			throw AudioFormatExc( "IAudioClient requested WAVEFORMATEX 'closest match' of unexpected format type." );
		if( closestMatch->cbSize != wfx->cbSize )
			throw AudioFormatExc( "IAudioClient requested WAVEFORMATEX 'closest match' of unexpected cbSize." );
		if( closestMatch->nSamplesPerSec != wfx->nSamplesPerSec )
			throw AudioFormatExc( "IAudioClient requested WAVEFORMATEX 'closest match' of unexpected samplerate." );

		CI_LOG_V( "requested format type wasn't accepted, requested channels: " << numChannels << ", closestMatch channels: " << closestMatch->nChannels );

		wfx->nChannels = closestMatch->nChannels;
		wfx->nAvgBytesPerSec = closestMatch->nAvgBytesPerSec;
		wfx->nBlockAlign = closestMatch->nBlockAlign;
		wfx->wBitsPerSample = closestMatch->wBitsPerSample;

		::CoTaskMemFree( closestMatch );
	}
	else if( hr != S_OK )
		throw AudioFormatExc( "Could not find a suitable format for IAudioClient" );

	mNumChannels = wfx->nChannels; // in preparation for using closesMatch

	::REFERENCE_TIME requestedDuration = samplesToReferenceTime( mAudioClientNumFrames, sampleRate );
	DWORD streamFlags = eventHandle ? AUDCLNT_STREAMFLAGS_EVENTCALLBACK : 0;

	hr = mAudioClient->Initialize( ::AUDCLNT_SHAREMODE_SHARED, streamFlags, requestedDuration, 0, wfx.get(), NULL ); 
	CI_ASSERT( hr == S_OK );

	if( eventHandle ) {
		// enable event driven rendering.
		HRESULT hr = mAudioClient->SetEventHandle( eventHandle );
		CI_ASSERT( hr == S_OK );
	}

	UINT32 actualNumFrames;
	hr = mAudioClient->GetBufferSize( &actualNumFrames );
	CI_ASSERT( hr == S_OK );

	mAudioClientNumFrames = actualNumFrames; // update with the actual size
}

// ----------------------------------------------------------------------------------------------------
// MARK: - WasapiRenderClientImpl
// ----------------------------------------------------------------------------------------------------

WasapiRenderClientImpl::WasapiRenderClientImpl( LineOutWasapi *lineOut )
	: WasapiAudioClientImpl(), mLineOut( lineOut )
{
	// create render events
	mRenderSamplesReadyEvent = ::CreateEvent( NULL, FALSE, FALSE, NULL );
	CI_ASSERT( mRenderSamplesReadyEvent );

	mRenderShouldQuitEvent = ::CreateEvent( NULL, FALSE, FALSE, NULL );
	CI_ASSERT( mRenderShouldQuitEvent );
}

WasapiRenderClientImpl::~WasapiRenderClientImpl()
{
	if( mRenderSamplesReadyEvent ) {
		BOOL success = ::CloseHandle( mRenderSamplesReadyEvent );
		CI_ASSERT( success );
	}
	if( mRenderShouldQuitEvent ) {
		BOOL success = ::CloseHandle( mRenderShouldQuitEvent );
		CI_ASSERT( success );
	}
}

void WasapiRenderClientImpl::init()
{
	// reset events in case there are in a signaled state from a previous use.
	BOOL success = ::ResetEvent( mRenderShouldQuitEvent );
	CI_ASSERT( success );
	success = ::ResetEvent( mRenderSamplesReadyEvent );
	CI_ASSERT( success );

	initAudioClient( mLineOut->getDevice(), mLineOut->getNumChannels(), mRenderSamplesReadyEvent );
	initRenderClient();
}

void WasapiRenderClientImpl::uninit()
{
	// signal quit event and join the render thread once it completes.
	BOOL success = ::SetEvent( mRenderShouldQuitEvent );
	CI_ASSERT( success );

	::WaitForSingleObject( mRenderThread, INFINITE );
	CloseHandle( mRenderThread );
	mRenderThread = NULL;

	// Release() IAudioRenderClient IAudioClient
	mRenderClient.reset();
	mAudioClient.reset();
}

void WasapiRenderClientImpl::initRenderClient()
{
	CI_ASSERT( mAudioClient );

	::IAudioRenderClient *renderClient;
	HRESULT hr = mAudioClient->GetService( __uuidof(::IAudioRenderClient), (void**)&renderClient );
	CI_ASSERT( hr == S_OK );
	mRenderClient = makeComUnique( renderClient );

	// set the ring bufer size to accomodate the IAudioClient's buffer size (in samples) plus one extra processing block size, to account for uneven sizes.
	const size_t ringBufferSize = ( mAudioClientNumFrames + mLineOut->getFramesPerBlock() ) * mNumChannels;
	mRingBuffer.reset( new dsp::RingBuffer( ringBufferSize ) );

	mRenderThread = ::CreateThread( NULL, 0, renderThreadEntryPoint, this, 0, NULL );
	CI_ASSERT( mRenderThread );
}

// static
DWORD WasapiRenderClientImpl::renderThreadEntryPoint( ::LPVOID lpParameter )
{
	WasapiRenderClientImpl *renderer = static_cast<WasapiRenderClientImpl *>( lpParameter );
	renderer->runRenderThread();

	return 0;
}

void WasapiRenderClientImpl::runRenderThread()
{
	increaseThreadPriority();

	HANDLE waitEvents[2] = { mRenderShouldQuitEvent, mRenderSamplesReadyEvent };
	bool running = true;

	while( running ) {
		DWORD waitResult = ::WaitForMultipleObjects( 2, waitEvents, FALSE, INFINITE );
		switch( waitResult ) {
			case WAIT_OBJECT_0 + 0:     // mRenderShouldQuitEvent
				running = false;
				break;
			case WAIT_OBJECT_0 + 1:		// mRenderSamplesReadyEvent
				renderAudio();
				break;
			default:
				CI_ASSERT_NOT_REACHABLE();
		}
	}
}

void WasapiRenderClientImpl::renderAudio()
{
	// the current padding represents the number of frames queued on the audio endpoint, waiting to be sent to hardware.
	UINT32 numFramesPadding;
	HRESULT hr = mAudioClient->GetCurrentPadding( &numFramesPadding );
	CI_ASSERT( hr == S_OK );

	size_t numWriteFramesAvailable = mAudioClientNumFrames - numFramesPadding;

	while( mNumFramesBuffered < numWriteFramesAvailable )
		mLineOut->renderInputs();

	float *renderBuffer;
	hr = mRenderClient->GetBuffer( numWriteFramesAvailable, (BYTE **)&renderBuffer );
	CI_ASSERT( hr == S_OK );

	DWORD bufferFlags = 0;
	size_t numReadSamples = numWriteFramesAvailable * mNumChannels;
	bool readSuccess = mRingBuffer->read( renderBuffer, numReadSamples );
	CI_ASSERT( readSuccess ); // since this is sync read / write, the read should always succeed.

	mNumFramesBuffered -= numWriteFramesAvailable;

	hr = mRenderClient->ReleaseBuffer( numWriteFramesAvailable, bufferFlags );
	CI_ASSERT( hr == S_OK );
}

// This uses the "Multimedia Class Scheduler Service" (MMCSS) to increase the priority of the current thread.
// The priority increase can be seen in the threads debugger, it should have Priority = "Time Critical"
void WasapiRenderClientImpl::increaseThreadPriority()
{
	DWORD taskIndex = 0;
	::HANDLE avrtHandle = ::AvSetMmThreadCharacteristics( L"Pro Audio", &taskIndex );
	if( ! avrtHandle )
		CI_LOG_W( "Unable to enable MMCSS for 'Pro Audio', error: " << GetLastError() );
}

// ----------------------------------------------------------------------------------------------------
// MARK: - WasapiCaptureClientImpl
// ----------------------------------------------------------------------------------------------------

WasapiCaptureClientImpl::WasapiCaptureClientImpl( LineInWasapi *lineIn )
	: WasapiAudioClientImpl(), mLineIn( lineIn ), mMaxReadFrames( 0 )
{
}

void WasapiCaptureClientImpl::init()
{
	auto device = mLineIn->getDevice();
	bool needsConverter = device->getSampleRate() != mLineIn->getSampleRate();

	if( needsConverter )
		mAudioClientNumFrames *= CAPTURE_CONVERSION_PADDING_FACTOR;

	initAudioClient( device, mLineIn->getNumChannels(), nullptr );
	initCapture();

	mMaxReadFrames = mAudioClientNumFrames;

	for( size_t ch = 0; ch < mNumChannels; ch++ )
		mRingBuffers.emplace_back( mMaxReadFrames * mNumChannels );

	mInterleavedBuffer = BufferInterleaved( mMaxReadFrames, mNumChannels );
	mReadBuffer.setSize( mMaxReadFrames, mNumChannels );

	if( needsConverter ) {
		mConverter = audio2::dsp::Converter::create( device->getSampleRate(), mLineIn->getSampleRate(), mNumChannels, mLineIn->getNumChannels(), mMaxReadFrames );
		mConvertedReadBuffer.setSize( mConverter->getDestMaxFramesPerBlock(), mNumChannels );
		CI_LOG_V( "created Converter for samplerate: " << mConverter->getSourceSampleRate() << " -> " << mConverter->getDestSampleRate() << ", channels: " << mConverter->getSourceNumChannels() << " -> " << mConverter->getDestNumChannels() );
	}
}

void WasapiCaptureClientImpl::uninit()
{
	// Release() the maintained IAudioCaptureClient and IAudioClient
	mCaptureClient.reset();
	mAudioClient.reset();
}

void WasapiCaptureClientImpl::initCapture()
{
	CI_ASSERT( mAudioClient );

	::IAudioCaptureClient *captureClient;
	HRESULT hr = mAudioClient->GetService( __uuidof(::IAudioCaptureClient), (void**)&captureClient );
	CI_ASSERT( hr == S_OK );

	mCaptureClient = makeComUnique( captureClient );
}

void WasapiCaptureClientImpl::captureAudio()
{
	UINT32 numPacketFrames;
	HRESULT hr = mCaptureClient->GetNextPacketSize( &numPacketFrames );
	CI_ASSERT( hr == S_OK );

	while( numPacketFrames ) {
		if( numPacketFrames > ( mMaxReadFrames - mNumFramesBuffered ) ) {
			mLineIn->markOverrun();
			return;
		}
	
		BYTE *audioData;
		UINT32 numFramesAvailable;
		DWORD flags;
		HRESULT hr = mCaptureClient->GetBuffer( &audioData, &numFramesAvailable, &flags, NULL, NULL );
		if( hr == AUDCLNT_S_BUFFER_EMPTY )
			return;
		else
			CI_ASSERT( hr == S_OK );

		const size_t numSamples = numFramesAvailable * mNumChannels;
		mReadBuffer.setNumFrames( numFramesAvailable );

		if( mNumChannels == 1 )
			memcpy( mReadBuffer.getData(), audioData, numSamples * sizeof( float ) );
		else {
			memcpy( mInterleavedBuffer.getData(), audioData, numSamples * sizeof( float ) );
			dsp::deinterleaveBuffer( &mInterleavedBuffer, &mReadBuffer );
		}

		if( mConverter ) {
			pair<size_t, size_t> count = mConverter->convert( &mReadBuffer, &mConvertedReadBuffer );
			for( size_t ch = 0; ch < mNumChannels; ch++ ) {
				if( ! mRingBuffers[ch].write( mConvertedReadBuffer.getChannel( ch ), count.second ) )
					mLineIn->markOverrun();
			}
			mNumFramesBuffered += count.second;
		}
		else {
			for( size_t ch = 0; ch < mNumChannels; ch++ ) {
				if( ! mRingBuffers[ch].write( mReadBuffer.getChannel( ch ), numFramesAvailable ) )
					mLineIn->markOverrun();
			}
			mNumFramesBuffered += numFramesAvailable;
		}

		hr = mCaptureClient->ReleaseBuffer( numFramesAvailable );
		CI_ASSERT( hr == S_OK );

		hr = mCaptureClient->GetNextPacketSize( &numPacketFrames );
		CI_ASSERT( hr == S_OK );
	}
}

// ----------------------------------------------------------------------------------------------------
// MARK: - LineOutWasapi
// ----------------------------------------------------------------------------------------------------

LineOutWasapi::LineOutWasapi( const DeviceRef &device, const Format &format )
	: LineOut( device, format ), mRenderImpl( new WasapiRenderClientImpl( this ) )
{
}

void LineOutWasapi::initialize()
{
	mRenderImpl->init();

	if( getNumChannels() != mRenderImpl->mNumChannels ) {
		CI_LOG_W( "number of channels is unsuitable for IAudioClient, requested: " << getNumChannels() << ", closest match: " << mRenderImpl->mNumChannels );

		// kludge: Update LineOut's channels to match the number requested from IAudioClient.
		// - this will call Node::uninitializeImpl(), but that won't do anything because we haven't completed our initialization yet.
		setNumChannels( mRenderImpl->mNumChannels );
	}

	setupProcessWithSumming();
	mInterleavedBuffer = BufferInterleaved( getFramesPerBlock(), getNumChannels() );
}

void LineOutWasapi::uninitialize()
{
	mRenderImpl->uninit();
}

void LineOutWasapi::enableProcessing()
{
	HRESULT hr = mRenderImpl->mAudioClient->Start();
	CI_ASSERT( hr == S_OK );
}

void LineOutWasapi::disableProcessing()
{
	HRESULT hr = mRenderImpl->mAudioClient->Stop();
	CI_ASSERT( hr == S_OK );
}

void LineOutWasapi::renderInputs()
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

	dsp::interleaveBuffer( internalBuffer, &mInterleavedBuffer );
	bool writeSuccess = mRenderImpl->mRingBuffer->write( mInterleavedBuffer.getData(), mInterleavedBuffer.getSize() );
	CI_ASSERT( writeSuccess ); // Since this is sync read / write, the write should always succeed.

	mRenderImpl->mNumFramesBuffered += mInterleavedBuffer.getNumFrames();
	postProcess();
}

// ----------------------------------------------------------------------------------------------------
// MARK: - LineInWasapi
// ----------------------------------------------------------------------------------------------------

LineInWasapi::LineInWasapi( const DeviceRef &device, const Format &format )
	: LineIn( device, format ), mCaptureImpl( new WasapiCaptureClientImpl( this ) )
{
}

LineInWasapi::~LineInWasapi()
{
}

void LineInWasapi::initialize()
{
	mCaptureImpl->init();
}

void LineInWasapi::uninitialize()
{
	mCaptureImpl->uninit();
}

void LineInWasapi::enableProcessing()
{
	HRESULT hr = mCaptureImpl->mAudioClient->Start();
	CI_ASSERT( hr == S_OK );	
}

void LineInWasapi::disableProcessing()
{
	HRESULT hr = mCaptureImpl->mAudioClient->Stop();
	CI_ASSERT( hr == S_OK );
}

void LineInWasapi::process( Buffer *buffer )
{
	mCaptureImpl->captureAudio();

	const size_t framesNeeded = buffer->getNumFrames();
	if( mCaptureImpl->mNumFramesBuffered < framesNeeded ) {
		markUnderrun();
		return;
	}

	for( size_t ch = 0; ch < getNumChannels(); ch++ ) {
		bool readSuccess = mCaptureImpl->mRingBuffers[ch].read( buffer->getChannel( ch ), framesNeeded );
		CI_ASSERT( readSuccess );
	}

	mCaptureImpl->mNumFramesBuffered -= framesNeeded;
}

// ----------------------------------------------------------------------------------------------------
// MARK: - ContextWasapi
// ----------------------------------------------------------------------------------------------------

LineOutRef ContextWasapi::createLineOut( const DeviceRef &device, const Node::Format &format )
{
	return makeNode( new LineOutWasapi( device, format ) );
}

LineInRef ContextWasapi::createLineIn( const DeviceRef &device, const Node::Format &format )
{
	return makeNode( new LineInWasapi( device, format ) );
}

} } } // namespace cinder::audio2::msw

#endif // ( _WIN32_WINNT >= _WIN32_WINNT_VISTA )
