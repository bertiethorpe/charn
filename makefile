# Compiler
CC = clang

# Output binary
TARGET = test

# Source files
SRC = src/main.c src/math3d.c src/mesh.c
HEADERS = src/math3d.h src/mesh.h

# Compiler & linker flags from pkg-config
CFLAGS = -Wall -Wextra -std=c99 $(shell pkg-config --cflags sdl3)
LDFLAGS = $(shell pkg-config --libs sdl3)

# Build target
all: $(TARGET)

$(TARGET): $(SRC) $(HEADERS)
	$(CC) $(SRC) -o $(TARGET) $(CFLAGS) $(LDFLAGS)

# Clean build files
clean:
	rm -f $(TARGET)

# Run the program
run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run
