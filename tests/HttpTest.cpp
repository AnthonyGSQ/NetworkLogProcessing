#include <gtest/gtest.h>
#include <signal.h>
#include <unistd.h>

#include <barrier>
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <chrono>
#include <cstdlib>
#include <sstream>
#include <thread>

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
    oss << "{\"guest_name\":\"" << guestName << "\",";
    oss << "\"guest_email\":\"" << guestEmail << "\",";
    oss << "\"guest_phone\":\"" << guestPhone << "\",";
    oss << "\"room_number\":" << roomNumber << ",";
    oss << "\"room_type\":\"Doble\",";
    oss << "\"number_of_guests\":2,";
    oss << "\"check_in_date\":\"2026-02-15\",";
    oss << "\"check_out_date\":\"2026-02-18\",";
    oss << "\"number_of_nights\":3,";
    oss << "\"price_per_night\":150.0,";
    oss << "\"total_price\":450.0,";
    oss << "\"payment_method\":\"credit_card\",";
    oss << "\"paid\":true,";
    oss << "\"reservation_status\":\"confirmed\",";
    oss << "\"special_requests\":\"Test reservation\",";
    oss << "\"created_at\":" << timestamp << ",";
    oss << "\"updated_at\":" << timestamp << "}";
    return oss.str();
}

// HttpServer infrastructure tests
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
        try {
            server.start();
        } catch (const std::exception& e) {
            std::cerr << "[HttpTest] Server error: " << e.what() << "\n";
        }
    });
    server_thread.detach();

    sync_point.arrive_and_wait();
    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    // Send valid reservation to test basic connectivity
    std::string json = getValidJson(0);
    std::string cmd =
        "timeout 2 curl -s -X POST "
        "http://localhost:8080/application/reservation -H 'Content-Type: "
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
        try {
            server.start();
        } catch (const std::exception& e) {
            std::cerr << "[HttpTest] Server error: " << e.what() << "\n";
        }
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
    server.stop();
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));

    // Cleanup
    cleanupTestData("TestGuest");
}

TEST(HttpServer, PortInUse) {
    std::barrier sync_point(2);
    ConfigManager config(".env");
    PostgresDB db(config);

    // Create and start first server on port 9999
    HttpServer server(&db, 9999);
    std::thread server_thread([&sync_point, &server]() {
        sync_point.arrive_and_wait();
        try {
            server.start();
        } catch (const std::exception& e) {
            std::cerr << "[HttpTest] Server error: " << e.what() << "\n";
        }
    });
    server_thread.detach();

    // Setup signal handling
    SignalManager sigManager;
    sigManager.setCallback([&server]() { server.stop(); });
    sigManager.setup();

    sync_point.arrive_and_wait();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Create second server on same port
    HttpServer server2(&db, 9999);

    // Attempting to start second server should throw because port is in use
    EXPECT_THROW(server2.start(), std::runtime_error);

    server.stop();
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));
}
