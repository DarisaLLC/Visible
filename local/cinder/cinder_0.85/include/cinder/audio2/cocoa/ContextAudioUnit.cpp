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

#include "cinder/audio2/cocoa/ContextAudioUnit.h"
#include "cinder/audio2/cocoa/CinderCoreAudio.h"
#include "cinder/audio2/CinderAssert.h"
#include "cinder/audio2/Debug.h"

#include "cinder/Utilities.h"

#if defined( CINDER_MAC )
	#include "cinder/audio2/cocoa/DeviceManagerCoreAudio.h"
#else
	#include "cinder/audio2/cocoa/DeviceManagerAudioSession.h"
#endif

using namespace std;

namespace cinder { namespace audio2 { namespace cocoa {

enum DeviceBus { OUTPUT = 0, INPUT = 1 };

// ----------------------------------------------------------------------------------------------------
// MARK: - NodeAudioUnit
// ----------------------------------------------------------------------------------------------------

NodeAudioUnit::~NodeAudioUnit()
{
	if( mAudioUnit && mOwnsAudioUnit ) {
		OSStatus status = ::AudioComponentInstanceDispose( mAudioUnit );
		CI_ASSERT( status == noErr );
	}
}

void NodeAudioUnit::initAu()
{
	OSStatus status = ::AudioUnitInitialize( mAudioUnit );
	CI_ASSERT( status == noErr );
}

void NodeAudioUnit::uninitAu()
{
	OSStatus status = ::AudioUnitUninitialize( mAudioUnit );
	CI_ASSERT( status == noErr );
}

// ----------------------------------------------------------------------------------------------------
// MARK: - LineOutAudioUnit
// ----------------------------------------------------------------------------------------------------

LineOutAudioUnit::LineOutAudioUnit( const DeviceRef &device, const Format &format )
: LineOut( device, format ), mSynchronousIO( false )
{
	findAndCreateAudioComponent( getOutputAudioUnitDesc(), &mAudioUnit );
}

void LineOutAudioUnit::initialize()
{
	// LineOut always needs an internal buffer to deliver to the ouput AU, so force one to be made.
	setupProcessWithSumming();

	mRenderData.node = this;
	mRenderData.context = dynamic_cast<ContextAudioUnit *>( getContext().get() );

	::AudioStreamBasicDescription asbd = createFloatAsbd( getOutputSampleRate(), getNumChannels() );
	setAudioUnitProperty( mAudioUnit, kAudioUnitProperty_StreamFormat, asbd, kAudioUnitScope_Input, DeviceBus::OUTPUT );

	UInt32 enableOutput = 1;
	setAudioUnitProperty( mAudioUnit, kAudioOutputUnitProperty_EnableIO, enableOutput, kAudioUnitScope_Output, DeviceBus::OUTPUT );

	UInt32 enableInput = mSynchronousIO;
	setAudioUnitProperty( mAudioUnit, kAudioOutputUnitProperty_EnableIO, enableInput, kAudioUnitScope_Input, DeviceBus::INPUT );

	if( enableInput )
		setAudioUnitProperty( mAudioUnit, kAudioUnitProperty_StreamFormat, asbd, kAudioUnitScope_Output, DeviceBus::INPUT );


	::AURenderCallbackStruct callbackStruct { LineOutAudioUnit::renderCallback, &mRenderData };
	setAudioUnitProperty( mAudioUnit, kAudioUnitProperty_SetRenderCallback, callbackStruct, kAudioUnitScope_Input, DeviceBus::OUTPUT );

#if defined( CINDER_MAC )
	auto manager = dynamic_cast<DeviceManagerCoreAudio *>( Context::deviceManager() );
	CI_ASSERT( manager );

	manager->setCurrentOutputDevice( mDevice, mAudioUnit );
#endif

	initAu();
}

void LineOutAudioUnit::uninitialize()
{
	uninitAu();
}

void LineOutAudioUnit::enableProcessing()
{
	OSStatus status = ::AudioOutputUnitStart( mAudioUnit );
	CI_ASSERT( status == noErr );
}

void LineOutAudioUnit::disableProcessing()
{
	OSStatus status = ::AudioOutputUnitStop( mAudioUnit );
	CI_ASSERT( status == noErr );
}

OSStatus LineOutAudioUnit::renderCallback( void *data, ::AudioUnitRenderActionFlags *flags, const ::AudioTimeStamp *timeStamp, UInt32 busNumber, UInt32 numFrames, ::AudioBufferList *bufferList )
{
	RenderData *renderData = static_cast<NodeAudioUnit::RenderData *>( data );

	auto ctx = renderData->node->getContext();
	if( ! ctx ) {
		zeroBufferList( bufferList );
		return noErr;
	}

	lock_guard<mutex> lock( ctx->getMutex() );

	// verify associated context still exists proceeding, which may not be true if we blocked in ~Context() and were then subsequently deallocated.
	ctx = renderData->node->getContext();
	if( ! ctx ) {
		zeroBufferList( bufferList );
		return noErr;
	}

	LineOutAudioUnit *lineOut = static_cast<LineOutAudioUnit *>( renderData->node );
	Buffer *internalBuffer = lineOut->getInternalBuffer();
	internalBuffer->zero();

	renderData->context->setCurrentTimeStamp( timeStamp );
	lineOut->pullInputs( internalBuffer );

	// if clip detection is enabled and buffer clipped, silence it
	if( lineOut->checkNotClipping() )
		zeroBufferList( bufferList );
	else
		copyToBufferList( bufferList, internalBuffer );

	lineOut->postProcess();

	return noErr;
}

// ----------------------------------------------------------------------------------------------------
// MARK: - LineInAudioUnit
// ----------------------------------------------------------------------------------------------------

LineInAudioUnit::LineInAudioUnit( const DeviceRef &device, const Format &format )
: LineIn( device, format ), mSynchronousIO( false ), mRingBufferPaddingFactor( 2 )
{
}

LineInAudioUnit::~LineInAudioUnit()
{
}

void LineInAudioUnit::initialize()
{
	mRenderData.node = this;
	mRenderData.context = dynamic_cast<ContextAudioUnit *>( getContext().get() );
	auto device = getDevice();

	// see if synchronous I/O is possible by looking at the LineOut
	auto lineOutAu = dynamic_pointer_cast<LineOutAudioUnit>( getContext()->getOutput() );
	CI_ASSERT( lineOutAu );

	if( device == lineOutAu->getDevice() && getNumChannels() == lineOutAu->getNumChannels() ) {
		mSynchronousIO = true;
		mAudioUnit = lineOutAu->getAudioUnit();
		mOwnsAudioUnit = false;
	}
	else {
		// make our own AudioUnit, if we don't already have one (from a previous initialize())
		findAndCreateAudioComponent( getOutputAudioUnitDesc(), &mAudioUnit );

		mSynchronousIO = false;
		mOwnsAudioUnit = true;
	}

	size_t framesPerBlock = lineOutAu->getOutputFramesPerBlock();
	size_t sampleRate = lineOutAu->getOutputSampleRate();
	::AudioStreamBasicDescription asbd = createFloatAsbd( sampleRate, getNumChannels() );

	if( mSynchronousIO ) {
		// LineOutAudioUnit is expected to initialize the AudioUnit, since it is pulling to here. But make sure input is enabled.
		// TODO: this path can surely be optimized to not require line out being initialized twice
		lineOutAu->mSynchronousIO = true;
		bool lineOutWasInitialized = lineOutAu->isInitialized();
		bool lineOutWasEnabled = lineOutAu->isEnabled();
		if( lineOutWasInitialized ) {
			lineOutAu->disable();
			lineOutAu->uninitialize();
		}

		mBufferList = createNonInterleavedBufferList( framesPerBlock, getNumChannels() );

		if( lineOutWasInitialized )
			lineOutAu->initialize();

		if( lineOutWasEnabled )
			lineOutAu->setEnabled();
	}
	else {
		if( device->getSampleRate() != sampleRate || device->getFramesPerBlock() != framesPerBlock )
			device->updateFormat( Device::Format().sampleRate( sampleRate ).framesPerBlock( framesPerBlock ) );

		mRingBuffer.resize( framesPerBlock * getNumChannels() * mRingBufferPaddingFactor );
		mBufferList = createNonInterleavedBufferList( framesPerBlock, getNumChannels() );

		::AURenderCallbackStruct callbackStruct = { LineInAudioUnit::inputCallback, &mRenderData };
		setAudioUnitProperty( mAudioUnit, kAudioOutputUnitProperty_SetInputCallback, callbackStruct, kAudioUnitScope_Global, DeviceBus::INPUT );

		UInt32 enableInput = 1;
		setAudioUnitProperty( mAudioUnit, kAudioOutputUnitProperty_EnableIO, enableInput, kAudioUnitScope_Input, DeviceBus::INPUT );

		UInt32 enableOutput = 0;
		setAudioUnitProperty( mAudioUnit, kAudioOutputUnitProperty_EnableIO, enableOutput, kAudioUnitScope_Output, DeviceBus::OUTPUT );

		setAudioUnitProperty( mAudioUnit, kAudioUnitProperty_StreamFormat, asbd, kAudioUnitScope_Output, DeviceBus::INPUT );

#if defined( CINDER_MAC )
		auto manager = dynamic_cast<DeviceManagerCoreAudio *>( Context::deviceManager() );
		CI_ASSERT( manager );

		manager->setCurrentInputDevice( device, mAudioUnit );
#endif
		initAu();
	}
}

// TODO: what about when synchronous IO and this guy is requested to uninit, does associated LineOutAudioUnit need to be uninitialized too?
// - line out should notify line in we're going out
void LineInAudioUnit::uninitialize()
{
	if( ! mSynchronousIO ) {
		uninitAu();
	}
}

void LineInAudioUnit::enableProcessing()
{
	if( ! mSynchronousIO ) {
		OSStatus status = ::AudioOutputUnitStart( mAudioUnit );
		CI_ASSERT( status == noErr );
	}
}

void LineInAudioUnit::disableProcessing()
{
	if( ! mSynchronousIO ) {
		OSStatus status = ::AudioOutputUnitStop( mAudioUnit );
		CI_ASSERT( status == noErr );
	}
}

void LineInAudioUnit::process( Buffer *buffer )
{
	if( mSynchronousIO ) {
		mProcessBuffer = buffer;
		::AudioUnitRenderActionFlags flags = 0;
		const ::AudioTimeStamp *timeStamp = mRenderData.context->getCurrentTimeStamp();
		OSStatus status = ::AudioUnitRender( mAudioUnit, &flags, timeStamp, DeviceBus::INPUT, (UInt32)buffer->getNumFrames(), mBufferList.get() );
		CI_ASSERT( status == noErr );

		copyFromBufferList( buffer, mBufferList.get() );
	}
	else {
		// copy from ringbuffer. If not possible, store the timestamp of the underrun
		if( ! mRingBuffer.read( buffer->getData(), buffer->getSize() ) )
			markUnderrun();
	}
}

// note: Not all AudioUnitRender status errors are fatal here. For instance, if samplerate just changed we may not be able to pull input just yet, but we will next frame.
OSStatus LineInAudioUnit::inputCallback( void *data, ::AudioUnitRenderActionFlags *flags, const ::AudioTimeStamp *timeStamp, UInt32 bus, UInt32 numFrames, ::AudioBufferList *bufferList )
{
	RenderData *renderData = static_cast<NodeAudioUnit::RenderData *>( data );
	LineInAudioUnit *lineIn = static_cast<LineInAudioUnit *>( renderData->node );

	// this method is called async by Core Audio, so first check that our Context hasn't already been destroyed.
	if( ! lineIn->getContext() )
		return noErr;

	::AudioBufferList *nodeBufferList = lineIn->mBufferList.get();
	OSStatus status = ::AudioUnitRender( lineIn->getAudioUnit(), flags, timeStamp, DeviceBus::INPUT, numFrames, nodeBufferList );
	if( status != noErr )
		return status;

	if( lineIn->mRingBuffer.getAvailableWrite() >= nodeBufferList->mNumberBuffers * numFrames ) {
		for( size_t ch = 0; ch < nodeBufferList->mNumberBuffers; ch++ ) {
			float *channel = static_cast<float *>( nodeBufferList->mBuffers[ch].mData );
			lineIn->mRingBuffer.write( channel, numFrames );
		}
	}
	else
		lineIn->markOverrun();

	return noErr;
}

// ----------------------------------------------------------------------------------------------------
// MARK: - NodeEffectAudioUnit
// ----------------------------------------------------------------------------------------------------

NodeEffectAudioUnit::NodeEffectAudioUnit( UInt32 effectSubType, const Format &format )
: NodeEffect( format ), mEffectSubType( effectSubType )
{
}

NodeEffectAudioUnit::~NodeEffectAudioUnit()
{
}

void NodeEffectAudioUnit::initialize()
{
	mRenderData.node = this;
	mRenderData.context = dynamic_cast<ContextAudioUnit *>( getContext().get() );

	::AudioComponentDescription comp{ 0 };
	comp.componentType = kAudioUnitType_Effect;
	comp.componentSubType = mEffectSubType;
	comp.componentManufacturer = kAudioUnitManufacturer_Apple;
	findAndCreateAudioComponent( comp, &mAudioUnit );

	mBufferList = createNonInterleavedBufferList( getFramesPerBlock(), getNumChannels() );

	::AudioStreamBasicDescription asbd = createFloatAsbd( getSampleRate(), getNumChannels() );
	setAudioUnitProperty( mAudioUnit, kAudioUnitProperty_StreamFormat, asbd, kAudioUnitScope_Input, 0 );
	setAudioUnitProperty( mAudioUnit, kAudioUnitProperty_StreamFormat, asbd, kAudioUnitScope_Output, 0 );

	::AURenderCallbackStruct callbackStruct = { NodeEffectAudioUnit::renderCallback, &mRenderData };
	setAudioUnitProperty( mAudioUnit, kAudioUnitProperty_SetRenderCallback, callbackStruct, kAudioUnitScope_Input, 0 );

	initAu();
}

void NodeEffectAudioUnit::uninitialize()
{
	uninitAu();
}

// TODO: try pointing buffer list at processBuffer instead of copying
void NodeEffectAudioUnit::process( Buffer *buffer )
{
	mProcessBuffer = buffer;

	::AudioUnitRenderActionFlags flags = 0;
	const ::AudioTimeStamp *timeStamp = mRenderData.context->getCurrentTimeStamp();
	OSStatus status = ::AudioUnitRender( mAudioUnit, &flags, timeStamp, 0, (UInt32)buffer->getNumFrames(), mBufferList.get() );
	CI_ASSERT( status == noErr );

	copyFromBufferList( buffer, mBufferList.get() );
}

OSStatus NodeEffectAudioUnit::renderCallback( void *data, ::AudioUnitRenderActionFlags *flags, const ::AudioTimeStamp *timeStamp, UInt32 busNumber, UInt32 numFrames, ::AudioBufferList *bufferList )
{
	RenderData *renderData = static_cast<NodeAudioUnit::RenderData *>( data );
	NodeEffectAudioUnit *effectNode = static_cast<NodeEffectAudioUnit *>( renderData->node );

	copyToBufferList( bufferList, effectNode->mProcessBuffer );
	return noErr;
}

void NodeEffectAudioUnit::setParameter( ::AudioUnitParameterID paramId, float val )
{
	setAudioUnitParam( mAudioUnit, paramId, val, kAudioUnitScope_Global, 0 );
}

// ----------------------------------------------------------------------------------------------------
// MARK: - ContextAudioUnit
// ----------------------------------------------------------------------------------------------------

LineOutRef ContextAudioUnit::createLineOut( const DeviceRef &device, const NodeOutput::Format &format )
{
	return makeNode( new LineOutAudioUnit( device, format ) );
}

LineInRef ContextAudioUnit::createLineIn( const DeviceRef &device, const Node::Format &format )
{
	return makeNode( new LineInAudioUnit( device, format ) );
}

ContextAudioUnit::~ContextAudioUnit()
{
}

} } } // namespace cinder::audio2::cocoa