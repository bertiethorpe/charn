#ifndef PIPELINE_H
#define PIPELINE_H

#include <SDL3/SDL.h>

SDL_GPUGraphicsPipeline *pipeline_create(
    SDL_GPUDevice *device,
    SDL_GPUShader *vertex_shader,
    SDL_GPUShader *fragment_shader,
    SDL_GPUTextureFormat color_format,
    SDL_GPUTextureFormat depth_format,
    SDL_GPUFillMode fill_mode
);

#endif