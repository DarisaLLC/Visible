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

#include "cinder/audio2/Node.h"
#include "cinder/audio2/NodeInput.h"
#include "cinder/audio2/NodeOutput.h"

#include <mutex>
#include <set>

namespace cinder { namespace audio2 {

class DeviceManager;

class Context : public std::enable_shared_from_this<Context> {
  public:
	virtual ~Context();

	//! Returns the master \a Context that manages hardware I/O and real-time processing, which is platform specific. If none is available, returns \a null.
	static Context*				master();
	//! Returns the platform-specific \a DeviceManager singleton instance, which is platform specific. If none is available, returns \a null.
	static DeviceManager*		deviceManager();
	//! Allows the user to set the master Context and DeviceManager, overriding the defaults.
	static void				setMaster( Context *masterContext, DeviceManager *deviceManager );

	//! Creates and returns a platform-specific LineOut, which delivers audio to the hardware output device specified by \a device. 
	virtual LineOutRef		createLineOut( const DeviceRef &device = Device::getDefaultOutput(), const Node::Format &format = Node::Format() ) = 0;
	//! Creates and returns a platform-specific LineIn, which captures audio from the hardware input device specified by \a device. 
	virtual LineInRef		createLineIn( const DeviceRef &device = Device::getDefaultInput(), const Node::Format &format = Node::Format() ) = 0;

	//! Interface for creating new Node's of type \a NodeT, which are thereafter owned by this Context.
	template<typename NodeT>
	std::shared_ptr<NodeT>		makeNode( NodeT *node );

	virtual void setOutput( const NodeOutputRef &output );

	//! If the output has not already been set, it is the default LineOut
	virtual const NodeOutputRef& getOutput();
	//! Enables audio processing. Effectively the same as calling getOutput()->enable()
	virtual void enable();
	//! Enables audio processing. Effectively the same as calling getOutput()->disable()
	virtual void disable();
	//! start / stop audio processing via boolean
	void setEnabled( bool b = true );
	//! Returns whether or not this \a Context is current enabled and processing audio.
	bool isEnabled() const		{ return mEnabled; }

	//! Called by \a node when it's connections have changed, default implementation is empty.
	virtual void connectionsDidChange( const NodeRef &node ) {} 

	//! Returns the samplerate of this Context, which is governed by the current NodeOutput.
	size_t		getSampleRate()				{ return getOutput()->getOutputSampleRate(); }
	//! Returns the number of frames processed in one block by this Node, which is governed by the current NodeOutput.
	size_t		getFramesPerBlock()			{ return getOutput()->getOutputFramesPerBlock(); }

	uint64_t	getNumProcessedFrames()		{ return getOutput()->getNumProcessedFrames(); }
	double		getNumProcessedSeconds()	{ return (double)getNumProcessedFrames() / (double)getSampleRate(); }

	std::mutex& getMutex() const			{ return mMutex; }

	void initializeNode( const NodeRef &node );
	void uninitializeNode( const NodeRef &node );

	//! Initialize all Node's related by this Context
	void initializeAllNodes();
	//! Uninitialize all Node's related by this Context
	void uninitializeAllNodes();
	//! Disconnect all Node's related by this Context
	virtual void disconnectAllNodes();

	//! Add \a node to the list of auto-pulled nodes, who will have their Node::pullInputs() method called after a LineOut implementation finishes pulling its inputs.
	//! \note Callers on the non-audio thread must synchronize with getMutex().
	void addAutoPulledNode( const NodeRef &node );
	//! Remove \a node from the list of auto-pulled nodes.
	//! \note Callers on the non-audio thread must synchronize with getMutex().
	void removeAutoPulledNode( const NodeRef &node );

	//! Calls Node::pullInputs() for any Node's that have registered with addAutoPulledNode()
	//! \note Expected to be called on the audio thread by a LineOut implementation at the end of its render loop.
	void processAutoPulledNodes();

	//! Prints the Node graph to console()
	void printGraph();

  protected:
	Context() : mEnabled( false ), mAutoPullRequired( false ), mAutoPullCacheDirty( false ) {}

	const std::vector<Node *>& getAutoPulledNodes(); // called if there are any nodes besides output that need to be pulled

	NodeOutputRef			mOutput;				// the 'heartbeat'

	// other nodes that don't have any outputs and need to be explictly pulled
	std::set<NodeRef>		mAutoPulledNodes;
	std::vector<Node *>		mAutoPullCache;
	bool					mAutoPullRequired, mAutoPullCacheDirty;
	BufferDynamic			mAutoPullBuffer;

	mutable std::mutex		mMutex;
	bool					mEnabled;

	// TODO: if this is singleton, why hold in shared_ptr?
	// - it's still stored in Node classes as a weak_ptr, so it needs to (for now) be created as a shared_ptr
	static std::shared_ptr<Context>			sMasterContext;
	static std::unique_ptr<DeviceManager>	sDeviceManager; // TODO: consider turning DeviceManager into a HardwareContext class


  private:
	void disconnectRecursive( const NodeRef &node, std::set<NodeRef> &traversedNodes );
	void initRecursisve( const NodeRef &node, std::set<NodeRef> &traversedNodes  );
	void uninitRecursisve( const NodeRef &node, std::set<NodeRef> &traversedNodes  );

	static void registerClearStatics();
};

template<typename NodeT>
std::shared_ptr<NodeT> Context::makeNode( NodeT *node )
{
	std::shared_ptr<NodeT> result( node );
	result->setContext( shared_from_this() );
	return result;
}

//! Returns the master \a Context that manages hardware I/O and real-time processing, which is platform specific. If none is available, returns \a null.
inline Context* master()	{ return Context::master(); }

} } // namespace cinder::audio2