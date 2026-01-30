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
#include "ThreadPool.hpp"

// shutdown (thread-safe) librarys
#include <atomic>
#include <condition_variable>
#include <mutex>
// signal handling
#include <signal.h>

namespace asio = boost::asio;
using tcp = asio::ip::tcp;

class HttpServer {
   public:
    // class constructor which ask for ip version and port to listen
    HttpServer(bool ipv4, int port);
    ~HttpServer();
    void start();
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
    // thread-safe shutdown mechanism
    std::atomic<bool> shouldStop{false};
    std::mutex serverMutex;
    std::condition_variable cond_var;
    // ThreadPool para procesar clientes de manera concurrente
    ThreadPool threadPool{4};  // 4 workers

    // singleton instance for signal handler
    static HttpServer* instance;
    static std::mutex instance_mtx;

    // this function waits until a client connect to the server, starting
    // the connection and making a thread detach to leave the main thread
    // free so it can accept more clients.
    void acceptConnections();
    // stop server (thread-safe) function
    void stopServer();
    // for checks if server is running
    bool isRunning() const;
    // start mutex, signals and singleton
    void setupSignalHandlers();
    // static signal handler
    static void handleSignal(int signal);
};

#endif