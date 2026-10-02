#include "game.h"

#include <math.h>

static Vec3 camera_forward_direction(const Game *game) {
    return (Vec3){
        .x = cosf(game->camera_pitch) * sinf(game->camera_yaw),
        .y = sinf(game->camera_pitch),
        .z = cosf(game->camera_pitch) * cosf(game->camera_yaw)
    };
}

static Mat4 camera_view_matrix(const Game *game) {
    Vec3 target = vec3_add(
        game->camera_position,
        camera_forward_direction(game)
    );

    return mat4_look_at(
        game->camera_position,
        target,
        (Vec3){0.0f, 1.0f, 0.0f}
    );
}

static bool set_mouse_capture(SDL_Window *window, bool enabled) {
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

bool game_init(Game *game, SDL_Window *window, SDL_GPUDevice *device) {
    if (!game || !window || !device) {
        SDL_Log("Cannot initialise game with invalid arguments");
        return false;
    }

    *game = (Game){0};
    game->camera_position = (Vec3){0.0f, 1.2f, -4.0f};
    game->show_texture = true;

    if (!demo_assets_init(&game->assets, device)) {
        return false;
    }

    if (!demo_scene_init(
            &game->scene,
            &game->assets.cube_mesh,
            &game->assets.floor_mesh,
            &game->assets.suzanne_mesh,
            &game->assets.cube_material,
            &game->assets.floor_material)) {
        goto fail;
    }

    if (!set_mouse_capture(window, true)) {
        goto fail;
    }

    return true;

fail:
    game_shutdown(game, device);
    return false;
}

void game_shutdown(Game *game, SDL_GPUDevice *device) {
    if (!game) {
        return;
    }

    demo_assets_destroy(&game->assets, device);
    *game = (Game){0};
}

bool game_handle_event(
    Game *game,
    SDL_Window *window,
    const SDL_Event *event
) {
    if (!game || !window || !event) {
        return false;
    }

    const float mouse_sensitivity = 0.0025f;
    const float maximum_pitch = 1.553343f;

    if (event->type == SDL_EVENT_KEY_DOWN) {
        if (event->key.scancode == SDL_SCANCODE_ESCAPE &&
            !event->key.repeat) {
            if (SDL_GetWindowRelativeMouseMode(window)) {
                set_mouse_capture(window, false);
            } else {
                return true;
            }
        } else if (event->key.scancode == SDL_SCANCODE_T &&
                   !event->key.repeat) {
            game->show_texture = !game->show_texture;
        } else if (event->key.scancode == SDL_SCANCODE_F &&
                   !event->key.repeat) {
            game->show_wireframe = !game->show_wireframe;
        } else if (event->key.scancode == SDL_SCANCODE_L &&
                   !event->key.repeat) {
            game->light_follows_camera = !game->light_follows_camera;
        }
    } else if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN &&
               event->button.button == SDL_BUTTON_LEFT &&
               !SDL_GetWindowRelativeMouseMode(window)) {
        set_mouse_capture(window, true);
    } else if (event->type == SDL_EVENT_WINDOW_FOCUS_LOST &&
               SDL_GetWindowRelativeMouseMode(window)) {
        set_mouse_capture(window, false);
    } else if (event->type == SDL_EVENT_MOUSE_MOTION &&
               SDL_GetWindowRelativeMouseMode(window)) {
        game->camera_yaw += event->motion.xrel * mouse_sensitivity;
        game->camera_pitch -= event->motion.yrel * mouse_sensitivity;

        if (game->camera_pitch < -maximum_pitch) {
            game->camera_pitch = -maximum_pitch;
        }
        if (game->camera_pitch > maximum_pitch) {
            game->camera_pitch = maximum_pitch;
        }
    }

    return false;
}

void game_update(Game *game, float dt) {
    if (!game || dt > 0.1f) {
        return;
    }

    const float camera_speed = 5.0f;
    const bool *keyboard = SDL_GetKeyboardState(NULL);

    demo_scene_update(&game->scene, dt);

    Vec3 camera_forward = camera_forward_direction(game);
    Vec3 movement_forward = vec3_normalise((Vec3){
        .x = camera_forward.x,
        .y = 0.0f,
        .z = camera_forward.z
    });

    Vec3 world_up = {0.0f, 1.0f, 0.0f};
    Vec3 camera_right = vec3_normalise(
        vec3_cross(world_up, movement_forward)
    );

    float movement_distance = camera_speed * dt;

    if (keyboard[SDL_SCANCODE_W]) {
        game->camera_position = vec3_add(
            game->camera_position,
            vec3_scale(movement_forward, movement_distance)
        );
    }

    if (keyboard[SDL_SCANCODE_S]) {
        game->camera_position = vec3_subtract(
            game->camera_position,
            vec3_scale(movement_forward, movement_distance)
        );
    }

    if (keyboard[SDL_SCANCODE_A]) {
        game->camera_position = vec3_subtract(
            game->camera_position,
            vec3_scale(camera_right, movement_distance)
        );
    }

    if (keyboard[SDL_SCANCODE_D]) {
        game->camera_position = vec3_add(
            game->camera_position,
            vec3_scale(camera_right, movement_distance)
        );
    }

    if (keyboard[SDL_SCANCODE_SPACE]) {
        game->camera_position.y += movement_distance;
    }

    if (keyboard[SDL_SCANCODE_LSHIFT]) {
        game->camera_position.y -= movement_distance;
    }
}

bool game_render(const Game *game, Renderer *renderer, SDL_Window *window) {
    if (!game) {
        return false;
    }

    SceneLighting lighting = game->scene.lighting;
    if (game->light_follows_camera) {
        lighting.light_position = game->camera_position;
    }

    return renderer_draw(
        renderer,
        window,
        &game->scene.world,
        camera_view_matrix(game),
        lighting,
        game->show_texture,
        game->show_wireframe,
        game->scene.objects,
        game->scene.object_count
    );
}
