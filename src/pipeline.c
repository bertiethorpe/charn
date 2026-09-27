#include <stddef.h>

#include "mesh.h"
#include "pipeline.h"

SDL_GPUGraphicsPipeline *pipeline_create(
    SDL_GPUDevice *device,
    SDL_GPUShader *vertex_shader,
    SDL_GPUShader *fragment_shader,
    SDL_GPUTextureFormat color_format,
    SDL_GPUTextureFormat depth_format,
    SDL_GPUFillMode fill_mode
) {
    if (!device || !vertex_shader || !fragment_shader) {
        SDL_Log("Cannot create pipeline with invalid arguments");
        return NULL;
    }

    SDL_GPUColorTargetDescription color_target_description = {
        .format = color_format
    };

    SDL_GPUVertexBufferDescription vertex_buffer_description = {
        .slot       = 0,
        .pitch      = (Uint32)sizeof(Vertex),
        .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX
    };

    SDL_GPUVertexAttribute vertex_attributes[3] = {
        {
            .location = 0,
            .buffer_slot = 0,
            .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
            .offset = (Uint32)offsetof(Vertex, position)
        },
        {
            .location = 1,
            .buffer_slot = 0,
            .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
            .offset = (Uint32)offsetof(Vertex, normal)
        },
        {
            .location = 2,
            .buffer_slot = 0,
            .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
            .offset = (Uint32)offsetof(Vertex, uv)
        }
    };

    SDL_GPUGraphicsPipelineCreateInfo pipeline_info = {0};

    pipeline_info.vertex_shader = vertex_shader;
    pipeline_info.fragment_shader = fragment_shader;
    pipeline_info.primitive_type =
        SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;

    pipeline_info.rasterizer_state.fill_mode = fill_mode;
    pipeline_info.rasterizer_state.cull_mode =
        SDL_GPU_CULLMODE_BACK;
    pipeline_info.rasterizer_state.front_face =
        SDL_GPU_FRONTFACE_CLOCKWISE;

    pipeline_info.depth_stencil_state.compare_op =
        SDL_GPU_COMPAREOP_LESS;
    pipeline_info.depth_stencil_state.enable_depth_test = true;
    pipeline_info.depth_stencil_state.enable_depth_write = true;

    pipeline_info.vertex_input_state.vertex_buffer_descriptions =
        &vertex_buffer_description;
    pipeline_info.vertex_input_state.num_vertex_buffers = 1;
    pipeline_info.vertex_input_state.vertex_attributes =
        vertex_attributes;
    pipeline_info.vertex_input_state.num_vertex_attributes = 3;

    pipeline_info.target_info.num_color_targets = 1;
    pipeline_info.target_info.color_target_descriptions =
        &color_target_description;
    pipeline_info.target_info.has_depth_stencil_target = true;
    pipeline_info.target_info.depth_stencil_format = depth_format;

    SDL_GPUGraphicsPipeline *pipeline =
        SDL_CreateGPUGraphicsPipeline(
            device,
            &pipeline_info
        );

    if (!pipeline) {
        SDL_Log(
            "Could not create graphics pipeline: %s",
            SDL_GetError()
        );
        return NULL;
    }

    return pipeline;
}
