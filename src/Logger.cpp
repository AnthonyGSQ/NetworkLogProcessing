#include "Logger.hpp"

#include <iostream>

Reservation Logger::parseJson(const std::string& jsonFile) {
    boost::json::object currentJson = boost::json::parse(jsonFile).as_object();
    Reservation currentReservation;
    // we take all the json information, if someting went wrong, returns nullptr
    try {
        currentReservation.owner = currentJson.at("owner").as_string();
        currentReservation.expirationDate = currentJson.at("expirationDate").as_string();
        currentReservation.category = currentJson.at("category").as_string();
        currentReservation.cost = currentJson.at("cost").as_int64();
        currentReservation.room = currentJson.at("room").as_int64();
    } catch (const std::exception& e) {
        throw std::invalid_argument("JSON parsing failed: " + std::string(e.what()));
    }
    return currentReservation;
};

bool Logger::validateJsonFormat(const Reservation& reservation) {
    if (reservation.owner.empty()) {
        std::cerr<<"owner cannot be empty\n";
        return false;
    }
    if (reservation.cost <= 0) {
        std::cerr<<"cost must be positive\n";
        return false;
    }
    if (reservation.room <=0) {
        std::cerr<<"invalid room number\n";
        return false;
    }
    return true;
}


