#ifndef SIGNALMANAGER_HPP
#define SIGNALMANAGER_HPP

#include <signal.h>

#include <functional>
#include <iostream>

class SignalManager {
   public:
    SignalManager();
    ~SignalManager();
    void setCallback(std::function<void()> cb) { callback = cb; }
    void setup() {
        signal(SIGINT, &SignalManager::handleSignal);   // Ctrl+C
        signal(SIGTERM, &SignalManager::handleSignal);  // kill <pid>
        signal(SIGTSTP, &SignalManager::handleSignal);  // Ctrl+Z
        std::cout << "Signal handlers registered (SIGTERM, SIGINT, SIGTSTP)\n";
    };

   private:
    static std::function<void()> callback;
    // this function is called by the kernel because we set the signals
    // kernel ask "What did you say you are gonna do when you receive a signal?"
    // and then, the kernel executes this function, which calls
    // Application->stop()
    static void handleSignal([[maybe_unused]] int signal) {
        if (callback) {
            callback();
        }
    }
};

#endif