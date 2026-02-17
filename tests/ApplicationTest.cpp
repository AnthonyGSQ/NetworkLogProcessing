#include <gtest/gtest.h>
#include <signal.h>

#include <barrier>
#include <chrono>
#include <thread>

#include "../src/Application/Application.hpp"

TEST(ApplicationTest, GoodConstructor) {
    std::barrier sync_point(2);
    Application app(".env", 9091);
    std::thread server_thread([&app, &sync_point]() {
        sync_point.arrive_and_wait();
        EXPECT_EQ(app.run(), 1);
    });
    server_thread.detach();
    sync_point.arrive_and_wait();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    app.stop();
}

TEST(ApplicationTest, BadConstructor) {
    try {
        EXPECT_THROW(Application app("invalid .env", 9092), std::runtime_error);
    } catch (...) {
        // nothing, it has to throw a runtime_err
    }
    try {
        EXPECT_THROW(Application app(".env", -100), std::runtime_error);
    } catch (...) {
        // nothing
    }
}