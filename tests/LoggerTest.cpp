#include <gtest/gtest.h>

#include "../src/Logger.hpp"

TEST(Logger, FunctionalJson) {
    std::string validJson = R"({
        "owner": "Juan",
        "expirationDate": "2026-01-25",
        "category": "deluxe",
        "cost": 150,
        "room": 101
    })";
    Logger logger;
    Reservation res = logger.parseJson(validJson);
    EXPECT_EQ(res.owner, "Juan");
    EXPECT_EQ(res.expirationDate, "2026-01-25");
    EXPECT_EQ(res.category, "deluxe");
    EXPECT_EQ(res.cost, 150);
    EXPECT_EQ(res.room, 101);
}

TEST(Logger, InvalidJson) {
    std::string invalidJson = R"({
        "owner": "Juan"
        // Falta expirationDate, category, etc
    })";
    Logger logger;
    EXPECT_THROW(logger.parseJson(invalidJson), std::invalid_argument);
    std::string voidJson = "";
    EXPECT_THROW(logger.parseJson(voidJson), std::invalid_argument);
    std::string duplicateJson = R"({
        "owner": "Juan",
        "expirationDate": "2026-01-25",
        "category": "deluxe",
        "cost": 150,
        "room": 101
        "owner": "Juan2",
        "expirationDate": "2026-01-25",
        "category": "deluxe2",
        "cost": 1500,
        "room": 1010
    })";
    EXPECT_THROW(logger.parseJson(duplicateJson), std::invalid_argument);

    std::string twoOwnersJson = R"({
        "owner": "Juan, Daniel",
        "expirationDate": "2026-01-25",
        "category": "deluxe",
        "cost": 150,
        "room": 101
    })";
}
