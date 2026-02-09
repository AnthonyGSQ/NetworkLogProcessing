#include "ClientConnection.hpp"

#include <iostream>

clientConnection::clientConnection(tcp::socket socket, PostgresDB* db)
    : db(db), clientSocket(std::move(socket)) {}

void clientConnection::execute() {
    try {
        http::read(clientSocket, socketBuffer, httpRequest);
        //std::cout << "Log:\n" << httpRequest.body() << "\n";

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
            if (db->insertReservation(reservation)) {
                httpResponse.result(http::status::ok);
                httpResponse.body() = "Reservation saved successfully\n";
                std::cout << "[ClientConnection] Reservation inserted into database\n";
            } else {
                httpResponse.result(http::status::internal_server_error);
                httpResponse.body() = "Failed to save reservation to database";
                std::cerr << "[ClientConnection] Failed to insert reservation\n";
            }
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
