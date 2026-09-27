#include "renderer.h"

#include "pipeline.h"
#include "shader.h"

bool renderer_init(
    Renderer *renderer,
    SDL_Window *window
) {
    if (!renderer || !window) {
        SDL_Log("Cannot initialize renderer with invalid arguments");
        return false;
    }

    if (renderer->device) {
        SDL_Log("Renderer is already initialized");
        return false;
    }

    *renderer = (Renderer){
        .depth_texture_format = SDL_GPU_TEXTUREFORMAT_D16_UNORM
    };

    renderer->device = SDL_CreateGPUDevice(
        SDL_GPU_SHADERFORMAT_MSL,
        true,
        NULL
    );

    if (!renderer->device) {
        SDL_Log(
            "GPU device creation failed: %s",
            SDL_GetError()
        );
        return false;
    }

    if (!SDL_ClaimWindowForGPUDevice(
            renderer->device,
            window)) {
        SDL_Log(
            "Could not claim window for GPU: %s",
            SDL_GetError()
        );
        renderer_destroy(renderer, window);
        return false;
    }

    renderer->window_claimed = true;

    SDL_GPUShader *vertex_shader = shader_load_msl(
        renderer->device,
        "shaders/triangle.vert.msl",
        "vertex_main",
        SDL_GPU_SHADERSTAGE_VERTEX,
        0,
        1
    );

    if (!vertex_shader) {
        renderer_destroy(renderer, window);
        return false;
    }

    SDL_GPUShader *fragment_shader = shader_load_msl(
        renderer->device,
        "shaders/triangle.frag.msl",
        "fragment_main",
        SDL_GPU_SHADERSTAGE_FRAGMENT,
        1,
        2
    );

    if (!fragment_shader) {
        SDL_ReleaseGPUShader(
            renderer->device,
            vertex_shader
        );
        renderer_destroy(renderer, window);
        return false;
    }

    SDL_GPUShader *wireframe_fragment_shader =
        shader_load_msl(
            renderer->device,
            "shaders/wireframe.frag.msl",
            "fragment_main",
            SDL_GPU_SHADERSTAGE_FRAGMENT,
            0,
            0
        );

    if (!wireframe_fragment_shader) {
        SDL_ReleaseGPUShader(
            renderer->device,
            fragment_shader
        );
        SDL_ReleaseGPUShader(
            renderer->device,
            vertex_shader
        );
        renderer_destroy(renderer, window);
        return false;
    }

    SDL_GPUTextureFormat color_format =
        SDL_GetGPUSwapchainTextureFormat(
            renderer->device,
            window
        );

    renderer->filled_pipeline = pipeline_create(
        renderer->device,
        vertex_shader,
        fragment_shader,
        color_format,
        renderer->depth_texture_format,
        SDL_GPU_FILLMODE_FILL
    );

    if (!renderer->filled_pipeline) {
        SDL_ReleaseGPUShader(
            renderer->device,
            wireframe_fragment_shader
        );
        SDL_ReleaseGPUShader(
            renderer->device,
            fragment_shader
        );
        SDL_ReleaseGPUShader(
            renderer->device,
            vertex_shader
        );
        renderer_destroy(renderer, window);
        return false;
    }

    renderer->wireframe_pipeline = pipeline_create(
        renderer->device,
        vertex_shader,
        wireframe_fragment_shader,
        color_format,
        renderer->depth_texture_format,
        SDL_GPU_FILLMODE_LINE
    );

    SDL_ReleaseGPUShader(
        renderer->device,
        wireframe_fragment_shader
    );
    SDL_ReleaseGPUShader(
        renderer->device,
        fragment_shader
    );
    SDL_ReleaseGPUShader(
        renderer->device,
        vertex_shader
    );

    if (!renderer->wireframe_pipeline) {
        renderer_destroy(renderer, window);
        return false;
    }

    SDL_Log("Created fill and wireframe graphics pipelines");

    return true;
}

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

        if (renderer->window_claimed && window) {
            SDL_ReleaseWindowFromGPUDevice(
                renderer->device,
                window
            );
        }

        SDL_DestroyGPUDevice(renderer->device);
    }

    *renderer = (Renderer){0};
}
