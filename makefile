# Compiler
CC = clang

# Output binary
TARGET = test

# Source files
SRC = src/main.c src/math3d.c src/mesh.c src/texture.c src/shader.c src/pipeline.c \
	  src/renderer.c src/transform.c

HEADERS = src/math3d.h src/mesh.h src/texture.h src/shader.h src/pipeline.h \
		  src/renderer.h src/transform.h

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
