#include <gtest/gtest.h>

#include "../src/Logger.hpp"

TEST(Logger, ValidReservationJson) {
    std::string validJson = R"({
        "guest_name": "Juan Pérez",
        "guest_email": "juan@example.com",
        "guest_phone": "+34 123 456 789",
        "room_number": 101,
        "room_type": "Double",
        "number_of_guests": 2,
        "check_in_date": "2026-02-15",
        "check_out_date": "2026-02-20",
        "number_of_nights": 5,
        "price_per_night": 150.50,
        "total_price": 752.50,
        "payment_method": "credit_card",
        "paid": false,
        "created_at": 1707124800,
        "updated_at": 1707124800
    })";
    Logger logger;
    Reservation res = logger.parseJson(validJson);
    EXPECT_EQ(res.guest_name, "Juan Pérez");
    EXPECT_EQ(res.guest_email, "juan@example.com");
    EXPECT_EQ(res.room_number, 101);
    EXPECT_EQ(res.room_type, "Double");
    EXPECT_EQ(res.number_of_guests, 2);
    EXPECT_EQ(res.price_per_night, 150.50);
}

TEST(Logger, MissingRequiredField) {
    std::string invalidJson = R"({
        "guest_name": "Juan Pérez"
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

TEST(Logger, ValidateJsonFormat_EmptyGuestName) {
    Logger logger;
    Reservation res;
    res.guest_name = "";
    res.guest_email = "juan@example.com";
    EXPECT_FALSE(logger.validateJsonFormat(res));
}

TEST(Logger, ValidateJsonFormat_InvalidEmail) {
    Logger logger;
    Reservation res;
    res.guest_name = "Juan";
    res.guest_email = "invalid-email";
    EXPECT_FALSE(logger.validateJsonFormat(res));
}

TEST(Logger, ValidateJsonFormat_InvalidRoom) {
    Logger logger;
    Reservation res;
    res.guest_name = "Juan";
    res.guest_email = "juan@example.com";
    res.room_number = -1;
    EXPECT_FALSE(logger.validateJsonFormat(res));
}

TEST(Logger, ValidateJsonFormat_InvalidPrice) {
    Logger logger;
    Reservation res;
    res.guest_name = "Juan";
    res.guest_email = "juan@example.com";
    res.room_number = 101;
    res.price_per_night = -50;
    EXPECT_FALSE(logger.validateJsonFormat(res));
}
