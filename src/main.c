#include <stdbool.h>
#include <math.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "demo_assets.h"
#include "demo_scene.h"
#include "math3d.h"
#include "renderer.h"

static const int width = 800;
static const int height = 600;

static SDL_Window *window = NULL;

static Renderer renderer = {0};
static DemoAssets demo_assets = {0};

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

    if (!demo_assets_init(&demo_assets, renderer.device)) {
        shutdown();
        return false;
    }

    SDL_SetWindowPosition(window, 50, 100);

    if (!set_mouse_capture(true)) {
        shutdown();
        return false;
    }

    return true;
}

static void shutdown(void) {
    if (renderer.device) {
        demo_assets_destroy(&demo_assets, renderer.device);
    }

    renderer_destroy(&renderer, window);

    SDL_DestroyWindow(window);
    window = NULL;

    SDL_Quit();
}

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
static bool run(void) {
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
            &demo_assets.cube_mesh,
            &demo_assets.floor_mesh,
            &demo_assets.suzanne_mesh,
            &demo_assets.cube_material,
            &demo_assets.floor_material)) {
        return false;
    }

    const float camera_speed = 5.0f;

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

        SceneLighting lighting = scene.lighting;
        if (light_follows_camera) {
            lighting.light_position = camera.position;
        }

        if (!renderer_draw(
                &renderer,
                window,
                &scene.world,
                camera_view_matrix(camera),
                lighting,
                show_texture,
                show_wireframe,
                scene.objects,
                scene.object_count)) {
            return false;
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

    return true;
}

// -------------------- Main --------------------
int main(void) {
    if (!init()) {
        return 1;
    }

    bool success = run();
    shutdown();

    return success ? 0 : 1;
}
