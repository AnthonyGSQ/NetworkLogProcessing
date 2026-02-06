#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <boost/json.hpp>
#include <stdexcept>
#include <string>

// this struct represents all the reservation fundamental information
struct Reservation {
    // guess data
    std::string guest_name;
    std::string guest_email;
    std::string guest_phone;
    // reservation info
    int room_number;
    std::string room_type;
    int number_of_guests;
    // dates
    std::string check_in_date;
    std::string check_out_date;
    int number_of_nights;
    // cost
    double price_per_night;
    double total_price;
    std::string payment_method;
    bool paid;
    // status
    std::string reservation_status;
    std::string special_requests;
    // reservation metadata
    long created_at;
    long updated_at;
};

// this class receives the json and then, parse it
class Logger {
   public:
    Logger() = default;
    // parse the json and returns a Reservation object
    Reservation parseJson(const std::string& jsonFile);
    // this function validates that the current json have all the reservation
    // information, if thats not the case, returns false
    // TODO: make this function able to send a message to the html/css so the
    // user can se what he needs to add to the reservation
    bool validateJsonFormat(const Reservation& reservation);

   private:
};

#endif