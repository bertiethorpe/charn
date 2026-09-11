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

static SDL_GPUShader *vertex_shader = NULL;
static SDL_GPUShader *fragment_shader = NULL;
static SDL_GPUGraphicsPipeline *graphics_pipeline = NULL;

// ------------------------- Data --------------------------
typedef struct {
    float position[3];
    float color[4];
} Vertex;

static const Vertex triangle_vertices[] = {
    {
        .position = { 0.0f,  0.5f, 0.0f},
        .color =    { 1.0f,  0.0f, 0.0f, 1.0f}
    },
    {
        .position = {-0.5f, -0.5f, 0.0f},
        .color =    { 0.0f,  1.0f, 0.0f, 1.0f}
    },
    {
        .position = { 0.5f, -0.5f, 0.0f},
        .color =    { 0.0f,  0.0f, 1.0f, 1.0f}
    }
};

// -------------------- Init / Shutdown --------------------
void shutdown(void);

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

    size_t vertex_shader_size = 0;
    Uint8 *vertex_shader_code = SDL_LoadFile(
        "shaders/triangle.vert.msl",
        &vertex_shader_size
    );

    if (!vertex_shader_code) {
        SDL_Log("Could not load vertex shader: %s", SDL_GetError());
        shutdown();
        return false;
    }

    size_t fragment_shader_size = 0;
    Uint8 *fragment_shader_code = SDL_LoadFile(
        "shaders/triangle.frag.msl",
        &fragment_shader_size
    );

    if (!fragment_shader_code) {
        SDL_Log("Could not load fragment shader: %s", SDL_GetError());
        SDL_free(vertex_shader_code);
        shutdown();
        return false;
    }

    SDL_Log("Loaded vertex shader: %zu bytes", vertex_shader_size);
    SDL_Log("Loaded fragment shader: %zu bytes", fragment_shader_size);

    SDL_GPUShaderCreateInfo vertex_shader_info = {0};
    vertex_shader_info.code = vertex_shader_code;
    vertex_shader_info.code_size = vertex_shader_size;
    vertex_shader_info.entrypoint = "vertex_main";
    vertex_shader_info.format = SDL_GPU_SHADERFORMAT_MSL;
    vertex_shader_info.stage = SDL_GPU_SHADERSTAGE_VERTEX;

    vertex_shader = SDL_CreateGPUShader(
        gpu_device,
        &vertex_shader_info
    );

    if (!vertex_shader) {
        SDL_Log("Could not create vertex shader: %s", SDL_GetError());
        SDL_free(vertex_shader_code);
        SDL_free(fragment_shader_code);
        shutdown();
        return false;
    }

    SDL_GPUShaderCreateInfo fragment_shader_info = {0};
    fragment_shader_info.code = fragment_shader_code;
    fragment_shader_info.code_size = fragment_shader_size;
    fragment_shader_info.entrypoint = "fragment_main";
    fragment_shader_info.format = SDL_GPU_SHADERFORMAT_MSL;
    fragment_shader_info.stage = SDL_GPU_SHADERSTAGE_FRAGMENT;

    fragment_shader = SDL_CreateGPUShader(
        gpu_device,
        &fragment_shader_info
    );

    if (!fragment_shader) {
        SDL_Log("Could not create fragment shader: %s", SDL_GetError());
        shutdown();
        return false;
    }

    SDL_Log("Created both GPU shaders");

    SDL_free(vertex_shader_code);
    SDL_free(fragment_shader_code);

    SDL_GPUColorTargetDescription color_target_description = {0};
    color_target_description.format =
        SDL_GetGPUSwapchainTextureFormat(gpu_device, window);

    SDL_GPUGraphicsPipelineCreateInfo pipeline_info = {0};

    pipeline_info.vertex_shader = vertex_shader;
    pipeline_info.fragment_shader = fragment_shader;
    pipeline_info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    pipeline_info.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;

    pipeline_info.target_info.num_color_targets = 1;
    pipeline_info.target_info.color_target_descriptions =
        &color_target_description;

    graphics_pipeline = SDL_CreateGPUGraphicsPipeline(
        gpu_device,
        &pipeline_info
    );

    if (!graphics_pipeline) {
        SDL_Log("Could not create graphics pipeline: %s", SDL_GetError());
        shutdown();
        return false;
    }

    SDL_Log("Created graphics pipeline");
    
    SDL_ReleaseGPUShader(gpu_device, vertex_shader);
    SDL_ReleaseGPUShader(gpu_device, fragment_shader);

    vertex_shader = NULL;
    fragment_shader = NULL;

    SDL_SetWindowPosition(window, 50, 100);
    return true;
}

void shutdown(void) {
    if (gpu_device) {
        if (graphics_pipeline) {
            SDL_ReleaseGPUGraphicsPipeline(
                gpu_device,
                graphics_pipeline
            );
        }

        if (vertex_shader) {
            SDL_ReleaseGPUShader(gpu_device, vertex_shader);
        }

        if (fragment_shader) {
            SDL_ReleaseGPUShader(gpu_device, fragment_shader);
        }
    }

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

        SDL_BindGPUGraphicsPipeline(
            render_pass,
            graphics_pipeline
        );

        SDL_DrawGPUPrimitives(
            render_pass,
            3,  // three vertices
            1,  // one triangle instance
            0,  // begin at vertex zero
            0   // begin at instance zero
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
