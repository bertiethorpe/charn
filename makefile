# Compiler
CC = clang

# Output binary
TARGET = build/test

SHADER_SOURCES = shaders/triangle.vert.msl shaders/triangle.frag.msl \
				 shaders/wireframe.frag.msl

SHADER_OUTPUTS = $(patsubst shaders/%,build/shaders/%,$(SHADER_SOURCES))

# Source files
SRC = src/main.c src/demo_assets.c src/demo_scene.c src/math3d.c \
	  src/mesh.c src/texture.c src/shader.c src/pipeline.c src/renderer.c \
	  src/transform.c src/world.c

HEADERS = src/demo_assets.h src/demo_scene.h src/math3d.h src/mesh.h \
		  src/texture.h src/shader.h src/pipeline.h src/renderer.h \
		  src/transform.h src/world.h

# Compiler & linker flags from pkg-config
CFLAGS = -Wall -Wextra -std=c99 $(shell pkg-config --cflags sdl3)
LDFLAGS = $(shell pkg-config --libs sdl3)

# Build target
all: $(TARGET) $(SHADER_OUTPUTS)

$(TARGET): $(SRC) $(HEADERS)
	mkdir -p build
	$(CC) $(SRC) -o $(TARGET) $(CFLAGS) $(LDFLAGS)

build/shaders/%: shaders/%
	mkdir -p build/shaders
	cp $< $@

# Clean build files
clean:
	rm -rf build

# Run the program
run: all
	./$(TARGET)

.PHONY: all clean run
