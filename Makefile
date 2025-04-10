# Makefile for the compression encoding project.

# Compiler and flags.
CXX      = g++
CXXFLAGS = -std=c++11 -O2 -Wall

# Directories.
SRC_DIR   = src
BUILD_DIR = build

# Target executable.
TARGET    = $(BUILD_DIR)/test_encodings

# Source and object files.
SOURCES   = $(wildcard $(SRC_DIR)/*.cpp)
OBJECTS   = $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%.o, $(SOURCES))

# Default target: build the executable.
all: $(TARGET)

# Link object files to create the target executable.
$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJECTS)

# Compile each source file into an object file in the build directory.
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Ensure the build directory exists.
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Target to run the executable.
run: $(TARGET)
	./$(TARGET)

# Clean object files and executable.
clean:
	rm -rf $(BUILD_DIR)

