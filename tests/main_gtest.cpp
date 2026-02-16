#include <gtest/gtest.h>
#include "../src/config/SignalManager.hpp"

int main(int argc, char** argv) {
    // Register signal handlers BEFORE gtest initializes
    // This prevents gtest from overwriting our handlers
    SignalManager sigManager;
    sigManager.setup();
    
    // Now initialize and run gtest
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
