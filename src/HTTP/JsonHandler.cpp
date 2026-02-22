#include "JsonHandler.hpp"

#include <iostream>

Reservation JsonHandler::parseJson(const std::string& jsonFile) {
    Reservation currentReservation;
    try {
        // parsing json
        boost::json::object currentJson =
            boost::json::parse(jsonFile).as_object();

        // Check required fields first
        std::vector<std::string> requiredFields = {
            "guest_name", "guest_email", "guest_phone",
            "room_number", "room_type", "number_of_guests",
            "check_in_date", "check_out_date", "number_of_nights",
            "price_per_night", "total_price", "payment_method", "paid",
            "created_at", "updated_at"
        };

        for (const auto& field : requiredFields) {
            if (!currentJson.contains(field)) {
                throw std::invalid_argument("Missing required field: " + field);
            }
        }

        // Guest data
        currentReservation.guest_name =
            currentJson.at("guest_name").as_string();
        currentReservation.guest_email =
            currentJson.at("guest_email").as_string();
        currentReservation.guest_phone =
            currentJson.at("guest_phone").as_string();

        // Reservation info
        currentReservation.room_number =
            currentJson.at("room_number").as_int64();
        currentReservation.room_type = currentJson.at("room_type").as_string();
        currentReservation.number_of_guests =
            currentJson.at("number_of_guests").as_int64();

        // Dates
        currentReservation.check_in_date =
            currentJson.at("check_in_date").as_string();
        currentReservation.check_out_date =
            currentJson.at("check_out_date").as_string();
        currentReservation.number_of_nights =
            currentJson.at("number_of_nights").as_int64();

        // Cost
        currentReservation.price_per_night =
            currentJson.at("price_per_night").as_double();
        currentReservation.total_price =
            currentJson.at("total_price").as_double();
        currentReservation.payment_method =
            currentJson.at("payment_method").as_string();
        currentReservation.paid = currentJson.at("paid").as_bool();

        // Status (opcional, tiene valor por defecto)
        if (currentJson.contains("reservation_status")) {
            currentReservation.reservation_status =
                currentJson.at("reservation_status").as_string();
        }

        // Special requests (opcional)
        if (currentJson.contains("special_requests")) {
            currentReservation.special_requests =
                currentJson.at("special_requests").as_string();
        }

        // Metadata timestamps
        currentReservation.created_at = currentJson.at("created_at").as_int64();
        currentReservation.updated_at = currentJson.at("updated_at").as_int64();

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

std::string JsonHandler::reservationToJson(const Reservation& res) {
    try {
        boost::json::object jsonObj;
        
        // Guest data
        jsonObj["guest_name"] = res.guest_name;
        jsonObj["guest_email"] = res.guest_email;
        jsonObj["guest_phone"] = res.guest_phone;
        
        // Reservation info
        jsonObj["room_number"] = res.room_number;
        jsonObj["room_type"] = res.room_type;
        jsonObj["number_of_guests"] = res.number_of_guests;
        
        // Dates
        jsonObj["check_in_date"] = res.check_in_date;
        jsonObj["check_out_date"] = res.check_out_date;
        jsonObj["number_of_nights"] = res.number_of_nights;
        
        // Cost
        jsonObj["price_per_night"] = res.price_per_night;
        jsonObj["total_price"] = res.total_price;
        jsonObj["payment_method"] = res.payment_method;
        jsonObj["paid"] = res.paid;
        
        // Status
        jsonObj["reservation_status"] = res.reservation_status;
        jsonObj["special_requests"] = res.special_requests;
        
        // Metadata timestamps
        jsonObj["created_at"] = res.created_at;
        jsonObj["updated_at"] = res.updated_at;
        
        // Convert to string
        return boost::json::serialize(jsonObj);
    } catch (const std::exception& e) {
        throw std::runtime_error("Failed to serialize reservation to JSON: " + 
                                std::string(e.what()));
    }
}

bool JsonHandler::validateJsonFormat(const Reservation& reservation) {
    // Validar datos del huésped
    if (reservation.guest_name.empty()) {
        std::cerr << "guest_name cannot be empty\n";
        return false;
    }
    if (reservation.guest_email.empty() ||
        reservation.guest_email.find('@') == std::string::npos) {
        std::cerr << "guest_email must be valid (contain @)\n";
        return false;
    }

    // Validar información de habitación
    if (reservation.room_number <= 0) {
        std::cerr << "room_number must be positive\n";
        return false;
    }
    if (reservation.room_type.empty()) {
        std::cerr << "room_type cannot be empty\n";
        return false;
    }
    if (reservation.number_of_guests <= 0) {
        std::cerr << "number_of_guests must be positive\n";
        return false;
    }

    // Validar fechas
    if (reservation.check_in_date.empty() ||
        reservation.check_out_date.empty()) {
        std::cerr << "check_in_date and check_out_date cannot be empty\n";
        return false;
    }
    if (reservation.check_in_date >= reservation.check_out_date) {
        std::cerr << "check_in_date must be before check_out_date\n";
        return false;
    }

    // Validar noches
    if (reservation.number_of_nights <= 0) {
        std::cerr << "number_of_nights must be positive\n";
        return false;
    }

    // Validar precios
    if (reservation.price_per_night <= 0) {
        std::cerr << "price_per_night must be positive\n";
        return false;
    }
    if (reservation.total_price <= 0) {
        std::cerr << "total_price must be positive\n";
        return false;
    }

    // Validar método de pago
    if (reservation.payment_method.empty()) {
        std::cerr << "payment_method cannot be empty\n";
        return false;
    }

    return true;
}
