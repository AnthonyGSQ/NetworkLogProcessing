#include <gtest/gtest.h>

#include <sstream>
#include <thread>
#include <vector>

#include "../src/PostgresDB.hpp"
#include "../src/config/ConfigManager.hpp"

// Función que retorna una Reservation base con valores por defecto
static Reservation createBaseReservation() {
    Reservation res;
    res.guest_name = "TESTING_OWNER";
    res.guest_email = "juan@example.com";
    res.guest_phone = "+34612345678";
    res.room_number = 100;  // Será modificado en cada test
    res.room_type = "Doble";
    res.number_of_guests = 2;
    res.check_in_date = "2026-02-15";   // Hardcodeado
    res.check_out_date = "2026-02-18";  // Hardcodeado
    res.number_of_nights = 3;
    res.price_per_night = 150.0;
    res.total_price = 450.0;
    res.payment_method = "credit_card";
    res.paid = true;
    res.reservation_status = "confirmed";
    res.special_requests = "Test";
    res.created_at = 1707427200;
    res.updated_at = 1707427200;
    return res;
}

// Helper to cleanup test data by room number range
static void cleanupTestData() {
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
        txn.exec("DELETE FROM reservations WHERE guest_name='TESTING_OWNER'");
        txn.commit();
    } catch (const std::exception& e) {
        std::cerr << "Cleanup failed: " << e.what() << "\n";
    }
}

// Test that database connection is established
TEST(PostgresDB, ConnectionEstablished) {
    ConfigManager config(".env");
    PostgresDB db(config);

    EXPECT_TRUE(db.isConnected());
}

// Test inserting a valid reservation
TEST(PostgresDB, InsertValidReservation) {
    cleanupTestData();
    ConfigManager config(".env");
    PostgresDB db(config);

    Reservation res = createBaseReservation();
    res.room_number = 101;  // Único identificador: room 101

    EXPECT_TRUE(db.insertReservation(res));

    cleanupTestData();
}

// Test inserting multiple reservations with different rooms
TEST(PostgresDB, InsertMultipleReservations) {
    cleanupTestData();
    ConfigManager config(".env");
    PostgresDB db(config);

    for (int i = 0; i < 3; i++) {
        Reservation res = createBaseReservation();
        res.room_number = 102 + i;  // Rooms 102, 103, 104 - sin overlap
        EXPECT_TRUE(db.insertReservation(res));
    }

    cleanupTestData();
}
// Test that overlapping reservations are REJECTED by constraint
TEST(PostgresDB, OverlappingReservationsRejected) {
    cleanupTestData();
    ConfigManager config(".env");
    PostgresDB db(config);

    // First insertion
    Reservation res1 = createBaseReservation();
    res1.room_number = 105;
    EXPECT_TRUE(db.insertReservation(res1));

    // Try to insert same room, same dates (should FAIL)
    Reservation res2 = createBaseReservation();
    res2.room_number = 105;           // Same room
    res2.guest_name = "María López";  // Different guest, pero mismo room+dates
    EXPECT_FALSE(db.insertReservation(res2))
        << "Overlapping reservations should be rejected";

    cleanupTestData();
}

// Test same room with different non-overlapping dates - should SUCCEED
TEST(PostgresDB, SameRoomDifferentDates) {
    cleanupTestData();
    ConfigManager config(".env");
    PostgresDB db(config);

    // First: Room 106, Feb 15-18
    Reservation res1 = createBaseReservation();
    res1.room_number = 106;
    EXPECT_TRUE(db.insertReservation(res1));

    // Second: Room 106, Feb 20-23 (same room, different dates - should SUCCEED)
    Reservation res2 = createBaseReservation();
    res2.room_number = 106;
    res2.check_in_date = "2026-02-20";
    res2.check_out_date = "2026-02-23";
    EXPECT_TRUE(db.insertReservation(res2));

    cleanupTestData();
}

// Test boundary overlap (checkout same day as next checkin) - should FAIL
TEST(PostgresDB, BoundaryOverlapRejected) {
    cleanupTestData();
    ConfigManager config(".env");
    PostgresDB db(config);

    // First: Room 107, Feb 15-18
    Reservation res1 = createBaseReservation();
    res1.room_number = 107;
    EXPECT_TRUE(db.insertReservation(res1));

    // Try Room 107, Feb 18-20 (checkout=checkin boundary, overlap) - SHOULD
    // FAIL
    Reservation res2 = createBaseReservation();
    res2.room_number = 107;
    res2.check_in_date = "2026-02-18";
    res2.check_out_date = "2026-02-20";
    EXPECT_FALSE(db.insertReservation(res2))
        << "Boundary overlap should be rejected";

    cleanupTestData();
}
// Test retrieving a reservation by ID
TEST(PostgresDB, GetReservationById) {
    ConfigManager config(".env");
    PostgresDB db(config);

    // Try to get non-existent ID - should throw
    try {
        Reservation res = db.getReservationById(999999);
        FAIL() << "Should throw exception for non-existent ID";
    } catch (const std::runtime_error& e) {
        EXPECT_NE(std::string(e.what()).find("not found"), std::string::npos);
    }
    Reservation res = createBaseReservation();
    res.room_number = 108;
    res.check_in_date = "2026-02-18";
    res.check_out_date = "2026-02-20";
    db.insertReservation(res);
    db.getReservationById();
}

// Test updating a reservation
TEST(PostgresDB, UpdateReservation) {
    ConfigManager config(".env");
    PostgresDB db(config);

    Reservation res = createBaseReservation();
    res.room_number = 110;

    // Try to update non-existent ID - should return false
    EXPECT_FALSE(db.updateReservation(999999, res));
}

// Test deleting a reservation
TEST(PostgresDB, DeleteReservation) {
    ConfigManager config(".env");
    PostgresDB db(config);

    // Try to delete non-existent ID - should return false
    EXPECT_FALSE(db.deleteReservation(999999));
}

// Test concurrent inserts
TEST(PostgresDB, ConcurrentInserts) {
    cleanupTestData();
    ConfigManager config(".env");
    PostgresDB db(config);

    std::vector<std::thread> threads;
    std::vector<bool> results(5);

    for (int i = 0; i < 5; i++) {
        threads.emplace_back([&results, i]() {
            ConfigManager cfg(".env");
            PostgresDB database(cfg);

            Reservation res = createBaseReservation();
            res.room_number = 120 + i;  // 120, 121, 122, 123, 124

            results[i] = database.insertReservation(res);
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    // All 5 inserts should succeed (different rooms)
    for (int i = 0; i < 5; i++) {
        EXPECT_TRUE(results[i]) << "Concurrent insert " << i << " failed";
    }

    cleanupTestData();
}

// Test data persistence
TEST(PostgresDB, DataPersistence) {
    cleanupTestData();

    // Insert with first connection
    {
        ConfigManager config(".env");
        PostgresDB db(config);

        Reservation res = createBaseReservation();
        res.room_number = 130;

        EXPECT_TRUE(db.insertReservation(res));
    }  // Connection closes

    // Open new connection and verify it's still connected
    {
        ConfigManager config(".env");
        PostgresDB db(config);
        EXPECT_TRUE(db.isConnected());
    }

    cleanupTestData();
}

// Test invalid environment information
TEST(PostgresDB, InvalidEnvInformation) {
    ConfigManager config(".env.example");
    EXPECT_THROW(PostgresDB db(config), std::runtime_error);
}
