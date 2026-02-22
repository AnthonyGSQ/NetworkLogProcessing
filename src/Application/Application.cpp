#include "Application.hpp"

#include <iostream>

Application::Application(const std::string& configPath, int recvPort)
    : port(recvPort) {
    try {
        configManager = std::make_unique<ConfigManager>(configPath);
        database = std::make_unique<PostgresDB>(*configManager);
        httpServer = std::make_unique<HttpServer>(database.get(), port);
        signalManager = std::make_unique<SignalManager>();
    } catch (const std::exception& e) {
        throw std::runtime_error(
            std::string("Application constructor failed: ") + e.what());
    }
}

void Application::run() {
    try {
        initializeConfigManager();
        initializeDatabase();
        initializeSignalManager();
        
        std::cout << "All services initialized successfully. Starting HTTP server...\n";
        httpServer->start();
    } catch (const std::exception& e) {
        throw std::runtime_error(
            std::string("Application::run() failed: ") + e.what());
    }
}

void Application::initializeConfigManager() {
    try {
        configManager->validateRequired();
        std::cout << "[ConfigManager] Configuration loaded successfully\n";
    } catch (const std::exception& e) {
        throw std::runtime_error(
            std::string("[ConfigManager] Initialization failed: ") + e.what());
    }
}

void Application::initializeDatabase() {
    try {
        if (!database || !database->isConnected()) {
            throw std::runtime_error("Failed to establish database connection");
        }
        std::cout << "[Database] Connection established\n";
    } catch (const std::exception& e) {
        throw std::runtime_error(
            std::string("[Database] Initialization failed: ") + e.what());
    }
}

void Application::initializeSignalManager() {
    try {
        signalManager->setCallback([this]() { this->stop(); });
        signalManager->setup();
        std::cout << "[SignalManager] Signal handlers registered\n";
    } catch (const std::exception& e) {
        throw std::runtime_error(
            std::string("[SignalManager] Initialization failed: ") + e.what());
    }
}

void Application::stop() { httpServer->stop(); }