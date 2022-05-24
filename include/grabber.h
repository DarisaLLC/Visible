#ifndef __SIMPLE_GRABBER__
#define __SIMPLE_GRABBER__

#include "exception.hpp"
#include <map>
#include <iostream>
#include <string>
#include <boost/signals2.hpp>
#include <boost/signals2/slot.hpp>
#include <typeinfo>
#include <vector>
#include <sstream>


// Error type
enum class grabber_error : int32 {
    OK = 0,            // No error
    Unknown,           // Unknown error
    Internal,          // Internal logic error
    Init,              // Unspecified init error
    FileInit,          // File (system) init error
    FileRead,          // File read error
    FileClose,         // File close error
    FileFormat,        // File invalid
    FileUnsupported,   // File format unsupported
    FileRevUnsupported,// File revision unsupported
    SystemResources,   // Inadequate system resources (shared memory,
    // heap)
    IPC,               // Error communicating with peer process
    QuicktimeInit,     // QuickTime init error
    QuicktimeRead,     // QuickTime read error
    CameraInit,        // Camera init error
    CameraCapture,     // Camera error during capture
    UnsupportedFormat, // Unsupported image format
    UnsupportedDepth,  // Unsupported image pixel depth
    NotImplemented,    // This feature not implemented yet
    FrameNotAvailable, // User specified no waiting and frame isn't
    // available
    OutOfMemory,       // Ran out of memory
    InvalidOptions     // Invalid options specified
};

// Status type
enum grabber_status {
    end_of_file = 0,    // There will be no more frames ever, stop requesting them
    OK,         // Valid frame
    NotStarted, // Grabbing process has not yet started
    NoFrame,    // No frame returned, one may be available later
    Error       // Fatal error, call getLastError() for details
};

/** \brief grabber interface for
 */
class simple_grabber
{
public:
    
    /** \brief Constructor. */
    simple_grabber () : signals_ (), connections_ (), shared_connections_ () {}
    
    /** \brief virtual desctructor. */
    virtual inline ~simple_grabber () throw ();
    
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
    virtual bool
    start () = 0;
    
    /** \brief For devices that are streaming, the streams are stopped.
     *        This method has no effect for triggered devices.
     */
    virtual bool
    stop () = 0;
    
    
    /** \brief returns the name of the concrete subclass.
     * \return the name of the concrete driver.
     */
    virtual std::string
    getName () const = 0;
    
    virtual bool isValid () const = 0;
    
    /** \brief Indicates whether the grabber is streaming or not. This value is not defined for triggered devices.
     * \return true if grabber is running / streaming. False otherwise.
     */
    virtual bool
    isRunning () {return true;}
    
    /** \brief returns fps. 0 if trigger based. */
    virtual float
    getFramesPerSecond () const {return -1.0f; }
    
    /** \brief returns frame count  */
    virtual int frameCount () const = 0;
    
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

simple_grabber::~simple_grabber () throw ()
{
    for (std::map<std::string, boost::signals2::signal_base*>::iterator signal_it = signals_.begin (); signal_it != signals_.end (); ++signal_it)
        delete signal_it->second;
}

template<typename T> boost::signals2::signal<T>*
simple_grabber::find_signal () const
{
    typedef boost::signals2::signal<T> Signal;
    
    std::map<std::string, boost::signals2::signal_base*>::const_iterator signal_it = signals_.find (typeid (T).name ());
    if (signal_it != signals_.end ())
        return (dynamic_cast<Signal*> (signal_it->second));
    
    return (NULL);
}

template<typename T> void
simple_grabber::disconnect_all_slots ()
{
    typedef boost::signals2::signal<T> Signal;
    
    if (signals_.find (typeid (T).name ()) != signals_.end ())
    {
        Signal* signal = dynamic_cast<Signal*> (signals_[typeid (T).name ()]);
        signal->disconnect_all_slots ();
    }
}

template<typename T> void
simple_grabber::block_signal ()
{
    if (connections_.find (typeid (T).name ()) != connections_.end ())
        for (std::vector<boost::signals2::shared_connection_block>::iterator cIt = shared_connections_[typeid (T).name ()].begin (); cIt != shared_connections_[typeid (T).name ()].end (); ++cIt)
            cIt->block ();
}

template<typename T> void
simple_grabber::unblock_signal ()
{
    if (connections_.find (typeid (T).name ()) != connections_.end ())
        for (std::vector<boost::signals2::shared_connection_block>::iterator cIt = shared_connections_[typeid (T).name ()].begin (); cIt != shared_connections_[typeid (T).name ()].end (); ++cIt)
            cIt->unblock ();
}

void
simple_grabber::block_signals ()
{
    for (std::map<std::string, boost::signals2::signal_base*>::iterator signal_it = signals_.begin (); signal_it != signals_.end (); ++signal_it)
        for (std::vector<boost::signals2::shared_connection_block>::iterator cIt = shared_connections_[signal_it->first].begin (); cIt != shared_connections_[signal_it->first].end (); ++cIt)
            cIt->block ();
}

void
simple_grabber::unblock_signals ()
{
    for (std::map<std::string, boost::signals2::signal_base*>::iterator signal_it = signals_.begin (); signal_it != signals_.end (); ++signal_it)
        for (std::vector<boost::signals2::shared_connection_block>::iterator cIt = shared_connections_[signal_it->first].begin (); cIt != shared_connections_[signal_it->first].end (); ++cIt)
            cIt->unblock ();
}

template<typename T> int
simple_grabber::num_slots () const
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
simple_grabber::createSignal ()
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
simple_grabber::registerCallback (const boost::function<T> & callback)
{
    typedef boost::signals2::signal<T> Signal;
    if (signals_.find (typeid (T).name ()) == signals_.end ())
    {
        std::stringstream sstream;
        
        sstream << "no callback for type:" << typeid (T).name ();
        throw vf_exception::io_error (sstream.str () );
    }
    Signal* signal = dynamic_cast<Signal*> (signals_[typeid (T).name ()]);
    boost::signals2::connection ret = signal->connect (callback);
    
    connections_[typeid (T).name ()].push_back (ret);
    shared_connections_[typeid (T).name ()].push_back (boost::signals2::shared_connection_block (connections_[typeid (T).name ()].back (), false));
    signalsChanged ();
    return (ret);
}

template<typename T> bool
simple_grabber::providesCallback () const
{
    if (signals_.find (typeid (T).name ()) == signals_.end ())
        return (false);
    return (true);
}



#endif
