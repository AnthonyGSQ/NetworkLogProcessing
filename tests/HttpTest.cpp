#include <gtest/gtest.h>
#include <signal.h>
#include <unistd.h>

#include <barrier>
#include <chrono>
#include <cstdlib>
#include <thread>
#include <sstream>

#include "../src/HttpServer.hpp"
#include "../src/config/ConfigManager.hpp"
#include "../src/PostgresDB.hpp"

// Helper function to delete test data from database
static void cleanupTestData(const std::string& guestNamePattern) {
    try {
        ConfigManager config(".env");
        
        // Build connection string from config
        std::ostringstream oss;
        oss << "host=" << config.get("DB_HOST")
            << " port=" << config.getInt("DB_PORT")
            << " dbname=" << config.get("DB_NAME")
            << " user=" << config.get("DB_USER")
            << " password=" << config.get("DB_PASSWORD");
        
        pqxx::connection conn(oss.str());
        pqxx::work txn(conn);
        
        // Delete test data with specific guest name pattern
        std::string deleteQuery = 
            "DELETE FROM reservations WHERE guest_name LIKE '" + guestNamePattern + "%'";
        txn.exec(deleteQuery);
        txn.commit();
        std::cout << "[HttpTest] Cleaned up test data for: " << guestNamePattern << "\n";
    } catch (const std::exception& e) {
        std::cerr << "[HttpTest] Cleanup failed: " << e.what() << "\n";
    }
}

// Valid JSON with all required fields
static std::string getValidJson(int variant = 0) {
    long timestamp = 1707427200 + (variant * 86400);
    std::string guestName = "TestGuest" + std::to_string(variant);
    std::string guestEmail = "test" + std::to_string(variant) + "@example.com";
    std::string guestPhone = "+3461234567" + std::to_string(variant % 10);
    int roomNumber = 100 + variant;
    
    std::ostringstream oss;
    oss << "{"
        << "\"guest_name\":\"" << guestName << "\","
        << "\"guest_email\":\"" << guestEmail << "\","
        << "\"guest_phone\":\"" << guestPhone << "\","
        << "\"room_number\":" << roomNumber << ","
        << "\"room_type\":\"Doble\","
        << "\"number_of_guests\":2,"
        << "\"check_in_date\":\"2026-02-15\","
        << "\"check_out_date\":\"2026-02-18\","
        << "\"number_of_nights\":3,"
        << "\"price_per_night\":150.0,"
        << "\"total_price\":450.0,"
        << "\"payment_method\":\"credit_card\","
        << "\"paid\":true,"
        << "\"reservation_status\":\"confirmed\","
        << "\"special_requests\":\"Test reservation\","
        << "\"created_at\":" << timestamp << ","
        << "\"updated_at\":" << timestamp
        << "}";
    return oss.str();
}

// Invalid JSON - missing required fields
static std::string getInvalidJson(int variant = 0) {
    std::string guestName = "InvalidGuest" + std::to_string(variant);
    int roomNumber = 200 + variant;
    
    std::ostringstream oss;
    oss << "{"
        << "\"guest_name\":\"" << guestName << "\","
        << "\"room_number\":" << roomNumber
        << "}";
    return oss.str();
}

TEST(HttpServer, ConstructorIpv4) {
    std::barrier sync_point(2);
    std::string testGuest = "IPv4Guest";

    std::thread server_thread([&sync_point]() {
        ConfigManager config(".env");
        HttpServer server(config, true, 18080);
        sync_point.arrive_and_wait();
        server.start();
    });
    server_thread.detach();

    sync_point.arrive_and_wait();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    // Send valid reservation
    std::string json = getValidJson(0);
    std::string cmd = "timeout 2 curl -s -X POST http://localhost:18080/ -H 'Content-Type: "
        "application/json' -d '" + json + "' > /dev/null 2>&1";
    system(cmd.c_str());
    
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    kill(getpid(), SIGINT);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // Cleanup
    cleanupTestData("TestGuest");
}

TEST(HttpServer, ConstructorIpv6) {
    std::barrier sync_point(2);

    std::thread server_thread([&sync_point]() {
        ConfigManager config(".env");
        HttpServer server(config, false, 18081);
        sync_point.arrive_and_wait();
        server.start();
    });
    server_thread.detach();

    sync_point.arrive_and_wait();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    std::string json = getValidJson(1);
    std::string cmd = "timeout 2 curl -s -X POST http://[::1]:18081/ -H 'Content-Type: "
        "application/json' -d '" + json + "' > /dev/null 2>&1";
    system(cmd.c_str());
    
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    kill(getpid(), SIGINT);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // Cleanup
    cleanupTestData("TestGuest");
}

TEST(HttpServer, NegativePort) {
    ConfigManager config(".env");
    HttpServer server(config, true, -100);
    server.start();
}

TEST(HttpServer, PortAlreadyInUse) {
    std::barrier sync_point(2);

    std::thread server1_thread([&sync_point]() {
        ConfigManager config(".env");
        HttpServer server(config, true, 18090);
        sync_point.arrive_and_wait();
        server.start();
    });
    server1_thread.detach();

    sync_point.arrive_and_wait();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    std::string json = getValidJson(2);
    std::string cmd = "timeout 2 curl -s -X POST http://localhost:18090/ -H 'Content-Type: "
        "application/json' -d '" + json + "' > /dev/null 2>&1";
    system(cmd.c_str());
    
    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    ConfigManager config(".env");
    HttpServer server2(config, true, 18090);
    server2.start();
    kill(getpid(), SIGINT);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // Cleanup
    cleanupTestData("TestGuest");
}

TEST(HttpServer, InvalidJsonRequest) {
    std::barrier sync_point(2);

    std::thread server_thread([&sync_point]() {
        ConfigManager config(".env");
        HttpServer server(config, true, 18091);
        sync_point.arrive_and_wait();
        server.start();
    });
    server_thread.detach();

    sync_point.arrive_and_wait();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    // Send invalid JSON - malformed
    system(
        "timeout 2 curl -s -X POST http://localhost:18091/ -H 'Content-Type: "
        "application/json' -d '{invalid json}' > /dev/null 2>&1");
    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    kill(getpid(), SIGINT);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
}

TEST(HttpServer, MissingRequiredFields) {
    std::barrier sync_point(2);

    std::thread server_thread([&sync_point]() {
        ConfigManager config(".env");
        HttpServer server(config, true, 18093);
        sync_point.arrive_and_wait();
        server.start();
    });
    server_thread.detach();

    sync_point.arrive_and_wait();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    // Send JSON with missing required fields
    std::string json = getInvalidJson(0);
    std::string cmd = "timeout 2 curl -s -X POST http://localhost:18093/ -H 'Content-Type: "
        "application/json' -d '" + json + "' > /dev/null 2>&1";
    system(cmd.c_str());
    
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    kill(getpid(), SIGINT);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
}

TEST(HttpServer, ConcurrentValidRequests) {
    std::barrier sync_point(2);

    std::thread server_thread([&sync_point]() {
        ConfigManager config(".env");
        HttpServer server(config, true, 18094);
        sync_point.arrive_and_wait();
        server.start();
    });
    server_thread.detach();

    sync_point.arrive_and_wait();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Send 5 concurrent valid requests
    std::vector<std::thread> clients;
    for (int i = 0; i < 5; i++) {
        clients.emplace_back([i]() {
            std::string json = getValidJson(i + 10);
            std::string cmd = "timeout 2 curl -s -X POST http://localhost:18094/ -H "
                "'Content-Type: application/json' -d '" + json + "' > /dev/null 2>&1";
            system(cmd.c_str());
        });
    }

    for (auto& client : clients) {
        client.join();
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    kill(getpid(), SIGINT);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // Cleanup
    cleanupTestData("TestGuest");
}

TEST(HttpServer, ConcurrentMixedRequests) {
    std::barrier sync_point(2);

    std::thread server_thread([&sync_point]() {
        ConfigManager config(".env");
        HttpServer server(config, true, 18095);
        sync_point.arrive_and_wait();
        server.start();
    });
    server_thread.detach();

    sync_point.arrive_and_wait();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Send mix of valid and invalid requests
    std::vector<std::thread> clients;
    for (int i = 0; i < 5; i++) {
        clients.emplace_back([i]() {
            std::string json = (i % 2 == 0) ? getValidJson(i + 20) : getInvalidJson(i + 20);
            std::string cmd = "timeout 2 curl -s -X POST http://localhost:18095/ -H "
                "'Content-Type: application/json' -d '" + json + "' > /dev/null 2>&1";
            system(cmd.c_str());
        });
    }

    for (auto& client : clients) {
        client.join();
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    kill(getpid(), SIGINT);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // Cleanup - only valid requests were inserted
    cleanupTestData("TestGuest");
}