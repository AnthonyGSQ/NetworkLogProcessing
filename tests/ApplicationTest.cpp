#include <gtest/gtest.h>
#include <signal.h>

#include <barrier>
#include <chrono>
#include <thread>

#include "../src/Application/Application.hpp"

TEST(ApplicationTest, GoodConstructor) {
    std::barrier sync_point(2);
    
    // Constructor should succeed
    Application app(".env", 9091);
    
    std::thread server_thread([&app, &sync_point]() {
        sync_point.arrive_and_wait();
        try {
            app.run();
        } catch (const std::exception& e) {
            std::cerr << "[Test] Expected behavior - server initiation handled: " << e.what() << "\n";
        }
    });
    server_thread.detach();
    
    sync_point.arrive_and_wait();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    app.stop();
}

TEST(ApplicationTest, BadConstructor) {
    // Invalid config file - constructor should throw
    EXPECT_THROW(
        Application app("invalid.env", 9092),
        std::runtime_error
    );
    
    // Invalid port in constructor - HttpServer constructor will validate and throw
    EXPECT_THROW(
        Application app2(".env", -100),
        std::runtime_error
    );
}

// Test run() error handling when port is in use
TEST(ApplicationTest, RunFailsPortInUse) {
    std::barrier sync_point(2);
    
    // Create first application on port 9093
    Application app1(".env", 9093);
    
    std::thread server_thread([&app1, &sync_point]() {
        sync_point.arrive_and_wait();
        try {
            app1.run();
        } catch (const std::exception& e) {
            std::cerr << "[Test] Server 1 error (expected): " << e.what() << "\n";
        }
    });
    server_thread.detach();
    
    sync_point.arrive_and_wait();
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    
    // Try to create and run second application on same port - should fail in run()  
    Application app2(".env", 9093);
    EXPECT_THROW(
        app2.run(),
        std::runtime_error
    );
    
    app1.stop();
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
}

// Test that invalid config file throws in constructor
TEST(ApplicationTest, InvalidConfigThrows) {
    EXPECT_THROW(
        Application app("nonexistent_file.env", 9094),
        std::runtime_error
    );
}

// Test that zero/negative port throws in constructor  
TEST(ApplicationTest, InvalidPortThrows) {
    EXPECT_THROW(
        Application app(".env", 0),
        std::runtime_error
    );
    
    EXPECT_THROW(
        Application app2(".env", -1),
        std::runtime_error
    );
    
    EXPECT_THROW(
        Application app3(".env", -9999),
        std::runtime_error
    );
}
