#include "renderer.h"

#include "pipeline.h"
#include "shader.h"

typedef struct {
    Mat4 transform;
    Mat4 model;
    Mat4 normal_matrix;
} VertexUniforms;

typedef struct {
    float texture_mix[4];
    float light_position[4];
    float ambient_strength;
    float point_light_strength;
    float falloff_distance;
    float padding;
} FragmentUniforms;

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

static void draw_render_object(
    SDL_GPUCommandBuffer *command_buffer,
    SDL_GPURenderPass *render_pass,
    const RenderObject *object,
    const Transform *transform,
    Mat4 view,
    Mat4 projection,
    bool show_wireframe
) {
    const Mesh *mesh = object->mesh;
    const Material *material = object->material;

    SDL_GPUBufferBinding vertex_binding = {
        .buffer = mesh->vertex_buffer,
        .offset = 0
    };

    SDL_BindGPUVertexBuffers(
        render_pass,
        0,
        &vertex_binding,
        1
    );

    SDL_GPUBufferBinding index_binding = {
        .buffer = mesh->index_buffer,
        .offset = 0
    };

    SDL_BindGPUIndexBuffer(
        render_pass,
        &index_binding,
        SDL_GPU_INDEXELEMENTSIZE_16BIT
    );

    if (!show_wireframe) {
        const Texture *texture = material->texture;

        SDL_GPUTextureSamplerBinding material_binding = {
            .texture = texture->texture,
            .sampler = texture->sampler
        };

        SDL_BindGPUFragmentSamplers(
            render_pass,
            0,
            &material_binding,
            1
        );
    }

    Mat4 model = transform_to_matrix(*transform);

    Mat4 normal_matrix;
    if (!mat4_normal_matrix(model, &normal_matrix)) {
        return;
    }

    Mat4 view_model = mat4_multiply(view, model);

    VertexUniforms uniforms = {
        .transform = mat4_multiply(projection, view_model),
        .model = model,
        .normal_matrix = normal_matrix
    };

    SDL_PushGPUVertexUniformData(
        command_buffer,
        0,
        &uniforms,
        sizeof(uniforms)
    );

    if (!show_wireframe) {
        SDL_PushGPUFragmentUniformData(
            command_buffer,
            1,
            material->tint,
            sizeof(material->tint)
        );
    }

    SDL_DrawGPUIndexedPrimitives(
        render_pass,
        mesh->index_count,
        1,
        0,
        0,
        0
    );
}

bool renderer_draw(
    Renderer *renderer,
    SDL_Window *window,
    const World *world,
    Mat4 view,
    SceneLighting lighting,
    bool show_texture,
    bool show_wireframe,
    const RenderObject *objects,
    size_t object_count
) {
    if (!renderer || !renderer->device || !window || !world ||
        (object_count > 0 && !objects)) {
        SDL_Log("Cannot draw with invalid renderer arguments");
        return false;
    }

    SDL_GPUCommandBuffer *command_buffer =
        SDL_AcquireGPUCommandBuffer(renderer->device);

    if (!command_buffer) {
        SDL_Log("Could not acquire command buffer: %s", SDL_GetError());
        return false;
    }

    SDL_GPUTexture *swapchain_texture = NULL;
    Uint32 swapchain_width = 0;
    Uint32 swapchain_height = 0;

    if (!SDL_WaitAndAcquireGPUSwapchainTexture(
            command_buffer,
            window,
            &swapchain_texture,
            &swapchain_width,
            &swapchain_height)) {
        SDL_Log("Could not acquire swapchain texture: %s", SDL_GetError());
        SDL_CancelGPUCommandBuffer(command_buffer);
        return false;
    }

    // A minimized window may not have a swapchain texture.
    if (swapchain_texture) {
        if (!renderer_ensure_depth_texture(
                renderer,
                swapchain_width,
                swapchain_height)) {
            if (!SDL_SubmitGPUCommandBuffer(command_buffer)) {
                SDL_Log(
                    "Could not submit failed frame: %s",
                    SDL_GetError()
                );
            }
            return false;
        }

        SDL_GPUColorTargetInfo color_target = {0};

        color_target.texture = swapchain_texture;
        color_target.clear_color =
            (SDL_FColor){100.0f / 255.0f,
                        149.0f / 255.0f,
                        237.0f / 255.0f,
                        1.0f};
        color_target.load_op = SDL_GPU_LOADOP_CLEAR;
        color_target.store_op = SDL_GPU_STOREOP_STORE;

        SDL_GPUDepthStencilTargetInfo depth_target = {0};

        depth_target.texture = renderer->depth_texture;
        depth_target.clear_depth = 1.0f;
        depth_target.load_op = SDL_GPU_LOADOP_CLEAR;
        depth_target.store_op = SDL_GPU_STOREOP_DONT_CARE;
        depth_target.stencil_load_op = SDL_GPU_LOADOP_DONT_CARE;
        depth_target.stencil_store_op = SDL_GPU_STOREOP_DONT_CARE;
        depth_target.cycle = true;

        SDL_GPURenderPass *render_pass =
            SDL_BeginGPURenderPass(
                command_buffer,
                &color_target,
                1,
                &depth_target
            );

        if (!render_pass) {
            SDL_Log(
                "Could not begin GPU render pass: %s",
                SDL_GetError()
            );
            if (!SDL_SubmitGPUCommandBuffer(command_buffer)) {
                SDL_Log(
                    "Could not submit failed frame: %s",
                    SDL_GetError()
                );
            }
            return false;
        }

        SDL_GPUGraphicsPipeline *active_pipeline =
            show_wireframe
                ? renderer->wireframe_pipeline
                : renderer->filled_pipeline;

        SDL_BindGPUGraphicsPipeline(render_pass, active_pipeline);

        if (!show_wireframe) {
            FragmentUniforms fragment_uniforms = {
                .texture_mix = {
                    show_texture ? 1.0f : 0.0f,
                    0.0f,
                    0.0f,
                    0.0f
                },
                .light_position = {
                    lighting.light_position.x,
                    lighting.light_position.y,
                    lighting.light_position.z,
                    0.0f
                },
                .ambient_strength = lighting.ambient_strength,
                .point_light_strength = lighting.point_light_strength,
                .falloff_distance = lighting.falloff_distance
            };

            SDL_PushGPUFragmentUniformData(
                command_buffer,
                0,
                &fragment_uniforms,
                sizeof(fragment_uniforms)
            );
        }

        float aspect =
            (float)swapchain_width / (float)swapchain_height;
        float vertical_fov =
            60.0f * (3.14159265359f / 180.0f); // radians

        Mat4 projection = mat4_perspective_projection(
            vertical_fov,
            aspect,
            0.1f,
            100.0f
        );

        for (size_t i = 0; i < object_count; ++i) {
            const Transform *transform = world_get_transform_const(
                world,
                objects[i].entity
            );

            if (!transform) {
                continue;
            }

            draw_render_object(
                command_buffer,
                render_pass,
                &objects[i],
                transform,
                view,
                projection,
                show_wireframe
            );
        }

        SDL_EndGPURenderPass(render_pass);
    }

    if (!SDL_SubmitGPUCommandBuffer(command_buffer)) {
        SDL_Log("Could not submit command buffer: %s", SDL_GetError());
        return false;
    }

    return true;
}
