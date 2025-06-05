# Set the cross-compilation prefix if CROSS_COMPILE is defined
CROSS_COMPILE ?=  # If not set, native compiler will be used

# Compiler and flags
CXX = $(CROSS_COMPILE)g++  # If CROSS_COMPILE is set, it will prefix the compiler
CC  = $(CROSS_COMPILE)gcc   # C compiler (for C programs)
CXXFLAGS = -std=c++17 -Wall -pthread
CXXDEBUG = -std=c++17 -Wall -g -pthread



# Target executable
TARGET = writer

# Source files
SRC = writer.cpp

# Object files (convert .cpp files to .o)
OBJ = $(SRC:.cpp=.o)

# Default target (what make builds when no target is specified)
all: $(TARGET)
	@echo "CROSS_COMPILE is $(CROSS_COMPILE)"

# Rule for linking object files into the final executable
$(TARGET): $(OBJ)
	@$(CXX) $(CXXFLAGS) $(OBJ) -o $(TARGET)

# Rule for compiling .cpp files into .o files using CXXFLAGS
%.o: %.cpp
	@$(CXX) $(CXXFLAGS) -c $< -o $@

# Debug build target
debug: CXXFLAGS = $(CXXDEBUG)
debug: $(OBJ)
	@$(CXX) $(CXXFLAGS) $(OBJ) -o $(TARGET)

# Clean up object and executable files
clean:
	@rm -f $(OBJ) $(TARGET)

# Phony targets (not actual files)
.PHONY: all clean debug
