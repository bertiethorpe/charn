#include "shader.h"

SDL_GPUShader *shader_load_msl(
    SDL_GPUDevice *device,
    const char *path,
    const char *entrypoint,
    SDL_GPUShaderStage stage,
    Uint32 sampler_count,
    Uint32 uniform_buffer_count
) {
    if (!device || !path || !entrypoint) {
        SDL_Log("Cannot load a shader with invalid arguments");
        return NULL;
    }

    size_t shader_code_size = 0;

    Uint8 *shader_code = SDL_LoadFile(
        path,
        &shader_code_size
    );

    if (!shader_code) {
        SDL_Log(
            "Could not load shader '%s': %s",
            path,
            SDL_GetError()
        );
        return NULL;
    }

    SDL_GPUShaderCreateInfo shader_info = {
        .code                = shader_code,
        .code_size           = shader_code_size,
        .entrypoint          = entrypoint,
        .format              = SDL_GPU_SHADERFORMAT_MSL,
        .stage               = stage,
        .num_samplers        = sampler_count,
        .num_uniform_buffers = uniform_buffer_count
    };

    SDL_GPUShader *shader = SDL_CreateGPUShader(
        device,
        &shader_info
    );

    if (!shader) {
        SDL_Log(
            "Could not create shader '%s': %s",
            path,
            SDL_GetError()
        );
        SDL_free(shader_code);
        return NULL;
    }

    SDL_Log(
        "Loaded shader '%s': %zu bytes",
        path,
        shader_code_size
    );

    SDL_free(shader_code);

    return shader;
}
