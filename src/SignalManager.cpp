#include "SignalManager.hpp"

// Initialize static member
std::function<void()> SignalManager::callback;

SignalManager::SignalManager() = default;

SignalManager::~SignalManager() = default;
