#include <gtest/gtest.h>
#include "../src/HttpServer.hpp"

TEST(HttpServer, ConstructorIpv4) {
    HttpServer server(true, 8080);
    EXPECT_TRUE(server.startAcceptor());
}

TEST(HttpServer, ConstructorIpv6) {
    HttpServer server(false, 8080);
    EXPECT_TRUE(server.startAcceptor());

}

TEST(HttpServer, NegativePort) {
    HttpServer server(true, -100);
    EXPECT_FALSE(server.startAcceptor());
}