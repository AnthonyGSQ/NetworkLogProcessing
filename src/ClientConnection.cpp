#include "ClientConnection.hpp"

#include <iostream>
#include <memory>

clientConnection::clientConnection(tcp::socket socket, PostgresDB* database)
    : db(database), clientSocket(std::move(socket)) {}

void clientConnection::execute() {
    try {
        http::read(clientSocket, socketBuffer, httpRequest);
        // std::cout << "Log:\n" << httpRequest.body() << "\n";

        http::response<http::string_body> httpResponse;
        processRequest(httpResponse);

        httpResponse.version(httpRequest.version());
        httpResponse.prepare_payload();
        http::write(clientSocket, httpResponse);
        clientSocket.shutdown(tcp::socket::shutdown_send);
    } catch (const std::exception& e) {
        sendErrorResponse(e);
    }
}

void clientConnection::processRequest(
    http::response<http::string_body>& httpResponse) {
    try {
        // Parse JSON from request body
        Reservation reservation = log.parseJson(httpRequest.body());

        // Insert into database if db connection is available
        if (db != nullptr) {
            /*
             * Acquire a connection from the pool
             * This is thread-safe: acquire() blocks until a connection is
             * available
             *
             * The connection is returned as unique_ptr, which ensures
             * automatic cleanup when conn goes out of scope (via RAII)
             */
            auto conn = db->getConnectionPool()->acquire();

            try {
                /*
                 * Insert using the pool connection instead of db->conn
                 * Multiple threads can do this simultaneously:
                 * - Thread 1 uses conn1, inserts data
                 * - Thread 2 uses conn2, inserts data in parallel
                 * - No blocking on db->conn
                 */
                int reservationId = db->insertReservation(*conn, reservation);

                if (reservationId != -1) {
                    httpResponse.result(http::status::ok);
                    httpResponse.body() =
                        "Reservation saved successfully with ID: " +
                        std::to_string(reservationId) + "\n";
                    std::cout << "[ClientConnection] Reservation inserted into "
                                 "database with ID: "
                              << reservationId << "\n";
                } else {
                    httpResponse.result(http::status::internal_server_error);
                    httpResponse.body() =
                        "Failed to save reservation to database";
                    std::cerr
                        << "[ClientConnection] Failed to insert reservation\n";
                }
            } catch (const std::exception& e) {
                httpResponse.result(http::status::bad_request);
                httpResponse.body() = std::string("Error: ") + e.what();
                std::cerr << "[ClientConnection] Exception: " << e.what()
                          << "\n";
            }

            /*
             * Release connection back to pool
             *
             * IMPORTANT: Release is called whether operation succeeded or
             * failed This ensures the connection returns to the pool for other
             * threads
             *
             * The unique_ptr is moved into release(), transferring ownership
             * back to the pool
             */
            db->getConnectionPool()->release(std::move(conn));
        } else {
            httpResponse.result(http::status::internal_server_error);
            httpResponse.body() = "Database connection not available";
            std::cerr << "[ClientConnection] Database pointer is null\n";
        }
    } catch (const std::exception& e) {
        httpResponse.result(http::status::bad_request);
        httpResponse.body() = std::string("Error: ") + e.what();
        std::cerr << "[ClientConnection] Exception: " << e.what() << "\n";
    }
}

void clientConnection::sendErrorResponse(const std::exception& e) noexcept {
    std::cerr << "clientConnection::execute() failed: " << e.what() << "\n";

    try {
        http::response<http::string_body> errorResponse;
        errorResponse.result(http::status::bad_request);
        errorResponse.body() = "Bad request";
        errorResponse.version(httpRequest.version());
        errorResponse.prepare_payload();
        http::write(clientSocket, errorResponse);
    } catch (const std::exception& writeError) {
        std::cerr << "Failed to send error response: " << writeError.what()
                  << "\n";
    }
}
