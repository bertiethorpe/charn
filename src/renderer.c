#include "renderer.h"

bool renderer_ensure_depth_texture(
    Renderer *renderer,
    Uint32 width,
    Uint32 height
) {
    if (!renderer ||
        !renderer->device ||
        width == 0 ||
        height == 0) {
        SDL_Log("Cannot create depth texture with invalid arguments");
        return false;
    }

    if (renderer->depth_texture &&
        renderer->depth_texture_width == width &&
        renderer->depth_texture_height == height) {
        return true;
    }

    if (renderer->depth_texture) {
        SDL_ReleaseGPUTexture(
            renderer->device,
            renderer->depth_texture
        );

        renderer->depth_texture = NULL;
    }

    SDL_GPUTextureCreateInfo texture_info = {
        .type = SDL_GPU_TEXTURETYPE_2D,
        .format = renderer->depth_texture_format,
        .usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET,
        .width = width,
        .height = height,
        .layer_count_or_depth = 1,
        .num_levels = 1,
        .sample_count = SDL_GPU_SAMPLECOUNT_1
    };

    renderer->depth_texture = SDL_CreateGPUTexture(
        renderer->device,
        &texture_info
    );

    if (!renderer->depth_texture) {
        renderer->depth_texture_width = 0;
        renderer->depth_texture_height = 0;

        SDL_Log(
            "Could not create depth texture: %s",
            SDL_GetError()
        );

        return false;
    }

    renderer->depth_texture_width = width;
    renderer->depth_texture_height = height;

    return true;
}

void renderer_destroy(
    Renderer *renderer,
    SDL_Window *window
) {
    if (!renderer) {
        return;
    }

    if (renderer->device) {
        if (renderer->filled_pipeline) {
            SDL_ReleaseGPUGraphicsPipeline(
                renderer->device,
                renderer->filled_pipeline
            );
        }

        if (renderer->wireframe_pipeline) {
            SDL_ReleaseGPUGraphicsPipeline(
                renderer->device,
                renderer->wireframe_pipeline
            );
        }

        if (renderer->depth_texture) {
            SDL_ReleaseGPUTexture(
                renderer->device,
                renderer->depth_texture
            );
        }

        if (window) {
            SDL_ReleaseWindowFromGPUDevice(
                renderer->device,
                window
            );
        }

        SDL_DestroyGPUDevice(renderer->device);
    }

    *renderer = (Renderer){0};
}
