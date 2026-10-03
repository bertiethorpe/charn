#ifndef GLTF_LOADER_H
#define GLTF_LOADER_H

#include <stdbool.h>

#include "mesh.h"
#include "texture.h"

bool gltf_load_mesh(
    SDL_GPUDevice *device,
    Mesh *mesh,
    Texture *base_color_texture,
    float base_color_factor[4],
    const char *path
);

#endif
