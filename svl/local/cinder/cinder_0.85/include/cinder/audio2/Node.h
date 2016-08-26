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
#include "cinder/audio2/Exception.h"

#include <boost/noncopyable.hpp>
#include <boost/logic/tribool.hpp>

#include <memory>
#include <atomic>
#include <map>

namespace cinder { namespace audio2 {

typedef std::shared_ptr<class Context>			ContextRef;
typedef std::shared_ptr<class Node>				NodeRef;

//! \brief Fundamental building block for creating an audio processing graph.
//!
//!	Node's allow for flexible combinations of synthesis, analysis, effects, file reading/writing, etc, and are designed so that
//! that it is easy for users to add their own custom subclasses to a processing graph.
//!
//! A Node is owned by a Context, and as such must by created using its makeNode() method, or one of the specializations for platform
//! specific Node's.
//!
//! Audio Node's are designed to operate on two different threads: a 'user' thread (i.e. main / UI) and an audio thread. Specifically,
//! methods for connecting and disconnecting are expected to come from the 'user' thread, while the Node's process() and internal pulling
//! methods are called from a hard real-time thread. A mutex is used (Context::getMutex()) to synchronize connection changes and pulling
//! the audio graph. Note that if the Node's initialize() method is heavy, it can be called before connected to anything, so as to not
//! block the audio graph. This must be done throught the Context::initializeNode() interface.
//!
//! Subclassing: implement process( Buffer *buffer ) to perform audio processing. A Node does not have access to its owning Context until
//! initialize() is called, uninitialize() is called before a Node is deallocated or channel counts change.
//!
//! \see NodeInput, NodeOutput, NodeEffect
class Node : public std::enable_shared_from_this<Node>, public boost::noncopyable {
  public:
	typedef std::map<size_t, std::shared_ptr<Node> >		InputsContainerT;		//! input bus, strong reference to this
	typedef std::map<size_t, std::weak_ptr<Node> >			OutputsContainerT;		//! output bus for this node, weak reference to this

	//! Used to specifiy how the corresponding channels are to be resolved between two connected Node's,
	//! based on either a Node's input (the default), it's output, or specified by user.
	enum class ChannelMode {
		//! Number of channels has been specified by user or is non-settable.
		SPECIFIED,
		//! This Node matches it's channels with it's input.
		MATCHES_INPUT,
		//! This Node matches it's channels with it's output.
		MATCHES_OUTPUT
	};

	struct Format {
		Format() : mChannels( 0 ), mChannelMode( ChannelMode::MATCHES_INPUT ), mAutoEnable( boost::logic::indeterminate ) {}

		//! Sets the number of channels for the Node.
		Format& channels( size_t ch )							{ mChannels = ch; return *this; }
		//! Controls how channels will be matched between connected Node's, if necessary. \see ChannelMode.
		Format& channelMode( ChannelMode mode )					{ mChannelMode = mode; return *this; }
		//! Whether or not the Node will be auto-enabled when connection changes occur.  Default is false for base \a Node class, although sub-classes may choose a different default.
		Format& autoEnable( boost::tribool tb = true )			{ mAutoEnable = tb; return *this; }

		size_t			getChannels() const						{ return mChannels; }
		ChannelMode		getChannelMode() const					{ return mChannelMode; }
		boost::tribool	getAutoEnable() const					{ return mAutoEnable; }

	  protected:
		size_t			mChannels;
		ChannelMode		mChannelMode;
		boost::tribool	mAutoEnable;
	};

	Node( const Format &format );
	virtual ~Node();

	//! Enables this Node for processing. Same as `setEnabled( true )`.
	void enable();
	//! Disables this Node for processing. Same as `setEnabled( false )`.
	void disable();
	//! Sets whether this Node is enabled for processing or not.
	void setEnabled( bool b = true );
	//! Returns whether this Node is enabled for processing or not.
	bool isEnabled() const						{ return mEnabled; }

	//! Connects this Node to \a output on bus 0. \a output then references this Node as an input on bus 0.
	void connect( const NodeRef &output )							{ connect( output, 0, 0 ); }
	//! Connects this Node to \a output on bus 0. \a output then references this Node as an input on bus \a inputBus.
	void connect( const NodeRef &output, size_t inputBus )			{ connect( output, 0, inputBus ); }
	//! Connects this Node to \a output on bus \a outputBus. \a output then references this Node as an input on \a inputBus.
	virtual void connect( const NodeRef &output, size_t outputBus, size_t inputBus );
	//! Connects this Node to \a output on the first available output bus. \a dest then references this Node as an input on the first available input bus.
	virtual void addConnection( const NodeRef &output );
	//! Disconnects this Node from the output Node located at bus \a outputBus.
	virtual void disconnect( size_t outputBus = 0 );
	//! Disconnects this Node from all inputs and outputs.
	virtual void disconnectAll();
	//! Disconnects this Node from all outputs.
	virtual void disconnectAllOutputs();
	//! Disconnects all of this Node's inputs.
	virtual void disconnectAllInputs();
	//! Returns the number of inputs connected to this Node.
	size_t getNumConnectedInputs() const;
	//! Returns the number of outputs this Node is connected to.
	size_t getNumConnectedOutputs() const;

	//! Returns the \a Context associated with this \a Node. \note Cannot be called from within a \a Node's constructor. Use initialize instead.
	ContextRef	getContext() const				{ return mContext.lock(); }
	//! Returns the number of channels this Node will process.
	size_t		getNumChannels() const			{ return mNumChannels; }
	//! Returns the channel mode. \see ChannelMode.
	ChannelMode getChannelMode() const			{ return mChannelMode; }
	//! Returns the maximum number of channels any input has.
	size_t		getMaxNumInputChannels() const;

	//! Returns the samplerate of this Node, which is governed by the Context's NodeOutput.
	size_t		getSampleRate() const;
	//! Returns the number of frames processed in one block by this Node, which is governed by the Context's NodeOutput.
	size_t		getFramesPerBlock() const;

	//! Sets whether this Node is automatically enabled / disabled when connected
	void	setAutoEnabled( bool b = true )		{ mAutoEnabled = b; }
	//! Returns whether this Node is automatically enabled / disabled when connected
	bool	isAutoEnabled() const				{ return mAutoEnabled; }

	// TODO: consider doing custom iterators and hiding these container types
	InputsContainerT&			getInputs()				{ return mInputs; }
	const InputsContainerT&		getInputs() const		{ return mInputs; }
	OutputsContainerT&			getOutputs()			{ return mOutputs; }
	const OutputsContainerT&	getOutputs() const		{ return mOutputs; }

	//! Returns a vector of the currently occupied input busses.
	std::vector<size_t> getOccupiedInputBusses() const;
	//! Returns a vector of the currently occupied output busses.
	std::vector<size_t> getOccupiedOutputBusses() const;
	//! Returns the first available input bus.
	size_t getFirstAvailableOutputBus();
	//! Returns the first available output bus.
	size_t getFirstAvailableInputBus();

	//! Returns whether this Node is in an initialized state and is capable of processing audio.
	bool isInitialized() const					{ return mInitialized; }
	//! Returns whether this Node will process audio with an in-place Buffer.
	bool getProcessInPlace() const				{ return mProcessInPlace; }

	//! Returns a string representing the name of this Node type. TODO: use typeid + abi de-mangling to ease the burden on sub-classes
	virtual std::string getName();

	//! Used when connecting Node's with operator>>() and the user needs to specify the input and/or output bus.
	struct BusConnector {
		BusConnector( const NodeRef &output, size_t outputBus, size_t inputBus ) : mOutput( output ), mOutputBus( outputBus ), mInputBus( inputBus )
		{}

		const NodeRef& getOutput() const	{ return mOutput; }

		size_t	getOutputBus() const	{ return mOutputBus; }
		size_t	getInputBus() const	{ return mInputBus; }

	private:
		NodeRef mOutput;
		size_t mOutputBus, mInputBus;
	};

	//! Returns a BusConnector, which is used when connecting Node's via operator>>(). \a outputBus is associated with the left-hand side's output's and \a inputBus is associated with the right-hand side's inputs.
	BusConnector bus( size_t outputBus, size_t inputBus )	{ return BusConnector( shared_from_this(), outputBus, inputBus ); }
	//! Returns a BusConnector, which is used when connecting Node's via operator>>(). \a inputBus is associated with the right-hand side's inputs, while the left-hand side's output bus is 0.
	BusConnector bus( size_t inputBus )						{ return BusConnector( shared_from_this(), 0, inputBus ); }

	// TODO: document newly exposed buffers and virtual sumInputs (that could use a better name, it isn't always summing anymore).
	Buffer*			getInternalBuffer()			{ return &mInternalBuffer; }
	const Buffer*	getInternalBuffer() const	{ return &mInternalBuffer; }

	//! Usually called internally by the Node, in special cases sub-classes may need to call this on other Node's.
	void pullInputs( Buffer *inPlaceBuffer );

  protected:

	//! Called before audio buffers need to be used. There is always a valid Context at this point.
	virtual void initialize()				{}
	//! Called once the contents of initialize are no longer relevant, i.e. connections have changed. \note Not guaranteed to be called at Node destruction.
	virtual void uninitialize()				{}
	//! Callled when this Node should enable processing. Initiated from Node::enable().
	virtual void enableProcessing()			{}
	//! Callled when this Node should disable processing. Initiated from Node::disable().
	virtual void disableProcessing()		{}
	//! Override to perform audio processing on \t buffer.
	virtual void process( Buffer *buffer )	{}

	virtual void sumInputs();

	//! Default implementation returns true if numChannels matches our format.
	virtual bool supportsInputNumChannels( size_t numChannels ) const	{ return mNumChannels == numChannels; }
	//! Default implementation returns false, return true if it makes sense for the Node to be processed in a cycle (eg. Delay).
	virtual bool supportsCycles() const									{ return false; }
	//! Default implementation returns true, subclasses should return false if they must process out-of-place (summing).
	virtual bool supportsProcessInPlace() const							{ return true; }

	//! Stores \a input at bus \a inputBus, replacing any Node currently existing there. Stores this Node at input's output bus \a outputBus. Returns whether a new connection was made or not.
	//! \note Must be called on a non-audio thread and synchronized with the Context's mutex.
	virtual void connectInput( const NodeRef &input, size_t bus );
	virtual void disconnectInput( const NodeRef &input );
	virtual void disconnectOutput( const NodeRef &output );
	virtual void configureConnections();

	void setupProcessWithSumming();
	void notifyConnectionsDidChange();

	//! Only Node subclasses can specify num channels directly - users specify via Format at construction time.
	void setNumChannels( size_t numChannels );
	//! Only Node subclasses can specify channel mode directly - users specify via Format at construction time.
	void setChannelMode( ChannelMode mode );
	//! Returns whether it is possible to connect to \a input, example reasons of failure would be this == Node, or Node is already an input.
	bool canConnectToInput( const NodeRef &input );

	//! Returns true if there is an unmanageable cycle betweeen \a sourceNode and \a destNode.
	bool checkCycle( const NodeRef &sourceNode, const NodeRef &destNode ) const;

	void initializeImpl();
	void uninitializeImpl();

	BufferDynamic*			getSummingBuffer()			{ return &mSummingBuffer; }
	const BufferDynamic*	getSummingBuffer() const	{ return &mSummingBuffer; }

  private:
	// The owning Context calls this.
	void setContext( const ContextRef &context )	{ mContext = context; }

	std::weak_ptr<Context>	mContext;
	std::atomic<bool>		mEnabled;
	bool					mInitialized;
	bool					mAutoEnabled;
	bool					mProcessInPlace;
	ChannelMode				mChannelMode;
	size_t					mNumChannels;
	uint64_t				mLastProcessedFrame;
	InputsContainerT		mInputs;
	OutputsContainerT		mOutputs;
	BufferDynamic			mInternalBuffer, mSummingBuffer;

	friend class Context;
	friend class Param;
};

//! Enable connection syntax: \code input >> output; \endcode. Connects on the first available input and output bus.  \return the connected \a output
inline const NodeRef& operator>>( const NodeRef &input, const NodeRef &output )
{
	input->addConnection( output );
	return output;
}

//! Enable connection syntax: \code input >> output->bus( inputBus, outputBus ); \endcode.  \return the connected \a output
inline const NodeRef& operator>>( const NodeRef &input, const Node::BusConnector &bus )
{
	input->connect( bus.getOutput(), bus.getOutputBus(), bus.getInputBus() );
	return bus.getOutput();
}

//! a Node that can be pulled without being connected to any outputs.
class NodeAutoPullable : public Node {
  public:
	virtual ~NodeAutoPullable();

	virtual void connect( const NodeRef &output, size_t outputBus = 0, size_t inputBus = 0 ) override;
	virtual void connectInput( const NodeRef &input, size_t bus )	override;
	virtual void disconnectInput( const NodeRef &input )			override;
	//! Overridden to also remove from  Context's auto-pulled list
	virtual void disconnectAllOutputs()								override;

  protected:
	NodeAutoPullable( const Format &format );
	void updatePullMethod();

	bool mIsPulledByContext;
};

//! RAII-style utility class to save the \a Node's current enabled state.
//  TODO: make this more in-line with other Scoped utils:
//	- pass in bool in constructor that does the enabling, disabling
//	- also make this work for Context as well, or add a separate one for it.
struct ScopedNodeEnabledState {
	ScopedNodeEnabledState( const NodeRef &node )
	: mNode( node )
	{
		mEnabled = ( mNode ? mNode->isEnabled() : false );
	}
	~ScopedNodeEnabledState()
	{
		if( mNode )
			mNode->setEnabled( mEnabled );
	}
  private:
	NodeRef mNode;
	bool mEnabled;
};

class NodeCycleExc : public AudioExc {
public:
	NodeCycleExc( const NodeRef &sourceNode, const NodeRef &destNode );
};

} } // namespace cinder::audio2
