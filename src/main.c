#include <stdbool.h>
#include <stddef.h>
#include <math.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "math3d.h"
#include "mesh.h"
#include "renderer.h"
#include "texture.h"

static const int width = 800;
static const int height = 600;

static SDL_Window *window = NULL;

static Renderer renderer = {0};

static const Uint32 checker_texture_width = 16;
static const Uint32 checker_texture_height = 16;
static const Uint32 checker_square_size = 4;
static const Uint32 checker_bytes_per_pixel = 4;

// ------------------------- Data --------------------------
static Mesh cube_mesh = {0};
static Mesh floor_mesh = {0};

typedef struct {
    Texture *texture;
    float tint[4];
} Material;

static Texture checker_texture = {0};
static Material cube_material = {
    .texture = &checker_texture,
    .tint    = {1.0f, 1.0f, 1.0f, 1.0f}
};
static Material floor_material = {
    .texture = &checker_texture,
    .tint    = {1.0f, 0.70f, 0.65f, 1.0f}
};

typedef struct {
    Mesh *mesh;
    Material *material;
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

// -------------------- Init / Shutdown --------------------
static void shutdown(void);

static bool init(void) {
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

    if (!renderer_init(&renderer, window)) {
        shutdown();
        return false;
    }

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

    if (!texture_create_rgba8(
            renderer.device,
            &checker_texture,
            checker_texture_width,
            checker_texture_height,
            checker_pixels)) {
        shutdown();
        return false;
    }

    const Uint32 cube_vertex_count =
        (Uint32)(sizeof(cube_vertices) / sizeof(cube_vertices[0]));

    const Uint32 cube_index_count =
        (Uint32)(sizeof(cube_indices) / sizeof(cube_indices[0]));

    if (!mesh_create(
            renderer.device,
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

    if (!mesh_create(
            renderer.device,
            &floor_mesh,
            floor_vertices,
            floor_vertex_count,
            floor_indices,
            floor_index_count)) {
        shutdown();
        return false;
    }

    SDL_Log("Created cube and floor meshes and uploaded checker texture");

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

static void shutdown(void) {
    if (renderer.device) {
        mesh_destroy(renderer.device, &cube_mesh);
        mesh_destroy(renderer.device, &floor_mesh);

        texture_destroy(
            renderer.device,
            &checker_texture
        );
    }

    renderer_destroy(&renderer, window);

    SDL_DestroyWindow(window);
    window = NULL;

    SDL_Quit();
}

// -------------------- Input --------------------
static void process_input(
    bool *quit,
    Camera *camera,
    bool *show_texture,
    bool *show_wireframe
) {
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
            else if (
                event.key.scancode == SDL_SCANCODE_F &&
                !event.key.repeat
            ) {
                *show_wireframe = !*show_wireframe;
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
    Mat4 projection,
    bool show_wireframe
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

    if (!show_wireframe) {
        const Material *material = object->material;
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

    if (!show_wireframe) {
        const Material *material = object->material;

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


// -------------------- Render --------------------
static bool render(
    Camera camera,
    bool show_texture,
    bool show_wireframe,
    const RenderObject *objects,
    size_t object_count
) {
    SDL_GPUCommandBuffer *command_buffer = 
        SDL_AcquireGPUCommandBuffer(renderer.device);

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
                &renderer,
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

        depth_target.texture = renderer.depth_texture;
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

        SDL_GPUGraphicsPipeline *active_pipeline =
            show_wireframe
                ? renderer.wireframe_pipeline
                : renderer.filled_pipeline;

        SDL_BindGPUGraphicsPipeline(render_pass, active_pipeline);

        if (!show_wireframe) {
            FragmentUniforms fragment_uniforms = {
                .texture_mix = {
                    show_texture ? 1.0f : 0.0f,
                    0.0f,
                    0.0f,
                    0.0f
                },
                .light_position = {-0.75f, 1.25f, -1.25f, 0.0f},
                .ambient_strength = 0.25f,
                .point_light_strength = 1.0f,
                .falloff_distance = 5.0f
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

        for (size_t i = 0; i < object_count; ++i) {
            draw_render_object(
                command_buffer,
                render_pass,
                &objects[i],
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

// -------------------- Game Loop --------------------
static void run(void) {
    bool quit = false;
    bool show_texture = true;
    bool show_wireframe = false;
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

    RenderObject objects[] = {
        {
            .mesh     = &cube_mesh,
            .material = &cube_material,
            .model    = mat4_identity()
        },
        {
            .mesh     = &cube_mesh,
            .material = &cube_material,
            .model    = mat4_translation(1.5f, 0.0f, 1.0f)
        },
        {
            .mesh     = &floor_mesh,
            .material = &floor_material,
            .model    = mat4_translation(0.0f, -1.0f, 0.0f)
        }
    };

    const size_t object_count =
        sizeof(objects) / sizeof(objects[0]);

    const float two_pi = 6.28318530718f;
    const float camera_speed = 4.0f;

    Uint64 last = SDL_GetPerformanceCounter();
    Uint64 frequency = SDL_GetPerformanceFrequency();
    float fps_elapsed = 0.0f;
    Uint32 fps_frame_count = 0;

    while (!quit) {
        Uint64 now = SDL_GetPerformanceCounter();
        float dt = (float)(now - last) / (float)frequency;
        last = now;

        process_input(
            &quit,
            &camera,
            &show_texture,
            &show_wireframe
        );

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

        Mat4 rotation_x = mat4_rotation_x(angle_x);
        Mat4 rotation_y = mat4_rotation_y(angle_y);
        objects[0].model = mat4_multiply(rotation_y, rotation_x);

        if (!render(
                camera,
                show_texture,
                show_wireframe,
                objects,
                object_count)) {
            quit = true;
        } else {
            fps_elapsed += dt;
            ++fps_frame_count;

            if (fps_elapsed >= 1.0f) {
                float average_fps =
                    (float)fps_frame_count / fps_elapsed;

                char window_title[64];
                SDL_snprintf(
                    window_title,
                    sizeof(window_title),
                    "3D Engine | FPS: %.1f",
                    average_fps
                );
                SDL_SetWindowTitle(window, window_title);

                fps_elapsed = 0.0f;
                fps_frame_count = 0;
            }
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
