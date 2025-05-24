# Makefile for Virtual Memory Manager Simulator

# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -g
GTKFLAGS = `pkg-config --cflags --libs gtk+-3.0`

# Source files
SOURCES = main.c vm_core.c vm_gui.c
OBJECTS = $(SOURCES:.c=.o)
HEADERS = vm_simulator.h

# Target executable
TARGET = vm_simulator

# Default target
all: $(TARGET)

# Build the main executable
$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(TARGET) $(GTKFLAGS)

# Compile source files
%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) `pkg-config --cflags gtk+-3.0` -c $< -o $@

# Clean build files
clean:
	rm -f $(OBJECTS) $(TARGET)

# Install dependencies (Ubuntu/Debian)
install-deps:
	sudo apt-get update
	sudo apt-get install build-essential pkg-config libgtk-3-dev

# Install dependencies (CentOS/RHEL/Fedora)
install-deps-rpm:
	sudo yum install gcc pkg-config gtk3-devel
	# For newer versions: sudo dnf install gcc pkg-config gtk3-devel

# Run the simulator
run: $(TARGET)
	./$(TARGET)

# Debug build
debug: CFLAGS += -DDEBUG -O0
debug: $(TARGET)

# Release build
release: CFLAGS += -O2 -DNDEBUG
release: $(TARGET)

# Check for memory leaks (requires valgrind)
valgrind: $(TARGET)
	valgrind --leak-check=full --show-leak-kinds=all ./$(TARGET)

# Create documentation (requires doxygen)
docs:
	doxygen Doxyfile

# Phony targets
.PHONY: all clean install-deps install-deps-rpm run debug release valgrind docs

# Help target
help:
	@echo "Available targets:"
	@echo "  all           - Build the simulator (default)"
	@echo "  clean         - Remove build files"
	@echo "  install-deps  - Install dependencies on Ubuntu/Debian"
	@echo "  install-deps-rpm - Install dependencies on CentOS/RHEL/Fedora"
	@echo "  run           - Build and run the simulator"
	@echo "  debug         - Build with debug flags"
	@echo "  release       - Build optimized release version"
	@echo "  valgrind      - Run with memory leak detection"
	@echo "  docs          - Generate documentation"
	@echo "  help          - Show this help message"