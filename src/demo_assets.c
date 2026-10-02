#include "demo_assets.h"
#include "gltf_loader.h"

static const Uint32 checker_texture_width = 16;
static const Uint32 checker_texture_height = 16;
static const Uint32 checker_square_size = 4;
static const Uint32 checker_bytes_per_pixel = 4;

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
        .uv       = { 0.0f,  0.0f}
    },
    {
        .position = {-5.0f,  0.0f,  5.0f},
        .normal   = { 0.0f,  1.0f,  0.0f},
        .uv       = { 0.0f,  5.0f}
    },
    {
        .position = { 5.0f,  0.0f,  5.0f},
        .normal   = { 0.0f,  1.0f,  0.0f},
        .uv       = { 5.0f,  5.0f}
    },
    {
        .position = { 5.0f,  0.0f, -5.0f},
        .normal   = { 0.0f,  1.0f,  0.0f},
        .uv       = { 5.0f,  0.0f}
    }
};

void demo_assets_destroy(DemoAssets *assets, SDL_GPUDevice *device) {
    if (!assets || !device) {
        return;
    }

    mesh_destroy(device, &assets->cube_mesh);
    mesh_destroy(device, &assets->floor_mesh);
    mesh_destroy(device, &assets->suzanne_mesh);
    texture_destroy(device, &assets->checker_texture);
    *assets = (DemoAssets){0};
}

bool demo_assets_init(DemoAssets *assets, SDL_GPUDevice *device) {
    if (!assets || !device) {
        SDL_Log("Could not initialise demo assets");
        return false;
    }

    *assets = (DemoAssets){0};

    const Uint32 checker_data_size =
        checker_texture_width *
        checker_texture_height *
        checker_bytes_per_pixel;

    Uint8 checker_pixels[checker_data_size];

    for (Uint32 y = 0; y < checker_texture_height; ++y) {
        for (Uint32 x = 0; x < checker_texture_width; ++x) {
            bool bright_square =
                ((x / checker_square_size) +
                 (y / checker_square_size)) % 2u == 0u;

            Uint8 color = bright_square ? 196 : 142;

            Uint32 pixel_offset =
                (y * checker_texture_width + x) *
                checker_bytes_per_pixel;

            checker_pixels[pixel_offset + 0] = color;
            checker_pixels[pixel_offset + 1] = color;
            checker_pixels[pixel_offset + 2] = color;
            checker_pixels[pixel_offset + 3] = 255;
        }
    }

    if (!texture_create_rgba8(
            device,
            &assets->checker_texture,
            checker_texture_width,
            checker_texture_height,
            checker_pixels)) {
        goto fail;
    }

    const Uint32 cube_vertex_count =
        (Uint32)(sizeof(cube_vertices) / sizeof(cube_vertices[0]));

    const Uint32 cube_index_count =
        (Uint32)(sizeof(cube_indices) / sizeof(cube_indices[0]));

    if (!mesh_create(
            device,
            &assets->cube_mesh,
            cube_vertices,
            cube_vertex_count,
            cube_indices,
            cube_index_count)) {
        goto fail;
    }

    const Uint32 floor_vertex_count =
        (Uint32)(sizeof(floor_vertices) / sizeof(floor_vertices[0]));

    const Uint32 floor_index_count =
        (Uint32)(sizeof(floor_indices) / sizeof(floor_indices[0]));

    if (!mesh_create(
            device,
            &assets->floor_mesh,
            floor_vertices,
            floor_vertex_count,
            floor_indices,
            floor_index_count)) {
        goto fail;
    }

    if (!gltf_load_mesh(
            device,
            &assets->suzanne_mesh,
            "models/Suzanne.gltf")) {
        goto fail;
    }

    assets->cube_material = (Material){
        .texture = &assets->checker_texture,
        .tint = {1.0f, 1.0f, 1.0f, 1.0f}
    };
    assets->floor_material = (Material){
        .texture = &assets->checker_texture,
        .tint = {0.85f, 0.42f, 0.32f, 1.0f}
    };

    SDL_Log("Created cube, floor, and Suzanne meshes and uploaded checker texture");
    return true;

fail:
    demo_assets_destroy(assets, device);
    return false;
}
