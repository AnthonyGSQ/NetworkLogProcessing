#ifndef HTTPSERVER_HPP
#define HTTPSERVER_HPP

// Internal utilitys, buffers and error management
#include <boost/beast/core.hpp>
// HTTP and WebSocket
#include <boost/beast/http.hpp>
// Network, sockets, I/O
#include <boost/asio.hpp>
// for concurrency
#include <thread>

namespace asio = boost::asio;
using tcp = asio::ip::tcp;

class HttpServer {
   public:
    // class constructor which ask for ip version and port to listen
    HttpServer(bool ipv4, int port);
    // initialize the acceptor tcp object, in case of fail, return false
    // otherwise, return true
    bool startAcceptor();

   private:
    bool ipv4;
    int port;
    // context initializer of the Http Server
    asio::io_context ioc;
    // configuration of the server
    tcp::acceptor acceptor{ioc};
    // this function waits until a client connect to the server, starting
    // the connection and making a thread detach to leave the main thread
    // free so it can accept more clients.
    void acceptConnections();
};

#endif