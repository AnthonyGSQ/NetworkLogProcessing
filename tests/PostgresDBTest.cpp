#include <gtest/gtest.h>

#include <sstream>
#include <thread>
#include <vector>

#include "../src/DataBase/PostgresDB.hpp"
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

    int id = db.insertReservation(res);
    EXPECT_NE(id, -1) << "Reservation should be inserted with a valid ID";

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
        int id = db.insertReservation(res);
        EXPECT_NE(id, -1) << "Room " << (102 + i)
                          << " should be inserted successfully";
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
    int id1 = db.insertReservation(res1);
    EXPECT_NE(id1, -1) << "First insertion should succeed";

    // Try to insert same room, same dates (should FAIL)
    Reservation res2 = createBaseReservation();
    res2.room_number = 105;           // Same room
    res2.guest_name = "TESTING_OWNER";  // Different guest, pero mismo room+dates
    int id2 = db.insertReservation(res2);
    EXPECT_EQ(id2, -1) << "Overlapping reservations should be rejected";

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
    int id1 = db.insertReservation(res1);
    EXPECT_NE(id1, -1) << "First reservation should succeed";

    // Second: Room 106, Feb 20-23 (same room, different dates - should SUCCEED)
    Reservation res2 = createBaseReservation();
    res2.room_number = 106;
    res2.check_in_date = "2026-02-20";
    res2.check_out_date = "2026-02-23";
    int id2 = db.insertReservation(res2);
    EXPECT_NE(id2, -1)
        << "Second reservation with different dates should succeed";

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
    int id1 = db.insertReservation(res1);
    EXPECT_NE(id1, -1) << "First reservation should succeed";

    // Try Room 107, Feb 18-20 (checkout=checkin boundary, overlap) - SHOULD
    // FAIL
    Reservation res2 = createBaseReservation();
    res2.room_number = 107;
    res2.check_in_date = "2026-02-18";
    res2.check_out_date = "2026-02-20";
    int id2 = db.insertReservation(res2);
    EXPECT_EQ(id2, -1) << "Boundary overlap should be rejected";

    cleanupTestData();
}
// Test retrieving a reservation by ID - Non-existent ID should throw
TEST(PostgresDB, GetReservationByIdNotFound) {
    cleanupTestData();
    ConfigManager config(".env");
    PostgresDB db(config);

    // Try to get non-existent ID - should throw std::runtime_error
    EXPECT_THROW(
        { Reservation res = db.getReservationById(999999); },
        std::runtime_error);

    cleanupTestData();
}

// Test retrieving a reservation by ID - Existing ID should return correct data
TEST(PostgresDB, GetReservationByIdExists) {
    cleanupTestData();
    ConfigManager config(".env");
    PostgresDB db(config);

    // Insert a reservation with unique room to ensure predictable data
    Reservation original = createBaseReservation();
    original.room_number = 150;
    original.guest_name = "TESTING_OWNER";
    original.guest_email = "getbyid@test.com";
    original.room_type = "Suite";
    original.number_of_guests = 4;
    int insertedId = db.insertReservation(original);
    ASSERT_NE(insertedId, -1) << "Reservation should be inserted successfully";

    // Now retrieve the reservation by ID
    Reservation retrieved = db.getReservationById(insertedId);

    // Verify all fields match
    EXPECT_EQ(retrieved.guest_name, original.guest_name);
    EXPECT_EQ(retrieved.guest_email, original.guest_email);
    EXPECT_EQ(retrieved.guest_phone, original.guest_phone);
    EXPECT_EQ(retrieved.room_number, original.room_number);
    EXPECT_EQ(retrieved.room_type, original.room_type);
    EXPECT_EQ(retrieved.number_of_guests, original.number_of_guests);
    EXPECT_EQ(retrieved.check_in_date, original.check_in_date);
    EXPECT_EQ(retrieved.check_out_date, original.check_out_date);
    EXPECT_EQ(retrieved.number_of_nights, original.number_of_nights);
    EXPECT_EQ(retrieved.price_per_night, original.price_per_night);
    EXPECT_EQ(retrieved.total_price, original.total_price);
    EXPECT_EQ(retrieved.payment_method, original.payment_method);
    EXPECT_EQ(retrieved.paid, original.paid);
    EXPECT_EQ(retrieved.reservation_status, original.reservation_status);
    EXPECT_EQ(retrieved.special_requests, original.special_requests);
    EXPECT_EQ(retrieved.created_at, original.created_at);
    EXPECT_EQ(retrieved.updated_at, original.updated_at);

    cleanupTestData();
}

// Test updating a reservation - Non-existent ID should fail
TEST(PostgresDB, UpdateReservationNotFound) {
    ConfigManager config(".env");
    PostgresDB db(config);

    Reservation res = createBaseReservation();
    res.room_number = 110;

    // Try to update non-existent ID - should return false
    EXPECT_FALSE(db.updateReservation(999999, res));
}

// Test updating a reservation - Existing ID should succeed
TEST(PostgresDB, UpdateReservationExists) {
    cleanupTestData();
    ConfigManager config(".env");
    PostgresDB db(config);

    // Insert initial reservation
    Reservation original = createBaseReservation();
    original.room_number = 160;
    int id = db.insertReservation(original);
    ASSERT_NE(id, -1) << "Initial insertion should succeed";

    // Update the reservation
    Reservation updated = createBaseReservation();
    updated.room_number = 160;
    updated.guest_name = "UPDATED_GUEST";
    updated.guest_email = "updated@example.com";
    updated.price_per_night = 200.0;
    updated.total_price = 1000.0;

    EXPECT_TRUE(db.updateReservation(id, updated))
        << "Update should succeed for existing ID";

    // Verify the update by retrieving it
    Reservation retrieved = db.getReservationById(id);
    EXPECT_EQ(retrieved.guest_name, "UPDATED_GUEST");
    EXPECT_EQ(retrieved.guest_email, "updated@example.com");
    EXPECT_EQ(retrieved.price_per_night, 200.0);

    cleanupTestData();
}

// Test deleting a reservation - Non-existent ID should fail
TEST(PostgresDB, DeleteReservationNotFound) {
    ConfigManager config(".env");
    PostgresDB db(config);

    // Try to delete non-existent ID - should return false
    EXPECT_FALSE(db.deleteReservation(999999));
}

// Test deleting a reservation - Existing ID should succeed
TEST(PostgresDB, DeleteReservationExists) {
    cleanupTestData();
    ConfigManager config(".env");
    PostgresDB db(config);

    // Insert a reservation
    Reservation res = createBaseReservation();
    res.room_number = 170;
    int id = db.insertReservation(res);
    ASSERT_NE(id, -1) << "Insertion should succeed";

    // Verify it exists by retrieving it
    Reservation retrieved = db.getReservationById(id);
    EXPECT_EQ(retrieved.room_number, 170);

    // Delete it
    EXPECT_TRUE(db.deleteReservation(id))
        << "Deletion should succeed for existing ID";

    // Verify it's deleted by trying to retrieve it
    EXPECT_THROW(
        { Reservation deleted = db.getReservationById(id); },
        std::runtime_error);

    cleanupTestData();
}

// Test concurrent inserts
TEST(PostgresDB, ConcurrentInserts) {
    cleanupTestData();
    ConfigManager config(".env");
    PostgresDB db(config);

    std::vector<std::thread> threads;
    std::vector<int> results(5);

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

    // All 5 inserts should succeed (different rooms) - IDs should not be -1
    for (int i = 0; i < 5; i++) {
        EXPECT_NE(results[i], -1) << "Concurrent insert " << i << " failed";
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

        int id = db.insertReservation(res);
        EXPECT_NE(id, -1) << "Insertion should succeed";
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
