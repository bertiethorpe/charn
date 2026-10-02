#ifndef GLTF_LOADER_H
#define GLTF_LOADER_H

#include <stdbool.h>

#include "mesh.h"

bool gltf_load_mesh(
    SDL_GPUDevice *device,
    Mesh *mesh,
    const char *relative_path
);

#endif
