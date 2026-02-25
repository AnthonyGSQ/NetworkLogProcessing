#include <iostream>

#include "Application.hpp"

int main() {
    try {
        Application app(".env");
        app.run();  // ← AQUÍ!
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}