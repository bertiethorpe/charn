#ifndef DEMO_ASSETS_H
#define DEMO_ASSETS_H

#include "renderer.h"

typedef struct {
    Mesh cube_mesh;
    Mesh floor_mesh;
    Mesh suzanne_mesh;
    Texture checker_texture;
    Material cube_material;
    Material floor_material;
} DemoAssets;

bool demo_assets_init(DemoAssets *assets, SDL_GPUDevice *device);
void demo_assets_destroy(DemoAssets *assets, SDL_GPUDevice *device);

#endif
