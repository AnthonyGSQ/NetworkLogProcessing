#include "PostgresDB.hpp"

#include <iostream>
#include <sstream>

#include "config/ConfigManager.hpp"

PostgresDB::PostgresDB(const ConfigManager& config)
    : conn(buildConnectionString(config)), 
      pool(buildConnectionString(config), 4) {
    /*
     * RAII in action:
     * - Constructor parameter: ConfigManager with validated credentials
     * - Member initializer 1: conn(buildConnectionString(config))
     *   This OPENS the primary connection immediately
     * - Member initializer 2: pool(buildConnectionString(config), 4)
     *   This OPENS 4 additional connections in the ConnectionPool
     *   Workers will acquire/release these connections via getConnectionPool()
     * - If connection fails, exception thrown before object is fully constructed
     * - Caller sees the exception, knows something is wrong
     */
    
    if (!conn.is_open()) {
        throw std::runtime_error("[PostgresDB] Failed to connect to database");
    }

    std::cout << "[PostgresDB] Connected successfully" << std::endl;
    std::cout << "[PostgresDB] Connection pool initialized with 4 connections" << std::endl;
}

PostgresDB::~PostgresDB() {
    /*
     * RAII cleanup:
     * - pqxx::connection destructor automatically closes the connection
     * - When PostgresDB object destroyed, connection is guaranteed closed
     * - No manual conn.close() needed, no risk of forgetting
     */

    if (conn.is_open()) {
        std::cout << "[PostgresDB] Closing connection" << std::endl;
    }
    // conn destructor runs here, closes connection
}

ConnectionPool* PostgresDB::getConnectionPool() const {
    /*
     * Provides access to the connection pool for worker threads
     * 
     * Usage in ClientConnection:
     *   auto conn = db->getConnectionPool()->acquire();
     *   // ... use connection ...
     *   db->getConnectionPool()->release(std::move(conn));
     * 
     * Why const_cast needed on pool?
     * - pool is mutable member (marked with mutable keyword)
     * - Allows returning non-const pointer from const method
     * - Necessary because acquire/release modify internal state
     */
    return const_cast<ConnectionPool*>(&pool);
}

std::string PostgresDB::buildConnectionString(const ConfigManager& config) {
    /*
     * Purpose: Take validated config and format it for PostgreSQL
     *
     * Input (from .env):
     *   DB_HOST=127.0.0.1
     *   DB_PORT=5432
     *   DB_NAME=hotel_reservations
     *   DB_USER=username
     *   DB_PASSWORD=mysecretpassword
     *
     * Output:
     *   "host=127.0.0.1 port=5432 dbname=hotel_reservations user=username
     * password=mysecretpassword"
     *
     * This is the format libpqxx expects in the connection string.
     */

    std::ostringstream oss;

    // Build the connection string piece by piece
    oss << "host=" << config.get("DB_HOST")
        << " port=" << config.getInt("DB_PORT")
        << " dbname=" << config.get("DB_NAME")
        << " user=" << config.get("DB_USER")
        << " password=" << config.get("DB_PASSWORD");
    
    return oss.str();
}

bool PostgresDB::isConnected() const { return conn.is_open(); }

int PostgresDB::insertReservation(const Reservation& res) {
    /*
     * Big picture: Take a Reservation C++ object, insert it into PostgreSQL
     *
     * Steps:
     * 1. Verify connection still open
     * 2. Start transaction (all-or-nothing)
     * 3. Execute INSERT SQL with parameters
     * 4. Commit transaction
     * 5. Return the assigned ID (or -1 on failure)
     */

    try {
        // Step 1: Sanity check
        if (!conn.is_open()) {
            std::cerr << "[PostgresDB] ERROR: Connection lost" << std::endl;
            return -1;
        }

        /*
         * Step 2: Create a transaction
         *
         * What is pqxx::work?
         * - A transaction object
         * - Everything you do with it (execute, insert, etc) is part of ONE
         * transaction
         * - If you commit() it, everything is saved
         * - If you throw an exception before commit(), EVERYTHING is rolled
         * back
         * - This is all-or-nothing safety
         *
         */
        pqxx::work txn(conn);

        /*
         * Step 3: Execute INSERT with prepared statement
         *
         * SQL prepared statement: INSERT with PLACEHOLDERS
         * Placeholders: $1, $2, $3, ... $N
         *
         * Why placeholders instead of concatenating strings?
         *
         * DANGEROUS (vulnerable to SQL injection):
         *   std::string query = "INSERT INTO reservations (guest_name) VALUES
         * ('"
         *                       + res.guest_name + "')";
         *
         * SAFE (protected by prepared statement):
         *   std::string query = "INSERT INTO reservations (guest_name) VALUES
         * ($1)"; txn.exec_params(query, res.guest_name);
         *
         *   libpqxx sends res.guest_name as DATA, not as SQL code.
         *   Server knows: "this is a value, not code"
         *   No injection possible.
         */

        // The INSERT statement with $1, $2, etc. placeholders
        // RETURNING id allows us to get the assigned ID
        std::string insertQuery = R"(
            INSERT INTO reservations (
                guest_name, guest_email, guest_phone,
                room_number, room_type, number_of_guests,
                check_in_date, check_out_date, number_of_nights,
                price_per_night, total_price, payment_method, paid,
                reservation_status, special_requests,
                created_at, updated_at
            ) VALUES (
                $1, $2, $3,
                $4, $5, $6,
                $7, $8, $9,
                $10, $11, $12, $13,
                $14, $15,
                $16, $17
            )
            RETURNING id
        )";

        /*
         * R"(...)" is a raw string literal
         * Useful here because we have lots of quotes and newlines
         * Without it, would need escaping: "INSERT ... WHERE name =
         * \\"value\\"" With raw string: no escaping needed
         */

        // Execute with parameters using new pqxx API
        pqxx::params p;
        p.append(res.guest_name);
        p.append(res.guest_email);
        p.append(res.guest_phone);
        p.append(res.room_number);
        p.append(res.room_type);
        p.append(res.number_of_guests);
        p.append(res.check_in_date);
        p.append(res.check_out_date);
        p.append(res.number_of_nights);
        p.append(res.price_per_night);
        p.append(res.total_price);
        p.append(res.payment_method);
        p.append(res.paid);
        p.append(res.reservation_status);
        p.append(res.special_requests);
        p.append(res.created_at);
        p.append(res.updated_at);
        auto result = txn.exec(insertQuery, p);

        // Step 4: Commit the transaction
        txn.commit();

        /*
         * - If commit succeeds, data is in database forever
         * - If commit fails, all changes are rolled back automatically
         */

        // Extract the returned ID
        int assignedId = result[0][0].as<int>();
        std::cout << "[PostgresDB] Reservation inserted successfully for: "
                  << res.guest_name << " with ID: " << assignedId << std::endl;

        return assignedId;

    } catch (const std::exception& e) {
        /*
         * If ANY step above throws:
         * - Transaction is automatically rolled back (no partial inserts)
         * - Exception caught here
         * - Return -1 to caller
         *
         * This is the safety of RAII + transactions
         */
        std::cerr << "[PostgresDB] Error inserting reservation: " << e.what()
                  << std::endl;
        return -1;
    }
}

int PostgresDB::insertReservation(pqxx::connection& conn, const Reservation& res) {
    /*
     * Overloaded version for pool connections
     * 
     * Same logic as insertReservation(const Reservation& res)
     * but uses the provided connection instead of this->conn
     * 
     * This allows multiple threads to execute concurrently:
     * - Thread 1 acquires conn1, inserts data
     * - Thread 2 acquires conn2, inserts data simultaneously
     * - No blocking: each thread has its own connection
     */

    try {
        if (!conn.is_open()) {
            std::cerr << "[PostgresDB] ERROR: Connection lost" << std::endl;
            return -1;
        }

        pqxx::work txn(conn);

        std::string insertQuery = R"(
            INSERT INTO reservations (
                guest_name, guest_email, guest_phone,
                room_number, room_type, number_of_guests,
                check_in_date, check_out_date, number_of_nights,
                price_per_night, total_price, payment_method, paid,
                reservation_status, special_requests,
                created_at, updated_at
            ) VALUES (
                $1, $2, $3,
                $4, $5, $6,
                $7, $8, $9,
                $10, $11, $12, $13,
                $14, $15,
                $16, $17
            )
            RETURNING id
        )";

        pqxx::params p;
        p.append(res.guest_name);
        p.append(res.guest_email);
        p.append(res.guest_phone);
        p.append(res.room_number);
        p.append(res.room_type);
        p.append(res.number_of_guests);
        p.append(res.check_in_date);
        p.append(res.check_out_date);
        p.append(res.number_of_nights);
        p.append(res.price_per_night);
        p.append(res.total_price);
        p.append(res.payment_method);
        p.append(res.paid);
        p.append(res.reservation_status);
        p.append(res.special_requests);
        p.append(res.created_at);
        p.append(res.updated_at);
        auto result = txn.exec(insertQuery, p);

        txn.commit();

        int assignedId = result[0][0].as<int>();
        std::cout << "[PostgresDB] [Pool] Reservation inserted successfully for: "
                  << res.guest_name << " with ID: " << assignedId << std::endl;

        return assignedId;

    } catch (const std::exception& e) {
        std::cerr << "[PostgresDB] [Pool] Error inserting reservation: " << e.what()
                  << std::endl;
        return -1;
    }
}

Reservation PostgresDB::getReservationById(int id) {
    /*
     * Purpose: SELECT a reservation from database, return as C++ object
     *
     * Steps:
     * 1. Execute SELECT query with ID as parameter
     * 2. Check if result has rows
     * 3. Extract row data into Reservation struct
     * 4. Return struct (or throw if not found)
     */

    try {
        if (!conn.is_open()) {
            throw std::runtime_error("Connection lost");
        }

        // For SELECT, use pqxx::work (same as INSERT)
        pqxx::work txn(conn);

        // SELECT query with $1 placeholder for ID
        std::string selectQuery = R"(
            SELECT guest_name, guest_email, guest_phone,
                   room_number, room_type, number_of_guests,
                   check_in_date, check_out_date, number_of_nights,
                   price_per_night, total_price, payment_method, paid,
                   reservation_status, special_requests,
                   created_at, updated_at
            FROM reservations
            WHERE id = $1
        )";

        // Execute with ID parameter using new pqxx API
        pqxx::params p;
        p.append(id);
        auto result = txn.exec(selectQuery, p);

        /*
         * exec_params returns a result set
         * For SELECT, you need to:
         * 1. Check if there are rows (result.size() > 0)
         * 2. Extract first row
         * 3. Get column values from that row
         */

        if (result.empty()) {
            throw std::runtime_error("Reservation with ID " +
                                     std::to_string(id) + " not found");
        }

        // Get the first row
        auto row = result[0];

        // Create Reservation object from row data
        Reservation res;
        // guest contact information
        res.guest_name = row[0].as<std::string>();
        res.guest_email = row[1].as<std::string>();
        res.guest_phone = row[2].as<std::string>();
        // room information
        res.room_number = row[3].as<int>();
        res.room_type = row[4].as<std::string>();
        res.number_of_guests = row[5].as<int>();
        // dates information
        res.check_in_date = row[6].as<std::string>();
        res.check_out_date = row[7].as<std::string>();
        res.number_of_nights = row[8].as<int>();
        // price and payment information
        res.price_per_night = row[9].as<double>();
        res.total_price = row[10].as<double>();
        res.payment_method = row[11].as<std::string>();
        res.paid = row[12].as<bool>();
        // extra information of the reservation
        res.reservation_status = row[13].as<std::string>();
        res.special_requests = row[14].as<std::string>();
        res.created_at = row[15].as<long>();
        res.updated_at = row[16].as<long>();
        txn.commit();
        return res;

    } catch (const std::exception& e) {
        std::cerr << "[PostgresDB] Error retrieving reservation: " << e.what()
                  << std::endl;
        throw;
    }
}

bool PostgresDB::updateReservation(int id, const Reservation& res) {
    /*
     * Uses WHERE clause to target specific row
     */

    try {
        if (!conn.is_open()) {
            return false;
        }

        pqxx::work txn(conn);

        std::string updateQuery = R"(
            UPDATE reservations SET
                guest_name = $1, guest_email = $2, guest_phone = $3,
                room_number = $4, room_type = $5, number_of_guests = $6,
                check_in_date = $7, check_out_date = $8, number_of_nights = $9,
                price_per_night = $10, total_price = $11, payment_method = $12, paid = $13,
                reservation_status = $14, special_requests = $15,
                updated_at = $16
            WHERE id = $17
        )";

        pqxx::params p;
        p.append(res.guest_name);
        p.append(res.guest_email);
        p.append(res.guest_phone);
        p.append(res.room_number);
        p.append(res.room_type);
        p.append(res.number_of_guests);
        p.append(res.check_in_date);
        p.append(res.check_out_date);
        p.append(res.number_of_nights);
        p.append(res.price_per_night);
        p.append(res.total_price);
        p.append(res.payment_method);
        p.append(res.paid);
        p.append(res.reservation_status);
        p.append(res.special_requests);
        p.append(res.updated_at);
        p.append(id);

        pqxx::result result = txn.exec(updateQuery, p);

        txn.commit();

        // Check if any row was actually updated
        if (result.affected_rows() == 0) {
            std::cerr << "[PostgresDB] No reservation found with ID: " << id
                      << std::endl;
            return false;
        }

        std::cout << "[PostgresDB] Reservation " << id << " updated"
                  << std::endl;
        return true;

    } catch (const std::exception& e) {
        std::cerr << "[PostgresDB] Error updating reservation: " << e.what()
                  << std::endl;
        return false;
    }
    return false;
}

bool PostgresDB::deleteReservation(int id) {
    try {
        if (!conn.is_open()) {
            return false;
        }

        pqxx::work txn(conn);

        std::string deleteQuery = "DELETE FROM reservations WHERE id = $1";

        pqxx::params p;
        p.append(id);
        pqxx::result result = txn.exec(deleteQuery, p);

        txn.commit();

        // Check if any row was actually deleted
        if (result.affected_rows() == 0) {
            std::cerr << "[PostgresDB] No reservation found with ID: " << id
                      << std::endl;
            return false;
        }

        std::cout << "[PostgresDB] Reservation " << id << " deleted"
                  << std::endl;
        return true;

    } catch (const std::exception& e) {
        std::cerr << "[PostgresDB] Error deleting reservation: " << e.what()
                  << std::endl;
        return false;
    }
}
