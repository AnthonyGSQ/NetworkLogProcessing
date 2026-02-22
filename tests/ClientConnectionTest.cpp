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

// Helper function to clean up test data
static void cleanupClientTestData(const std::string& guestNamePattern) {
    try {
        ConfigManager config(".env");
        std::ostringstream oss;
        oss << "host=" << config.get("DB_HOST")
            << " port=" << config.getInt("DB_PORT")
            << " dbname=" << config.get("DB_NAME")
            << " user=" << config.get("DB_USER")
            << " password=" << config.get("DB_PASSWORD");

        pqxx::connection conn(oss.str());
        pqxx::work txn(conn);
        std::string deleteQuery =
            "DELETE FROM reservations WHERE guest_name LIKE '" +
            guestNamePattern + "%'";
        txn.exec(deleteQuery);
        txn.commit();
    } catch (const std::exception& e) {
        std::cerr << "[ClientConnectionTest] Cleanup failed: " << e.what() << "\n";
    }
}

// Helper to create valid JSON for tests
static std::string createValidJson(const std::string& guestName, int variant = 0) {
    long timestamp = 1707427200 + (variant * 86400);
    std::string guestEmail = guestName + "@example.com";
    std::string guestPhone = "+34612345670";
    int roomNumber = 500 + variant;

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

static std::string createInvalidJson(int variant = 0) {
    std::string guestName = "InvalidGuest" + std::to_string(variant);
    int roomNumber = 200 + variant;

    std::ostringstream oss;
    oss << "{"
        << "\"guest_name\":\"" << guestName << "\","
        << "\"room_number\":" << roomNumber << "}";
    return oss.str();
}

// Test invalid JSON request - ClientConnection should handle gracefully
TEST(ClientConnection, InvalidJsonRequest) {
    std::barrier sync_point(2);
    ConfigManager config(".env");
    PostgresDB db(config);
    HttpServer server(&db, 8082);

    std::thread server_thread([&sync_point, &server]() {
        sync_point.arrive_and_wait();
        try {
            server.start();
        } catch (const std::exception& e) {
            std::cerr << "[ClientConnectionTest] Server error: " << e.what() << "\n";
        }
    });
    server_thread.detach();

    SignalManager sigManager;
    sigManager.setCallback([&server]() { server.stop(); });
    sigManager.setup();

    sync_point.arrive_and_wait();
    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    // Send invalid JSON - malformed
    system(
        "timeout 2 curl -s -X POST http://localhost:8082/application/reservation -H 'Content-Type: "
        "application/json' -d '{invalid json}' > /dev/null 2>&1");
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    server.stop();
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));
}

// Test missing required fields
TEST(ClientConnection, MissingRequiredFields) {
    std::barrier sync_point(2);
    ConfigManager config(".env");
    PostgresDB db(config);
    HttpServer server(&db, 8083);

    std::thread server_thread([&sync_point, &server]() {
        sync_point.arrive_and_wait();
        try {
            server.start();
        } catch (const std::exception& e) {
            std::cerr << "[ClientConnectionTest] Server error: " << e.what() << "\n";
        }
    });
    server_thread.detach();

    SignalManager sigManager;
    sigManager.setCallback([&server]() { server.stop(); });
    sigManager.setup();

    sync_point.arrive_and_wait();
    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    // Send JSON with missing required fields
    std::string json = createInvalidJson(0);
    std::string cmd =
        "timeout 2 curl -s -X POST http://localhost:8083/application/reservation -H 'Content-Type: "
        "application/json' -d '" +
        json + "' > /dev/null 2>&1";
    system(cmd.c_str());

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    server.stop();
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));
}

// Test concurrent valid requests
TEST(ClientConnection, ConcurrentValidRequests) {
    std::barrier sync_point(2);
    ConfigManager config(".env");
    PostgresDB db(config);
    HttpServer server(&db, 8084);

    std::thread server_thread([&sync_point, &server]() {
        sync_point.arrive_and_wait();
        try {
            server.start();
        } catch (const std::exception& e) {
            std::cerr << "[ClientConnectionTest] Server error: " << e.what() << "\n";
        }
    });
    server_thread.detach();

    SignalManager sigManager;
    sigManager.setCallback([&server]() { server.stop(); });
    sigManager.setup();

    sync_point.arrive_and_wait();
    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    // Send 5 concurrent valid requests
    std::vector<std::thread> clients;
    for (int i = 0; i < 5; i++) {
        clients.emplace_back([i]() {
            std::string json = createValidJson("ConcurrentValidGuest" + std::to_string(i), i + 10);
            std::string cmd =
                "timeout 2 curl -s -X POST http://localhost:8084/application/reservation -H "
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

    cleanupClientTestData("ConcurrentValidGuest");
}

// Test concurrent mixed valid/invalid requests
TEST(ClientConnection, ConcurrentMixedRequests) {
    std::barrier sync_point(2);
    ConfigManager config(".env");
    PostgresDB db(config);
    HttpServer server(&db, 8085);

    std::thread server_thread([&sync_point, &server]() {
        sync_point.arrive_and_wait();
        try {
            server.start();
        } catch (const std::exception& e) {
            std::cerr << "[ClientConnectionTest] Server error: " << e.what() << "\n";
        }
    });
    server_thread.detach();

    SignalManager sigManager;
    sigManager.setCallback([&server]() { server.stop(); });
    sigManager.setup();

    sync_point.arrive_and_wait();
    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    // Send mix of valid and invalid requests
    std::vector<std::thread> clients;
    for (int i = 0; i < 5; i++) {
        clients.emplace_back([i]() {
            try {
                std::string json = createValidJson("MixedGuest" + std::to_string(i));
                std::string cmd =
                    "timeout 2 curl -s -X POST http://localhost:8085/application/reservation -H "
                    "'Content-Type: application/json' -d '" +
                    json + "' > /dev/null 2>&1";
                system(cmd.c_str());
            } catch (const std::exception& e) {
                std::cerr << "[ClientConnectionTest] Client error: " << e.what() << "\n";
            }
        });
    }

    for (auto& client : clients) {
        client.join();
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    server.stop();
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));

    cleanupClientTestData("MixedGuest");
}

// Test malformed JSON
TEST(ClientConnection, MalformedJson) {
    std::barrier sync_point(2);
    ConfigManager config(".env");
    PostgresDB db(config);
    HttpServer server(&db, 8089);
    
    std::thread server_thread([&sync_point, &server]() {
        sync_point.arrive_and_wait();
        try {
            server.start();
        } catch (const std::exception& e) {
            std::cerr << "[ClientConnectionTest] Server error: " << e.what() << "\n";
        }
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
        "timeout 2 curl -s -X POST http://localhost:8089/application/reservation -H 'Content-Type: "
        "application/json' -d '" +
        malformedJson + "' > /dev/null 2>&1";
    system(cmd.c_str());

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    server.stop();
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));
}

// Test missing fields in JSON
TEST(ClientConnection, InvalidJsonMissingFields) {
    std::barrier sync_point(2);
    ConfigManager config(".env");
    PostgresDB db(config);
    HttpServer server(&db, 8090);
    
    std::thread server_thread([&sync_point, &server]() {
        sync_point.arrive_and_wait();
        try {
            server.start();
        } catch (const std::exception& e) {
            std::cerr << "[ClientConnectionTest] Server error: " << e.what() << "\n";
        }
    });
    server_thread.detach();
    
    SignalManager sigManager;
    sigManager.setCallback([&server]() { server.stop(); });
    sigManager.setup();

    sync_point.arrive_and_wait();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Send JSON missing required fields
    std::string json = createInvalidJson(0);
    std::string cmd =
        "timeout 2 curl -s -X POST http://localhost:8090/application/reservation -H 'Content-Type: "
        "application/json' -d '" +
        json + "' > /dev/null 2>&1";
    system(cmd.c_str());

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    server.stop();
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));
}

// Test empty request body
TEST(ClientConnection, EmptyRequestBody) {
    std::barrier sync_point(2);
    ConfigManager config(".env");
    PostgresDB db(config);
    HttpServer server(&db, 8091);
    
    std::thread server_thread([&sync_point, &server]() {
        sync_point.arrive_and_wait();
        try {
            server.start();
        } catch (const std::exception& e) {
            std::cerr << "[ClientConnectionTest] Server error: " << e.what() << "\n";
        }
    });
    server_thread.detach();
    
    SignalManager sigManager;
    sigManager.setCallback([&server]() { server.stop(); });
    sigManager.setup();

    sync_point.arrive_and_wait();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Send empty body
    std::string cmd =
        "timeout 2 curl -s -X POST http://localhost:8091/application/reservation -H 'Content-Type: "
        "application/json' > /dev/null 2>&1";
    system(cmd.c_str());

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    server.stop();
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));
}

// Test socket closed mid-connection
TEST(ClientConnection, SocketClosedMidConnection) {
    std::barrier sync_point(2);
    ConfigManager config(".env");
    PostgresDB db(config);
    HttpServer server(&db, 7777);

    std::thread server_thread([&sync_point, &server]() {
        sync_point.arrive_and_wait();
        try {
            server.start();
        } catch (const std::exception& e) {
            std::cerr << "[ClientConnectionTest] Server error: " << e.what() << "\n";
        }
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
            std::string partialRequest =
                "POST / HTTP/1.1\r\nHost: localhost:7777\r\n";
            stream.write_some(boost::asio::buffer(partialRequest));

            // Close socket abruptly
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

// Test socket closed immediately
TEST(ClientConnection, SocketClosedImmediately) {
    std::barrier sync_point(2);
    ConfigManager config(".env");
    PostgresDB db(config);
    HttpServer server(&db, 7778);

    std::thread server_thread([&sync_point, &server]() {
        sync_point.arrive_and_wait();
        try {
            server.start();
        } catch (const std::exception& e) {
            std::cerr << "[ClientConnectionTest] Server error: " << e.what() << "\n";
        }
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

// Test socket closed during write
TEST(ClientConnection, SocketClosedDuringWrite) {
    std::barrier sync_point(2);
    ConfigManager config(".env");
    PostgresDB db(config);
    HttpServer server(&db, 7779);

    std::thread server_thread([&sync_point, &server]() {
        sync_point.arrive_and_wait();
        try {
            server.start();
        } catch (const std::exception& e) {
            std::cerr << "[ClientConnectionTest] Server error: " << e.what() << "\n";
        }
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
            std::string validJson = createValidJson("SocketCloseGuest", 99);
            std::string httpRequest =
                "POST / HTTP/1.1\r\n"
                "Host: localhost:7779\r\n"
                "Content-Type: application/json\r\n"
                "Content-Length: " +
                std::to_string(validJson.length()) +
                "\r\n"
                "\r\n" +
                validJson;

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

// Test POST, GET, PUT, DELETE methods
TEST(ClientConnection, PostReservation) {
    std::barrier sync_point(2);
    ConfigManager config(".env");
    PostgresDB db(config);
    HttpServer server(&db, 8800);

    std::thread server_thread([&sync_point, &server]() {
        sync_point.arrive_and_wait();
        try {
            server.start();
        } catch (const std::exception& e) {
            std::cerr << "[ClientConnectionTest] Server error: " << e.what() << "\n";
        }
    });
    server_thread.detach();

    SignalManager sigManager;
    sigManager.setCallback([&server]() { server.stop(); });
    sigManager.setup();

    sync_point.arrive_and_wait();
    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    // Send POST request to create reservation
    std::string json = createValidJson("PostTestGuest", 0);
    std::string cmd =
        "curl -s -X POST http://localhost:8800/application/reservation "
        "-H 'Content-Type: application/json' -d '" + json + "'";
    system(cmd.c_str());
    
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    server.stop();
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));

    cleanupClientTestData("PostTestGuest");
}

// Test GET request - retrieve reservation by ID
TEST(ClientConnection, GetReservation) {
    std::barrier sync_point(2);
    ConfigManager config(".env");
    PostgresDB db(config);
    HttpServer server(&db, 8801);

    std::thread server_thread([&sync_point, &server]() {
        sync_point.arrive_and_wait();
        try {
            server.start();
        } catch (const std::exception& e) {
            std::cerr << "[ClientConnectionTest] Server error: " << e.what() << "\n";
        }
    });
    server_thread.detach();

    SignalManager sigManager;
    sigManager.setCallback([&server]() { server.stop(); });
    sigManager.setup();

    sync_point.arrive_and_wait();
    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    // Create a reservation first
    std::string json = createValidJson("GetTestGuest", 0);
    std::string postCmd =
        "curl -s -X POST http://localhost:8801/application/reservation "
        "-H 'Content-Type: application/json' -d '" + json + "' > /dev/null 2>&1";
    system(postCmd.c_str());
    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    // Then GET it
    std::string getCmd =
        "curl -s -X GET http://localhost:8801/application/reservation/1 "
        "-H 'Content-Type: application/json'";
    system(getCmd.c_str());

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    server.stop();
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));

    cleanupClientTestData("GetTestGuest");
}

// Test PUT request - update reservation
TEST(ClientConnection, PutReservation) {
    std::barrier sync_point(2);
    ConfigManager config(".env");
    PostgresDB db(config);
    HttpServer server(&db, 8802);

    std::thread server_thread([&sync_point, &server]() {
        sync_point.arrive_and_wait();
        try {
            server.start();
        } catch (const std::exception& e) {
            std::cerr << "[ClientConnectionTest] Server error: " << e.what() << "\n";
        }
    });
    server_thread.detach();

    SignalManager sigManager;
    sigManager.setCallback([&server]() { server.stop(); });
    sigManager.setup();

    sync_point.arrive_and_wait();
    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    // Create initial reservation
    std::string json = createValidJson("PutTestGuest", 0);
    std::string postCmd =
        "curl -s -X POST http://localhost:8802/application/reservation "
        "-H 'Content-Type: application/json' -d '" + json + "' > /dev/null 2>&1";
    system(postCmd.c_str());
    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    // Update with PUT
    std::string updatedJson = createValidJson("PutTestGuestUpdated", 0);
    std::string putCmd =
        "curl -s -X PUT http://localhost:8802/application/reservation/1 "
        "-H 'Content-Type: application/json' -d '" + updatedJson + "'";
    system(putCmd.c_str());

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    server.stop();
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));

    cleanupClientTestData("PutTestGuest");
    cleanupClientTestData("PutTestGuestUpdated");
}

// Test DELETE request - remove reservation
TEST(ClientConnection, DeleteReservation) {
    std::barrier sync_point(2);
    ConfigManager config(".env");
    PostgresDB db(config);
    HttpServer server(&db, 8803);

    std::thread server_thread([&sync_point, &server]() {
        sync_point.arrive_and_wait();
        try {
            server.start();
        } catch (const std::exception& e) {
            std::cerr << "[ClientConnectionTest] Server error: " << e.what() << "\n";
        }
    });
    server_thread.detach();

    SignalManager sigManager;
    sigManager.setCallback([&server]() { server.stop(); });
    sigManager.setup();

    sync_point.arrive_and_wait();
    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    // Create a reservation first
    std::string json = createValidJson("DeleteTestGuest", 0);
    std::string postCmd =
        "curl -s -X POST http://localhost:8803/application/reservation "
        "-H 'Content-Type: application/json' -d '" + json + "' > /dev/null 2>&1";
    system(postCmd.c_str());
    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    // Delete it
    std::string deleteCmd =
        "curl -s -X DELETE http://localhost:8803/application/reservation/1 "
        "-H 'Content-Type: application/json'";
    system(deleteCmd.c_str());

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    server.stop();
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));

    cleanupClientTestData("DeleteTestGuest");
}

// Test invalid endpoint - should trigger "Endpoint not found" error
TEST(ClientConnection, InvalidEndpoint) {
    std::barrier sync_point(2);
    ConfigManager config(".env");
    PostgresDB db(config);
    HttpServer server(&db, 8806);

    std::thread server_thread([&sync_point, &server]() {
        sync_point.arrive_and_wait();
        try {
            server.start();
        } catch (const std::exception& e) {
            std::cerr << "[ClientConnectionTest] Server error: " << e.what() << "\n";
        }
    });
    server_thread.detach();

    SignalManager sigManager;
    sigManager.setCallback([&server]() { server.stop(); });
    sigManager.setup();

    sync_point.arrive_and_wait();
    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    // Send request to invalid endpoint (PATCH to /invalid/path)
    std::string json = createValidJson("InvalidEndpointGuest", 0);
    std::string cmd =
        "curl -s -X PATCH http://localhost:8806/invalid/endpoint "
        "-H 'Content-Type: application/json' -d '" + json + "'";
    system(cmd.c_str());

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    server.stop();
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));
}
