#include "PostgresDB.hpp"
#include "config/ConfigManager.hpp"
#include <iostream>
#include <sstream>

PostgresDB::PostgresDB(const ConfigManager& config)
    : conn(buildConnectionString(config))
{
    /*
     * RAII in action:
     * - Constructor parameter: ConfigManager with validated credentials
     * - Member initializer: conn(buildConnectionString(config))
     *   This OPENS the connection immediately
     * - If connection fails, exception thrown before object is fully constructed
     * - Caller sees the exception, knows something is wrong
     */
    
    if (!conn.is_open()) {
        throw std::runtime_error("[PostgresDB] Failed to connect to database");
    }
    
    std::cout << "[PostgresDB] Connected successfully" << std::endl;
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
     *   "host=127.0.0.1 port=5432 dbname=hotel_reservations user=username password=mysecretpassword"
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

bool PostgresDB::isConnected() const {
    return conn.is_open();
}

bool PostgresDB::insertReservation(const Reservation& res) {
    /*
     * Big picture: Take a Reservation C++ object, insert it into PostgreSQL
     * 
     * Steps:
     * 1. Verify connection still open
     * 2. Start transaction (all-or-nothing)
     * 3. Execute INSERT SQL with parameters
     * 4. Commit transaction
     * 5. Return success/failure
     */
    
    try {
        // Step 1: Sanity check
        if (!conn.is_open()) {
            std::cerr << "[PostgresDB] ERROR: Connection lost" << std::endl;
            return false;
        }
        
        /*
         * Step 2: Create a transaction
         * 
         * What is pqxx::work?
         * - A transaction object
         * - Everything you do with it (execute, insert, etc) is part of ONE transaction
         * - If you commit() it, everything is saved
         * - If you throw an exception before commit(), EVERYTHING is rolled back
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
         *   std::string query = "INSERT INTO reservations (guest_name) VALUES ('" 
         *                       + res.guest_name + "')";
         *   
         * SAFE (protected by prepared statement):
         *   std::string query = "INSERT INTO reservations (guest_name) VALUES ($1)";
         *   txn.exec_params(query, res.guest_name);
         *   
         *   libpqxx sends res.guest_name as DATA, not as SQL code.
         *   Server knows: "this is a value, not code"
         *   No injection possible.
         */
        
        // The INSERT statement with $1, $2, etc. placeholders
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
        )";
        
        /*
         * R"(...)" is a raw string literal
         * Useful here because we have lots of quotes and newlines
         * Without it, would need escaping: "INSERT ... WHERE name = \\"value\\""
         * With raw string: no escaping needed
         */
        
        // Execute with parameters
        txn.exec_params(insertQuery,
            // Guest info ($1, $2, $3)
            res.guest_name, res.guest_email, res.guest_phone,
            
            // Reservation info ($4, $5, $6)
            res.room_number, res.room_type, res.number_of_guests,
            
            // Dates ($7, $8, $9)
            res.check_in_date, res.check_out_date, res.number_of_nights,
            
            // Cost ($10, $11, $12, $13)
            res.price_per_night, res.total_price, res.payment_method, res.paid,
            
            // Status ($14, $15)
            res.reservation_status, res.special_requests,
            
            // Metadata ($16, $17)
            res.created_at, res.updated_at
        );
        
        // Step 4: Commit the transaction
        txn.commit();
        
        /*
         * - If commit succeeds, data is in database forever
         * - If commit fails, all changes are rolled back automatically
         */
        
        std::cout << "[PostgresDB] Reservation inserted successfully for: " 
                  << res.guest_name << std::endl;
        
        return true;
        
    } catch (const std::exception& e) {
        /*
         * If ANY step above throws:
         * - Transaction is automatically rolled back (no partial inserts)
         * - Exception caught here
         * - Return false to caller
         * 
         * This is the safety of RAII + transactions
         */
        std::cerr << "[PostgresDB] Error inserting reservation: " << e.what() << std::endl;
        return false;
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
        
        // Execute with ID parameter
        auto result = txn.exec_params(selectQuery, id);
        
        /*
         * exec_params returns a result set
         * For SELECT, you need to:
         * 1. Check if there are rows (result.size() > 0)
         * 2. Extract first row
         * 3. Get column values from that row
         */
        
        if (result.empty()) {
            throw std::runtime_error("Reservation with ID " + std::to_string(id) + " not found");
        }
        
        // Get the first row
        auto row = result[0];
        
        // Create Reservation object from row data
        Reservation res;
        // guess contact information
        res.guest_name = row[0].as<std::string>();
        res.guest_email = row[1].as<std::string>();
        res.guest_phone = row[3].as<std::string>();
        // room information
        res.room_number = row[4].as<int>();
        res.room_type = row[5].as<std::string>();
        res.number_of_guests = row[6].as<int>();
        // dates information
        res.check_in_date = row[7].as<std::string>();
        res.check_out_date = row[8].as<std::string>();
        res.number_of_nights = row[9].as<std::string>();
        // price and payment information
        res.price_per_night = row[10].as<std::string>();
        res.total_price = row[11].as<std::string>();
        res.payment_method = row[12].as<std::string>();
        res.total_price = row[13].as<std::string>();
        res.payment_method = row[14].as<std::string>();
        res.paid = row[15].as<std::string>();
        // extra information of the reservation
        res.reservation_status = row[16].as<std::string>();
        res.special_requests = row[17].as<std::string>();
        res.created_at = row[17].as<std::string>();
        res.updated_at = row[17].as<std::string>();
        txn.commit();
        return res;
        
    } catch (const std::exception& e) {
        std::cerr << "[PostgresDB] Error retrieving reservation: " << e.what() << std::endl;
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
        
        txn.exec_params(updateQuery,
            res.guest_name, res.guest_email, res.guest_phone,
            res.room_number, res.room_type, res.number_of_guests,
            res.check_in_date, res.check_out_date, res.number_of_nights,
            res.price_per_night, res.total_price, res.payment_method, res.paid,
            res.reservation_status, res.special_requests,
            res.updated_at,
            id  // $17 - the WHERE id = $17
        );
        
        txn.commit();
        
        std::cout << "[PostgresDB] Reservation " << id << " updated" << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "[PostgresDB] Error updating reservation: " << e.what() << std::endl;
        return false;
    }
}

bool PostgresDB::deleteReservation(int id) {
    
    try {
        if (!conn.is_open()) {
            return false;
        }
        
        pqxx::work txn(conn);
        
        std::string deleteQuery = "DELETE FROM reservations WHERE id = $1";
        txn.exec_params(deleteQuery, id);
        
        txn.commit();
        
        std::cout << "[PostgresDB] Reservation " << id << " deleted" << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "[PostgresDB] Error deleting reservation: " << e.what() << std::endl;
        return false;
    }
}
