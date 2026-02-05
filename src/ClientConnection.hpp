#ifndef CLIENTCONNECTION_HPP
#define CLIENTCONNECTION_HPP

// Internal utilitys, buffers and error management
#include <boost/beast/core.hpp>
// HTTP and WebSocket
#include <boost/beast/http.hpp>
// Network, sockets, I/O
#include <boost/asio.hpp>

#include "Logger.hpp"
// Task interface
#include "TaskInterface.hpp"

// alias
namespace beast = boost::beast;
namespace http = beast::http;
namespace asio = boost::asio;
using tcp = asio::ip::tcp;

// ClientConnection HEREDA de Task (interfaz polimórfica)
class clientConnection : public Task {
   public:
    explicit clientConnection(tcp::socket socket);

    // Implementa el método puro virtual de Task
    void execute() override;

   private:
    Logger log;
    tcp::socket clientSocket;
    beast::flat_buffer socketBuffer;
    http::request<http::string_body> httpRequest;

    void processRequest(http::response<http::string_body>& httpResponse);
    void sendErrorResponse(const std::exception& e) noexcept;
};

#endif