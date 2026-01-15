#include "ClientConnection.hpp"
#include <iostream>

clientConnection::clientConnection(tcp::socket socket) :
    clientSocket(std::move(socket)) {}

void clientConnection::operator()() {
    try {
        // first, we read and print the log of the http request
        http::read(clientSocket, socketBuffer, httpRequest);
        std::cout<< "Log:\n"<<httpRequest.body()<<"\n";
        // once we already read the http request, we start building the response
        // which would be 200 OK if everything goes well, otherwise it return a
        // fail response
        http::response<http::string_body> httpResponse;
        httpResponse.result(http::status::ok);
        httpResponse.body() = "OK";
        httpResponse.version(httpRequest.version());
        httpResponse.prepare_payload();
        // once we build the response, we write it in the client buffer
        http::write(clientSocket, httpResponse);
        // at this point, the http request was processed correctly, so we end
        // the client conection
        clientSocket.shutdown(tcp::socket::shutdown_send);
    }
    // if someting went wrong, we build and send the error response here
    catch (const std::exception& e) {
        std::cerr<<"clientConnection::operator() failed: "<<e.what()<<"\n";
        httpResponse.result(http::status::bad_request);
        httpResponse.body() = ""
        httpResponse.
        httpResponse.
    }
}
