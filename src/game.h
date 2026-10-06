#ifndef GAME_H
#define GAME_H

#include <stdbool.h>

#include <SDL3/SDL.h>

#include "camera.h"
#include "demo_assets.h"
#include "demo_scene.h"

typedef struct {
    Vec3 feet_position;
    float yaw;
    float pitch;
} Player;

typedef struct {
    DemoAssets assets;
    DemoScene scene;
    Player player;
    Camera debug_camera;
    bool use_debug_camera;
    bool show_texture;
    bool show_wireframe;
    bool light_follows_camera;
} Game;

bool game_init(
    Game *game,
    SDL_Window *window,
    SDL_GPUDevice *device,
    const char *asset_root
);

void game_shutdown(
    Game *game,
    SDL_GPUDevice *device
);

// Returns true when the player asks to quit.
bool game_handle_event(
    Game *game,
    SDL_Window *window,
    const SDL_Event *event
);

void game_update(
    Game *game,
    float dt
);

bool game_render(
    const Game *game,
    Renderer *renderer,
    SDL_Window *window
);

#endif
