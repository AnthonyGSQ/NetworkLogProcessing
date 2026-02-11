#include <gtest/gtest.h>

#include <fstream>

#include "../src/config/ConfigManager.hpp"

// Helper to create a test .env file
void createTestEnvFile(const std::string& filename,
                       const std::string& content) {
    std::ofstream file(filename);
    file << content;
    file.close();
}

// Helper to clean up test .env file
void deleteTestEnvFile(const std::string& filename) {
    std::remove(filename.c_str());
}

TEST(ConfigManager, LoadValidEnvFile) {
    std::string testEnvContent = R"(
DB_HOST=127.0.0.1
DB_PORT=5432
DB_NAME=test_db
DB_USER=testuser
DB_PASSWORD=testpass
)";

    createTestEnvFile(".env.test", testEnvContent);

    try {
        ConfigManager config(".env.test");
        EXPECT_EQ(config.get("DB_HOST"), "127.0.0.1");
        EXPECT_EQ(config.getInt("DB_PORT"), 5432);
        EXPECT_EQ(config.get("DB_NAME"), "test_db");
    } catch (...) {
        deleteTestEnvFile(".env.test");
        throw;
    }

    deleteTestEnvFile(".env.test");
}

TEST(ConfigManager, LoadEnvFileWithComments) {
    std::string testEnvContent = R"(
# This is a comment
DB_HOST=localhost
# Another comment
DB_PORT=5432
DB_NAME=mydb
DB_USER=user
DB_PASSWORD=pass
)";

    createTestEnvFile(".env.test", testEnvContent);

    try {
        ConfigManager config(".env.test");
        EXPECT_EQ(config.get("DB_HOST"), "localhost");
    } catch (...) {
        deleteTestEnvFile(".env.test");
        throw;
    }

    deleteTestEnvFile(".env.test");
}

TEST(ConfigManager, FileNotFound) {
    EXPECT_THROW(ConfigManager config(".env.nonexistent"), std::runtime_error);
}

TEST(ConfigManager, MissingRequiredFields) {
    std::string testEnvContent = R"(
DB_HOST=localhost
DB_PORT=5432
)";

    createTestEnvFile(".env.test", testEnvContent);

    try {
        EXPECT_THROW(ConfigManager config(".env.test"), std::runtime_error);
    } catch (...) {
        deleteTestEnvFile(".env.test");
        throw;
    }

    deleteTestEnvFile(".env.test");
}

TEST(ConfigManager, InvalidLineFormat) {
    std::string testEnvContent = R"(
DB_HOST=127.0.0.1
INVALID_LINE_NO_EQUALS
DB_PORT=5432
DB_NAME=test_db
DB_USER=testuser
DB_PASSWORD=testpass
)";

    createTestEnvFile(".env.test", testEnvContent);

    try {
        EXPECT_THROW(ConfigManager config(".env.test"), std::runtime_error);
    } catch (...) {
        deleteTestEnvFile(".env.test");
        throw;
    }

    deleteTestEnvFile(".env.test");
}

TEST(ConfigManager, EmptyKeyAfterTrimming) {
    std::string testEnvContent = R"(
  =somevalue
DB_HOST=127.0.0.1
DB_PORT=5432
DB_NAME=test_db
DB_USER=testuser
DB_PASSWORD=testpass
)";

    createTestEnvFile(".env.test", testEnvContent);

    try {
        EXPECT_THROW(ConfigManager config(".env.test"), std::runtime_error);
    } catch (...) {
        deleteTestEnvFile(".env.test");
        throw;
    }

    deleteTestEnvFile(".env.test");
}

TEST(ConfigManager, GetExistingKey) {
    std::string testEnvContent = R"(
DB_HOST=myhost.com
DB_PORT=5432
DB_NAME=test_db
DB_USER=testuser
DB_PASSWORD=testpass
)";

    createTestEnvFile(".env.test", testEnvContent);

    try {
        ConfigManager config(".env.test");
        EXPECT_EQ(config.get("DB_HOST"), "myhost.com");
    } catch (...) {
        deleteTestEnvFile(".env.test");
        throw;
    }

    deleteTestEnvFile(".env.test");
}

TEST(ConfigManager, GetNonExistentKey) {
    std::string testEnvContent = R"(
DB_HOST=127.0.0.1
DB_PORT=5432
DB_NAME=test_db
DB_USER=testuser
DB_PASSWORD=testpass
)";

    createTestEnvFile(".env.test", testEnvContent);

    try {
        ConfigManager config(".env.test");
        EXPECT_THROW(config.get("NONEXISTENT_KEY"), std::runtime_error);
    } catch (...) {
        deleteTestEnvFile(".env.test");
        throw;
    }

    deleteTestEnvFile(".env.test");
}

TEST(ConfigManager, GetIntValidValue) {
    std::string testEnvContent = R"(
DB_HOST=127.0.0.1
DB_PORT=5432
DB_NAME=test_db
DB_USER=testuser
DB_PASSWORD=testpass
)";

    createTestEnvFile(".env.test", testEnvContent);

    try {
        ConfigManager config(".env.test");
        EXPECT_EQ(config.getInt("DB_PORT"), 5432);
    } catch (...) {
        deleteTestEnvFile(".env.test");
        throw;
    }

    deleteTestEnvFile(".env.test");
}

TEST(ConfigManager, GetIntInvalidValue) {
    std::string testEnvContent = R"(
DB_HOST=127.0.0.1
DB_PORT=not_a_number
DB_NAME=test_db
DB_USER=testuser
DB_PASSWORD=testpass
)";

    createTestEnvFile(".env.test", testEnvContent);

    try {
        ConfigManager config(".env.test");
        EXPECT_THROW(config.getInt("DB_PORT"), std::runtime_error);
    } catch (...) {
        deleteTestEnvFile(".env.test");
        throw;
    }

    deleteTestEnvFile(".env.test");
}

TEST(ConfigManager, GetIntNonExistentKey) {
    std::string testEnvContent = R"(
DB_HOST=127.0.0.1
DB_PORT=5432
DB_NAME=test_db
DB_USER=testuser
DB_PASSWORD=testpass
)";

    createTestEnvFile(".env.test", testEnvContent);

    try {
        ConfigManager config(".env.test");
        EXPECT_THROW(config.getInt("NONEXISTENT_KEY"), std::runtime_error);
    } catch (...) {
        deleteTestEnvFile(".env.test");
        throw;
    }

    deleteTestEnvFile(".env.test");
}

TEST(ConfigManager, HasExistingKey) {
    std::string testEnvContent = R"(
DB_HOST=127.0.0.1
DB_PORT=5432
DB_NAME=test_db
DB_USER=testuser
DB_PASSWORD=testpass
)";

    createTestEnvFile(".env.test", testEnvContent);

    try {
        ConfigManager config(".env.test");
        EXPECT_TRUE(config.has("DB_HOST"));
    } catch (...) {
        deleteTestEnvFile(".env.test");
        throw;
    }

    deleteTestEnvFile(".env.test");
}

TEST(ConfigManager, HasNonExistentKey) {
    std::string testEnvContent = R"(
DB_HOST=127.0.0.1
DB_PORT=5432
DB_NAME=test_db
DB_USER=testuser
DB_PASSWORD=testpass
)";

    createTestEnvFile(".env.test", testEnvContent);

    try {
        ConfigManager config(".env.test");
        EXPECT_FALSE(config.has("NONEXISTENT_KEY"));
    } catch (...) {
        deleteTestEnvFile(".env.test");
        throw;
    }

    deleteTestEnvFile(".env.test");
}

TEST(ConfigManager, TrimWhitespace) {
    std::string testEnvContent = R"(
DB_HOST   =   127.0.0.1   
DB_PORT=5432
DB_NAME=test_db
DB_USER=testuser
DB_PASSWORD=testpass
)";

    createTestEnvFile(".env.test", testEnvContent);

    try {
        ConfigManager config(".env.test");
        EXPECT_EQ(config.get("DB_HOST"), "127.0.0.1");
    } catch (...) {
        deleteTestEnvFile(".env.test");
        throw;
    }

    deleteTestEnvFile(".env.test");
}
