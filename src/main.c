#include <stdbool.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "game.h"
#include "renderer.h"

static const int width = 800;
static const int height = 600;

static SDL_Window *window = NULL;
static Renderer renderer = {0};
static Game game = {0};

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

    SDL_SetWindowPosition(window, 50, 100);

    if (!game_init(&game, window, renderer.device)) {
        shutdown();
        return false;
    }

    return true;
}

static void shutdown(void) {
    game_shutdown(&game, renderer.device);
    renderer_destroy(&renderer, window);

    SDL_DestroyWindow(window);
    window = NULL;

    SDL_Quit();
}

static bool run(void) {
    bool quit = false;
    Uint64 last = SDL_GetPerformanceCounter();
    Uint64 frequency = SDL_GetPerformanceFrequency();
    float fps_elapsed = 0.0f;
    Uint32 fps_frame_count = 0;

    while (!quit) {
        Uint64 now = SDL_GetPerformanceCounter();
        float dt = (float)(now - last) / (float)frequency;
        last = now;

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT ||
                game_handle_event(&game, window, &event)) {
                quit = true;
            }
        }

        game_update(&game, dt);

        if (!game_render(&game, &renderer, window)) {
            return false;
        }

        fps_elapsed += dt;
        ++fps_frame_count;

        if (fps_elapsed >= 1.0f) {
            float average_fps = (float)fps_frame_count / fps_elapsed;

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

    return true;
}

int main(void) {
    if (!init()) {
        return 1;
    }

    bool success = run();
    shutdown();

    return success ? 0 : 1;
}
