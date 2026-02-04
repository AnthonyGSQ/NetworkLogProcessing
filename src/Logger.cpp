#include "Logger.hpp"

#include <iostream>

Reservation Logger::parseJson(const std::string& jsonFile) {
    Reservation currentReservation;
    // TODO: si el usuario manda un json valido sin esta info, cae en un catch
    // no muy informativo
    try {
        // parsing json
        boost::json::object currentJson =
            boost::json::parse(jsonFile).as_object();
        // get all the json information about the reservaton
        currentReservation.owner = currentJson.at("owner").as_string();
        currentReservation.expirationDate =
            currentJson.at("expirationDate").as_string();
        currentReservation.category = currentJson.at("category").as_string();
        currentReservation.cost = currentJson.at("cost").as_int64();
        currentReservation.room = currentJson.at("room").as_int64();
        // if any data of the reservation is invalid/void, throws a invalid
        // argument err
        if (!validateJsonFormat(currentReservation)) {
            throw std::invalid_argument("Invalid reservation format");
        }
    } catch (const std::exception& e) {
        throw std::invalid_argument("JSON parsing failed: " +
                                    std::string(e.what()));
    }

    return currentReservation;
};

bool Logger::validateJsonFormat(const Reservation& reservation) {
    if (reservation.owner.empty()) {
        std::cerr << "owner cannot be empty\n";
        return false;
    }
    if (reservation.cost <= 0) {
        std::cerr << "cost must be positive\n";
        return false;
    }
    if (reservation.room <= 0) {
        std::cerr << "invalid room number\n";
        return false;
    }
    return true;
}
