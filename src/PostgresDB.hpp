#ifndef POSTGRESDB_HPP
#define POSTGRESDB_HPP

#include <pqxx/pqxx>
#include <stdexcept>
#include <string>

#include "ConnectionPool.hpp"
#include "Logger.hpp"

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
     * return: ID of inserted reservation if successful, -1 otherwise
     * throws: exception with descriptive message if connection lost
     *
     * What happens inside:
     * 1. Start transaction
     * 2. Insert all fields from reservation into reservations table
     * 3. Commit transaction (all or nothing)
     * 4. Return the assigned ID (or -1 on failure)
     *
     */
    int insertReservation(const Reservation& res);

    /**
     * Insert a new reservation using a connection from the pool
     *
     * param: conn - Reference to a connection from ConnectionPool
     * param: reservation - Reservation object to insert
     * return: ID of inserted reservation if successful, -1 otherwise
     *
     * Used by: Worker threads (ClientConnection) that acquire connections from
     * pool Allows multiple threads to insert concurrently without blocking
     */
    int insertReservation(pqxx::connection& conn, const Reservation& res);

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

    /**
     * Get access to connection pool
     *
     * Used by worker threads (ClientConnection) to acquire DB connections
     * return: pointer to ConnectionPool maintained by this PostgresDB
     */
    ConnectionPool* getConnectionPool() const;

   private:
    /**
     * The actual database connection object (provided by libpqxx)
     * pqxx::connection comes from #include <pqxx/pqxx.h>
     */
    pqxx::connection conn;

    /**
     * Connection pool for multi-threaded access
     *
     * - Maintains 4 database connections
     * - Worker threads acquire/release connections via this pool
     * - Thread-safe: protected by mutex and condition_variable
     * - Shared resource: all ClientConnection threads use this same pool
     */
    mutable ConnectionPool pool;

    /**
     * Build PostgreSQL connection string from ConfigManager
     *
     * Example output:
     * "host=127.0.0.1 port=5432 dbname=hotel_reservations user=anthony
     * password=xxx"
     *
     * Why private? Only PostgresDB constructor needs this.
     * It's an implementation detail, not part of the public interface.
     */
    static std::string buildConnectionString(const ConfigManager& config);
};

#endif  // POSTGRESDB_HPP
