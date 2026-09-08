#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

static const int width = 800;
static const int height = 600;

static SDL_Window *window = NULL;
static SDL_GPUDevice *gpu_device = NULL;

void shutdown(void);

// -------------------- Init / Shutdown --------------------
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

    SDL_SetWindowPosition(window, 50, 100);
    return true;
}

void shutdown(void) {
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
bool render(void){ 
    SDL_GPUCommandBuffer *command_buffer = 
        SDL_AcquireGPUCommandBuffer(gpu_device);

    if (!command_buffer) {
        SDL_Log("Could not acquire command buffer: %s", SDL_GetError());
        return false;
    }

    SDL_GPUTexture *swapchain_texture = NULL;

    if (!SDL_WaitAndAcquireGPUSwapchainTexture(
            command_buffer,
            window,
            &swapchain_texture,
            NULL,
            NULL)) {
        SDL_Log("Could not acquire swapchain texture: %s", SDL_GetError());
        SDL_CancelGPUCommandBuffer(command_buffer);
        return false;
    }

    // A minimized window may not have a swapchain texture.
    if (swapchain_texture) {
        SDL_GPUColorTargetInfo color_target = {0};

        color_target.texture = swapchain_texture;
        color_target.clear_color =
            (SDL_FColor){100.0f / 255.0f,
                        149.0f / 255.0f,
                        237.0f / 255.0f,
                        1.0f};
        color_target.load_op = SDL_GPU_LOADOP_CLEAR;
        color_target.store_op = SDL_GPU_STOREOP_STORE;

        SDL_GPURenderPass *render_pass =
            SDL_BeginGPURenderPass(
                command_buffer,
                &color_target,
                1,
                NULL
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

    while (!quit) {
        process_input(&quit);

        if (!render()) {
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
