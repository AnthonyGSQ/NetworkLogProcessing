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
        server.startAcceptor();
    });
    server_thread.detach();

    sync_point.arrive_and_wait();  // Sincroniza con el thread del servidor

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    system("timeout 2 curl -s http://localhost:18080/ > /dev/null 2>&1");
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    kill(getpid(), SIGINT);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
}

TEST(HttpServer, ConstructorIpv6) {
    std::barrier sync_point(2);

    std::thread server_thread([&sync_point]() {
        HttpServer server(false, 18081);
        sync_point.arrive_and_wait();
        server.startAcceptor();
    });
    server_thread.detach();

    sync_point.arrive_and_wait();

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    system("timeout 2 curl -s http://[::1]:18081/ > /dev/null 2>&1");
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
        HttpServer server(true, 18090);
        sync_point.arrive_and_wait();
        server.startAcceptor();
    });
    server1_thread.detach();

    sync_point.arrive_and_wait();

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Try to start second server on same port - should fail
    HttpServer server2(true, 18090);
    bool result = server2.startAcceptor();
    EXPECT_FALSE(result);

    kill(getpid(), SIGINT);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
}

TEST(HttpServer, StopServerClosesAcceptor) {
    std::barrier sync_point(2);

    std::thread server_thread([&sync_point]() {
        HttpServer server(true, 18091);
        sync_point.arrive_and_wait();
        server.startAcceptor();
    });
    server_thread.detach();

    sync_point.arrive_and_wait();

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    system("timeout 2 curl -s http://localhost:18091/ > /dev/null 2>&1");
    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    kill(getpid(), SIGINT);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
}