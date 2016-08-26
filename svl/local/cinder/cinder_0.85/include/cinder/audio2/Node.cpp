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


#include "cinder/audio2/Node.h"
#include "cinder/audio2/Context.h"
#include "cinder/audio2/NodeEffect.h"
#include "cinder/audio2/dsp/Dsp.h"
#include "cinder/audio2/dsp/Converter.h"
#include "cinder/audio2/Debug.h"
#include "cinder/audio2/CinderAssert.h"
#include "cinder/audio2/Utilities.h"

#include "cinder/Utilities.h"

#include <limits>

using namespace std;

namespace cinder { namespace audio2 {

// ----------------------------------------------------------------------------------------------------
// MARK: - Node
// ----------------------------------------------------------------------------------------------------

Node::Node( const Format &format )
	: mInitialized( false ), mEnabled( false ),	mChannelMode( format.getChannelMode() ),
		mNumChannels( 1 ), mAutoEnabled( false ), mProcessInPlace( true ), mLastProcessedFrame( numeric_limits<uint64_t>::max() )
{
	if( format.getChannels() ) {
		mNumChannels = format.getChannels();
		mChannelMode = ChannelMode::SPECIFIED;
	}

	if( ! boost::indeterminate( format.getAutoEnable() ) )
		setAutoEnabled( format.getAutoEnable() );
}

Node::~Node()
{
}

void Node::connect( const NodeRef &output, size_t outputBus, size_t inputBus )
{
	// make a reference to ourselves so that we aren't deallocated in the case of the last owner
	// disconnecting us, which we may need later anyway
	NodeRef thisRef = shared_from_this();

	if( ! output->canConnectToInput( thisRef ) )
		return;

	if( checkCycle( thisRef, output ) )
		throw NodeCycleExc( thisRef, output );

	auto currentOutIt = mOutputs.find( outputBus );
	if( currentOutIt != mOutputs.end() ) {
		NodeRef currentOutput = currentOutIt->second.lock();
		// in some cases, an output may have lost all references and is no longer valid, so it is safe to overwrite without disconnecting.
		if( currentOutput )
			currentOutput->disconnectInput( thisRef );
		else
			mOutputs.erase( currentOutIt );
	}

	mOutputs[outputBus] = output; // set output bus first, so that it is visible in configureConnections()
	output->connectInput( thisRef, inputBus );

	output->notifyConnectionsDidChange();
}

void Node::addConnection( const NodeRef &output )
{
	connect( output, getFirstAvailableOutputBus(), output->getFirstAvailableInputBus() );
}

void Node::disconnect( size_t outputBus )
{
	auto outIt = mOutputs.find( outputBus );
	if( outIt == mOutputs.end() )
		return;

	NodeRef output = outIt->second.lock();
	mOutputs.erase( outIt );

	// in some cases, the output may have been destroyed without disconnecting first.
	if( output ) {
		output->disconnectInput( shared_from_this() );
		output->notifyConnectionsDidChange();
	}
}

void Node::disconnectAll()
{
	disconnectAllInputs();
	disconnectAllOutputs();
}

void Node::disconnectAllOutputs()
{
	NodeRef thisRef = shared_from_this();

	for( size_t &outBus : getOccupiedOutputBusses() )
		disconnect( outBus );
}

void Node::disconnectAllInputs()
{
	NodeRef thisRef = shared_from_this();

	for( auto &in : mInputs )
		in.second->disconnectOutput( thisRef );

	mInputs.clear();
	notifyConnectionsDidChange();
}

void Node::connectInput( const NodeRef &input, size_t bus )
{
	lock_guard<mutex> lock( getContext()->getMutex() );

	mInputs[bus] = input;
	configureConnections();
}

void Node::disconnectInput( const NodeRef &input )
{
	lock_guard<mutex> lock( getContext()->getMutex() );

	for( auto inIt = mInputs.begin(); inIt != mInputs.end(); ++inIt ) {
		if( inIt->second == input ) {
			mInputs.erase( inIt );
			break;
		}
	}
}

void Node::disconnectOutput( const NodeRef &output )
{
	lock_guard<mutex> lock( getContext()->getMutex() );

	for( auto outIt = mOutputs.begin(); outIt != mOutputs.end(); ++outIt ) {
		if( outIt->second.lock() == output ) {
			mOutputs.erase( outIt );
			break;
		}
	}
}

vector<size_t> Node::getOccupiedInputBusses() const
{
	vector<size_t> result;
	for( const auto &in : getInputs() )
		result.push_back( in.first );

	return result;
}

vector<size_t> Node::getOccupiedOutputBusses() const
{
	vector<size_t> result;
	for( const auto &in : getOutputs() )
		result.push_back( in.first );

	return result;
}

void Node::enable()
{
	if( ! mInitialized )
		initializeImpl();

	if( mEnabled )
		return;

	mEnabled = true;
	enableProcessing();
}

void Node::disable()
{
	if( ! mEnabled )
		return;

	mEnabled = false;
	disableProcessing();
}

void Node::setEnabled( bool b )
{
	if( b )
		enable();
	else
		disable();
}

size_t Node::getNumConnectedInputs() const
{
	return mInputs.size();
}

size_t Node::getNumConnectedOutputs() const
{
	return mOutputs.size();
}

void Node::initializeImpl()
{
	if( mInitialized )
		return;

	initialize();
	mInitialized = true;

	if( mAutoEnabled )
		enable();
}


void Node::uninitializeImpl()
{
	if( ! mInitialized )
		return;

	disable();
	uninitialize();
	mInitialized = false;
}

void Node::setNumChannels( size_t numChannels )
{
	if( mNumChannels == numChannels )
		return;

	uninitializeImpl();
	mNumChannels = numChannels;
}

void Node::setChannelMode( ChannelMode mode )
{
	CI_ASSERT_MSG( ! mInitialized, "setting the channel mode must be done before initializing" );

	if( mChannelMode == mode )
		return;

	mChannelMode = mode;
}

size_t Node::getMaxNumInputChannels() const
{
	size_t result = 0;
	for( auto &in : mInputs )
		result = max( result, in.second->getNumChannels() );

	return result;
}

size_t Node::getSampleRate() const
{
	return getContext()->getSampleRate();
}

size_t Node::getFramesPerBlock() const
{
	return getContext()->getFramesPerBlock();
}

// TODO: Checking for Delay below is a kludge and will not work for other types that want to support feedback.
//		 With more investigation it might be possible to avoid this, or at least define some interface that
//       specifies whether this input needs to be summed.
void Node::configureConnections()
{
	CI_ASSERT( getContext() );

	mProcessInPlace = supportsProcessInPlace();

	if( mProcessInPlace ) {
		if( getNumConnectedInputs() > 1 || getNumConnectedOutputs() > 1 )
			mProcessInPlace = false;

		bool isDelay = ( dynamic_cast<Delay *>( this ) != nullptr ); // see note above

		for( auto &in : mInputs ) {
			NodeRef input = in.second;
			bool inputProcessInPlace = true;

			size_t inputNumChannels = input->getNumChannels();
			if( ! supportsInputNumChannels( inputNumChannels ) ) {
				if( mChannelMode == ChannelMode::MATCHES_INPUT )
					setNumChannels( getMaxNumInputChannels() );
				else if( input->getChannelMode() == ChannelMode::MATCHES_OUTPUT ) {
					input->setNumChannels( mNumChannels );
					input->configureConnections();
				}
				else {
					mProcessInPlace = false;
					inputProcessInPlace = false;
				}
			}

			// inputs with more than one output cannot process in-place, so make them sum
			if( input->getProcessInPlace() && input->getNumConnectedOutputs() > 1 )
				inputProcessInPlace = false;

			// if we're unable to process in-place and we're a Delay, its possible that the input may be part of a feedback loop, in which case input must sum.
			if( ! mProcessInPlace && isDelay ) {
				CI_LOG_V( "detected possible cycle with delay: " << getName() << ", disabling in-place processing for input: " << input->getName() );
				inputProcessInPlace = false;
			}

			if( ! inputProcessInPlace )
				input->setupProcessWithSumming();

			input->initializeImpl();
		}

		for( auto &out : mOutputs ) {
			NodeRef output = out.second.lock();
			if( ! output ) {
				CI_LOG_W( "expired output on bus: " << out.first );
				continue; // TODO: consider filtering these out before this method is reached.
			}
			if( ! output->supportsInputNumChannels( mNumChannels ) ) {
				if( output->getChannelMode() == ChannelMode::MATCHES_INPUT ) {
					output->setNumChannels( mNumChannels );
					output->configureConnections();
				}
				else
					mProcessInPlace = false;
			}
		}
	}

	if( ! mProcessInPlace )
		setupProcessWithSumming();

	initializeImpl();
}

void Node::pullInputs( Buffer *inPlaceBuffer )
{
	CI_ASSERT( getContext() );

	if( mProcessInPlace ) {
		if( mInputs.empty() ) {
			// Fastest route: no inputs and process in-place. If disabled, get rid of any previously processsed samples.
			if( mEnabled )
				process( inPlaceBuffer );
			else
				inPlaceBuffer->zero();
		}
		else {
			// First pull the input (can only be one when in-place), then run process() if input did any processing.
			auto &input = mInputs.begin()->second;
			input->pullInputs( inPlaceBuffer );

			if( ! input->getProcessInPlace() )
				dsp::mixBuffers( input->getInternalBuffer(), inPlaceBuffer );

			if( mEnabled )
				process( inPlaceBuffer );
		}
	}
	else {
		// Pull and sum all enabled inputs. Only do this once per processing block, which is checked by the current number of processed frames.
		uint64_t numProcessedFrames = getContext()->getNumProcessedFrames();
		if( mLastProcessedFrame != numProcessedFrames ) {
			mLastProcessedFrame = numProcessedFrames;

			mSummingBuffer.zero();
			sumInputs();
		}
	}
}

void Node::sumInputs()
{
	// Pull all inputs, summing the results from the buffer that input used for processing.
	// mInternalBuffer is not zero'ed before pulling inputs to allow for feedback.
	for( auto &in : mInputs ) {
		NodeRef &input = in.second;
		input->pullInputs( &mInternalBuffer );
		const Buffer *processedBuffer = input->getProcessInPlace() ? &mInternalBuffer : input->getInternalBuffer();
		dsp::sumBuffers( processedBuffer, &mSummingBuffer );
	}

	// Process the summed results if enabled.
	if( mEnabled )
		process( &mSummingBuffer );

	// copy summed buffer back to internal so downstream can get it.
	dsp::mixBuffers( &mSummingBuffer, &mInternalBuffer );
}

void Node::setupProcessWithSumming()
{
	CI_ASSERT( getContext() );

	mProcessInPlace = false;
	size_t framesPerBlock = getFramesPerBlock();

	mInternalBuffer.setSize( framesPerBlock, mNumChannels );
	mSummingBuffer.setSize( framesPerBlock, mNumChannels );
}

bool Node::checkCycle( const NodeRef &sourceNode, const NodeRef &destNode ) const
{
	if( sourceNode == destNode )
		return true;

	if( sourceNode->supportsCycles() || destNode->supportsCycles() )
		return false;

	for( const auto &in : sourceNode->getInputs() ) {
		if( checkCycle( in.second, destNode ) )
			return true;
	}

	return false;
}

void Node::notifyConnectionsDidChange()
{
	getContext()->connectionsDidChange( shared_from_this() );
}

bool Node::canConnectToInput( const NodeRef &input )
{
	if( ! input || input == shared_from_this() )
		return false;

	for( const auto& in : mInputs )
		if( input == in.second )
			return false;

	return true;
}

size_t Node::getFirstAvailableOutputBus()
{
	size_t result = 0;
	for( const auto& output : mOutputs ) {
		if( output.first != result )
			break;

		result++;
	}

	return result;
}

size_t Node::getFirstAvailableInputBus()
{
	size_t result = 0;
	for( const auto& input : mInputs ) {
		if( input.first != result )
			break;

		result++;
	}

	return result;
}

std::string Node::getName()
{
	return demangledTypeName( typeid( *this ).name() );
}

// ----------------------------------------------------------------------------------------------------
// MARK: - NodeAutoPullable
// ----------------------------------------------------------------------------------------------------

NodeAutoPullable::NodeAutoPullable( const Format &format )
	: Node( format ), mIsPulledByContext( false )
{
}

NodeAutoPullable::~NodeAutoPullable()
{
}

void NodeAutoPullable::connect( const NodeRef &output, size_t outputBus, size_t inputBus )
{
	Node::connect( output, outputBus, inputBus );
	updatePullMethod();
}

void NodeAutoPullable::connectInput( const NodeRef &input, size_t bus )
{
	Node::connectInput( input, bus );
	updatePullMethod();
}

void NodeAutoPullable::disconnectInput( const NodeRef &input )
{
	Node::disconnectInput( input );
	updatePullMethod();
}

void NodeAutoPullable::disconnectAllOutputs()
{
	Node::disconnectAllOutputs();

	if( mIsPulledByContext ) {
		mIsPulledByContext = false;
		getContext()->removeAutoPulledNode( shared_from_this() );
	}
}

void NodeAutoPullable::updatePullMethod()
{
	bool hasOutputs = ! getOutputs().empty();
	if( ! hasOutputs && ! mIsPulledByContext ) {
		mIsPulledByContext = true;
		getContext()->addAutoPulledNode( shared_from_this() );
	}
	else if( hasOutputs && mIsPulledByContext ) {
		mIsPulledByContext = false;
		getContext()->removeAutoPulledNode( shared_from_this() );
	}
}

// ----------------------------------------------------------------------------------------------------
// MARK: - Exceptions
// ----------------------------------------------------------------------------------------------------

NodeCycleExc::NodeCycleExc( const NodeRef &sourceNode, const NodeRef &destNode )
	: AudioExc( string( "cyclical connection between source: " ) + sourceNode->getName() + string( " and dest: " ) + destNode->getName() )
{
}

} } // namespace cinder::audio2
