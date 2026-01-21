#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <boost/json.hpp>
#include <stdexcept>
#include <string>

// this struct represents all the reservation fundamental information
struct Reservation {
    std::string owner;
    std::string expirationDate;
    std::string category;
    int cost;
    int room;
};

// this class receives the json and then, parse it
class Logger {
   public:
    Logger() = default;
    // parse the json and returns a Reservation object
    Reservation parseJson(const std::string& jsonFile);

   private:
    // this function validates that the current json have all the reservation
    // information, if thats not the case, returns false
    // TODO: make this function able to send a message to the html/css so the
    // user can se what he needs to add to the reservation
    bool validateJsonFormat(const Reservation& reservation);
};

#endif