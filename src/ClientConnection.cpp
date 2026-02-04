#include "ClientConnection.hpp"

#include <iostream>

clientConnection::clientConnection(tcp::socket socket)
    : clientSocket(std::move(socket)) {}

void clientConnection::execute() {
    try {
        http::read(clientSocket, socketBuffer, httpRequest);
        std::cout << "Log:\n" << httpRequest.body() << "\n";

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
    // TODO: after we parseJson, we need to save the reservation
    log.parseJson(httpRequest.body());
    httpResponse.result(http::status::ok);
    httpResponse.body() = "OK";
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
