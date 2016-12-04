#ifndef TCP_HPP
#define TCP_HPP
/*------------------------------------------------------------------------------

  Author:    Andy Rushton
  Copyright: (c) Andy Rushton, 2004
  License:   BSD License, see ../docs/license.html

  A platform-independent (Unix and Windows anyway) interface to TCP

  ------------------------------------------------------------------------------*/
#include "os_fixes.hpp"
#include "smart_ptr.hpp"
#include <string>

////////////////////////////////////////////////////////////////////////////////
// Internals

class TCP_connection_data;
class TCP_server_data;
class TCP_client_data;

////////////////////////////////////////////////////////////////////////////////
// Server Classes: A server creates a listening port which waits for incoming
// connections. This is placed on the port number appropriate for the service
// - for example, a Telnet server would typically use port 23. For a new
// service you should of course use any port not allocated to a standard
// service. I believe that RFC 1700 defines the standard service port numbers.
// When an incoming connection is made, the server accepts it and in the
// process creates a new connection on a different port. This leaves the
// standard port listening for further connections. In effect, the server
// farms out the handling of the connections themselves and only takes
// responsibility for accepting the connection. This is reflected in the class
// structure. A TCP_server performs the listening and accepting roles, but
// creates a TCP_connection to handle the accepted connection.

class TCP_connection
{
public:
  TCP_connection(void);
  ~TCP_connection(void);

  TCP_connection(const TCP_connection&);
  TCP_connection& operator=(const TCP_connection&);

  int error(void) const;
  std::string message(void) const;
  bool initialised(void) const;
  unsigned long address(void) const;
  unsigned short port(void) const;
  bool send_ready(unsigned wait);
  bool send (std::string& data);
  bool receive_ready(unsigned wait);
  bool receive (std::string& data);
  bool close(void);

protected:
  friend class TCP_connection_data;
  friend class TCP_server_data;
  smart_ptr<TCP_connection_data> m_data;
};

class TCP_server
{
public:
  TCP_server(void);
  TCP_server(unsigned short port, unsigned short queue);
  ~TCP_server(void);

  TCP_server(const TCP_server&);
  TCP_server& operator=(const TCP_server&);

  bool initialise(unsigned short port, unsigned short queue);
  bool initialised(void) const;
  int error(void) const;
  std::string message(void) const;
  bool close(void);

  bool connection_ready(unsigned wait);
  TCP_connection connection(void);

private:
  friend class TCP_server_data;
  smart_ptr<TCP_server_data> m_data;
};

////////////////////////////////////////////////////////////////////////////////
// Client Class: a client is simpler in that there is no listening port - you
// just create a connection and get on with it. Thus the client class does the
// whole job - create the connection and handle communications to/from it.
//
// Blocking mode: To use the client in blocking mode, use non-zero timeout for
// the initialisation method. In this mode, the connection operation must
// complete before the call will return or an error is indicated if the
// timeout is reached without completion. This usage was designed for
// applications which either just to TCP and nothing else or which do TCP
// operations in a separate thread.
//
// Non-blocking mode: To use the client in non-blocking mode, use a zero
// timeout for the initialisation method. Instead, you can ask it if it has
// connected once you've initialised it. It is not an error for it to be
// initialised but not connected. This usage was designed so that you can poll
// the connection periodically to implement a wait for as long as you like for
// the connection to occur without blocking the thread that uses the client.
//
// In both modes, the send_ready/receive_ready methods can be called with any
// timeout including zero.

class TCP_client
{
public:
  TCP_client(void);
  TCP_client(const std::string& address, unsigned short port, unsigned int timeout=0);
  ~TCP_client(void);

  TCP_client(const TCP_client&);
  TCP_client& operator=(const TCP_client&);

  int error(void) const;
  std::string message(void) const;
  bool initialise(const std::string& address, unsigned short port, unsigned int timeout=0);
  bool initialised(void) const;
  bool connected(void);
  unsigned long address(void) const;
  unsigned short port(void) const;
  bool send_ready(unsigned wait = 0);
  bool send (std::string& data);
  bool receive_ready(unsigned wait = 0);
  bool receive (std::string& data);
  bool close(void);

private:
  friend class TCP_client_data;
  smart_ptr<TCP_client_data> m_data;
};

////////////////////////////////////////////////////////////////////////////////
#endif
