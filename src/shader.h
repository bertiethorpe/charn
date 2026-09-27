#ifndef SHADER_H
#define SHADER_H

#include <SDL3/SDL.h>

SDL_GPUShader *shader_load_msl(
    SDL_GPUDevice *device,
    const char *path,
    const char *entrypoint,
    SDL_GPUShaderStage stage,
    Uint32 sampler_count,
    Uint32 uniform_buffer_count
);

#endif
