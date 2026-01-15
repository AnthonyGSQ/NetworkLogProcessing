#include <iostream>
#include "HttpServer.hpp"

int main() {
    HttpServer server(true, 8080);
    server.startAcceptor();
    return 0;
}