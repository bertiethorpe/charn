#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stddef.h>
#include <math.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "math3d.h"

static const int width = 800;
static const int height = 600;

static SDL_Window *window = NULL;
static SDL_GPUDevice *gpu_device = NULL;

static SDL_GPUShader *vertex_shader = NULL;
static SDL_GPUShader *fragment_shader = NULL;
static SDL_GPUGraphicsPipeline *graphics_pipeline = NULL;
static SDL_GPUBuffer *vertex_buffer = NULL;
static SDL_GPUBuffer *index_buffer = NULL;

static SDL_GPUTexture *depth_texture = NULL;

static const SDL_GPUTextureFormat depth_texture_format =
    SDL_GPU_TEXTUREFORMAT_D16_UNORM;

static Uint32 depth_texture_width = 0;
static Uint32 depth_texture_height = 0;

// ------------------------- Data --------------------------
typedef struct {
    float position[3];
    float color[4];
} Vertex;

typedef struct {
    Mat4 transform;
} VertexUniforms;

typedef struct {
    Vec3 position;
    float yaw;
    float pitch;
} Camera;

static const Uint16 cube_indices[] = {
    0, 1, 2, 0, 2, 3,  // front:  -Z
    4, 7, 6, 4, 6, 5,  // back:   +Z
    4, 5, 1, 4, 1, 0,  // left:   -X
    3, 2, 6, 3, 6, 7,  // right:  +X
    1, 5, 6, 1, 6, 2,  // top:    +Y
    4, 0, 3, 4, 3, 7   // bottom: -Y
};

static const Vertex cube_vertices[] = {
    // Front: z = -0.5
    { .position = {-0.5f, -0.5f, -0.5f},
      .color    = { 1.0f,  0.0f,  0.0f, 1.0f} }, // 0
    { .position = {-0.5f,  0.5f, -0.5f},
      .color    = { 0.0f,  1.0f,  0.0f, 1.0f} }, // 1
    { .position = { 0.5f,  0.5f, -0.5f},
      .color    = { 0.0f,  0.0f,  1.0f, 1.0f} }, // 2
    { .position = { 0.5f, -0.5f, -0.5f},
      .color    = { 1.0f,  1.0f,  0.0f, 1.0f} }, // 3

    // Back: z = +0.5
    { .position = {-0.5f, -0.5f,  0.5f},
      .color    = { 0.0f,  1.0f,  1.0f, 1.0f} }, // 4
    { .position = {-0.5f,  0.5f,  0.5f},
      .color    = { 1.0f,  0.0f,  1.0f, 1.0f} }, // 5
    { .position = { 0.5f,  0.5f,  0.5f},
      .color    = { 1.0f,  1.0f,  1.0f, 1.0f} }, // 6
    { .position = { 0.5f, -0.5f,  0.5f},
      .color    = { 1.0f,  0.5f,  0.0f, 1.0f} }  // 7
};

static bool ensure_depth_texture(
    Uint32 texture_width,
    Uint32 texture_height
) {
    if (depth_texture &&
        depth_texture_width == texture_width &&
        depth_texture_height == texture_height) {
        return true;
    }

    if (depth_texture) {
        SDL_ReleaseGPUTexture(gpu_device, depth_texture);
        depth_texture = NULL;
    }
    
    SDL_GPUTextureCreateInfo texture_info = {0};

    texture_info.type = SDL_GPU_TEXTURETYPE_2D;
    texture_info.format = depth_texture_format;
    texture_info.usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET;
    texture_info.width = texture_width;
    texture_info.height = texture_height;
    texture_info.layer_count_or_depth = 1;
    texture_info.num_levels = 1;
    texture_info.sample_count = SDL_GPU_SAMPLECOUNT_1;

    depth_texture =
        SDL_CreateGPUTexture(gpu_device, &texture_info);

    if (!depth_texture) {
        depth_texture_width = 0;
        depth_texture_height = 0;

        SDL_Log(
            "Could not create depth texture: %s",
            SDL_GetError()
        );

        return false;
    }

    depth_texture_width = texture_width;
    depth_texture_height = texture_height;

    return true;
}

// -------------------- Init / Shutdown --------------------
void shutdown(void);

bool init(void) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL_Init Error: %s", SDL_GetError());
        return false;
    }

    window = SDL_CreateWindow(
        "3D Engine",
        width,
        height,
        SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY
    );

    if (!window) {
        SDL_Log("Window creation failed: %s", SDL_GetError());
        shutdown();
        return false;
    }

    gpu_device = SDL_CreateGPUDevice(
        SDL_GPU_SHADERFORMAT_MSL,
        true, // enable GPU validation for dev.
        NULL // default device
    );

    if (!gpu_device) {
        SDL_Log("GPU device creation failed: %s", SDL_GetError());
        shutdown();
        return false;
    }

    if (!SDL_ClaimWindowForGPUDevice(gpu_device, window)) {
        SDL_Log("Could not claim window for GPU: %s", SDL_GetError());
        shutdown();
        return false;
    }

    const Uint32 vertex_data_size = (Uint32)sizeof(cube_vertices);
    const Uint32 index_data_size = (Uint32)sizeof(cube_indices);

    SDL_GPUBufferCreateInfo vertex_buffer_info = {0};
    vertex_buffer_info.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
    vertex_buffer_info.size = vertex_data_size;

    vertex_buffer = SDL_CreateGPUBuffer(gpu_device, &vertex_buffer_info);
    if (!vertex_buffer) {
        SDL_Log("Could not create vertex buffer: %s", SDL_GetError());
        shutdown();
        return false;
    }

    SDL_GPUBufferCreateInfo index_info = {0};
    index_info.usage = SDL_GPU_BUFFERUSAGE_INDEX;
    index_info.size = index_data_size;

    index_buffer = SDL_CreateGPUBuffer(gpu_device, &index_info);
    if (!index_buffer) {
        SDL_Log("Could not create index buffer: %s", SDL_GetError());
        shutdown();
        return false;
    }

    SDL_GPUTransferBufferCreateInfo transfer_info = {0};
    transfer_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transfer_info.size = vertex_data_size + index_data_size;

    SDL_GPUTransferBuffer *transfer_buffer = SDL_CreateGPUTransferBuffer(
        gpu_device,
        &transfer_info
    );

    if (!transfer_buffer) {
        SDL_Log("Could not create transfer buffer: %s", SDL_GetError());
        shutdown();
        return false;
    }

    void *mapped_data = SDL_MapGPUTransferBuffer(
        gpu_device,
        transfer_buffer,
        false
    );

    if (!mapped_data) {
        SDL_Log("Could not map transfer buffer: %s", SDL_GetError());
        SDL_ReleaseGPUTransferBuffer(gpu_device, transfer_buffer);
        shutdown();
        return false;
    }

    SDL_memcpy(
        mapped_data,
        cube_vertices,
        vertex_data_size
    );

    SDL_memcpy(
        (Uint8 *)mapped_data + vertex_data_size,
        cube_indices,
        index_data_size
    );

    SDL_UnmapGPUTransferBuffer(gpu_device, transfer_buffer);

    SDL_GPUCommandBuffer *upload_commands =
        SDL_AcquireGPUCommandBuffer(gpu_device);

    if (!upload_commands) {
        SDL_Log("Could not acquire upload command buffer: %s", SDL_GetError());
        SDL_ReleaseGPUTransferBuffer(gpu_device, transfer_buffer);
        shutdown();
        return false;
    }

    SDL_GPUCopyPass *copy_pass =
        SDL_BeginGPUCopyPass(upload_commands);

    SDL_GPUTransferBufferLocation source = {0};
    source.transfer_buffer = transfer_buffer;
    source.offset = 0;

    SDL_GPUBufferRegion destination = {0};
    destination.buffer = vertex_buffer;
    destination.offset = 0;
    destination.size = vertex_data_size;

    SDL_UploadToGPUBuffer(
        copy_pass,
        &source,
        &destination,
        false
    );

    SDL_GPUTransferBufferLocation index_source = {0};
    index_source.transfer_buffer = transfer_buffer;
    index_source.offset = vertex_data_size;

    SDL_GPUBufferRegion index_destination = {0};
    index_destination.buffer = index_buffer;
    index_destination.offset = 0;
    index_destination.size = index_data_size;

    SDL_UploadToGPUBuffer(
        copy_pass,
        &index_source,
        &index_destination,
        false
    );

    SDL_EndGPUCopyPass(copy_pass);

    if (!SDL_SubmitGPUCommandBuffer(upload_commands)) {
        SDL_Log("Could not submit vertex upload: %s", SDL_GetError());
        SDL_ReleaseGPUTransferBuffer(gpu_device, transfer_buffer);
        shutdown();
        return false;
    }

    SDL_ReleaseGPUTransferBuffer(gpu_device, transfer_buffer);

    SDL_Log("Uploaded face vertices");

    size_t vertex_shader_size = 0;
    Uint8 *vertex_shader_code = SDL_LoadFile(
        "shaders/triangle.vert.msl",
        &vertex_shader_size
    );

    if (!vertex_shader_code) {
        SDL_Log("Could not load vertex shader: %s", SDL_GetError());
        shutdown();
        return false;
    }

    size_t fragment_shader_size = 0;
    Uint8 *fragment_shader_code = SDL_LoadFile(
        "shaders/triangle.frag.msl",
        &fragment_shader_size
    );

    if (!fragment_shader_code) {
        SDL_Log("Could not load fragment shader: %s", SDL_GetError());
        SDL_free(vertex_shader_code);
        shutdown();
        return false;
    }

    SDL_Log("Loaded vertex shader: %zu bytes", vertex_shader_size);
    SDL_Log("Loaded fragment shader: %zu bytes", fragment_shader_size);

    SDL_GPUShaderCreateInfo vertex_shader_info = {0};
    vertex_shader_info.code = vertex_shader_code;
    vertex_shader_info.code_size = vertex_shader_size;
    vertex_shader_info.entrypoint = "vertex_main";
    vertex_shader_info.format = SDL_GPU_SHADERFORMAT_MSL;
    vertex_shader_info.stage = SDL_GPU_SHADERSTAGE_VERTEX;
    vertex_shader_info.num_uniform_buffers = 1;

    vertex_shader = SDL_CreateGPUShader(
        gpu_device,
        &vertex_shader_info
    );

    if (!vertex_shader) {
        SDL_Log("Could not create vertex shader: %s", SDL_GetError());
        SDL_free(vertex_shader_code);
        SDL_free(fragment_shader_code);
        shutdown();
        return false;
    }

    SDL_GPUShaderCreateInfo fragment_shader_info = {0};
    fragment_shader_info.code = fragment_shader_code;
    fragment_shader_info.code_size = fragment_shader_size;
    fragment_shader_info.entrypoint = "fragment_main";
    fragment_shader_info.format = SDL_GPU_SHADERFORMAT_MSL;
    fragment_shader_info.stage = SDL_GPU_SHADERSTAGE_FRAGMENT;

    fragment_shader = SDL_CreateGPUShader(
        gpu_device,
        &fragment_shader_info
    );

    if (!fragment_shader) {
        SDL_Log("Could not create fragment shader: %s", SDL_GetError());
        SDL_free(vertex_shader_code);
        SDL_free(fragment_shader_code);
        shutdown();
        return false;
    }

    SDL_Log("Created both GPU shaders");

    SDL_free(vertex_shader_code);
    SDL_free(fragment_shader_code);

    SDL_GPUColorTargetDescription color_target_description = {0};
    color_target_description.format =
        SDL_GetGPUSwapchainTextureFormat(gpu_device, window);

    SDL_GPUGraphicsPipelineCreateInfo pipeline_info = {0};

    SDL_GPUVertexBufferDescription vertex_buffer_description = {0};
    vertex_buffer_description.slot = 0;
    vertex_buffer_description.pitch = sizeof(Vertex);
    vertex_buffer_description.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;

    SDL_GPUVertexAttribute vertex_attributes[2] = {0};

    vertex_attributes[0].location = 0;
    vertex_attributes[0].buffer_slot = 0;
    vertex_attributes[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
    vertex_attributes[0].offset = (Uint32)offsetof(Vertex, position);

    vertex_attributes[1].location = 1;
    vertex_attributes[1].buffer_slot = 0;
    vertex_attributes[1].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4;
    vertex_attributes[1].offset = (Uint32)offsetof(Vertex, color);

    pipeline_info.vertex_shader = vertex_shader;
    pipeline_info.fragment_shader = fragment_shader;
    pipeline_info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    pipeline_info.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
    pipeline_info.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_BACK;
    pipeline_info.rasterizer_state.front_face = SDL_GPU_FRONTFACE_CLOCKWISE;
    pipeline_info.depth_stencil_state.compare_op = SDL_GPU_COMPAREOP_LESS;
    pipeline_info.depth_stencil_state.enable_depth_test = true;
    pipeline_info.depth_stencil_state.enable_depth_write = true;

    pipeline_info.vertex_input_state.vertex_buffer_descriptions =
        &vertex_buffer_description;
    pipeline_info.vertex_input_state.num_vertex_buffers = 1;
    pipeline_info.vertex_input_state.vertex_attributes = vertex_attributes;
    pipeline_info.vertex_input_state.num_vertex_attributes = 2;

    pipeline_info.target_info.num_color_targets = 1;
    pipeline_info.target_info.color_target_descriptions =
        &color_target_description;
    pipeline_info.target_info.has_depth_stencil_target = true;
    pipeline_info.target_info.depth_stencil_format = depth_texture_format;

    graphics_pipeline = SDL_CreateGPUGraphicsPipeline(
        gpu_device,
        &pipeline_info
    );

    if (!graphics_pipeline) {
        SDL_Log("Could not create graphics pipeline: %s", SDL_GetError());
        shutdown();
        return false;
    }

    SDL_Log("Created graphics pipeline");
    
    SDL_ReleaseGPUShader(gpu_device, vertex_shader);
    SDL_ReleaseGPUShader(gpu_device, fragment_shader);

    vertex_shader = NULL;
    fragment_shader = NULL;

    SDL_SetWindowPosition(window, 50, 100);
    return true;
}

void shutdown(void) {
    if (gpu_device) {
        if (graphics_pipeline) {
            SDL_ReleaseGPUGraphicsPipeline(
                gpu_device,
                graphics_pipeline
            );
        }

        if (vertex_shader) {
            SDL_ReleaseGPUShader(gpu_device, vertex_shader);
        }

        if (fragment_shader) {
            SDL_ReleaseGPUShader(gpu_device, fragment_shader);
        }

        if (vertex_buffer) {
            SDL_ReleaseGPUBuffer(gpu_device, vertex_buffer);
        }

        if (index_buffer) {
            SDL_ReleaseGPUBuffer(gpu_device, index_buffer);
        }

        if (depth_texture) {
            SDL_ReleaseGPUTexture(gpu_device, depth_texture);
        }
    }

    if (gpu_device && window) {
        SDL_ReleaseWindowFromGPUDevice(gpu_device, window);
    }

    SDL_DestroyGPUDevice(gpu_device);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

// -------------------- Input --------------------
void process_input(bool *quit) {
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT) {
            *quit = true;
        }
        else if (event.type == SDL_EVENT_KEY_DOWN) {
            if (event.key.scancode == SDL_SCANCODE_ESCAPE) {
                *quit = true;
            }
        }
    }
}

// -------------------- Render --------------------
bool render(
    float angle_x,
    float angle_y,
    Camera camera
) {
    SDL_GPUCommandBuffer *command_buffer = 
        SDL_AcquireGPUCommandBuffer(gpu_device);

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
        if (!ensure_depth_texture(
                swapchain_width,
                swapchain_height)) {
            SDL_CancelGPUCommandBuffer(command_buffer);
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

        depth_target.texture = depth_texture;
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

        SDL_BindGPUGraphicsPipeline(
            render_pass,
            graphics_pipeline
        );

        SDL_GPUBufferBinding vertex_binding = {0};
        vertex_binding.buffer = vertex_buffer;
        vertex_binding.offset = 0;

        SDL_BindGPUVertexBuffers(
            render_pass,
            0,
            &vertex_binding,
            1
        );

        SDL_GPUBufferBinding index_binding = {0};
        index_binding.buffer = index_buffer;
        index_binding.offset = 0;

        SDL_BindGPUIndexBuffer(
            render_pass,
            &index_binding,
            SDL_GPU_INDEXELEMENTSIZE_16BIT
        );

        float aspect =
            (float)swapchain_width / (float)swapchain_height;
        float vertical_fov =
            80.0f * (3.14159265359f / 180.0f); // radians

        Mat4 rotation_x = mat4_rotation_x(angle_x);
        Mat4 rotation_y = mat4_rotation_y(angle_y);
        Mat4 model = mat4_multiply(rotation_y, rotation_x);

        Vec3 camera_forward = {
            .x = cosf(camera.pitch) * sinf(camera.yaw),
            .y = sinf(camera.pitch),
            .z = cosf(camera.pitch) * cosf(camera.yaw)
        };

        Vec3 camera_target = vec3_add(
            camera.position,
            camera_forward
        );

        Vec3 world_up = {
            .x = 0.0f,
            .y = 1.0f,
            .z = 0.0f
        };

        Mat4 view = mat4_look_at(
            camera.position,
            camera_target,
            world_up
        );

        Mat4 projection = mat4_perspective_projection(
            vertical_fov,
            aspect,
            0.1f,
            100.0f
        );

        Mat4 view_model = mat4_multiply(view, model);
        VertexUniforms uniforms = {
            .transform = mat4_multiply(projection, view_model)
        };

        SDL_PushGPUVertexUniformData(
            command_buffer,
            0, // corresponds to [[buffer(0)]] in the msl vert shader
            &uniforms,
            sizeof(uniforms)
        );

        SDL_DrawGPUIndexedPrimitives(
            render_pass,
            (Uint32)(sizeof(cube_indices) / sizeof(cube_indices[0])),
            1,  // one instance
            0,  // first index
            0,  // vertex offset
            0   // first instance
        );

        SDL_EndGPURenderPass(render_pass);
    }

    if (!SDL_SubmitGPUCommandBuffer(command_buffer)) {
        SDL_Log("Could not submit command buffer: %s", SDL_GetError());
        return false;
    }

    return true;
}

// -------------------- Game Loop --------------------
void run(void) {
    bool quit = false;
    float angle_x = 0.0f;
    float angle_y = 0.0f;
    Camera camera = {
        .position = {
            .x = 0.0f,
            .y = 0.0f,
            .z = -2.0f
        },
        .yaw = 0.0f,
        .pitch = 0.0f
    };

    const float two_pi = 6.28318530718f;
    const float camera_speed = 1.5f;
    const float camera_turn_speed = 1.5f;
    const float maximum_pitch = 1.553343f; // 89 degrees

    Uint64 last = SDL_GetPerformanceCounter();
    Uint64 frequency = SDL_GetPerformanceFrequency();

    while (!quit) {
        Uint64 now = SDL_GetPerformanceCounter();
        float dt = (float)(now - last) / (float)frequency;
        last = now;

        process_input(&quit);

        const bool *keyboard = SDL_GetKeyboardState(NULL);

        if (dt <= 0.1f) {
            angle_x += 1.2f * dt;
            angle_y += 2.0f * dt;

            if (keyboard[SDL_SCANCODE_W]) {
                camera.position.z += camera_speed * dt;
            }

            if (keyboard[SDL_SCANCODE_S]) {
                camera.position.z -= camera_speed * dt;
            }

            if (keyboard[SDL_SCANCODE_A]) {
                camera.position.x -= camera_speed * dt;
            }

            if (keyboard[SDL_SCANCODE_D]) {
                camera.position.x += camera_speed * dt;
            }

            if (keyboard[SDL_SCANCODE_SPACE]) {
                camera.position.y += camera_speed * dt;
            }

            if (keyboard[SDL_SCANCODE_LSHIFT]) {
                camera.position.y -= camera_speed * dt;
            }

            if (keyboard[SDL_SCANCODE_LEFT]) {
                camera.yaw -= camera_turn_speed * dt;
            }

            if (keyboard[SDL_SCANCODE_RIGHT]) {
                camera.yaw += camera_turn_speed * dt;
            }

            if (keyboard[SDL_SCANCODE_UP]) {
                camera.pitch += camera_turn_speed * dt;
            }

            if (keyboard[SDL_SCANCODE_DOWN]) {
                camera.pitch -= camera_turn_speed * dt;
            }

            if (camera.pitch > maximum_pitch) {
                camera.pitch = maximum_pitch;
            }

            if (camera.pitch < -maximum_pitch) {
                camera.pitch = -maximum_pitch;
            }
        }

        if (angle_x >= two_pi) {
            angle_x -= two_pi;
        }

        if (angle_y >= two_pi) {
            angle_y -= two_pi;
        }

        if (!render(angle_x, angle_y, camera)) {
            quit = true;
        }
    }
}

// -------------------- Main --------------------
int main(void) {
    if (!init()) {
        return 1;
    }

    run();
    shutdown();

    return 0;
}
