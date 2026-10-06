#include "game.h"

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

static Camera player_view_camera(const Player *player) {
    return (Camera){
        .position = {
            player->feet_position.x,
            player->feet_position.y + 1.2f,
            player->feet_position.z
        },
        .yaw = player->yaw,
        .pitch = player->pitch
    };
}

static Camera active_camera(const Game *game) {
    if (game->use_debug_camera) {
        return game->debug_camera;
    }

    return player_view_camera(&game->player);
}

bool game_init(
    Game *game,
    SDL_Window *window,
    SDL_GPUDevice *device,
    const char *asset_root
) {
    if (!game || !window || !device || !asset_root) {
        SDL_Log("Cannot initialise game with invalid arguments");
        return false;
    }

    *game = (Game){0};
    game->player.feet_position = (Vec3){0.0f, 0.0f, -4.0f};
    game->debug_camera = player_view_camera(&game->player);
    game->use_debug_camera = true;
    game->show_texture = true;

    if (!demo_assets_init(&game->assets, device, asset_root)) {
        return false;
    }

    if (!demo_scene_init(
            &game->scene,
            &game->assets.cube_mesh,
            &game->assets.floor_mesh,
            &game->assets.suzanne_mesh,
            &game->assets.cube_material,
            &game->assets.floor_material,
            &game->assets.suzanne_material)) {
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

    if (event->type == SDL_EVENT_KEY_DOWN) {
        if (event->key.scancode == SDL_SCANCODE_ESCAPE &&
            !event->key.repeat) {
            if (SDL_GetWindowRelativeMouseMode(window)) {
                set_mouse_capture(window, false);
            } else {
                return true;
            }
        } else if (event->key.scancode == SDL_SCANCODE_F1 &&
                   !event->key.repeat) {
            if (!game->use_debug_camera) {
                game->debug_camera = player_view_camera(&game->player);
            }
            game->use_debug_camera = !game->use_debug_camera;
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
        float yaw_delta = event->motion.xrel * mouse_sensitivity;
        float pitch_delta = -event->motion.yrel * mouse_sensitivity;

        if (game->use_debug_camera) {
            camera_rotate_fps(
                &game->debug_camera, yaw_delta, pitch_delta
            );
        } else {
            Camera view = player_view_camera(&game->player);
            camera_rotate_fps(&view, yaw_delta, pitch_delta);
            game->player.yaw = view.yaw;
            game->player.pitch = view.pitch;
        }
    }

    return false;
}

void game_update(Game *game, float dt) {
    if (!game || dt > 0.1f) {
        return;
    }

    demo_scene_update(&game->scene, dt);

    if (!game->use_debug_camera) {
        const bool *keyboard = SDL_GetKeyboardState(NULL);
        const float player_speed = 4.0f;

        Camera view = player_view_camera(&game->player);
        Vec3 look_forward = camera_forward_direction(&view);
        Vec3 forward = vec3_normalise((Vec3){
            .x = look_forward.x,
            .y = 0.0f,
            .z = look_forward.z
        });
        Vec3 right = vec3_cross((Vec3){0.0f, 1.0f, 0.0f}, forward);

        Vec3 direction = {0.0f, 0.0f, 0.0f};
        if (keyboard[SDL_SCANCODE_W]) {
            direction = vec3_add(direction, forward);
        }
        if (keyboard[SDL_SCANCODE_S]) {
            direction = vec3_subtract(direction, forward);
        }
        if (keyboard[SDL_SCANCODE_D]) {
            direction = vec3_add(direction, right);
        }
        if (keyboard[SDL_SCANCODE_A]) {
            direction = vec3_subtract(direction, right);
        }

        game->player.feet_position = vec3_add(
            game->player.feet_position,
            vec3_scale(vec3_normalise(direction), player_speed * dt)
        );
        return;
    }

    const float camera_speed = 5.0f;
    const bool *keyboard = SDL_GetKeyboardState(NULL);

    Vec3 camera_forward = camera_forward_direction(&game->debug_camera);
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
        game->debug_camera.position = vec3_add(
            game->debug_camera.position,
            vec3_scale(movement_forward, movement_distance)
        );
    }

    if (keyboard[SDL_SCANCODE_S]) {
        game->debug_camera.position = vec3_subtract(
            game->debug_camera.position,
            vec3_scale(movement_forward, movement_distance)
        );
    }

    if (keyboard[SDL_SCANCODE_A]) {
        game->debug_camera.position = vec3_subtract(
            game->debug_camera.position,
            vec3_scale(camera_right, movement_distance)
        );
    }

    if (keyboard[SDL_SCANCODE_D]) {
        game->debug_camera.position = vec3_add(
            game->debug_camera.position,
            vec3_scale(camera_right, movement_distance)
        );
    }

    if (keyboard[SDL_SCANCODE_SPACE]) {
        game->debug_camera.position.y += movement_distance;
    }

    if (keyboard[SDL_SCANCODE_LSHIFT]) {
        game->debug_camera.position.y -= movement_distance;
    }
}

bool game_render(const Game *game, Renderer *renderer, SDL_Window *window) {
    if (!game) {
        return false;
    }

    Camera view = active_camera(game);
    SceneLighting lighting = game->scene.lighting;
    if (game->light_follows_camera) {
        lighting.light_position = view.position;
    }

    return renderer_draw(
        renderer,
        window,
        &game->scene.world,
        camera_view_matrix(&view),
        lighting,
        game->show_texture,
        game->show_wireframe,
        game->scene.objects,
        game->scene.object_count
    );
}
