#include "ClientConnection.hpp"

#include <iostream>
#include <memory>

clientConnection::clientConnection(tcp::socket socket, PostgresDB* database)
    : db(database), clientSocket(std::move(socket)) {}

void clientConnection::execute() {
    try {
        http::read(clientSocket, socketBuffer, httpRequest);
        http::response<http::string_body> httpResponse;
        processRequest(httpResponse);

        httpResponse.version(httpRequest.version());
        httpResponse.prepare_payload();
        http::write(clientSocket, httpResponse);
        clientSocket.shutdown(tcp::socket::shutdown_send);
    } catch (const std::exception& e) {
        std::cerr<<"client execute error\n";
        http::response<http::string_body> httpResponse;
        http::write(clientSocket, httpResponse);
        clientSocket.shutdown(tcp::socket::shutdown_send);
    }
}

void clientConnection::processRequest(http::response<http::string_body>& httpResponse) {
    try {
        std::string method = httpRequest.method_string();
        std::string target = httpRequest.target();
        
        if (method == "POST" && target.find("/application/reservation") == 0) {
            handlePostHTTP(httpResponse);
        }
        else if (method == "GET" && target.find("/application/reservation/") == 0) {    
            handleGetHTTP(httpResponse);
        }
        else if (method == "PUT" && target.find("/application/reservation/") == 0) {
            handlePutHTTP(httpResponse);
        }
        else if (method == "DELETE" && target.find("/application/reservation/") == 0) {
            handleDeleteHTTP(httpResponse);
        } else {
            std::cerr<<"client processRequest() error\n";
            httpResponse.result(http::status::not_found);
            httpResponse.body() = "Endpoint not found";
        }
    } catch(const std::exception& e) {
        std::cerr<<"client processRequest() error\n";
        httpResponse.result(http::status::bad_request);
        httpResponse.body() = std::string("Error: ") + e.what();
    }
}

void clientConnection::handlePostHTTP(
    http::response<http::string_body>& httpResponse) {
    try {
        std::cerr << "[ClientConnection] POST /reservations - parsing JSON...\n";
        Reservation reservation = jsonHandler.parseJson(httpRequest.body());
        std::cerr << "[ClientConnection] JSON parsed successfully for guest: " << reservation.guest_name << "\n";
        
        std::cerr << "[ClientConnection] Acquiring connection from pool...\n";
        auto conn = db->getConnectionPool()->acquire();
        std::cerr << "[ClientConnection] Connection acquired, inserting into DB...\n";
        
        int reservationId = db->insertReservation(*conn, reservation);
        std::cerr << "[ClientConnection] insertReservation returned ID: " << reservationId << "\n";
        
        if (reservationId != -1) {
            httpResponse.result(http::status::ok);
            httpResponse.body() = "Reservation saved with ID: " + std::to_string(reservationId);
            std::cerr << "[ClientConnection] SUCCESS - Reservation saved with ID: " << reservationId << "\n";
        } else {
            httpResponse.result(http::status::internal_server_error);
            httpResponse.body() = "Failed to make the reservation";
            std::cerr << "[ClientConnection] ERROR - insertReservation returned -1\n";
        }
        
        db->getConnectionPool()->release(std::move(conn));
    } catch (const std::exception& e) {
        httpResponse.result(http::status::internal_server_error);
        httpResponse.body() = std::string("Error: ") + e.what();
        std::cerr << "[ClientConnection] EXCEPTION in handlePostHTTP: " << e.what() << "\n";
    }
}
void clientConnection::handleGetHTTP(
    http::response<http::string_body>& httpResponse) {
    try {
        size_t pos = httpRequest.target().find_last_of('/');
        int id = std::stoi(std::string(httpRequest.target().substr(pos+1)));
        
        Reservation currentRes = db->getReservationById(id);
        httpResponse.result(http::status::ok);
        httpResponse.body() = jsonHandler.reservationToJson(currentRes);
    } catch (const std::runtime_error&) {
        httpResponse.result(http::status::not_found);
        httpResponse.body() = "Reservation not found";
    } catch (const std::exception& e) {
        httpResponse.result(http::status::bad_request);
        httpResponse.body() = std::string("Error: ") + e.what();
    }
}
void clientConnection::handlePutHTTP(
    http::response<http::string_body>& httpResponse) {
    auto conn = db->getConnectionPool()->acquire();
    
    try {
        size_t pos = httpRequest.target().find_last_of('/');
        int id = std::stoi(std::string(httpRequest.target().substr(pos+1)));
        
        Reservation updated = jsonHandler.parseJson(httpRequest.body());
        if (db->updateReservation(id, updated)) {
            httpResponse.result(http::status::ok);
            httpResponse.body() = "Reservation updated";
        } else {
            httpResponse.result(http::status::not_found);
            httpResponse.body() = "Reservation not found";
        }
    } catch (const std::exception& e) {
        httpResponse.result(http::status::bad_request);
        httpResponse.body() = std::string("Error: ") + e.what();
    }
    
    db->getConnectionPool()->release(std::move(conn));
}
void clientConnection::handleDeleteHTTP(
    http::response<http::string_body>& httpResponse) {
    try {
        size_t pos = httpRequest.target().find_last_of('/');
        int id = std::stoi(std::string(httpRequest.target().substr(pos+1)));
        
        if (db->deleteReservation(id)) {
            httpResponse.result(http::status::ok);
            httpResponse.body() = "Reservation deleted";
        } else {
            httpResponse.result(http::status::not_found);
            httpResponse.body() = "Reservation not found";
        }
    } catch (const std::exception& e) {
        httpResponse.result(http::status::bad_request);
        httpResponse.body() = std::string("Error: ") + e.what();
    }
}