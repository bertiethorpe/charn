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

static SDL_GPUTexture *checker_texture = NULL;
static SDL_GPUSampler *texture_sampler = NULL;
static const Uint32 checker_texture_width = 16;
static const Uint32 checker_texture_height = 16;
static const Uint32 checker_square_size = 4;
static const Uint32 checker_bytes_per_pixel = 4;

static SDL_GPUTexture *depth_texture = NULL;

static const SDL_GPUTextureFormat depth_texture_format =
    SDL_GPU_TEXTUREFORMAT_D16_UNORM;

static Uint32 depth_texture_width = 0;
static Uint32 depth_texture_height = 0;

// ------------------------- Data --------------------------
typedef struct {
    float position[3];
    float normal[3];
    float uv[2];
} Vertex;

typedef struct {
    SDL_GPUBuffer *vertex_buffer;
    SDL_GPUBuffer *index_buffer;
    Uint32 index_count;
} Mesh;

static Mesh cube_mesh = {0};
static Mesh floor_mesh = {0};

typedef struct {
    Mesh *mesh;
    Mat4 model;
} RenderObject;

typedef struct {
    Mat4 transform;
    Mat4 model;
} VertexUniforms;

typedef struct {
    float texture_mix[4];
    float light_position[4];
    float ambient_strength;
    float point_light_strength;
    float falloff_distance;
    float padding;
} FragmentUniforms;

typedef struct {
    Vec3 position;
    float yaw;
    float pitch;
} Camera;

static Vec3 camera_forward_direction(Camera camera) {
    return (Vec3){
        .x = cosf(camera.pitch) * sinf(camera.yaw),
        .y = sinf(camera.pitch),
        .z = cosf(camera.pitch) * cosf(camera.yaw)
    };
}

static const Uint16 cube_indices[] = {
     0,  1,  2,  0,  2,  3,  // front:  -Z
     4,  5,  6,  4,  6,  7,  // back:   +Z
     8,  9, 10,  8, 10, 11,  // left:   -X
    12, 13, 14, 12, 14, 15,  // right:  +X
    16, 17, 18, 16, 18, 19,  // top:    +Y
    20, 21, 22, 20, 22, 23   // bottom: -Y
};

static const Vertex cube_vertices[] = {
    // Front: -Z
    { .position = {-0.5f, -0.5f, -0.5f},
      .normal   = { 0.0f,  0.0f, -1.0f},
      .uv       = { 0.0f,  1.0f} }, // 0
    { .position = {-0.5f,  0.5f, -0.5f},
      .normal   = { 0.0f,  0.0f, -1.0f},
      .uv       = { 0.0f,  0.0f} }, // 1
    { .position = { 0.5f,  0.5f, -0.5f},
      .normal   = { 0.0f,  0.0f, -1.0f},
      .uv       = { 1.0f,  0.0f} }, // 2
    { .position = { 0.5f, -0.5f, -0.5f},
      .normal   = { 0.0f,  0.0f, -1.0f},
      .uv       = { 1.0f,  1.0f} }, // 3

    // Back: +Z
    { .position = {-0.5f, -0.5f,  0.5f},
      .normal   = { 0.0f,  0.0f,  1.0f},
      .uv       = { 1.0f,  1.0f} }, // 4
    { .position = { 0.5f, -0.5f,  0.5f},
      .normal   = { 0.0f,  0.0f,  1.0f},
      .uv       = { 0.0f,  1.0f} }, // 5
    { .position = { 0.5f,  0.5f,  0.5f},
      .normal   = { 0.0f,  0.0f,  1.0f},
      .uv       = { 0.0f,  0.0f} }, // 6
    { .position = {-0.5f,  0.5f,  0.5f},
      .normal   = { 0.0f,  0.0f,  1.0f},
      .uv       = { 1.0f,  0.0f} }, // 7

    // Left: -X
    { .position = {-0.5f, -0.5f,  0.5f},
      .normal   = {-1.0f,  0.0f,  0.0f},
      .uv       = { 0.0f,  1.0f} }, // 8
    { .position = {-0.5f,  0.5f,  0.5f},
      .normal   = {-1.0f,  0.0f,  0.0f},
      .uv       = { 0.0f,  0.0f} }, // 9
    { .position = {-0.5f,  0.5f, -0.5f},
      .normal   = {-1.0f,  0.0f,  0.0f},
      .uv       = { 1.0f,  0.0f} }, // 10
    { .position = {-0.5f, -0.5f, -0.5f},
      .normal   = {-1.0f,  0.0f,  0.0f},
      .uv       = { 1.0f,  1.0f} }, // 11

    // Right: +X
    { .position = { 0.5f, -0.5f, -0.5f},
      .normal   = { 1.0f,  0.0f,  0.0f},
      .uv       = { 0.0f,  1.0f} }, // 12
    { .position = { 0.5f,  0.5f, -0.5f},
      .normal   = { 1.0f,  0.0f,  0.0f},
      .uv       = { 0.0f,  0.0f} }, // 13
    { .position = { 0.5f,  0.5f,  0.5f},
      .normal   = { 1.0f,  0.0f,  0.0f},
      .uv       = { 1.0f,  0.0f} }, // 14
    { .position = { 0.5f, -0.5f,  0.5f},
      .normal   = { 1.0f,  0.0f,  0.0f},
      .uv       = { 1.0f,  1.0f} }, // 15

    // Top: +Y
    { .position = {-0.5f,  0.5f, -0.5f},
      .normal   = { 0.0f,  1.0f,  0.0f},
      .uv       = { 0.0f,  1.0f} }, // 16
    { .position = {-0.5f,  0.5f,  0.5f},
      .normal   = { 0.0f,  1.0f,  0.0f},
      .uv       = { 0.0f,  0.0f} }, // 17
    { .position = { 0.5f,  0.5f,  0.5f},
      .normal   = { 0.0f,  1.0f,  0.0f},
      .uv       = { 1.0f,  0.0f} }, // 18
    { .position = { 0.5f,  0.5f, -0.5f},
      .normal   = { 0.0f,  1.0f,  0.0f},
      .uv       = { 1.0f,  1.0f} }, // 19

    // Bottom: -Y
    { .position = {-0.5f, -0.5f,  0.5f},
      .normal   = { 0.0f, -1.0f,  0.0f},
      .uv       = { 0.0f,  1.0f} }, // 20
    { .position = {-0.5f, -0.5f, -0.5f},
      .normal   = { 0.0f, -1.0f,  0.0f},
      .uv       = { 0.0f,  0.0f} }, // 21
    { .position = { 0.5f, -0.5f, -0.5f},
      .normal   = { 0.0f, -1.0f,  0.0f},
      .uv       = { 1.0f,  0.0f} }, // 22
    { .position = { 0.5f, -0.5f,  0.5f},
      .normal   = { 0.0f, -1.0f,  0.0f},
      .uv       = { 1.0f,  1.0f} }  // 23
};

static const Uint16 floor_indices[] = {
    0, 1, 2,
    0, 2, 3
};

static const Vertex floor_vertices[] = {
    {
        .position = {-5.0f,  0.0f, -5.0f},
        .normal   = { 0.0f,  1.0f,  0.0f},
        .uv       = { 0.0f,  5.0f}
    },
    {
        .position = {-5.0f,  0.0f,  5.0f},
        .normal   = { 0.0f,  1.0f,  0.0f},
        .uv       = { 0.0f,  5.0f}
    },
    {
        .position = { 5.0f,  0.0f,  5.0f},
        .normal   = { 0.0f,  1.0f,  0.0f},
        .uv       = { 0.0f,  5.0f}
    },
    {
        .position = { 5.0f,  0.0f, -5.0f},
        .normal   = { 0.0f,  1.0f,  0.0f},
        .uv       = { 0.0f,  5.0f}
    }
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

static void destroy_mesh(Mesh *mesh) {
    if (!mesh) {
        return;
    }

    if (gpu_device) {
        if (mesh->vertex_buffer) {
            SDL_ReleaseGPUBuffer(
                gpu_device,
                mesh->vertex_buffer
            );
        }

        if (mesh->index_buffer) {
            SDL_ReleaseGPUBuffer(
                gpu_device,
                mesh->index_buffer
            );
        }
    }

    *mesh = (Mesh){0};
}

static bool create_mesh(
    Mesh *mesh,
    const Vertex *vertices,
    Uint32 vertex_count,
    const Uint16 *indices,
    Uint32 index_count
) {
    if (!mesh ||
        !vertices ||
        vertex_count == 0 ||
        !indices ||
        index_count == 0) {
        SDL_Log("Cannot create a mesh from empty data");
        return false;
    }

    destroy_mesh(mesh);

    const Uint32 vertex_data_size = vertex_count * (Uint32)sizeof(Vertex);
    const Uint32 index_data_size = index_count * (Uint32)sizeof(Uint16);
    const Uint32 index_data_offset = (vertex_data_size +3u) & ~3u;

    SDL_GPUBufferCreateInfo vertex_buffer_info = {
        .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
        .size = vertex_data_size
    };

    mesh->vertex_buffer = SDL_CreateGPUBuffer(
        gpu_device,
        &vertex_buffer_info
    );

    if(!mesh->vertex_buffer) {
        SDL_Log(
            "Could not create vertex buffer: %s",
            SDL_GetError()
        );
        destroy_mesh(mesh);
        return false;
    }

    SDL_GPUBufferCreateInfo index_buffer_info = {
        .usage = SDL_GPU_BUFFERUSAGE_INDEX,
        .size = index_data_size
    };

    mesh->index_buffer = SDL_CreateGPUBuffer(
        gpu_device,
        &index_buffer_info
    );

    if (!mesh->index_buffer) {
        SDL_Log(
            "Could not create index buffer: %s",
            SDL_GetError()
        );
        destroy_mesh(mesh);
        return false;
    }

    SDL_GPUTransferBufferCreateInfo transfer_info = {
        .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
        .size = index_data_offset + index_data_size
    };

    SDL_GPUTransferBuffer *transfer_buffer =
        SDL_CreateGPUTransferBuffer(
            gpu_device,
            &transfer_info
        );

    if (!transfer_buffer) {
        SDL_Log(
            "Could not create mesh transfer buffer: %s",
            SDL_GetError()
        );
        destroy_mesh(mesh);
        return false;
    }

    void *mapped_data = SDL_MapGPUTransferBuffer(
        gpu_device,
        transfer_buffer,
        false
    );

    if (!mapped_data) {
        SDL_Log(
            "Could not map mesh transfer buffer: %s",
            SDL_GetError()
        );
        SDL_ReleaseGPUTransferBuffer(
            gpu_device,
            transfer_buffer
        );
        destroy_mesh(mesh);
        return false;
    }

    SDL_memcpy(
        mapped_data,
        vertices,
        vertex_data_size
    );

    SDL_memcpy(
        (Uint8 *)mapped_data + index_data_offset,
        indices,
        index_data_size
    );

    SDL_UnmapGPUTransferBuffer(
        gpu_device,
        transfer_buffer
    );

    SDL_GPUCommandBuffer *command_buffer =
        SDL_AcquireGPUCommandBuffer(gpu_device);

    if (!command_buffer) {
        SDL_Log(
            "Could not acquire mesh upload command buffer: %s",
            SDL_GetError()
        );
        SDL_ReleaseGPUTransferBuffer(
            gpu_device,
            transfer_buffer
        );
        destroy_mesh(mesh);
        return false;
    }

    SDL_GPUCopyPass *copy_pass =
        SDL_BeginGPUCopyPass(command_buffer);

    if (!copy_pass) {
        SDL_Log(
            "Could not begin mesh copy pass: %s",
            SDL_GetError()
        );
        SDL_CancelGPUCommandBuffer(command_buffer);
        SDL_ReleaseGPUTransferBuffer(
            gpu_device,
            transfer_buffer
        );
        destroy_mesh(mesh);
        return false;
    }

    SDL_GPUTransferBufferLocation vertex_source = {
        .transfer_buffer = transfer_buffer,
        .offset = 0
    };

    SDL_GPUBufferRegion vertex_destination = {
        .buffer = mesh->vertex_buffer,
        .offset = 0,
        .size = vertex_data_size
    };

    SDL_UploadToGPUBuffer(
        copy_pass,
        &vertex_source,
        &vertex_destination,
        false
    );

    SDL_GPUTransferBufferLocation index_source = {
        .transfer_buffer = transfer_buffer,
        .offset = index_data_offset
    };

    SDL_GPUBufferRegion index_destination = {
        .buffer = mesh->index_buffer,
        .offset = 0,
        .size = index_data_size
    };

    SDL_UploadToGPUBuffer(
        copy_pass,
        &index_source,
        &index_destination,
        false
    );

    SDL_EndGPUCopyPass(copy_pass);

    if (!SDL_SubmitGPUCommandBuffer(command_buffer)) {
        SDL_Log(
            "Could not submit mesh upload: %s",
            SDL_GetError()
        );
        SDL_ReleaseGPUTransferBuffer(
            gpu_device,
            transfer_buffer
        );
        destroy_mesh(mesh);
        return false;
    }

    SDL_ReleaseGPUTransferBuffer(
        gpu_device,
        transfer_buffer
    );

    mesh->index_count = index_count;
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

    SDL_GPUTextureCreateInfo checker_texture_info = {0};

    checker_texture_info.type = SDL_GPU_TEXTURETYPE_2D;
    checker_texture_info.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    checker_texture_info.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
    checker_texture_info.width = checker_texture_width;
    checker_texture_info.height = checker_texture_height;
    checker_texture_info.layer_count_or_depth = 1;
    checker_texture_info.num_levels = 1;
    checker_texture_info.sample_count = SDL_GPU_SAMPLECOUNT_1;

    checker_texture = SDL_CreateGPUTexture(
        gpu_device,
        &checker_texture_info
    );

    if (!checker_texture) {
        SDL_Log(
            "Could not create checker texture: %s",
            SDL_GetError()
        );
        shutdown();
        return false;
    }

    SDL_GPUSamplerCreateInfo sampler_info = {0};

    sampler_info.min_filter = SDL_GPU_FILTER_NEAREST;
    sampler_info.mag_filter = SDL_GPU_FILTER_NEAREST;
    sampler_info.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST;
    sampler_info.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
    sampler_info.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
    sampler_info.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;

    texture_sampler = SDL_CreateGPUSampler(
        gpu_device,
        &sampler_info
    );

    if (!texture_sampler) {
        SDL_Log(
            "Could not create texture sampler: %s",
            SDL_GetError()
        );
        shutdown();
        return false;
    }

    const Uint32 cube_vertex_count =
        (Uint32)(sizeof(cube_vertices) / sizeof(cube_vertices[0]));

    const Uint32 cube_index_count =
        (Uint32)(sizeof(cube_indices) / sizeof(cube_indices[0]));

    if (!create_mesh(
            &cube_mesh,
            cube_vertices,
            cube_vertex_count,
            cube_indices,
            cube_index_count)) {
        shutdown();
        return false;
    }

    const Uint32 floor_vertex_count =
        (Uint32)(sizeof(floor_vertices) / sizeof(floor_vertices[0]));

    const Uint32 floor_index_count =
        (Uint32)(sizeof(floor_indices) / sizeof(floor_indices[0]));

    if (!create_mesh(
            &floor_mesh,
            floor_vertices,
            floor_vertex_count,
            floor_indices,
            floor_index_count)) {
        shutdown();
        return false;
    }

    const Uint32 checker_data_size =
        checker_texture_width *
        checker_texture_height *
        checker_bytes_per_pixel;

    SDL_GPUTransferBufferCreateInfo transfer_info = {0};
    transfer_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transfer_info.size = checker_data_size;

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

    Uint8 *checker_pixels = mapped_data;

    for (Uint32 y = 0; y < checker_texture_height; ++y) {
        for (Uint32 x = 0; x < checker_texture_width; ++x) {
            bool bright_square =
                ((x / checker_square_size) +
                 (y / checker_square_size)) % 2u == 0u;

            Uint8 color = bright_square ? 255 : 32;

            Uint32 pixel_offset =
                (y * checker_texture_width + x) *
                checker_bytes_per_pixel;

            checker_pixels[pixel_offset + 0] = color;
            checker_pixels[pixel_offset + 1] = color;
            checker_pixels[pixel_offset + 2] = color;
            checker_pixels[pixel_offset + 3] = 255;
        }
    }

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

    SDL_GPUTextureTransferInfo checker_source = {0};

    checker_source.transfer_buffer = transfer_buffer;
    checker_source.offset = 0;
    checker_source.pixels_per_row = checker_texture_width;
    checker_source.rows_per_layer = checker_texture_height;

    SDL_GPUTextureRegion checker_destination = {0};

    checker_destination.texture = checker_texture;
    checker_destination.mip_level = 0;
    checker_destination.layer = 0;
    checker_destination.x = 0;
    checker_destination.y = 0;
    checker_destination.z = 0;
    checker_destination.w = checker_texture_width;
    checker_destination.h = checker_texture_height;
    checker_destination.d = 1;

    SDL_UploadToGPUTexture(
        copy_pass,
        &checker_source,
        &checker_destination,
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

    SDL_Log("Created cube mesh and uploaded checker texture");

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
    fragment_shader_info.num_samplers = 1;
    fragment_shader_info.num_uniform_buffers = 1;

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

    SDL_GPUVertexAttribute vertex_attributes[3] = {0};

    vertex_attributes[0].location = 0;
    vertex_attributes[0].buffer_slot = 0;
    vertex_attributes[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
    vertex_attributes[0].offset = (Uint32)offsetof(Vertex, position);

    vertex_attributes[1].location = 1;
    vertex_attributes[1].buffer_slot = 0;
    vertex_attributes[1].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
    vertex_attributes[1].offset = (Uint32)offsetof(Vertex, normal);

    vertex_attributes[2].location = 2;
    vertex_attributes[2].buffer_slot = 0;
    vertex_attributes[2].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
    vertex_attributes[2].offset = (Uint32)offsetof(Vertex, uv);

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
    pipeline_info.vertex_input_state.num_vertex_attributes = 3;

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

    if (!SDL_SetWindowRelativeMouseMode(window, true)) {
        SDL_Log(
            "Could not enable relative mouse mode: %s",
            SDL_GetError()
        );
        shutdown();
        return false;
    }

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

        destroy_mesh(&cube_mesh);
        destroy_mesh(&floor_mesh);

        if (depth_texture) {
            SDL_ReleaseGPUTexture(gpu_device, depth_texture);
        }

        if (texture_sampler) {
            SDL_ReleaseGPUSampler(gpu_device, texture_sampler);
        }

        if (checker_texture) {
            SDL_ReleaseGPUTexture(gpu_device, checker_texture);
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
void process_input(bool *quit, Camera *camera, bool *show_texture) {
    const float mouse_sensitivity = 0.0025f;
    const float maximum_pitch = 1.553343f;

    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT) {
            *quit = true;
        }
        else if (event.type == SDL_EVENT_KEY_DOWN) {
            if (event.key.scancode == SDL_SCANCODE_ESCAPE) {
                *quit = true;
            }
            else if (
                event.key.scancode == SDL_SCANCODE_T &&
                !event.key.repeat
            ) {
                *show_texture = !*show_texture;
            }
        }
        else if (event.type == SDL_EVENT_MOUSE_MOTION) {
            camera->yaw += // pointer required because this modifies the actual camera.
                event.motion.xrel * mouse_sensitivity;

            camera->pitch -=
                event.motion.yrel * mouse_sensitivity;

            if (camera->pitch < -maximum_pitch) {
                camera->pitch = -maximum_pitch;
            }

            if (camera->pitch > maximum_pitch) {
                camera->pitch = maximum_pitch;
            }
        }
    }
}

static void draw_render_object(
    SDL_GPUCommandBuffer *command_buffer,
    SDL_GPURenderPass *render_pass,
    const RenderObject *object,
    Mat4 view,
    Mat4 projection
) {
    const Mesh *mesh = object->mesh;

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

    Mat4 view_model = mat4_multiply(
        view,
        object->model
    );

    VertexUniforms uniforms = {
        .transform = mat4_multiply(projection, view_model),
        .model = object->model
    };

    SDL_PushGPUVertexUniformData(
        command_buffer,
        0,
        &uniforms,
        sizeof(uniforms)
    );

    SDL_DrawGPUIndexedPrimitives(
        render_pass,
        mesh->index_count,
        1,
        0,
        0,
        0
    );
}


// -------------------- Render --------------------
bool render(
    float angle_x,
    float angle_y,
    Camera camera,
    bool show_texture
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

        SDL_GPUTextureSamplerBinding checker_binding = {0};

        checker_binding.texture = checker_texture;
        checker_binding.sampler = texture_sampler;

        SDL_BindGPUFragmentSamplers(
            render_pass,
            0,
            &checker_binding,
            1
        );

        FragmentUniforms fragment_uniforms = {
            .texture_mix = {
                show_texture ? 1.0f : 0.0f,
                0.0f,
                0.0f,
                0.0f
            },
            .light_position = {-0.75f, 1.25f, -1.25f, 0.0f},
            .ambient_strength = 0.15f,
            .point_light_strength = 0.8f,
            .falloff_distance = 3.0f
        };

        SDL_PushGPUFragmentUniformData(
            command_buffer,
            0,
            &fragment_uniforms,
            sizeof(fragment_uniforms)
        );

        float aspect =
            (float)swapchain_width / (float)swapchain_height;
        float vertical_fov =
            60.0f * (3.14159265359f / 180.0f); // radians

        Mat4 rotation_x = mat4_rotation_x(angle_x);
        Mat4 rotation_y = mat4_rotation_y(angle_y);
        Mat4 rotating_model = mat4_multiply(rotation_y, rotation_x);

        Vec3 camera_forward = camera_forward_direction(camera);

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

        RenderObject objects[] = {
            {
                .mesh  = &cube_mesh,
                .model = rotating_model
            },
            {
                .mesh  = &cube_mesh,
                .model = mat4_translation(1.5f, 0.0f, 1.0f)
            },
            {
                .mesh  = &floor_mesh,
                .model = mat4_translation(0.0f, -1.0f, 0.0f)
            }
        };

        const size_t object_count =
            sizeof(objects) / sizeof(objects[0]);

        for (size_t i = 0; i < object_count; ++i) {
            draw_render_object(
                command_buffer,
                render_pass,
                &objects[i],
                view,
                projection
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

// -------------------- Game Loop --------------------
void run(void) {
    bool quit = false;
    bool show_texture = true;
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

    Uint64 last = SDL_GetPerformanceCounter();
    Uint64 frequency = SDL_GetPerformanceFrequency();

    while (!quit) {
        Uint64 now = SDL_GetPerformanceCounter();
        float dt = (float)(now - last) / (float)frequency;
        last = now;

        process_input(&quit, &camera, &show_texture);

        const bool *keyboard = SDL_GetKeyboardState(NULL);

        if (dt <= 0.1f) {
            angle_x += 1.2f * dt;
            angle_y += 2.0f * dt;

            Vec3 camera_forward = camera_forward_direction(camera);

            Vec3 movement_forward = vec3_normalise((Vec3){
                .x = camera_forward.x,
                .y = 0.0f,
                .z = camera_forward.z
            });

            Vec3 world_up = {
                .x = 0.0f,
                .y = 1.0f,
                .z = 0.0f
            };

            Vec3 camera_right = vec3_normalise(
                vec3_cross(world_up, movement_forward)
            );

            float movement_distance = camera_speed * dt;

            if (keyboard[SDL_SCANCODE_W]) {
                camera.position = vec3_add(
                    camera.position,
                    vec3_scale(movement_forward, movement_distance)
                );
            }

            if (keyboard[SDL_SCANCODE_S]) {
                camera.position = vec3_subtract(
                    camera.position,
                    vec3_scale(movement_forward, movement_distance)
                );
            }

            if (keyboard[SDL_SCANCODE_A]) {
                camera.position = vec3_subtract(
                    camera.position,
                    vec3_scale(camera_right, movement_distance)
                );
            }

            if (keyboard[SDL_SCANCODE_D]) {
                camera.position = vec3_add(
                    camera.position,
                    vec3_scale(camera_right, movement_distance)
                );
            }

            if (keyboard[SDL_SCANCODE_SPACE]) {
                camera.position.y += movement_distance;
            }

            if (keyboard[SDL_SCANCODE_LSHIFT]) {
                camera.position.y -= movement_distance;
            }
        }

        if (angle_x >= two_pi) {
            angle_x -= two_pi;
        }

        if (angle_y >= two_pi) {
            angle_y -= two_pi;
        }

        if (!render(angle_x, angle_y, camera, show_texture)) {
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
