CXX = g++
CXXFLAGS = -std=c++20 -Wall -Wextra -Werror
COVERAGE_FLAGS = --coverage
# Add libpqxx for PostgreSQL support
LDFLAGS = -lboost_json -lpqxx -lpq

SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin
TEST_DIR = tests

# Find all .cpp files in src/ and its subdirectories (including config/)
SOURCES = $(wildcard $(SRC_DIR)/*.cpp) $(wildcard $(SRC_DIR)/**/*.cpp)
OBJECTS = $(SOURCES:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)
TARGET = $(BIN_DIR)/network_log_processor

TEST_SOURCES = $(filter-out $(TEST_DIR)/main_gtest.cpp, $(wildcard $(TEST_DIR)/*.cpp))
TEST_OBJECTS = $(TEST_SOURCES:$(TEST_DIR)/%.cpp=$(OBJ_DIR)/test_%.o)
TEST_MAIN = $(OBJ_DIR)/test_main_gtest.o
TEST_TARGET = $(BIN_DIR)/run_tests
GTEST_FLAGS = -lgtest -pthread

all: $(TARGET)

$(TARGET): $(OBJECTS)
	mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(COVERAGE_FLAGS) $^ -o $@ $(LDFLAGS)

# Pattern rule: src/any/file.cpp → obj/any/file.o
# The % matches the path component (e.g., config/ConfigManager)
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(COVERAGE_FLAGS) -c $< -o $@

$(OBJ_DIR)/test_%.o: $(TEST_DIR)/%.cpp
	mkdir -p $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) $(COVERAGE_FLAGS) -c $< -o $@

$(OBJ_DIR)/test_main_gtest.o: $(TEST_DIR)/main_gtest.cpp
	mkdir -p $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) $(COVERAGE_FLAGS) -c $< -o $@

$(TEST_TARGET): $(TEST_OBJECTS) $(TEST_MAIN) $(filter-out $(OBJ_DIR)/Application/main.o, $(OBJECTS))
	mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(COVERAGE_FLAGS) $^ -o $@ $(GTEST_FLAGS) $(LDFLAGS)

test: $(TEST_TARGET)
	timeout --signal=SIGINT 60 ./$(TEST_TARGET) || true
	gcovr -r . --exclude 'tests' --print-summary --fail-under-line 90

coverage: $(TEST_TARGET)
	timeout --signal=SIGINT 60 ./$(TEST_TARGET) || true
	mkdir -p coverage
	gcovr -r . --exclude 'tests' --html-details coverage/index.html --print-summary --fail-under-line 90
	@echo "Coverage report generated at coverage/index.html"

valgrind: all
	timeout --signal=SIGINT 5 valgrind --leak-check=full --error-exitcode=1 --show-leak-kinds=all ./$(TARGET) || true

help:
	@echo "all"
	@echo "test"
	@echo "coverage"
	@echo "valgrind"
	@echo "instdeps"
	@echo "format"
	@echo "check-format"
	@echo "clean"
	@echo "help"

instdeps:
	sudo dnf install -y \
		gcc-c++ \
		make \
		gtest-devel \
		gcovr \
		boost-devel \
		libpqxx-devel \
		libpq-devel \
		postgresql-devel \
		clang-tools-extra \
		valgrind

format:
	find src tests -name "*.cpp" -o -name "*.hpp" | xargs clang-format -i

check-format:
	find src tests -name "*.cpp" -o -name "*.hpp" | xargs clang-format --dry-run --Werror

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)
	find . -name "*.gcda" -o -name "*.gcno" -o -name "*.gcov" | xargs rm -f

.PHONY: all clean test coverage coverage-html valgrind help instdeps format check-format
