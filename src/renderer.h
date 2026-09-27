#ifndef RENDERER_H
#define RENDERER_H

#include <stdbool.h>

#include <SDL3/SDL.h>

typedef struct {
    SDL_GPUDevice *device;
    bool window_claimed;

    SDL_GPUGraphicsPipeline *filled_pipeline;
    SDL_GPUGraphicsPipeline *wireframe_pipeline;

    SDL_GPUTexture *depth_texture;
    SDL_GPUTextureFormat depth_texture_format;
    Uint32 depth_texture_width;
    Uint32 depth_texture_height;
} Renderer;

bool renderer_init(
    Renderer *renderer,
    SDL_Window *window
);

bool renderer_ensure_depth_texture(
    Renderer *renderer,
    Uint32 width,
    Uint32 height
);

void renderer_destroy(
    Renderer *renderer,
    SDL_Window *window
);

#endif
