#include <gtest/gtest.h>
#include <signal.h>
#include <unistd.h>

#include <barrier>
#include <chrono>
#include <cstdlib>
#include <sstream>
#include <thread>

#include <boost/asio.hpp>
#include <boost/beast.hpp>

#include "../src/DataBase/PostgresDB.hpp"
#include "../src/HTTP/HttpServer.hpp"
#include "../src/config/ConfigManager.hpp"
#include "../src/config/SignalManager.hpp"

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;

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
            "DELETE FROM reservations WHERE guest_name LIKE '" +
            guestNamePattern + "%'";
        txn.exec(deleteQuery);
        txn.commit();
        std::cout << "[HttpTest] Cleaned up test data for: " << guestNamePattern
                  << "\n";
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
        << "\"updated_at\":" << timestamp << "}";
    return oss.str();
}

// Invalid JSON - missing required fields
static std::string getInvalidJson(int variant = 0) {
    std::string guestName = "InvalidGuest" + std::to_string(variant);
    int roomNumber = 200 + variant;

    std::ostringstream oss;
    oss << "{"
        << "\"guest_name\":\"" << guestName << "\","
        << "\"room_number\":" << roomNumber << "}";
    return oss.str();
}

TEST(HttpServer, ConstructorIpv4) {
    std::barrier sync_point(2);
    std::string testGuest = "IPv4Guest";
    ConfigManager config(".env");
    PostgresDB db(config);
    HttpServer server(&db, 8080);
    // Setup signal handling for this server
    SignalManager sigManager;
    sigManager.setCallback([&server]() { server.stop(); });
    sigManager.setup();
    std::thread server_thread([&sync_point, &server]() {
        sync_point.arrive_and_wait();
        server.start();
    });
    server_thread.detach();

    sync_point.arrive_and_wait();
    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    // Send valid reservation
    std::string json = getValidJson(0);
    std::string cmd =
        "timeout 2 curl -s -X POST http://localhost:8080/ -H 'Content-Type: "
        "application/json' -d '" +
        json + "' > /dev/null 2>&1";
    system(cmd.c_str());

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    server.stop();
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));

    // Cleanup
    cleanupTestData("TestGuest");
}

TEST(HttpServer, ConstructorIpv6) {
    std::barrier sync_point(2);
    ConfigManager config(".env");
    PostgresDB db(config);
    HttpServer server(&db, 8081);
    std::thread server_thread([&sync_point, &server]() {
        sync_point.arrive_and_wait();
        server.start();
    });
    server_thread.detach();
    // Setup signal handling for this server
    SignalManager sigManager;
    sigManager.setCallback([&server]() { server.stop(); });
    sigManager.setup();
    sync_point.arrive_and_wait();
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    std::string json = getValidJson(1);
    std::string cmd =
        "timeout 2 curl -s -X POST http://[::1]:8081/ -H 'Content-Type: "
        "application/json' -d '" +
        json + "' > /dev/null 2>&1";
    system(cmd.c_str());

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    kill(getpid(), SIGINT);
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));

    // Cleanup
    cleanupTestData("TestGuest");
}

TEST(HttpServer, PortInUse) {
    std::barrier sync_point(2);
    ConfigManager config(".env");
    PostgresDB db(config);
    HttpServer server(&db, 8081);
    std::thread server_thread([&sync_point, &server]() {
        sync_point.arrive_and_wait();
        try {
            EXPECT_THROW(server.start(), std::runtime_error);
        } catch(...) {
            // nothing
        }
    });
    server_thread.detach();
    // Setup signal handling for this server
    SignalManager sigManager;
    sigManager.setCallback([&server]() { server.stop(); });
    sigManager.setup();
    sync_point.arrive_and_wait();
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    try {
        EXPECT_THROW(HttpServer server2(&db, 8081), std::runtime_error);
    } catch (...) {
        // nothing
    }
    kill(getpid(), SIGINT);
}

TEST(HttpServer, InvalidJsonRequest) {
    std::barrier sync_point(2);
    ConfigManager config(".env");
    PostgresDB db(config);
    HttpServer server(&db, 8082);

    std::thread server_thread([&sync_point, &server]() {
        sync_point.arrive_and_wait();
        server.start();
    });
    server_thread.detach();

    // Setup signal handling for this server
    SignalManager sigManager;
    sigManager.setCallback([&server]() { server.stop(); });
    sigManager.setup();

    sync_point.arrive_and_wait();
    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    // Send invalid JSON - malformed
    system(
        "timeout 2 curl -s -X POST http://localhost:8082/ -H 'Content-Type: "
        "application/json' -d '{invalid json}' > /dev/null 2>&1");
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    server.stop();
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));
}

TEST(HttpServer, MissingRequiredFields) {
    std::barrier sync_point(2);
    ConfigManager config(".env");
    PostgresDB db(config);
    HttpServer server(&db, 8083);

    std::thread server_thread([&sync_point, &server]() {
        sync_point.arrive_and_wait();
        server.start();
    });
    server_thread.detach();

    // Setup signal handling for this server
    SignalManager sigManager;
    sigManager.setCallback([&server]() { server.stop(); });
    sigManager.setup();

    sync_point.arrive_and_wait();
    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    // Send JSON with missing required fields
    std::string json = getInvalidJson(0);
    std::string cmd =
        "timeout 2 curl -s -X POST http://localhost:8083/ -H 'Content-Type: "
        "application/json' -d '" +
        json + "' > /dev/null 2>&1";
    system(cmd.c_str());

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    server.stop();
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));
}

TEST(HttpServer, ConcurrentValidRequests) {
    std::barrier sync_point(2);
    ConfigManager config(".env");
    PostgresDB db(config);
    HttpServer server(&db, 8084);

    std::thread server_thread([&sync_point, &server]() {
        sync_point.arrive_and_wait();
        server.start();
    });
    server_thread.detach();

    // Setup signal handling for this server
    SignalManager sigManager;
    sigManager.setCallback([&server]() { server.stop(); });
    sigManager.setup();

    sync_point.arrive_and_wait();
    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    // Send 5 concurrent valid requests
    std::vector<std::thread> clients;
    for (int i = 0; i < 5; i++) {
        clients.emplace_back([i]() {
            std::string json = getValidJson(i + 10);
            std::string cmd =
                "timeout 2 curl -s -X POST http://localhost:8084/ -H "
                "'Content-Type: application/json' -d '" +
                json + "' > /dev/null 2>&1";
            system(cmd.c_str());
        });
    }

    for (auto& client : clients) {
        client.join();
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    server.stop();
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));

    // Cleanup
    cleanupTestData("TestGuest");
}

TEST(HttpServer, ConcurrentMixedRequests) {
    std::barrier sync_point(2);
    ConfigManager config(".env");
    PostgresDB db(config);
    HttpServer server(&db, 8085);

    std::thread server_thread([&sync_point, &server]() {
        sync_point.arrive_and_wait();
        server.start();
    });
    server_thread.detach();

    // Setup signal handling for this server
    SignalManager sigManager;
    sigManager.setCallback([&server]() { server.stop(); });
    sigManager.setup();

    sync_point.arrive_and_wait();
    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    // Send mix of valid and invalid requests
    std::vector<std::thread> clients;
    for (int i = 0; i < 5; i++) {
        clients.emplace_back([i]() {
            std::string json =
                (i % 2 == 0) ? getValidJson(i + 20) : getInvalidJson(i + 20);
            std::string cmd =
                "timeout 2 curl -s -X POST http://localhost:8085/ -H "
                "'Content-Type: application/json' -d '" +
                json + "' > /dev/null 2>&1";
            system(cmd.c_str());
        });
    }

    for (auto& client : clients) {
        client.join();
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    server.stop();
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));

    // Cleanup - only valid requests were inserted
    cleanupTestData("TestGuest");
}

// Test malformed JSON to exercise exception handling in ClientConnection::processRequest
TEST(HttpServer, MalformedJson) {
    std::barrier sync_point(2);
    ConfigManager config(".env");
    PostgresDB db(config);
    HttpServer server(&db, 8089);
    std::thread server_thread([&sync_point, &server]() {
        sync_point.arrive_and_wait();
        server.start();
    });
    server_thread.detach();
    SignalManager sigManager;
    sigManager.setCallback([&server]() { server.stop(); });
    sigManager.setup();

    sync_point.arrive_and_wait();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Send malformed JSON - unterminated string
    std::string malformedJson = "{\"guest_name\": \"TESTING_OWNER";
    std::string cmd =
        "timeout 2 curl -s -X POST http://localhost:8089/ -H 'Content-Type: "
        "application/json' -d '" +
        malformedJson + "' > /dev/null 2>&1";
    system(cmd.c_str());

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    server.stop();
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));
}

// Test missing required fields - triggers parseJson exception
TEST(HttpServer, InvalidJsonMissingFields) {
    std::barrier sync_point(2);
    ConfigManager config(".env");
    PostgresDB db(config);
    HttpServer server(&db, 8090);
    std::thread server_thread([&sync_point, &server]() {
        sync_point.arrive_and_wait();
        server.start();
    });
    server_thread.detach();
    SignalManager sigManager;
    sigManager.setCallback([&server]() { server.stop(); });
    sigManager.setup();

    sync_point.arrive_and_wait();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Send JSON missing required fields - will trigger parseJson exception
    std::string json = getInvalidJson(0);
    std::string cmd =
        "timeout 2 curl -s -X POST http://localhost:8090/ -H 'Content-Type: "
        "application/json' -d '" +
        json + "' > /dev/null 2>&1";
    system(cmd.c_str());

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    server.stop();
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));
}

// Test empty request body - causes parseJson to fail
TEST(HttpServer, EmptyRequestBody) {
    std::barrier sync_point(2);
    ConfigManager config(".env");
    PostgresDB db(config);
    HttpServer server(&db, 8091);
    std::thread server_thread([&sync_point, &server]() {
        sync_point.arrive_and_wait();
        server.start();
    });
    server_thread.detach();
    SignalManager sigManager;
    sigManager.setCallback([&server]() { server.stop(); });
    sigManager.setup();

    sync_point.arrive_and_wait();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Send empty body
    std::string cmd =
        "timeout 2 curl -s -X POST http://localhost:8091/ -H 'Content-Type: "
        "application/json' > /dev/null 2>&1";
    system(cmd.c_str());

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    server.stop();
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));
}

// Test socket closed mid-connection - triggers exception in http::read (line 11)
TEST(HttpServer, SocketClosedMidConnection) {
    std::barrier sync_point(2);
    ConfigManager config(".env");
    PostgresDB db(config);
    HttpServer server(&db, 7777);
    
    std::thread server_thread([&sync_point, &server]() {
        sync_point.arrive_and_wait();
        server.start();
    });
    server_thread.detach();
    
    SignalManager sigManager;
    sigManager.setCallback([&server]() { server.stop(); });
    sigManager.setup();
    
    sync_point.arrive_and_wait();
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    
    // Use Beast to connect and close socket abruptly
    std::thread client_thread([]() {
        try {
            boost::asio::io_context ioc;
            beast::tcp_stream stream(ioc);
            
            // Connect to server
            tcp::endpoint endpoint(
                boost::asio::ip::address::from_string("127.0.0.1"), 7777);
            stream.connect(endpoint);
            
            // Send partial HTTP request (incomplete headers)
            std::string partialRequest = "POST / HTTP/1.1\r\nHost: localhost:7777\r\n";
            stream.write_some(boost::asio::buffer(partialRequest));
            
            // Close socket abruptly - this triggers exception in server's http::read
            stream.socket().close();
        } catch (...) {
            // Expected - socket closed
        }
    });
    
    client_thread.join();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    server.stop();
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));
}

// Test socket closed immediately after connect - triggers exception in http::read
TEST(HttpServer, SocketClosedImmediately) {
    std::barrier sync_point(2);
    ConfigManager config(".env");
    PostgresDB db(config);
    HttpServer server(&db, 7778);
    
    std::thread server_thread([&sync_point, &server]() {
        sync_point.arrive_and_wait();
        server.start();
    });
    server_thread.detach();
    
    SignalManager sigManager;
    sigManager.setCallback([&server]() { server.stop(); });
    sigManager.setup();
    
    sync_point.arrive_and_wait();
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    
    // Connect and close immediately
    std::thread client_thread([]() {
        try {
            boost::asio::io_context ioc;
            beast::tcp_stream stream(ioc);
            
            tcp::endpoint endpoint(
                boost::asio::ip::address::from_string("127.0.0.1"), 7778);
            stream.connect(endpoint);
            
            // Close socket without sending anything
            stream.socket().close();
        } catch (...) {
            // Expected
        }
    });
    
    client_thread.join();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    server.stop();
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));
}

// Test socket closed after partial write - triggers exception in http::write (line 17)
TEST(HttpServer, SocketClosedDuringWrite) {
    std::barrier sync_point(2);
    ConfigManager config(".env");
    PostgresDB db(config);
    HttpServer server(&db, 7779);
    
    std::thread server_thread([&sync_point, &server]() {
        sync_point.arrive_and_wait();
        server.start();
    });
    server_thread.detach();
    
    SignalManager sigManager;
    sigManager.setCallback([&server]() { server.stop(); });
    sigManager.setup();
    
    sync_point.arrive_and_wait();
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    
    // Send valid request but close before receiving response
    std::thread client_thread([]() {
        try {
            boost::asio::io_context ioc;
            beast::tcp_stream stream(ioc);
            
            tcp::endpoint endpoint(
                boost::asio::ip::address::from_string("127.0.0.1"), 7779);
            stream.connect(endpoint);
            
            // Send valid JSON request
            std::string validJson = getValidJson(99);
            std::string httpRequest = 
                "POST / HTTP/1.1\r\n"
                "Host: localhost:7779\r\n"
                "Content-Type: application/json\r\n"
                "Content-Length: " + std::to_string(validJson.length()) + "\r\n"
                "\r\n" + validJson;
            
            stream.write_some(boost::asio::buffer(httpRequest));
            
            // Close socket before reading response
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            stream.socket().close();
        } catch (...) {
            // Expected
        }
    });
    
    client_thread.join();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    server.stop();
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));
}