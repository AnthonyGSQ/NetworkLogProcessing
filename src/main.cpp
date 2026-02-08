#include <iostream>

#include "HttpServer.hpp"
#include "config/ConfigManager.hpp"

int main() {
    try {
        ConfigManager config;
        HttpServer server(config, true, 8080);
        server.start();
    } catch (const std::exception& e) {
        std::cerr << "[main] Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}