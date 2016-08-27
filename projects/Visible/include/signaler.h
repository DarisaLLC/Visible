#ifndef __IO_GRABBER__
#define __IO_GRABBER__

#include "svl_exception.hpp"
#include <map>
#include <iostream>
#include <string>
#include <boost/signals2.hpp>
#include <boost/signals2/slot.hpp>
#include <typeinfo>
#include <vector>
#include <sstream>



  /** \brief signaler interface for 
   
   // make callback function from member function
    boost::function<void (..onearg..)> f = boost::bind (&Observer::cb_, this, _1);
   
   // connect callback function for desired signal. In this case its a point cloud with color values
   boost::signals2::connection c = interface->registerCallback (f);

    */
  class base_signaler
  {
    public:

      /** \brief Constructor. */
      base_signaler () : signals_ (), connections_ (), shared_connections_ () {}

      /** \brief virtual desctructor. */
      virtual inline ~base_signaler () throw ();

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

      /** \brief returns the name of the concrete subclass.
        * \return the name of the concrete driver.
        */
      virtual std::string 
      getName () const = 0;

     
    protected:

      virtual void
      signalsChanged () { }

      template<typename T> boost::signals2::signal<T>* 
      find_signal () const;

      template<typename T> int 
      num_slots () const;

      template<typename T> void 
      disconnect_all_slots ();

      template<typename T> void 
      block_signal ();
      
      template<typename T> void 
      unblock_signal ();
      
      inline void 
      block_signals ();
      
      inline void 
      unblock_signals ();

      template<typename T> boost::signals2::signal<T>* 
      createSignal ();

      std::map<std::string, boost::signals2::signal_base*> signals_;
      std::map<std::string, std::vector<boost::signals2::connection> > connections_;
      std::map<std::string, std::vector<boost::signals2::shared_connection_block> > shared_connections_;
  } ;

  base_signaler::~base_signaler () throw ()
  {
    for (std::map<std::string, boost::signals2::signal_base*>::iterator signal_it = signals_.begin (); signal_it != signals_.end (); ++signal_it)
      delete signal_it->second;
  }

  template<typename T> boost::signals2::signal<T>*
  base_signaler::find_signal () const
  {
    typedef boost::signals2::signal<T> Signal;

    std::map<std::string, boost::signals2::signal_base*>::const_iterator signal_it = signals_.find (typeid (T).name ());
    if (signal_it != signals_.end ())
      return (dynamic_cast<Signal*> (signal_it->second));

    return (NULL);
  }

  template<typename T> void
  base_signaler::disconnect_all_slots ()
  {
    typedef boost::signals2::signal<T> Signal;

    if (signals_.find (typeid (T).name ()) != signals_.end ())
    {
      Signal* signal = dynamic_cast<Signal*> (signals_[typeid (T).name ()]);
      signal->disconnect_all_slots ();
    }
  }

  template<typename T> void
  base_signaler::block_signal ()
  {
    if (connections_.find (typeid (T).name ()) != connections_.end ())
      for (std::vector<boost::signals2::shared_connection_block>::iterator cIt = shared_connections_[typeid (T).name ()].begin (); cIt != shared_connections_[typeid (T).name ()].end (); ++cIt)
        cIt->block ();
  }

  template<typename T> void
  base_signaler::unblock_signal ()
  {
    if (connections_.find (typeid (T).name ()) != connections_.end ())
      for (std::vector<boost::signals2::shared_connection_block>::iterator cIt = shared_connections_[typeid (T).name ()].begin (); cIt != shared_connections_[typeid (T).name ()].end (); ++cIt)
        cIt->unblock ();
  }

  void
  base_signaler::block_signals ()
  {
    for (std::map<std::string, boost::signals2::signal_base*>::iterator signal_it = signals_.begin (); signal_it != signals_.end (); ++signal_it)
      for (std::vector<boost::signals2::shared_connection_block>::iterator cIt = shared_connections_[signal_it->first].begin (); cIt != shared_connections_[signal_it->first].end (); ++cIt)
        cIt->block ();
  }

  void
  base_signaler::unblock_signals ()
  {
    for (std::map<std::string, boost::signals2::signal_base*>::iterator signal_it = signals_.begin (); signal_it != signals_.end (); ++signal_it)
      for (std::vector<boost::signals2::shared_connection_block>::iterator cIt = shared_connections_[signal_it->first].begin (); cIt != shared_connections_[signal_it->first].end (); ++cIt)
        cIt->unblock ();
  }

  template<typename T> int
  base_signaler::num_slots () const
  {
    typedef boost::signals2::signal<T> Signal;

    // see if we have a signal for this type
    std::map<std::string, boost::signals2::signal_base*>::const_iterator signal_it = signals_.find (typeid (T).name ());
    if (signal_it != signals_.end ())
    {
      Signal* signal = dynamic_cast<Signal*> (signal_it->second);
      return (static_cast<int> (signal->num_slots ()));
    }
    return (0);
  }

  template<typename T> boost::signals2::signal<T>*
  base_signaler::createSignal ()
  {
    typedef boost::signals2::signal<T> Signal;

    if (signals_.find (typeid (T).name ()) == signals_.end ())
    {
      Signal* signal = new Signal ();
      signals_[typeid (T).name ()] = signal;
      return (signal);
    }
    return (0);
  }

  template<typename T> boost::signals2::connection
  base_signaler::registerCallback (const boost::function<T> & callback)
  {
    typedef boost::signals2::signal<T> Signal;
    if (signals_.find (typeid (T).name ()) == signals_.end ())
    {
      std::stringstream sstream;

      sstream << "no callback for type:" << typeid (T).name ();
        throw svl_exception::type_error (sstream.str () );
    }
    Signal* signal = dynamic_cast<Signal*> (signals_[typeid (T).name ()]);
    boost::signals2::connection ret = signal->connect (callback);

    connections_[typeid (T).name ()].push_back (ret);
    shared_connections_[typeid (T).name ()].push_back (boost::signals2::shared_connection_block (connections_[typeid (T).name ()].back (), false));
    signalsChanged ();
    return (ret);
  }

  template<typename T> bool
  base_signaler::providesCallback () const
  {
    if (signals_.find (typeid (T).name ()) == signals_.end ())
      return (false);
    return (true);
  }



#endif
