#include "HttpServer.hpp"
#include "ClientConnection.hpp"
#include <iostream>
#include <thread>

HttpServer::HttpServer(bool ipv4, int port) :
    ipv4(ipv4), port(port), acceptor(ioc) {}


bool HttpServer::startAcceptor() {
    try {
        if (ipv4) {
            acceptor.open(tcp::v4());
        } else {
            acceptor.open(tcp::v6());
        }
        acceptor.bind(tcp::endpoint(tcp::v4(), port));
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
    for(;;) {
        tcp::socket currentSocket{ioc};
        acceptor.accept(currentSocket);
        
        clientConnection currentClient(std::move(currentSocket));
        std::thread t(std::move(currentClient));
        t.detach();
    }
}
