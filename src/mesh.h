#ifndef MESH_H
#define MESH_H

#include <stdbool.h>

#include <SDL3/SDL.h>

typedef struct {
    float position[3];
    float normal[3];
    float uv[2];
} Vertex;

typedef struct {
    SDL_GPUBuffer *vertex_buffer;
    SDL_GPUBuffer *index_buffer;
    Uint32 index_count;
} Mesh;

bool mesh_create(
    SDL_GPUDevice *device,
    Mesh *mesh,
    const Vertex *vertices,
    Uint32 vertex_count,
    const Uint16 *indices,
    Uint32 index_count
);

void mesh_destroy(
    SDL_GPUDevice *device,
    Mesh *mesh
);

#endif
