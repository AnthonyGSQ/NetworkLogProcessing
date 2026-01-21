CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Werror
COVERAGE_FLAGS = --coverage
LDFLAGS = -lboost_json

SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin
TEST_DIR = tests

SOURCES = $(wildcard $(SRC_DIR)/*.cpp)
OBJECTS = $(SOURCES:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)
TARGET = $(BIN_DIR)/network_log_processor

TEST_SOURCES = $(wildcard $(TEST_DIR)/*.cpp)
TEST_OBJECTS = $(TEST_SOURCES:$(TEST_DIR)/%.cpp=$(OBJ_DIR)/test_%.o)
TEST_TARGET = $(BIN_DIR)/run_tests
GTEST_FLAGS = -lgtest -lgtest_main -pthread

all: $(TARGET)

$(TARGET): $(OBJECTS)
	mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(COVERAGE_FLAGS) $^ -o $@ $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	mkdir -p $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) $(COVERAGE_FLAGS) -c $< -o $@

$(OBJ_DIR)/test_%.o: $(TEST_DIR)/%.cpp
	mkdir -p $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) $(COVERAGE_FLAGS) -c $< -o $@

$(TEST_TARGET): $(TEST_OBJECTS) $(filter-out $(OBJ_DIR)/main.o, $(OBJECTS))
	mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(COVERAGE_FLAGS) $^ -o $@ $(GTEST_FLAGS) $(LDFLAGS)

test: $(TEST_TARGET)
	./$(TEST_TARGET)

coverage: clean
	$(MAKE) CXXFLAGS="$(CXXFLAGS) $(COVERAGE_FLAGS)" test
	gcovr -r . --print-summary --fail-under-lines 90

valgrind: all
	timeout -s SIGINT 5 valgrind --leak-check=full --error-exitcode=1 --show-leak-kinds=all ./$(TARGET) || true

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
		valgrind

format:
	find src tests -name "*.cpp" -o -name "*.hpp" | xargs clang-format -i

check-format:
	find src tests -name "*.cpp" -o -name "*.hpp" | xargs clang-format --dry-run --Werror

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

.PHONY: all clean test coverage valgrind help instdeps format check-format
