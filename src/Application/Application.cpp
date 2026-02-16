#include "Application.hpp"

#include <iostream>

Application::Application(const std::string& configPath) {
    configManager = std::make_unique<ConfigManager>(configPath);
    database = std::make_unique<PostgresDB>(*configManager);
    httpServer = std::make_unique<HttpServer>(database.get());
    signalManager = std::make_unique<SignalManager>();
}

void Application::run() {
    if (initializeConfigManager() < 0) {
        std::cerr << "Application::initializeConfigManager(): Error, couldn't "
                     "initialize config manager\n";
        return;
    }
    if (initializeDatabase() < 0) {
        std::cerr << "Application::initializeDatabase(): Error, couldn't "
                     "initialize database\n";
        return;
    }
    if (initializeHttpServer() < 0) {
        std::cerr << "Application::initializeHttpServer(): Error, couldn't "
                     "initialize HttpServer\n";
        return;
    }
    if (initializeSignalManager() < 0) {
        std::cerr << "Application::initializeSignalManager(): Error, couldn't "
                     "initialize signal manager\n";
        return;
    }
    std::cout << "SignalManager, HttpServer, Database and ConfigManager ready!\n";
    httpServer->start();
}

int Application::initializeConfigManager() {
    configManager->validateRequired();
    std::cout << "[ConfigManager] Configuration loaded successfully"
        << std::endl;
    return 0;
}

int Application::initializeDatabase() {
    if (!database || !database->isConnected()) {
        std::cerr << "[Database] Failed to connect to database\n";
        return -1;
    }
    std::cout << "[Database] Connection established\n";
    return 0;
}

int Application::initializeHttpServer() {
    if (!httpServer->startAcceptor()) {
        std::cerr << "[HttpServer] Failed to start acceptor\n";
        return -1;
    }
    std::cout << "[HttpServer] Acceptor started successfully\n";
    return 0;
}

int Application::initializeSignalManager() {
    signalManager->setCallback([this]() {
        this->stop();
    });
    signalManager->setup();
    return 0;
}

void Application::stop() {
    httpServer->stop();
}