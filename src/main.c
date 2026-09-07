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

// -------------------- Data --------------------
typedef struct {
    float x, y;
    float angle;
    float angular_vel;
    float size;
} Triangle;

// -------------------- Helpers --------------------
SDL_Vertex Vertex(float x, float y, SDL_FColor color) {
    SDL_Vertex v = {0};
    v.position.x = x;
    v.position.y = y;
    v.color = color;
    v.tex_coord.x = 0.0f;
    v.tex_coord.y = 0.0f;
    return v;
}

SDL_FPoint rotate_point(float x, float y, float cx, float cy, float angle) {
    float s = sinf(angle);
    float c = cosf(angle);

    x -= cx;
    y -= cy;

    float xnew = x * c - y * s;
    float ynew = x * s + y * c;

    SDL_FPoint p = { xnew + cx, ynew + cy };
    return p;
}

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

// -------------------- Rendering Helpers --------------------
void clear_screen(void) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
}

void draw_background(SDL_FRect *viewport) {
    SDL_SetRenderDrawColor(renderer, 100, 149, 237, 255);
    SDL_RenderFillRect(renderer, viewport);
}

// -------------------- Triangle Logic --------------------
void update_triangle(Triangle *t, float dt) {
    t->angle += t->angular_vel * dt;

    // keep angle in range (prevents floating drift)
    if (t->angle > 6.283185f) {
        t->angle -= 6.283185f;
    }
}

void draw_triangle(Triangle *t) {
    float cx = t->x;
    float cy = t->y;

    float side = t->size;
    float triangle_height = (sqrtf(3.0f) / 2.0f) * side;

    SDL_FPoint p1 = { cx, cy - (2.0f/3.0f) * triangle_height };
    SDL_FPoint p2 = { cx - side / 2.0f, cy + (1.0f/3.0f) * triangle_height };
    SDL_FPoint p3 = { cx + side / 2.0f, cy + (1.0f/3.0f) * triangle_height };

    p1 = rotate_point(p1.x, p1.y, cx, cy, t->angle);
    p2 = rotate_point(p2.x, p2.y, cx, cy, t->angle);
    p3 = rotate_point(p3.x, p3.y, cx, cy, t->angle);

    SDL_FColor red = {1.0f, 0.0f, 0.0f, 1.0f};
    SDL_FColor green = {0.0f, 1.0f, 0.0f, 1.0f};
    SDL_FColor blue = {0.0f, 0.0f, 1.0f, 1.0f};

    SDL_Vertex vertices[3] = {
        Vertex(p1.x, p1.y, red),
        Vertex(p2.x, p2.y, green),
        Vertex(p3.x, p3.y, blue)
    };

    int indices[3] = {0, 1, 2};
    SDL_RenderGeometry(renderer, NULL, vertices, 3, indices, 3);
}

// -------------------- Render --------------------
void render(Triangle *tri) {
    clear_screen();

    SDL_FRect viewport = {0.0f, 0.0f, (float)width, (float)height};

    draw_background(&viewport);

    // SDL scales these logical coordinates to the window automatically.
    tri->x = width / 2.0f;
    tri->y = height / 2.0f;

    draw_triangle(tri);

    SDL_RenderPresent(renderer);
}

// -------------------- Game Loop --------------------
void run(void) {
    bool quit = false;

    Triangle tri = {0};
    tri.angle = 0.0f;
    tri.angular_vel = 2.0f;
    tri.size = 300.0f;

    Uint64 last = SDL_GetPerformanceCounter();  // ← matches the counter used in the loop
    Uint64 freq = SDL_GetPerformanceFrequency();

    while (!quit) {
        Uint64 now = SDL_GetPerformanceCounter();
        
        float dt = (now - last) / (float)freq;
        last = now;

        // Treat long interruptions, such as macOS live resizing, as paused time.
        if (dt > 0.1f) {
            dt = 0.0f;
        }

        process_input(&quit);
        update_triangle(&tri, dt);
        render(&tri);
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
