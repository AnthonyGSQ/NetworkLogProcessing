/*
 * Copyright © 2026 Anthony Sanchez
 * Licensed under CC BY 4.0 (Creative Commons Attribution 4.0 International)
 * https://creativecommons.org/licenses/by/4.0/
 *
 * Network Log Processing - HTTP Server Implementation
 */

#ifndef HTTPSERVER_HPP
#define HTTPSERVER_HPP

// Internal utilitys, buffers and error management
#include <boost/beast/core.hpp>
// HTTP and WebSocket
#include <boost/beast/http.hpp>
// Network, sockets, I/O
#include <boost/asio.hpp>
// for concurrency
#include <memory>
#include <thread>

#include "PostgresDB.hpp"
#include "ThreadPool.hpp"
#include "config/ConfigManager.hpp"

// shutdown (thread-safe) librarys
#include <atomic>
#include <condition_variable>
#include <mutex>
// signal handling
#include <signal.h>

namespace asio = boost::asio;
using tcp = asio::ip::tcp;

// HTTP server with thread pool for concurrent request handling.
// Accepts connections on a specified port and delegates request processing
// to worker threads. Graceful shutdown is triggered by signals (SIGINT,
// SIGTERM, SIGTSTP) or through destructor cleanup.
class HttpServer {
   public:
    // Constructor: initializes server with config and database connection
    HttpServer(PostgresDB* db);
    // Destructor: triggers graceful shutdown sequence
    ~HttpServer();
    // Starts the server: opens acceptor and accepts connections (blocking)
    void start();
    // Stops the server gracefully
    void stop();
    // Opens TCP acceptor on configured port. Returns false on failure.
    bool startAcceptor();

   private:
    bool ipv4;
    int port;
    PostgresDB* database;
    // ASIO context managing all I/O operations
    asio::io_context ioc;
    // TCP acceptor listening for incoming connections
    std::unique_ptr<tcp::acceptor> acceptor;
    // Thread-safe flag: set to true by signal handler or stop sequence
    std::atomic<bool> shouldStop{false};
    std::mutex serverMutex;
    std::condition_variable cond_var;
    // Worker thread pool (4 threads) for processing client requests
    ThreadPool threadPool{4};

    // Accepts incoming connections and enqueues them for processing.
    // Runs in calling thread of start(). Detects shutdown via shouldStop flag.
    void acceptConnections();
    // Initiates graceful shutdown: closes acceptor, stops IO context,
    // and signals worker threads to complete their tasks.
    void stopServer();
    // Thread-safe check of server running state
    bool isRunning() const;
};

#endif