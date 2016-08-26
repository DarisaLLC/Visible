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

#include "cinder/audio2/Voice.h"
#include "cinder/audio2/Context.h"
#include "cinder/audio2/NodeEffect.h"
#include "cinder/audio2/Debug.h"

#include <map>

using namespace std;
using namespace ci;

namespace cinder { namespace audio2 {

// ----------------------------------------------------------------------------------------------------
// MARK: - MixerImpl
// ----------------------------------------------------------------------------------------------------

// TODO: replace this private Mixer, which composits a Gain + NodePan per voice, into a NodeMixer
// that has a custom pullInputs and performs that gain / pan as a post-processing step

class MixerImpl {
public:

	static MixerImpl *get();

	//! returns the number of connected busses.
	void	setBusVolume( size_t busId, float volume );
	float	getBusVolume( size_t busId );
	void	setBusPan( size_t busId, float pos );
	float	getBusPan( size_t busId );
	NodeRef getOutputNode( size_t busId ) const;

	void	addVoice( const VoiceRef &source, bool shouldConnectToMaster );
	void	removeVoice( size_t busId );

	BufferRef loadBuffer( const SourceFileRef &sourceFile, size_t numChannels );

private:
	MixerImpl();

	struct Bus {
		weak_ptr<Voice>		mVoice; // stored as weak reference so that when the user's VoiceRef goes out of scope, it will be removed.
		GainRef				mGain;
		Pan2dRef			mPan;
	};

	size_t getFirstAvailableBusId() const;

	map<size_t, Bus> mBusses;											// key is bus id
	map<std::pair<SourceFileRef, size_t>, BufferRef> mBufferCache;		// key is [shared_ptr, num channels]
};

MixerImpl* MixerImpl::get()
{
	static unique_ptr<MixerImpl> sMixer;
	if( ! sMixer )
		sMixer.reset( new MixerImpl );

	return sMixer.get();
}

MixerImpl::MixerImpl()
{
	Context::master()->enable();
}

void MixerImpl::addVoice( const VoiceRef &source, bool shouldConnectToMaster )
{
	Context *ctx = Context::master();

	source->mBusId = getFirstAvailableBusId();

	MixerImpl::Bus &bus = mBusses[source->mBusId];

	bus.mVoice = source;
	bus.mGain = ctx->makeNode( new Gain() );
	bus.mPan = ctx->makeNode( new Pan2d() );

	source->getInputNode() >> bus.mGain >> bus.mPan;

	if( shouldConnectToMaster )
		bus.mPan >> ctx->getOutput();
}

void MixerImpl::removeVoice( size_t busId )
{
	auto it = mBusses.find( busId );
	CI_ASSERT( it != mBusses.end() );

	it->second.mPan->disconnectAllOutputs();
	mBusses.erase( it );
}

BufferRef MixerImpl::loadBuffer( const SourceFileRef &sourceFile, size_t numChannels )
{
	auto key = make_pair( sourceFile, numChannels );
	auto cached = mBufferCache.find( key );
	if( cached != mBufferCache.end() )
		return cached->second;
	else {
		BufferRef result = sourceFile->loadBuffer();
		mBufferCache.insert( make_pair( key, result ) );
		return result;
	}
}

size_t MixerImpl::getFirstAvailableBusId() const
{
	size_t result = 0;
	for( const auto &bus : mBusses ) {
		if( bus.first != result )
			break;

		result++;
	}

	return result;
}

void MixerImpl::setBusVolume( size_t busId, float volume )
{
	mBusses[busId].mGain->setValue( volume );
}

float MixerImpl::getBusVolume( size_t busId )
{
	return mBusses[busId].mGain->getValue();
}

void MixerImpl::setBusPan( size_t busId, float pos )
{
	auto pan = mBusses[busId].mPan;
	if( pan )
		pan->setPos( pos );
}

float MixerImpl::getBusPan( size_t busId )
{
	auto pan = mBusses[busId].mPan;
	if( pan )
		return mBusses[busId].mPan->getPos();

	return 0;
}

NodeRef MixerImpl::getOutputNode( size_t busId ) const
{
	const auto &bus = mBusses.at( busId );

	if( bus.mPan )
		return bus.mPan;
	else
		return bus.mGain;
}

// ----------------------------------------------------------------------------------------------------
// MARK: - Voice
// ----------------------------------------------------------------------------------------------------

VoiceRef Voice::create( const CallbackProcessorFn &callbackFn, const Options &options )
{
	VoiceRef result( new VoiceCallbackProcessor( callbackFn, options ) );
	MixerImpl::get()->addVoice( result, options.getConnectToMaster() );

	return result;
}

VoiceSamplePlayerRef Voice::create( const SourceFileRef &sourceFile, const Options &options )
{
	VoiceSamplePlayerRef result( new VoiceSamplePlayer( sourceFile, options ) );
	MixerImpl::get()->addVoice( result, options.getConnectToMaster() );

	return result;
}

Voice::~Voice()
{
	MixerImpl::get()->removeVoice( mBusId );
}

float Voice::getVolume() const
{
	return MixerImpl::get()->getBusVolume( mBusId );
}

float Voice::getPan() const
{
	return MixerImpl::get()->getBusPan( mBusId );
}

void Voice::setVolume( float volume )
{
	MixerImpl::get()->setBusVolume( mBusId, volume );
}

void Voice::setPan( float pan )
{
	MixerImpl::get()->setBusPan( mBusId, pan );
}

void Voice::start()
{
	getInputNode()->enable();
}

void Voice::pause()
{
	getInputNode()->disable();
}

void Voice::stop()
{
	getInputNode()->disable();
}

bool Voice::isPlaying() const
{
	return getInputNode()->isEnabled();
}

NodeRef Voice::getOutputNode() const
{
	return MixerImpl::get()->getOutputNode( mBusId );
}

// ----------------------------------------------------------------------------------------------------
// MARK: - VoiceSamplePlayer
// ----------------------------------------------------------------------------------------------------

VoiceSamplePlayer::VoiceSamplePlayer( const SourceFileRef &sourceFile, const Options &options )
{
	sourceFile->setOutputFormat( audio2::master()->getSampleRate(), options.getChannels() );

	if( sourceFile->getNumFrames() <= options.getMaxFramesForBufferPlayback() ) {
		BufferRef buffer = MixerImpl::get()->loadBuffer( sourceFile, options.getChannels() );
		mNode = Context::master()->makeNode( new BufferPlayer( buffer ) );
	} else
		mNode = Context::master()->makeNode( new FilePlayer( sourceFile ) );
}

void VoiceSamplePlayer::start()
{
	if( mNode->isEof() )
		mNode->start();

	mNode->enable();
}

void VoiceSamplePlayer::stop()
{
	mNode->stop();
}

// ----------------------------------------------------------------------------------------------------
// MARK: - VoiceCallbackProcessor
// ----------------------------------------------------------------------------------------------------

VoiceCallbackProcessor::VoiceCallbackProcessor( const CallbackProcessorFn &callbackFn, const Options &options )
{
	mNode = Context::master()->makeNode( new CallbackProcessor( callbackFn, Node::Format().channels( options.getChannels() ) ) );
}

} } // namespace cinder::audio2