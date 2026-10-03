#ifndef DEMO_ASSETS_H
#define DEMO_ASSETS_H

#include "renderer.h"

typedef struct {
    Mesh cube_mesh;
    Mesh floor_mesh;
    Mesh suzanne_mesh;
    Texture checker_texture;
    Texture suzanne_texture;
    Material cube_material;
    Material floor_material;
    Material suzanne_material;
} DemoAssets;

bool demo_assets_init(
    DemoAssets *assets,
    SDL_GPUDevice *device,
    const char *asset_root
);

void demo_assets_destroy(DemoAssets *assets, SDL_GPUDevice *device);

#endif
