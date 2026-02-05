#include <gtest/gtest.h>
#include <signal.h>
#include <unistd.h>

#include <barrier>
#include <chrono>
#include <cstdlib>
#include <thread>

#include "../src/HttpServer.hpp"

TEST(HttpServer, ConstructorIpv4) {
    std::barrier sync_point(2);

    std::thread server_thread([&sync_point]() {
        HttpServer server(true, 18080);
        sync_point
            .arrive_and_wait();  // Espera aquí hasta que ambos threads lleguen
        server.start();
    });
    server_thread.detach();

    sync_point.arrive_and_wait();  // Sincroniza con el thread del servidor

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    system(
        "timeout 2 curl -s -X POST http://localhost:18080/ -H 'Content-Type: "
        "application/json' -d '{\"owner\": \"John Doe\", \"expirationDate\": "
        "\"2026-12-31\", \"category\": \"standard\", \"cost\": 150, \"room\": "
        "101}' > /dev/null 2>&1");
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    kill(getpid(), SIGINT);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
}

TEST(HttpServer, ConstructorIpv6) {
    std::barrier sync_point(2);

    std::thread server_thread([&sync_point]() {
        HttpServer server(false, 18081);
        sync_point.arrive_and_wait();
        server.start();
    });
    server_thread.detach();

    sync_point.arrive_and_wait();

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    system(
        "timeout 2 curl -s -X POST http://[::1]:18081/ -H 'Content-Type: "
        "application/json' -d '{\"owner\": \"Jane Smith\", \"expirationDate\": "
        "\"2026-06-30\", \"category\": \"premium\", \"cost\": 250, \"room\": "
        "205}' > /dev/null 2>&1");
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    kill(getpid(), SIGINT);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
}

TEST(HttpServer, NegativePort) {
    HttpServer server(true, -100);
    // start() es void, la validación del puerto ocurre internamente en
    // startAcceptor()
    server.start();
}

TEST(HttpServer, PortAlreadyInUse) {
    std::barrier sync_point(2);

    std::thread server1_thread([&sync_point]() {
        HttpServer server(true, 18090);
        sync_point.arrive_and_wait();
        server.start();
    });
    server1_thread.detach();

    sync_point.arrive_and_wait();

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    system(
        "timeout 2 curl -s -X POST http://localhost:18090/ -H 'Content-Type: "
        "application/json' -d '{\"owner\": \"Alice Johnson\", "
        "\"expirationDate\": \"2026-03-15\", \"category\": \"deluxe\", "
        "\"cost\": 300, \"room\": 310}' > /dev/null 2>&1");
    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    // Try to start second server on same port - startAcceptor() captura la
    // excepción internamente
    HttpServer server2(true, 18090);
    server2.start();

    kill(getpid(), SIGINT);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
}

TEST(HttpServer, InvalidJsonRequest) {
    std::barrier sync_point(2);

    std::thread server_thread([&sync_point]() {
        HttpServer server(true, 18091);
        sync_point.arrive_and_wait();
        server.start();
    });
    server_thread.detach();

    sync_point.arrive_and_wait();

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    // Send invalid JSON
    system(
        "timeout 2 curl -s -X POST http://localhost:18091/ -H 'Content-Type: "
        "application/json' -d '{invalid json}' > /dev/null 2>&1");
    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    kill(getpid(), SIGINT);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
}

TEST(HttpServer, ConcurrentRequests) {
    std::barrier sync_point(2);

    std::thread server_thread([&sync_point]() {
        HttpServer server(true, 18092);
        sync_point.arrive_and_wait();
        server.start();
    });
    server_thread.detach();

    sync_point.arrive_and_wait();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Enviar 10 solicitudes concurrentes desde diferentes threads
    std::vector<std::thread> clients;
    for (int i = 0; i < 10; i++) {
        clients.emplace_back([i]() {
            // Cada thread envía una solicitud con JSON válido
            std::string owner = "Guest" + std::to_string(i);
            std::string json = "{\"owner\": \"" + owner +
                               "\", \"expirationDate\": \"2026-12-31\", "
                               "\"category\": \"standard\", \"cost\": " +
                               std::to_string(100 + i * 10) +
                               ", \"room\": " + std::to_string(400 + i) + "}";
            std::string cmd =
                "timeout 2 curl -s -X POST http://localhost:18092/ -H "
                "'Content-Type: application/json' -d '" +
                json + "' > /dev/null 2>&1";
            system(cmd.c_str());
        });
    }

    // Esperar a que todos los clientes terminen
    for (auto& client : clients) {
        client.join();
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    kill(getpid(), SIGINT);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
}