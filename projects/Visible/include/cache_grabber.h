#ifndef __CACHE_GRABBER__
#define __CACHE_GRABBER__


#include "qtime_cache.h"
#include <map>
#include <iostream>
#include <string>
#include <boost/signals2.hpp>
#include <boost/signals2/slot.hpp>
#include <typeinfo>
#include <vector>
#include <sstream>
#include "grabber.h"


  /** \brief grabber interface for 
    */
class cache_grabber : public simple_grabber
{
    public:
      
      typedef void (sig_cb_native_image_buffer) (const cached_frame_ref&);
      typedef void (sig_cb_native_acquire_complete) ();

      /** \brief Constructor. */
      cache_grabber (SharedQtimeCache& cache_) : cache_ (cache_), _last_frame_count(0), _curFrame(0), _started(false)
      {
          BOOST_ASSERT(cache_->isValid());
          native_image_signal_ = createSignal<sig_cb_native_image_buffer>();
          native_acq_complete_signal_ = createSignal<sig_cb_native_acquire_complete>();
      }


      /** \brief registers a callback function/method to a signal with the corresponding signature
        * \param[in] callback: the callback function/method
        * \return Connection object, that can be used to disconnect the callback method from the signal again.
        */
      template<typename T> boost::signals2::connection 
      registerCallback (const boost::function<T>& callback);

      /** \brief indicates whether a signal with given parameter-type exists or not
        * \return true if signal exists, false otherwise
        */
      template<typename T> bool 
      providesCallback () const;

      /** \brief For devices that are streaming, the streams are started by calling this method.
        *        Trigger-based devices, just trigger the device once for each call of start.
        */
      virtual bool start ()
      {
          if (!cache_->isValid())
              return false;
          
          if (_started == false) {
              _last_frame_count = (int32) cache_->frameCount() - 1;
              _curFrame = 0;
              _started = true;
          }
          return true;
      }


      /** \brief @todo
        *
        */
      virtual bool stop ()
      {
          if (!cache_->isValid())
              return false;
          
          _started = false;
          return true;
      }

      // @todo fix error and status
      virtual grabber_status get_next_frame (cached_frame_ref& frame)
      {
          {
              if (!cache_->isValid())
                  return grabber_status::Error;
              
              if (!_started)
                  return grabber_status::Error;
              
              if (_curFrame  == _last_frame_count)
                  return grabber_status::end_of_file;
              
              
              /*
               * Get reference to frame, but don't lock it so it won't be forced
               * into memory before it is needed.
               */
              QtimeCacheError error;
              QtimeCacheStatus status = cache_->getFrame(_curFrame++, frame, &error, false);
              
              if (status != QtimeCacheStatus::OK)
              {
                  return grabber_status::Error;
              }
              
              return grabber_status::end_of_file;
          }
 
      }
      
      /** \brief returns the name of the concrete subclass.
        * \return the name of the concrete driver.
        */
      virtual std::string 
      getName () const
       { return cache_->getInputSourceName(); }


      /** \brief returns frame count  */
      virtual int frameCount () const  { return cache_->frameCount(); }

      
      // Returns the size of the cache, in frames.
    virtual int32 cacheSize() const { return cache_->cacheSize(); }
    
    virtual bool isValid () const { return cache_ ? cache_->isValid () : false; }
    

    protected:

      boost::signals2::signal<sig_cb_native_image_buffer>* native_image_signal_;
      boost::signals2::signal<sig_cb_native_acquire_complete>* native_acq_complete_signal_;
      
      
  private:
      // Prevent default copy and assignment
      cache_grabber (const cache_grabber& c);
      cache_grabber& operator= (const cache_grabber& c);
      
      SharedQtimeCache& cache_;
      
      int32             _last_frame_count;
      int32             _curFrame;
      bool                _started;
      
      
  } ;



#endif
