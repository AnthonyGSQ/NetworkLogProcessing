#ifndef CLIENTCONNECTION_HPP
#define CLIENTCONNECTION_HPP

// Internal utilitys, buffers and error management
#include <boost/beast/core.hpp>
// HTTP and WebSocket
#include <boost/beast/http.hpp>
// Network, sockets, I/O
#include <boost/asio.hpp>

#include "Logger.hpp"
#include "PostgresDB.hpp"
// Task interface
#include "TaskInterface.hpp"

// alias
namespace beast = boost::beast;
namespace http = beast::http;
namespace asio = boost::asio;
using tcp = asio::ip::tcp;

// Handles a single client HTTP request. Implements Task interface for
// thread pool execution. Parses HTTP request, validates JSON reservation data,
// and sends appropriate HTTP response.
class clientConnection : public Task {
   public:
    explicit clientConnection(tcp::socket socket, PostgresDB* db);

    // Implements Task interface. Called by worker thread.
    // Reads HTTP request, parses and validates JSON, sends response.
    void execute() override;

   private:
    Logger log;
    PostgresDB* db;
    tcp::socket clientSocket;
    beast::flat_buffer socketBuffer;
    http::request<http::string_body> httpRequest;

    // Parses request body JSON, validates reservation, builds response
    void processRequest(http::response<http::string_body>& httpResponse);
    // Sends HTTP error response when exception occurs
    void sendErrorResponse(const std::exception& e) noexcept;
};

#endif