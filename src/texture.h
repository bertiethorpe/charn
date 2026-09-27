#ifndef TEXTURE_H
#define TEXTURE_H

#include <stdbool.h>

#include <SDL3/SDL.h>

typedef struct {
    SDL_GPUTexture *texture;
    SDL_GPUSampler *sampler;
} Texture;

void texture_destroy(
    SDL_GPUDevice *device,
    Texture *texture
);

bool texture_create_rgba8(
    SDL_GPUDevice *device,
    Texture *texture,
    Uint32 width,
    Uint32 height,
    const Uint8 *pixels
);

#endif
