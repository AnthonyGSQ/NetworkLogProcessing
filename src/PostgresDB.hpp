#ifndef POSTGRESDB_HPP
#define POSTGRESDB_HPP

#include "Logger.hpp"
#include <pqxx/pqxx>
#include <string>
#include <stdexcept>

// Forward declaration to avoid circular includes
class ConfigManager;

/*
 * PostgresDB.hpp
 * 
 * Responsible for:
 * - Managing PostgreSQL connection
 * - Executing database operations (INSERT, UPDATE, DELETE, SELECT)
 * - Handling transaction safety
 * - Mapping C++ Reservation objects to SQL rows
 * 
 * Design Pattern: RAII (Resource Acquisition Is Initialization)
 * - Constructor opens connection
 * - Destructor closes connection
 * - Connection is automatically cleaned up, never leaked
 */

class PostgresDB {
public:
    /**
     * Constructor: Establishes connection to PostgreSQL database
     * 
     * param: config - ConfigManager instance with validated DB credentials
     * throws: std::runtime_error if connection fails
     * 
     * Why ConfigManager parameter?
     * - Decouples database from configuration loading
     * - Caller handles .env parsing, PostgresDB focuses on DB operations
     * - Makes testing easier: can pass mock ConfigManager in tests
     */
    explicit PostgresDB(const ConfigManager& config);
    
    /**
     * Destructor: Closes database connection
     * 
     * RAII principle: Destructor automatically cleans up connection.
     * User never calls close() manually. If PostgresDB goes out of scope,
     * connection closes automatically. Zero leak risk.
     */
    ~PostgresDB();
    
    /**
     * Check if connection is active
     * 
     * Useful for:
     * - Debugging connection issues
     * - Checking if server should accept new requests
     */
    bool isConnected() const;
    
    /**
     * Insert a new reservation
     * 
     * param: reservation - Reservation object to insert
     * return: true if successful, false otherwise
     * throws: exception with descriptive message if connection lost
     * 
     * What happens inside:
     * 1. Start transaction
     * 2. Insert all fields from reservation into reservations table
     * 3. Commit transaction (all or nothing)
     * 4. Return success/failure
     * 
     */
    bool insertReservation(const Reservation& res);
    
    /**
     * Retrieve a reservation by ID
     * 
     * param: id - reservation ID
     * return: Reservation object if found
     * throws: std::runtime_error if no reservation with that ID exists
     */
    Reservation getReservationById(int id);
    
    /**
     * Update an existing reservation
     * 
     * param: id - reservation ID to update
     * param: reservation - new data (replaces old)
     * return: true if successful, false otherwise
     */
    bool updateReservation(int id, const Reservation& res);
    
    /**
     * Delete a reservation by ID
     * 
     * param: id - reservation ID to delete
     * return: true if successful, false otherwise
     */
    bool deleteReservation(int id);
    
private:
    /**
     * The actual database connection object (provided by libpqxx)
     * pqxx::connection comes from #include <pqxx/pqxx.h>
     */
    pqxx::connection conn;
    
    /**
     * Build PostgreSQL connection string from ConfigManager
     * 
     * Example output:
     * "host=127.0.0.1 port=5432 dbname=hotel_reservations user=anthony password=xxx"
     * 
     * Why private? Only PostgresDB constructor needs this.
     * It's an implementation detail, not part of the public interface.
     */
    static std::string buildConnectionString(const ConfigManager& config);
};

#endif // POSTGRESDB_HPP
