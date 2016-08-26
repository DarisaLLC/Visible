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

#include "cinder/audio2/SamplePlayer.h"
#include "cinder/audio2/Context.h"
#include "cinder/audio2/Debug.h"
#include "cinder/CinderMath.h"

using namespace ci;
using namespace std;

namespace cinder { namespace audio2 {

// ----------------------------------------------------------------------------------------------------
// MARK: - SamplePlayer
// ----------------------------------------------------------------------------------------------------

SamplePlayer::SamplePlayer( const Format &format )
	: NodeInput( format ), mNumFrames( 0 ), mReadPos( 0 ), mLoop( false ),
		mLoopBegin( 0 ), mLoopEnd( 0 )
{
}

void SamplePlayer::start()
{
	seek( 0 );
	enable();
}

void SamplePlayer::stop()
{
	seek( 0 );
	disable();
}

void SamplePlayer::setLoopBegin( size_t positionFrames )
{
	mLoopBegin = positionFrames < mLoopEnd ? positionFrames : mLoopEnd.load();
}

void SamplePlayer::setLoopEnd( size_t positionFrames )
{
	mLoopEnd = positionFrames < mNumFrames ? positionFrames : mNumFrames;

	if( mLoopBegin > mLoopEnd )
		mLoopBegin = mLoopEnd.load();
}

void SamplePlayer::seekToTime( double readPositionSeconds )
{
	seek( size_t( readPositionSeconds * (double)getSampleRate() ) );
}

void SamplePlayer::setLoopBeginTime( double positionSeconds )
{
	setLoopBegin( size_t( positionSeconds * (double)getSampleRate() ) );
}

void SamplePlayer::setLoopEndTime( double positionSeconds )
{
	setLoopEnd( size_t( positionSeconds * (double)getSampleRate() ) );
}

double SamplePlayer::getReadPositionTime() const
{
	return (double)mReadPos / (double)getSampleRate();
}

double SamplePlayer::getLoopBeginTime() const
{
	return (double)mLoopBegin / (double)getSampleRate();
}

double SamplePlayer::getLoopEndTime() const
{
	return (double)mLoopEnd / (double)getSampleRate();
}

double SamplePlayer::getNumSeconds() const
{
	return (double)mNumFrames / (double)getSampleRate();
}

// ----------------------------------------------------------------------------------------------------
// MARK: - BufferPlayer
// ----------------------------------------------------------------------------------------------------

BufferPlayer::BufferPlayer( const Format &format )
	: SamplePlayer( format )
{
}

BufferPlayer::BufferPlayer( const BufferRef &buffer, const Format &format )
	: SamplePlayer( format ), mBuffer( buffer )
{
	mNumFrames = mLoopEnd = mBuffer->getNumFrames();

	// force channel mode to match buffer
	setChannelMode( ChannelMode::SPECIFIED );
	setNumChannels( mBuffer->getNumChannels() );
}

void BufferPlayer::enableProcessing()
{
	if( ! mBuffer ) {
		CI_LOG_W( "no audio buffer, disabling." );
		disable();
		return;
	}

	mIsEof = false;
}

void BufferPlayer::disableProcessing()
{
}

void BufferPlayer::seek( size_t readPositionFrames )
{
	mIsEof = false;
	mReadPos = math<size_t>::clamp( readPositionFrames, 0, mNumFrames );
}

void BufferPlayer::setBuffer( const BufferRef &buffer )
{
	lock_guard<mutex> lock( getContext()->getMutex() );

	bool wasEnabled = isEnabled();
	if( wasEnabled )
		disable();

	if( getNumChannels() != buffer->getNumChannels() ) {
		setNumChannels( buffer->getNumChannels() );
		configureConnections();
	}

	mBuffer = buffer;
	mNumFrames = buffer->getNumFrames();

	if( ! mLoopEnd  || mLoopEnd > mNumFrames )
		mLoopEnd = mNumFrames;

	if( wasEnabled )
		enable();
}

void BufferPlayer::loadBuffer( const SourceFileRef &sourceFile )
{
	auto sf = sourceFile->clone();

	sf->setOutputFormat( getSampleRate() );
	setBuffer( sf->loadBuffer() );
}

void BufferPlayer::process( Buffer *buffer )
{
	size_t readPos = mReadPos;
	size_t numFrames = buffer->getNumFrames();
	size_t readEnd = mLoop ? mLoopEnd.load() : mNumFrames;
	size_t readCount = readEnd < readPos ? 0 : min( readEnd - readPos, numFrames );

	buffer->copyOffset( *mBuffer, readCount, 0, readPos );

	if( readCount < numFrames  ) {
		// TODO: if looping, copy from mLoopBegin instead of zero'ing
		buffer->zero( readCount, numFrames - readCount );

		if( mLoop ) {
			mReadPos.store( mLoopBegin );
			return;
		} else {
			mIsEof = true;
			disable();
		}
	}

	mReadPos += readCount;
}

// ----------------------------------------------------------------------------------------------------
// MARK: - FilePlayer
// ----------------------------------------------------------------------------------------------------

FilePlayer::FilePlayer( const Format &format )
	: SamplePlayer( format ), mRingBufferPaddingFactor( 2 ), mLastUnderrun( 0 ), mLastOverrun( 0 )
{
	// force channel mode to match buffer
	setChannelMode( ChannelMode::SPECIFIED );
}

FilePlayer::FilePlayer( const SourceFileRef &sourceFile, bool isReadAsync, const Format &format )
	: SamplePlayer( format ), mSourceFile( sourceFile ), mIsReadAsync( isReadAsync ), mRingBufferPaddingFactor( 2 ),
		mLastUnderrun( 0 ), mLastOverrun( 0 )
{
	// force channel mode to match buffer
	setChannelMode( ChannelMode::SPECIFIED );
	setNumChannels( mSourceFile->getNumChannels() );

	// framerate will be updated once SourceFile's output samplerate is set, in initialize().
}

FilePlayer::~FilePlayer()
{
	if( isInitialized() )
		destroyReadThreadImpl();
}

void FilePlayer::initialize()
{
	if( mSourceFile ) {
		mSourceFile->setOutputFormat( getSampleRate() );
		mNumFrames = mSourceFile->getNumFrames();
	}

	if( ! mLoopEnd  || mLoopEnd > mNumFrames )
		mLoopEnd = mNumFrames;

	mIoBuffer.setSize( mSourceFile->getMaxFramesPerRead(), getNumChannels() );

	for( size_t i = 0; i < getNumChannels(); i++ )
		mRingBuffers.emplace_back( mSourceFile->getMaxFramesPerRead() * mRingBufferPaddingFactor );

	mBufferFramesThreshold = mRingBuffers[0].getSize() / 2;

	if( mIsReadAsync ) {
		mAsyncReadShouldQuit = false;
		mReadThread = unique_ptr<thread>( new thread( bind( &FilePlayer::readAsyncImpl, this ) ) );
	}
}

void FilePlayer::uninitialize()
{
	destroyReadThreadImpl();
}

void FilePlayer::enableProcessing()
{
	if( ! mSourceFile ) {
		CI_LOG_W( "no source file, disabling." );
		disable();
		return;
	}

	mIsEof = false;
}

void FilePlayer::disableProcessing()
{
}

void FilePlayer::seek( size_t readPositionFrames )
{
	if( ! mSourceFile ) {
		CI_LOG_E( "no source file, returning." );
		return;
	}

	// Synchronize with the mutex that protects the read thread, which is different depending on if
	// read is async or sync (done on audio thread)
	mutex &m = mIsReadAsync ? mAsyncReadMutex : getContext()->getMutex();
	lock_guard<mutex> lock( m );

	mIsEof = false;
	seekImpl( readPositionFrames );
}

void FilePlayer::setSourceFile( const SourceFileRef &sourceFile )
{
	// update source's samplerate to match context
	sourceFile->setOutputFormat( getSampleRate(), sourceFile->getNumChannels() );

	lock_guard<mutex> lock( getContext()->getMutex() );

	bool wasEnabled = isEnabled();
	if( wasEnabled )
		disable();

	if( getNumChannels() != sourceFile->getNumChannels() ) {
		setNumChannels( sourceFile->getNumChannels() );
		configureConnections();
	}

	mSourceFile = sourceFile;
	mNumFrames = sourceFile->getNumFrames();

	if( ! mLoopEnd  || mLoopEnd > mNumFrames )
		mLoopEnd = mNumFrames;

	if( wasEnabled )
		enable();
}

uint64_t FilePlayer::getLastUnderrun()
{
	uint64_t result = mLastUnderrun;
	mLastUnderrun = 0;
	return result;
}

uint64_t FilePlayer::getLastOverrun()
{
	uint64_t result = mLastOverrun;
	mLastOverrun = 0;
	return result;
}

void FilePlayer::process( Buffer *buffer )
{
	size_t numFrames = buffer->getNumFrames();
	size_t readPos = mReadPos;
	size_t numReadAvail = mRingBuffers[0].getAvailableRead();

	if( numReadAvail < mBufferFramesThreshold ) {
		if( mIsReadAsync )
			mIssueAsyncReadCond.notify_one();
		else
			readImpl();
	}

	size_t readCount = std::min( numReadAvail, numFrames );

	for( size_t ch = 0; ch < buffer->getNumChannels(); ch++ ) {
		if( ! mRingBuffers[ch].read( buffer->getChannel( ch ), readCount ) )
			mLastUnderrun = getContext()->getNumProcessedFrames();
	}

	// zero any unused frames, handle loop or EOF
	if( readCount < numFrames ) {
		// TODO: if looping, should fill with samples from the beginning of file
		// - these should also already be in the ringbuffer, since a seek is done there as well. Rethink this path.
		buffer->zero( readCount, numFrames - readCount );

		readPos += readCount;
		if( mLoop && readPos >= mLoopEnd )
			seekImpl( mLoopBegin );
		else if( readPos >= mNumFrames ) {
			mIsEof = true;
			disable();
		}
	}
}

void FilePlayer::readAsyncImpl()
{
	size_t lastReadPos = mReadPos;
	while( true ) {
		unique_lock<mutex> lock( mAsyncReadMutex );
		mIssueAsyncReadCond.wait( lock );

		if( mAsyncReadShouldQuit )
			return;

		size_t readPos = mReadPos;
		if( readPos != lastReadPos )
			mSourceFile->seek( readPos );

		readImpl();
		lastReadPos = mReadPos;
	}
}

void FilePlayer::readImpl()
{
	size_t readPos = mReadPos;
	size_t availableWrite = mRingBuffers[0].getAvailableWrite();
	size_t readEnd = mLoop ? mLoopEnd.load() : mNumFrames;
	size_t numFramesToRead = readEnd < readPos ? 0 : min( availableWrite, readEnd - readPos );

	if( ! numFramesToRead ) {
		mLastOverrun = getContext()->getNumProcessedFrames();
		return;
	}

	mIoBuffer.setNumFrames( numFramesToRead );

	size_t numRead = mSourceFile->read( &mIoBuffer );
	mReadPos += numRead;

	for( size_t ch = 0; ch < getNumChannels(); ch++ ) {
		if( ! mRingBuffers[ch].write( mIoBuffer.getChannel( ch ), numRead ) ) {
			mLastOverrun = getContext()->getNumProcessedFrames();
			return;
		}
	}
}

void FilePlayer::seekImpl( size_t readPos )
{
	mReadPos = math<size_t>::clamp( readPos, 0, mNumFrames );

	// if async mode, readAsyncImpl() will notice mReadPos was updated and do the seek there.
	if( ! mIsReadAsync )
		mSourceFile->seek( mReadPos );
}

void FilePlayer::destroyReadThreadImpl()
{
	if( mIsReadAsync && mReadThread ) {
		mAsyncReadShouldQuit = true;
		mIssueAsyncReadCond.notify_one();
		mReadThread->join();
	}
}

} } // namespace cinder::audio2