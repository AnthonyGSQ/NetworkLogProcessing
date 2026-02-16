#include "HttpServer.hpp"

#include <iostream>
#include <stdexcept>
#include <thread>

#include "ClientConnection.hpp"

HttpServer::HttpServer(PostgresDB* db, int port_param)
    : ipv4(true), port(port_param), database(db), acceptor(nullptr) {
    // TODO: Get port from config if available when port_param is 0
    // if (port == 0) port = config.getInt("HTTP_PORT", 8080);
}

HttpServer::~HttpServer() { stopServer(); }

void HttpServer::start() {
    if (!startAcceptor()) {
        throw std::runtime_error("Failed to start HTTP acceptor");
    }
    acceptConnections();
}

void HttpServer::stop() { stopServer(); }

bool HttpServer::startAcceptor() {
    try {
        // Validate port FIRST before any operations
        if (port <= 0) {
            std::cerr << "HttpServer::startAcceptor(): Error: invalid port "
                      << port << "\n";
            return false;
        }

        // Create acceptor now (deferred from constructor)
        acceptor = std::make_unique<tcp::acceptor>(ioc);

        if (ipv4) {
            acceptor->open(tcp::v4());
            // Set reuse_address BEFORE bind to allow rapid port reuse
            acceptor->set_option(tcp::acceptor::reuse_address(true));
            acceptor->bind(tcp::endpoint(tcp::v4(), port));
        } else {
            acceptor->open(tcp::v6());
            // Set reuse_address BEFORE bind to allow rapid port reuse
            acceptor->set_option(tcp::acceptor::reuse_address(true));
            acceptor->bind(tcp::endpoint(tcp::v6(), port));
        }
        acceptor->listen(asio::socket_base::max_listen_connections);

        std::cout << "Server listening on port " << port << "\n";
        return true;
    } catch (const std::exception& e) {
        std::cerr << "HttpServer::startAcceptor() failed: " << e.what() << "\n";
        return false;
    }
}

void HttpServer::acceptConnections() {
    while (!this->shouldStop) {
        try {
            // Check if acceptor is open before attempting to accept
            if (!acceptor || !acceptor->is_open()) {
                std::cerr << "HttpServer::acceptConnections(): acceptor is not "
                             "open\n";
                break;
            }

            tcp::socket currentSocket{ioc};
            acceptor->accept(currentSocket);

            // we create the clientconnection with his respective socket and
            // database
            clientConnection client(std::move(currentSocket), database);
            // now we put the task clientConnection in the queue to be consumed
            // by a thread
            threadPool.enqueueTask(std::move(client));

        } catch (const std::exception& e) {
            if (!shouldStop) {
                std::cerr << "HttpServer::acceptConnections(): error accepting "
                             "connections: "
                          << e.what() << "\n";
            }
        }
    }
    // wait until all threads have finishes his work or the ioc stopped
    ioc.run();
}

void HttpServer::stopServer() {
    {
        std::unique_lock<std::mutex> lock(this->serverMutex);
        this->shouldStop = true;
    }
    this->cond_var.notify_all();
    try {
        if (acceptor && acceptor->is_open()) {
            acceptor->close();
        }
    } catch (const std::exception& e) {
        std::cerr << "HttpServer::stopServer(): Warning! acceptor is already "
                     "closed\n";
    }
    ioc.stop();
    std::cerr << "Server shutting down gracefull..\n";
}

bool HttpServer::isRunning() const { return !shouldStop; }
