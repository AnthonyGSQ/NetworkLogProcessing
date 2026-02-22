#ifndef APPLICATION_HPP
#define APPLICATION_HPP

#include "../DataBase/PostgresDB.hpp"
#include "../HTTP/HttpServer.hpp"
#include "../config/ConfigManager.hpp"
#include "../config/SignalManager.hpp"

class Application {
   private:
    int port;
    std::unique_ptr<SignalManager> signalManager;
    std::unique_ptr<HttpServer> httpServer;
    std::unique_ptr<PostgresDB> database;
    std::unique_ptr<ConfigManager> configManager;
    
    // Initialization methods throw std::runtime_error on failure
    void initializeSignalManager();
    void initializeDatabase();
    void initializeConfigManager();

   public:
    Application(const std::string& configPath, int port = 8080);
    ~Application() = default;
    // run() throws std::runtime_error on any initialization or startup failure
    void run();
    void stop();
};

#endif