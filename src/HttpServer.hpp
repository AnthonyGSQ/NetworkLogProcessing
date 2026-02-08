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
#include <thread>
#include <memory>

#include "ThreadPool.hpp"
#include "PostgresDB.hpp"
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
    // Constructor: initializes server for specified IP version and port
    HttpServer(const ConfigManager& config, bool ipv4, int port);
    // Destructor: triggers graceful shutdown sequence
    ~HttpServer();
    // Blocks while accepting and processing connections.
    // Returns when shutdown signal is received.
    void start();
    // Opens TCP acceptor on configured port. Returns false on failure.
    bool startAcceptor();

   private:
    bool ipv4;
    int port;
    std::unique_ptr<PostgresDB> database;
    // ASIO context managing all I/O operations
    asio::io_context ioc;
    // TCP acceptor listening for incoming connections
    tcp::acceptor acceptor{ioc};
    // Thread-safe flag: set to true by signal handler or stop sequence
    std::atomic<bool> shouldStop{false};
    std::mutex serverMutex;
    std::condition_variable cond_var;
    // Worker thread pool (4 threads) for processing client requests
    ThreadPool threadPool{4};

    // Singleton instance for static signal handler callback
    static HttpServer* instance;
    static std::mutex instance_mtx;

    // Accepts incoming connections and enqueues them for processing.
    // Runs in calling thread of start(). Detects shutdown via shouldStop flag.
    void acceptConnections();
    // Initiates graceful shutdown: closes acceptor, stops IO context,
    // and signals worker threads to complete their tasks.
    void stopServer();
    // Thread-safe check of server running state
    bool isRunning() const;
    // Registers signal handlers (SIGINT, SIGTERM, SIGTSTP) and sets singleton
    void setupSignalHandlers();
    // Static callback for signal handling. Calls stopServer() on instance.
    static void handleSignal(int signal);
};

#endif