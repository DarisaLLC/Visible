#ifndef __IO_base_signaler__
#define __IO_base_signaler__

#include "core/svl_exception.hpp"
#include <map>
#include <iostream>
#include <string>
#include <typeinfo>
#include <vector>
#include <sstream>
#include <boost/signals2.hpp>
#include <boost/signals2/slot.hpp>


  /** \brief signaler interface for 
   
   // make callback function from member function
    std::function<void (..onearg..)> f = boost::bind (&Observer::cb_, this, _1);
   
   // connect callback function for desired signal. In this case its a point cloud with color values
   boost::signals2::connection c = interface->registerCallback (f);

    */
  class base_signaler
  {
    public:

      /** \brief Constructor. */
      base_signaler () = default;

      virtual inline ~base_signaler () noexcept = default;
      
      /**
          * \brief No copy ctor since base_signaler can't be copied
          */
         base_signaler(const base_signaler&) = delete;

         /**
          * \brief No copy assign operator since base_signaler can't be copied
          */
         base_signaler& operator=(const base_signaler&) = delete;

         /**
          * \brief Move ctor
          */
         base_signaler(base_signaler&&) = default;

         /**
          * \brief Move assign operator
          */
         base_signaler& operator=(base_signaler&&) = default;

      /** \brief registers a callback function/method to a signal with the corresponding signature
        * \param[in] callback: the callback function/method
        * \return Connection object, that can be used to disconnect the callback method from the signal again.
        */
      template<typename T> boost::signals2::connection 
      registerCallback (const std::function<T>& callback);

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



  template<typename T> boost::signals2::signal<T>*
  base_signaler::find_signal () const
  {
    typedef boost::signals2::signal<T> Signal;

    const auto signal_it = signals_.find (typeid (T).name ());
    if (signal_it != signals_.end ())
      return (dynamic_cast<Signal*> (signal_it->second));

    return (NULL);
  }

  template<typename T> void
  base_signaler::disconnect_all_slots ()
  {
      const auto signal = find_signal<T> ();
       if (signal != nullptr)
       {
         signal->disconnect_all_slots ();
       }

  }

  template<typename T> void
  base_signaler::block_signal ()
  {
      if (connections_.find (typeid (T).name ()) != connections_.end ())
          for (auto &connection : shared_connections_[typeid (T).name ()])
            connection.block ();
  }

  template<typename T> void
  base_signaler::unblock_signal ()
  {
    if (connections_.find (typeid (T).name ()) != connections_.end ())
        for (auto &connection : shared_connections_[typeid (T).name ()])
          connection.unblock ();
  }

  void
  base_signaler::block_signals ()
  {
      for (const auto &signal : signals_)
          for (auto &connection : shared_connections_[signal.first])
            connection.block ();
  }

  void
  base_signaler::unblock_signals ()
  {
      for (const auto &signal : signals_)
         for (auto &connection : shared_connections_[signal.first])
           connection.unblock ();
  }

  template<typename T> int
  base_signaler::num_slots () const
  {
   const auto signal = find_signal<T> ();
      if (signal != nullptr)
      {
        return static_cast<int> (signal->num_slots ());
      }
      return 0;
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
  base_signaler::registerCallback (const std::function<T> & callback)
  {
     const auto signal = find_signal<T> ();
      if (signal == nullptr)
      {
        std::stringstream sstream;
        sstream << "no callback for type:" << typeid (T).name ();
        throw svl::type_error (sstream.str () );
      }
      boost::signals2::connection ret = signal->connect (callback);

      connections_[typeid (T).name ()].push_back (ret);
      shared_connections_[typeid (T).name ()].push_back (boost::signals2::shared_connection_block (connections_[typeid (T).name ()].back (), false));
      signalsChanged ();
      return (ret);
  }

  template<typename T> bool
  base_signaler::providesCallback () const
  {
    return find_signal<T> ();
  }



#endif
