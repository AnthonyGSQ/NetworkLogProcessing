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
}

TEST(Logger, EmptyJson) {
    Logger logger;
    std::string voidJson = "";
    EXPECT_THROW(logger.parseJson(voidJson), std::invalid_argument);
}

TEST(Logger, MalformedJson) {
    Logger logger;
    std::string malformedJson = R"({invalid json})";
    EXPECT_THROW(logger.parseJson(malformedJson), std::invalid_argument);
}

TEST(Logger, ValidateJsonFormat_EmptyOwner) {
    Logger logger;
    Reservation res{"", "2026-01-25", "deluxe", 150, 101};
    EXPECT_FALSE(logger.validateJsonFormat(res));
}

TEST(Logger, ValidateJsonFormat_InvalidCost) {
    Logger logger;
    Reservation res{"Juan", "2026-01-25", "deluxe", -50, 101};
    EXPECT_FALSE(logger.validateJsonFormat(res));
}

TEST(Logger, ValidateJsonFormat_InvalidRoom) {
    Logger logger;
    Reservation res{"Juan", "2026-01-25", "deluxe", 150, 0};
    EXPECT_FALSE(logger.validateJsonFormat(res));
}

TEST(Logger, InvalidFormatThrowsException) {
    Logger logger;
    std::string jsonWithZeroCost = R"({
        "owner": "Juan",
        "expirationDate": "2026-01-25",
        "category": "deluxe",
        "cost": 0,
        "room": 101
    })";
    EXPECT_THROW(logger.parseJson(jsonWithZeroCost), std::invalid_argument);
}

TEST(Logger, InvalidFormatZeroRoom) {
    Logger logger;
    std::string jsonWithZeroRoom = R"({
        "owner": "Juan",
        "expirationDate": "2026-01-25",
        "category": "deluxe",
        "cost": 150,
        "room": 0
    })";
    EXPECT_THROW(logger.parseJson(jsonWithZeroRoom), std::invalid_argument);
}
