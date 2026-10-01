#include <stdbool.h>
#include <math.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "demo_scene.h"
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

static Texture checker_texture = {0};
static Material cube_material = {
    .texture = &checker_texture,
    .tint    = {1.0f, 1.0f, 1.0f, 1.0f}
};
static Material floor_material = {
    .texture = &checker_texture,
    .tint    = {0.85f, 0.42f, 0.32f, 1.0f}
};

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

static Mat4 camera_view_matrix(Camera camera) {
    Vec3 target = vec3_add(
        camera.position,
        camera_forward_direction(camera)
    );

    return mat4_look_at(
        camera.position,
        target,
        (Vec3){0.0f, 1.0f, 0.0f}
    );
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

static bool set_mouse_capture(bool enabled) {
    if (!SDL_SetWindowRelativeMouseMode(window, enabled)) {
        SDL_Log(
            "Could not %s relative mouse mode: %s",
            enabled ? "enable" : "disable",
            SDL_GetError()
        );
        return false;
    }

    return true;
}

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

    if (!set_mouse_capture(true)) {
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
    bool *show_wireframe,
    bool *light_follows_camera
) {
    const float mouse_sensitivity = 0.0025f;
    const float maximum_pitch = 1.553343f;

    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT) {
            *quit = true;
        }
        else if (event.type == SDL_EVENT_KEY_DOWN) {
            if (
                event.key.scancode == SDL_SCANCODE_ESCAPE &&
                !event.key.repeat
            ) {
                if (SDL_GetWindowRelativeMouseMode(window)) {
                    set_mouse_capture(false);
                } else {
                    *quit = true;
                }
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
            else if (
                event.key.scancode == SDL_SCANCODE_L &&
                !event.key.repeat
            ) {
                *light_follows_camera = !*light_follows_camera;
            }
        }
        else if (
            event.type == SDL_EVENT_MOUSE_BUTTON_DOWN &&
            event.button.button == SDL_BUTTON_LEFT &&
            !SDL_GetWindowRelativeMouseMode(window)
        ) {
            set_mouse_capture(true);
        }
        else if (
            event.type == SDL_EVENT_WINDOW_FOCUS_LOST &&
            SDL_GetWindowRelativeMouseMode(window)
        ) {
            set_mouse_capture(false);
        }
        else if (
            event.type == SDL_EVENT_MOUSE_MOTION &&
            SDL_GetWindowRelativeMouseMode(window)
        ) {
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

// -------------------- Game Loop --------------------
static void run(void) {
    bool quit = false;
    bool show_texture = true;
    bool show_wireframe = false;
    bool light_follows_camera = false;
    Camera camera = {
        .position = {
            .x = 0.0f,
            .y = 1.2f,
            .z = -4.0f
        },
        .yaw = 0.0f,
        .pitch = 0.0f
    };

    DemoScene scene;
    if (!demo_scene_init(
            &scene,
            &cube_mesh,
            &floor_mesh,
            &cube_material,
            &floor_material)) {
        return;
    }

    const float camera_speed = 4.0f;

    Uint64 last = SDL_GetPerformanceCounter();
    Uint64 frequency = SDL_GetPerformanceFrequency();
    float fps_elapsed = 0.0f;
    Uint32 fps_frame_count = 0;

    const Vec3 fixed_light_position = {-1.25f, 2.25f, -1.25f};

    while (!quit) {
        Uint64 now = SDL_GetPerformanceCounter();
        float dt = (float)(now - last) / (float)frequency;
        last = now;

        process_input(
            &quit,
            &camera,
            &show_texture,
            &show_wireframe,
            &light_follows_camera
        );

        const bool *keyboard = SDL_GetKeyboardState(NULL);

        if (dt <= 0.1f) {
            demo_scene_update(&scene, dt);

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

        Vec3 light_position = light_follows_camera
            ? camera.position
            : fixed_light_position;

        if (!renderer_draw(
                &renderer,
                window,
                &scene.world,
                camera_view_matrix(camera),
                light_position,
                show_texture,
                show_wireframe,
                scene.objects,
                scene.object_count)) {
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
                    "FPS: %.1f",
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
