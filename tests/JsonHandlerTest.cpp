#include <gtest/gtest.h>

#include "../src/HTTP/JsonHandler.hpp"

TEST(JsonHandler, ValidReservationJson) {
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
    JsonHandler jsonHandler;
    Reservation res = jsonHandler.parseJson(validJson);
    EXPECT_EQ(res.guest_name, "Juan Pérez");
    EXPECT_EQ(res.guest_email, "juan@example.com");
    EXPECT_EQ(res.room_number, 101);
    EXPECT_EQ(res.room_type, "Double");
    EXPECT_EQ(res.number_of_guests, 2);
    EXPECT_EQ(res.price_per_night, 150.50);
}

TEST(JsonHandler, MissingRequiredField) {
    std::string invalidJson = R"({
        "guest_name": "Juan Pérez"
    })";
    JsonHandler jsonHandler;
    EXPECT_THROW(jsonHandler.parseJson(invalidJson), std::invalid_argument);
}

TEST(JsonHandler, EmptyJson) {
    JsonHandler jsonHandler;
    std::string voidJson = "";
    EXPECT_THROW(jsonHandler.parseJson(voidJson), std::invalid_argument);
}

TEST(JsonHandler, MalformedJson) {
    JsonHandler jsonHandler;
    std::string malformedJson = R"({invalid json})";
    EXPECT_THROW(jsonHandler.parseJson(malformedJson), std::invalid_argument);
}

TEST(JsonHandler, ValidateJsonFormat_EmptyGuestName) {
    JsonHandler jsonHandler;
    Reservation res;
    res.guest_name = "";
    res.guest_email = "juan@example.com";
    EXPECT_FALSE(jsonHandler.validateJsonFormat(res));
}

TEST(JsonHandler, ValidateJsonFormat_InvalidEmail) {
    JsonHandler jsonHandler;
    Reservation res;
    res.guest_name = "Juan";
    res.guest_email = "invalid-email";
    EXPECT_FALSE(jsonHandler.validateJsonFormat(res));
}

TEST(Logger, ValidateJsonFormat_EmptyEmail) {
    JsonHandler jsonHandler;
    Reservation res;
    res.guest_name = "Juan";
    res.guest_email = "";
    EXPECT_FALSE(jsonHandler.validateJsonFormat(res));
}

TEST(Logger, ValidateJsonFormat_InvalidRoom) {
    JsonHandler jsonHandler;
    Reservation res;
    res.guest_name = "Juan";
    res.guest_email = "juan@example.com";
    res.room_number = -1;
    EXPECT_FALSE(jsonHandler.validateJsonFormat(res));
}

TEST(Logger, ValidateJsonFormat_InvalidRoomZero) {
    JsonHandler jsonHandler;
    Reservation res;
    res.guest_name = "Juan";
    res.guest_email = "juan@example.com";
    res.room_number = 0;
    EXPECT_FALSE(jsonHandler.validateJsonFormat(res));
}

TEST(Logger, ValidateJsonFormat_EmptyRoomType) {
    JsonHandler jsonHandler;
    Reservation res;
    res.guest_name = "Juan";
    res.guest_email = "juan@example.com";
    res.room_number = 101;
    res.room_type = "";
    EXPECT_FALSE(jsonHandler.validateJsonFormat(res));
}

TEST(Logger, ValidateJsonFormat_InvalidGuestsCount) {
    JsonHandler jsonHandler;
    Reservation res;
    res.guest_name = "Juan";
    res.guest_email = "juan@example.com";
    res.room_number = 101;
    res.room_type = "Double";
    res.number_of_guests = 0;
    EXPECT_FALSE(jsonHandler.validateJsonFormat(res));
}

TEST(Logger, ValidateJsonFormat_EmptyCheckInDate) {
    JsonHandler jsonHandler;
    Reservation res;
    res.guest_name = "Juan";
    res.guest_email = "juan@example.com";
    res.room_number = 101;
    res.room_type = "Double";
    res.number_of_guests = 2;
    res.check_in_date = "";
    res.check_out_date = "2026-02-20";
    EXPECT_FALSE(jsonHandler.validateJsonFormat(res));
}

TEST(Logger, ValidateJsonFormat_EmptyCheckOutDate) {
    JsonHandler jsonHandler;
    Reservation res;
    res.guest_name = "Juan";
    res.guest_email = "juan@example.com";
    res.room_number = 101;
    res.room_type = "Double";
    res.number_of_guests = 2;
    res.check_in_date = "2026-02-15";
    res.check_out_date = "";
    EXPECT_FALSE(jsonHandler.validateJsonFormat(res));
}

TEST(Logger, ValidateJsonFormat_CheckInAfterCheckOut) {
    JsonHandler jsonHandler;
    Reservation res;
    res.guest_name = "Juan";
    res.guest_email = "juan@example.com";
    res.room_number = 101;
    res.room_type = "Double";
    res.number_of_guests = 2;
    res.check_in_date = "2026-02-20";
    res.check_out_date = "2026-02-15";
    EXPECT_FALSE(jsonHandler.validateJsonFormat(res));
}

TEST(Logger, ValidateJsonFormat_CheckInEqualsCheckOut) {
    JsonHandler jsonHandler;
    Reservation res;
    res.guest_name = "Juan";
    res.guest_email = "juan@example.com";
    res.room_number = 101;
    res.room_type = "Double";
    res.number_of_guests = 2;
    res.check_in_date = "2026-02-15";
    res.check_out_date = "2026-02-15";
    EXPECT_FALSE(jsonHandler.validateJsonFormat(res));
}

TEST(Logger, ValidateJsonFormat_InvalidNights) {
    JsonHandler jsonHandler;
    Reservation res;
    res.guest_name = "Juan";
    res.guest_email = "juan@example.com";
    res.room_number = 101;
    res.room_type = "Double";
    res.number_of_guests = 2;
    res.check_in_date = "2026-02-15";
    res.check_out_date = "2026-02-20";
    res.number_of_nights = 0;
    EXPECT_FALSE(jsonHandler.validateJsonFormat(res));
}

TEST(Logger, ValidateJsonFormat_InvalidPrice) {
    JsonHandler jsonHandler;
    Reservation res;
    res.guest_name = "Juan";
    res.guest_email = "juan@example.com";
    res.room_number = 101;
    res.room_type = "Double";
    res.number_of_guests = 2;
    res.check_in_date = "2026-02-15";
    res.check_out_date = "2026-02-20";
    res.number_of_nights = 5;
    res.price_per_night = -50;
    EXPECT_FALSE(jsonHandler.validateJsonFormat(res));
}

TEST(Logger, ValidateJsonFormat_InvalidPriceZero) {
    JsonHandler jsonHandler;
    Reservation res;
    res.guest_name = "Juan";
    res.guest_email = "juan@example.com";
    res.room_number = 101;
    res.room_type = "Double";
    res.number_of_guests = 2;
    res.check_in_date = "2026-02-15";
    res.check_out_date = "2026-02-20";
    res.number_of_nights = 5;
    res.price_per_night = 0;
    EXPECT_FALSE(jsonHandler.validateJsonFormat(res));
}

TEST(Logger, ValidateJsonFormat_InvalidTotalPrice) {
    JsonHandler jsonHandler;
    Reservation res;
    res.guest_name = "Juan";
    res.guest_email = "juan@example.com";
    res.room_number = 101;
    res.room_type = "Double";
    res.number_of_guests = 2;
    res.check_in_date = "2026-02-15";
    res.check_out_date = "2026-02-20";
    res.number_of_nights = 5;
    res.price_per_night = 50;
    res.total_price = -250;
    EXPECT_FALSE(jsonHandler.validateJsonFormat(res));
}

TEST(Logger, ValidateJsonFormat_InvalidPaymentMethod) {
    JsonHandler jsonHandler;
    Reservation res;
    res.guest_name = "Juan";
    res.guest_email = "juan@example.com";
    res.room_number = 101;
    res.room_type = "Double";
    res.number_of_guests = 2;
    res.check_in_date = "2026-02-15";
    res.check_out_date = "2026-02-20";
    res.number_of_nights = 5;
    res.price_per_night = 50;
    res.total_price = 250;
    res.payment_method = "";
    EXPECT_FALSE(jsonHandler.validateJsonFormat(res));
}
