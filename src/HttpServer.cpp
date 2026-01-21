#include "HttpServer.hpp"

#include <iostream>
#include <thread>

#include "ClientConnection.hpp"

HttpServer* HttpServer::instance = nullptr;
std::mutex HttpServer::instance_mtx;
HttpServer::HttpServer(bool ipv4, int port)
    : ipv4(ipv4), port(port), acceptor(ioc) {
    this->setupSignalHandlers();
}

bool HttpServer::startAcceptor() {
    try {
        if (ipv4) {
            acceptor.open(tcp::v4());
            acceptor.bind(tcp::endpoint(tcp::v4(), port));
        } else {
            acceptor.open(tcp::v6());
            acceptor.bind(tcp::endpoint(tcp::v6(), port));
        }
        if (port <= 0) {
            std::cerr << "HttpServer::startAcceptor(): Error: invalid port\n";
            return false;
        }
        // TODO: a little risky if there is old http requests in the network
        // buffer
        acceptor.set_option(tcp::acceptor::reuse_address(true));
        acceptor.listen(asio::socket_base::max_listen_connections);

        std::cout << "Server listening on port " << port << "\n";
        acceptConnections();
        ioc.run();
        return true;
    } catch (const std::exception& e) {
        std::cerr << "HttpServer::startAcceptor() failed: " << e.what() << "\n";
        return false;
    }
}

void HttpServer::acceptConnections() {
    while (!this->shouldStop) {
        try {
            tcp::socket currentSocket{ioc};
            acceptor.accept(currentSocket);

            clientConnection currentClient(std::move(currentSocket));
            std::thread t(std::move(currentClient));
            t.detach();
        } catch (const std::exception& e) {
            if (shouldStop) break;
            std::cerr << "HttpServer::acceptConnections(): error accepting "
                         "connections\n";
        }
    }
}

void HttpServer::stopServer() {
    {
        std::unique_lock<std::mutex> lock(this->serverMutex);
        this->shouldStop = true;
    }
    this->cond_var.notify_all();
    try {
        acceptor.close();
    } catch (const std::exception& e) {
        std::cerr << "HttpServer::stopServer(): Warning! acceptor is already "
                     "closed\n";
    }
    ioc.stop();
    std::cerr << "Server shutting down gracefull..\n";
}

bool HttpServer::isRunning() const { return !shouldStop; }

void HttpServer::handleSignal(int signal) {
    std::unique_lock<std::mutex> lock(instance_mtx);
    if (!instance) {
        return;
    }
    if (signal == SIGTERM || signal == SIGINT || signal == SIGTSTP) {
        std::cout << "\n[SIGNAL] Received signal " << signal
                  << ", initiating graceful shutdown...\n";
        instance->stopServer();
    }
}

void HttpServer::setupSignalHandlers() {
    {
        std::unique_lock<std::mutex> lock(instance_mtx);
        // set singleton
        instance = this;
    }
    signal(SIGINT, HttpServer::handleSignal);   // Ctrl+C
    signal(SIGTERM, HttpServer::handleSignal);  // kill <pid>
    signal(SIGTSTP, HttpServer::handleSignal);  // Ctrl+Z
    std::cout << "Signal handlers registered (SIGTERM, SIGINT, SIGTSTP)\n";
}
