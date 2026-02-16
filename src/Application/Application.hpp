#ifndef APPLICATION_HPP
#define APPLICATION_HPP

#include "../config/ConfigManager.hpp"
#include "../HTTP/HttpServer.hpp"
#include "../DataBase/PostgresDB.hpp"
#include "../config/SignalManager.hpp"

class Application {
   private:
    std::unique_ptr<SignalManager> signalManager;
    std::unique_ptr<HttpServer> httpServer;
    std::unique_ptr<PostgresDB> database;
    std::unique_ptr<ConfigManager> configManager;
    int initializeSignalManager();
    int initializeHttpServer();
    int initializeDatabase();
    int initializeConfigManager();

   public:
    Application(const std::string& configPath);
    ~Application() = default;
    void run();
    void stop();
};

#endif