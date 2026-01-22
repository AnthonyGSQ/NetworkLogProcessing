#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <signal.h>
#include <cstdlib>
#include <unistd.h>
#include <barrier>

#include "../src/HttpServer.hpp"

TEST(HttpServer, ConstructorIpv4) {
    std::barrier sync_point(2);

    std::thread server_thread([&sync_point]() {
        HttpServer server(true, 8080);
        sync_point.arrive_and_wait();  // Espera aquí hasta que ambos threads lleguen
        server.startAcceptor();
    });
    server_thread.detach();

    sync_point.arrive_and_wait();  // Sincroniza con el thread del servidor

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    system("timeout 2 curl -s http://localhost:8080/ > /dev/null 2>&1");
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    kill(getpid(), SIGINT);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
}

TEST(HttpServer, ConstructorIpv6) {
    std::barrier sync_point(2);

    std::thread server_thread([&sync_point]() {
        HttpServer server(false, 8081);
        sync_point.arrive_and_wait();
        server.startAcceptor();
    });
    server_thread.detach();

    sync_point.arrive_and_wait();

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    system("timeout 2 curl -s http://[::1]:8081/ > /dev/null 2>&1");
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    kill(getpid(), SIGINT);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
}

TEST(HttpServer, NegativePort) {
    HttpServer server(true, -100);
    EXPECT_FALSE(server.startAcceptor());
}

TEST(HttpServer, PortAlreadyInUse) {
    std::barrier sync_point(2);

    std::thread server1_thread([&sync_point]() {
        HttpServer server(true, 8090);
        sync_point.arrive_and_wait();
        server.startAcceptor();
    });
    server1_thread.detach();

    sync_point.arrive_and_wait();

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Try to start second server on same port - should fail
    HttpServer server2(true, 8090);
    bool result = server2.startAcceptor();
    EXPECT_FALSE(result);

    kill(getpid(), SIGINT);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
}

TEST(HttpServer, StopServerClosesAcceptor) {
    std::barrier sync_point(2);

    std::thread server_thread([&sync_point]() {
        HttpServer server(true, 8091);
        sync_point.arrive_and_wait();
        server.startAcceptor();
    });
    server_thread.detach();

    sync_point.arrive_and_wait();

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    system("timeout 2 curl -s http://localhost:8091/ > /dev/null 2>&1");
    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    kill(getpid(), SIGINT);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
}