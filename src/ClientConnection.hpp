#ifndef CLIENTCONNECTION_HPP
#define CLIENTCONNECTION_HPP

// Internal utilitys, buffers and error management
#include <boost/beast/core.hpp>
// HTTP and WebSocket
#include <boost/beast/http.hpp>
// Network, sockets, I/O
#include <boost/asio.hpp>

// alias
namespace beast = boost::beast;
namespace http = beast::http;
namespace asio = boost::asio;
using tcp = asio::ip::tcp;

class clientConnection {
    public:
        explicit clientConnection(tcp::socket socket);
        void operator()();
    private:
        tcp::socket clientSocket;
        beast::flat_buffer socketBuffer;
        http::request<http::string_body> httpRequest;
};


#endif