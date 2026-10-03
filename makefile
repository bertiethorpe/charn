# Compiler
CC = clang

# Output binary
TARGET = build/test

MODEL_SOURCES = assets/models/Suzanne.gltf assets/models/Suzanne.bin \
				assets/models/Suzanne_BaseColor.png \
				assets/models/Suzanne_MetallicRoughness.png

MODEL_OUTPUTS = $(patsubst assets/%,build/%,$(MODEL_SOURCES))

SHADER_SOURCES = shaders/triangle.vert.msl shaders/triangle.frag.msl \
				 shaders/wireframe.frag.msl

SHADER_OUTPUTS = $(patsubst shaders/%,build/shaders/%,$(SHADER_SOURCES))

# Source files
SRC = src/main.c src/game.c src/demo_assets.c src/demo_scene.c src/math3d.c \
	  src/mesh.c src/texture.c src/shader.c src/pipeline.c src/renderer.c \
	  src/transform.c src/world.c src/gltf_loader.c

HEADERS = src/game.h src/demo_assets.h src/demo_scene.h src/math3d.h src/mesh.h \
		  src/texture.h src/shader.h src/pipeline.h src/renderer.h \
		  src/transform.h src/world.h src/gltf_loader.h third_party/cgltf.h

# Compiler & linker flags from pkg-config
CFLAGS = -Wall -Wextra -std=c99 -Ithird_party $(shell pkg-config --cflags sdl3 sdl3-image)
LDFLAGS = $(shell pkg-config --libs sdl3 sdl3-image)

# Build target
all: $(TARGET) $(SHADER_OUTPUTS) $(MODEL_OUTPUTS)

$(TARGET): $(SRC) $(HEADERS)
	mkdir -p build
	$(CC) $(SRC) -o $(TARGET) $(CFLAGS) $(LDFLAGS)

build/shaders/%: shaders/%
	mkdir -p build/shaders
	cp $< $@

build/models/%: assets/models/%
	mkdir -p build/models
	cp $< $@

# Clean build files
clean:
	rm -rf build

# Run the program
run: all
	./$(TARGET)

.PHONY: all clean run
