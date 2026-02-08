#ifndef CONFIG_MANAGER_HPP
#define CONFIG_MANAGER_HPP

#include <vector>
#include <string>
#include <map>
#include <stdexcept>
#include <fstream>
#include <sstream>

class ConfigManager {
public:
    /**
     * Constructor: Loads and parses .env file
     * 
     * param: envFilePath - path to .env file (default: ".env")
     * throws: std::runtime_error if file doesn't exist or required fields missing
     */
    explicit ConfigManager(const std::string& envFilePath = ".env");
    
    /**
     * Retrieve a configuration value as string
     * 
     * param: key - configuration key (e.g., "DB_HOST")
     * return: the value (e.g., "127.0.0.1")
     * throws: std::runtime_error if key not found
     */
    std::string get(const std::string& key) const;
    
    /**
     * Retrieve a configuration value as integer
     * 
     * Useful for port numbers, timeouts, etc.
     * throws: std::runtime_error if key not found or value not a valid integer
     */
    int getInt(const std::string& key) const;
    
    /**
     * Check if a key exists in configuration
     * 
     * Useful for optional settings
     */
    bool has(const std::string& key) const;
    
private:
    // Storage: maps key → value (e.g., "DB_HOST" → "127.0.0.1")
    std::map<std::string, std::string> config;
    
    /**
     * Load configuration from file
     * 
     * Why private? Only ConfigManager should call this, in constructor.
     * Callers just construct ConfigManager(), the loading happens automatically.
     */
    void loadFromFile(const std::string& envFilePath);
    
    /**
     * Parse a single line from .env file
     * 
     * Example input: "DB_HOST=127.0.0.1"
     * Extracts: key="DB_HOST", value="127.0.0.1"
     * Ignores: empty lines, comments (lines starting with #)
     */
    void parseLine(const std::string& line);
    
    /**
     * Validate that all required fields exist
     * 
     * If any required field is missing, throws exception with clear message.
     * This fails EARLY - at startup time, not at runtime when you try to connect.
     */
    void validateRequired();
};

#endif // CONFIG_MANAGER_HPP
