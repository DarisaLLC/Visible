
#include "time_index.h"

using namespace cinder;


/*
  A schedule is a list of time indices. A node in this list is at a greater or equal time than its predecessors. 
  Mutliple track_nodes belonging to the same time / index are legal and indicate that the order they are presented can be
  arbitary. 

  A schedule is created and sized to cover a time duration, starting at start_time and ending at end_time
  A schedule can contain multiple segments. 
  Processing of an schedule:
    An scehdule is stepped by a a time generating event such as an audio or video update or an ad hoc updater.
    The step method increments the time probe and dispatches callbacks for all the track_nodes in the new time point. 
    
    A schedule is "process" by calling the callbacks 
  

    
   A track_node is a timed / index proxy to a displable / plottable / coutable thing repreented by the template parameter.
   A track_node is initially uninitialized. When it is attached to a metronome, it is initialzied with a timestamp or index. 
   intialized time_nodes are waiting to be loaded. 
   loading means to fill the track_node with data it needs for presentation. 
   When a track_node is marked as done, its storage may be reused

   A track_node maybe fixed at one time or at all times ( such as a fixed graphic iterm )
   a track_node is loaded asynchronously through its callback. 
   a track_node can be stepped in time. 


   For example, we can create specific classes as follows:
   class image_track: public trackerItemTypeMaker <image_track> {};
   

 */

class trackItem : public index_time_t
{ public:

      enum state { uninitialized = 0, unloaded = 1, loaded = 2, loading = 3, done = 4 };
      
      virtual trackItem* Clone() const = 0;
      virtual ~trackItem () {}
   };

template <typename Derived>
class trackerItemTypeMaker: public trackItem
{
public:
      trackItem* Clone() const
      {
	return new Derived(dynamic_cast<Derived const&>(*this));
      }
};



#if 0

//! Callback used to allow simple audio processing without subclassing a Node. First parameter is the Buffer to which to write samples, second parameter is the samplerate.
typedef std::function<void( Buffer *, size_t )> CallbackProcessorFn;

//! InputNode that processes audio with a std::function callback. \see CallbackProcessorFn
class CallbackProcessorNode : public InputNode {
  public:
	CallbackProcessorNode( const CallbackProcessorFn &callbackFn, const Format &format = Format() );
	virtual ~CallbackProcessorNode() {}

  protected:
	void process( Buffer *buffer ) override;

  private:
	CallbackProcessorFn mCallbackFn;
};

#endif
