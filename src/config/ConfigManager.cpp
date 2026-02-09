#include "ConfigManager.hpp"
#include <iostream>
#include <algorithm>

ConfigManager::ConfigManager(const std::string& recEnvFilePath) : envFilePath(recEnvFilePath) {
    /*
     * Constructor flow:
     * 1. Load the file (parse all key=value pairs)
     * 2. Validate that required fields exist
     * 3. If anything fails, throw exception BEFORE returning
     */
    
    std::cout << "[ConfigManager] Loading configuration from: " << envFilePath << std::endl;
    
    try {
        loadFromFile(envFilePath);
        validateRequired();
        std::cout << "[ConfigManager] Configuration loaded successfully" << std::endl;
    } catch (const std::exception& e) {
        // Re-throw with context
        throw std::runtime_error(std::string("[ConfigManager] Initialization failed: ") + e.what());
    }
}

void ConfigManager::loadFromFile(const std::string& envFilePath) {
    
    std::ifstream envFile(envFilePath);
    
    // Check if file opened successfully
    if (!envFile.is_open()) {
        throw std::runtime_error("Cannot open .env file: " + envFilePath + 
                                 "\nCreate one by copying .env.example");
    }
    
    std::string line;
    int lineNumber = 0;
    
    // Read file line by line
    while (std::getline(envFile, line)) {
        lineNumber++;
        
        // Skip empty lines and comments
        if (line.empty() || line[0] == '#') {
            continue;
        }
        
        try {
            parseLine(line);
        } catch (const std::exception& e) {
            throw std::runtime_error("Error on line " + std::to_string(lineNumber) + 
                                     ": " + e.what());
        }
    }
    
    std::cout << "[ConfigManager] Loaded " << config.size() << " configuration values" << std::endl;
}

void ConfigManager::parseLine(const std::string& line) {
    /*
     * Parse a single line: "KEY=VALUE"
     * 
     * Example:
     * Input:  "DB_HOST=127.0.0.1"
     * Output: config["DB_HOST"] = "127.0.0.1"
     * 
     * Steps:
     * 1. Find the '=' character
     * 2. Extract key (before '=')
     * 3. Extract value (after '=')
     * 4. Trim whitespace
     * 5. Store
     */
    
    size_t delimPos = line.find('=');
    
    if (delimPos == std::string::npos) {
        // No '=' found - invalid line
        throw std::runtime_error("Invalid format, expected KEY=VALUE");
    }
    
    // Extract key and value
    std::string key = line.substr(0, delimPos);
    std::string value = line.substr(delimPos + 1);
    
    // Trim whitespace from key
    key.erase(0, key.find_first_not_of(" \t"));
    key.erase(key.find_last_not_of(" \t") + 1);
    
    // Trim whitespace from value
    value.erase(0, value.find_first_not_of(" \t"));
    value.erase(value.find_last_not_of(" \t") + 1);
    
    // Validate non-empty
    if (key.empty()) {
        throw std::runtime_error("Empty key");
    }
    
    // Store in map
    config[key] = value;   
}

void ConfigManager::validateRequired() {
    /*
     * Check that all REQUIRED fields exist
     * 
     * These are the fields that PostgresDB will need.
     * If any is missing, we fail NOW, not later when connecting.
     * 
     * Exception message will tell the user exactly what's missing.
     */
    
    const std::vector<std::string> requiredKeys = {
        "DB_HOST",
        "DB_PORT",
        "DB_NAME",
        "DB_USER",
        "DB_PASSWORD"
    };
    
    std::string missing;
    
    for (const auto& key : requiredKeys) {
        if (config.find(key) == config.end()) {
            // Key not found
            if (!missing.empty()) {
                missing += ", ";
            }
            missing += key;
        }
    }
    
    if (!missing.empty()) {
        throw std::runtime_error("Missing required configuration: " + missing + 
                                 "\nCheck .env file has all required fields");
    }
}

std::string ConfigManager::get(const std::string& key) const {
    /*
     * Retrieve a string value
     * 
     * Why throw if not found?
     * - Better to crash loudly at the exact line that tries to use it
     * - Than silently return empty string and have weird behavior later
     * 
     * This is a design choice: "fail fast and clear"
     */
    
    auto it = config.find(key);
    
    if (it == config.end()) {
        throw std::runtime_error("Configuration key not found: " + key);
    }
    
    return it->second;
}

int ConfigManager::getInt(const std::string& key) const {
    /*
     * Retrieve and convert to integer
     * 
     * Why separate method?
     * - Ports are integers, not strings
     * - This method validates the conversion
     * - If value is "abc" instead of "5432", we get a clear error
     */
    
    std::string value = get(key);  // Will throw if key not found
    
    try {
        // std::stoi = "string to int"
        return std::stoi(value);
    } catch (const std::exception& e) {
        throw std::runtime_error("Configuration key '" + key + "' is not a valid integer: '" + 
                                 value + "'");
    }
}

bool ConfigManager::has(const std::string& key) const {
    /*
     * Check if key exists without throwing
     * 
     * Useful for optional configuration (e.g., logging level)
     */
    
    return config.find(key) != config.end();
}
