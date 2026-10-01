#ifndef RENDERER_H
#define RENDERER_H

#include <stdbool.h>
#include <stddef.h>

#include <SDL3/SDL.h>

#include "mesh.h"
#include "texture.h"
#include "world.h"

typedef struct {
    Texture *texture;
    float tint[4];
} Material;

typedef struct {
    EntityId entity;
    const Mesh *mesh;
    const Material *material;
} RenderObject;

typedef struct {
    Vec3 light_position;
    float ambient_strength;
    float point_light_strength;
    float falloff_distance;
} SceneLighting;

typedef struct {
    SDL_GPUDevice *device;
    bool window_claimed;

    SDL_GPUGraphicsPipeline *filled_pipeline;
    SDL_GPUGraphicsPipeline *wireframe_pipeline;

    SDL_GPUTexture *depth_texture;
    SDL_GPUTextureFormat depth_texture_format;
    Uint32 depth_texture_width;
    Uint32 depth_texture_height;
} Renderer;

bool renderer_init(
    Renderer *renderer,
    SDL_Window *window
);

bool renderer_ensure_depth_texture(
    Renderer *renderer,
    Uint32 width,
    Uint32 height
);

bool renderer_draw(
    Renderer *renderer,
    SDL_Window *window,
    const World *world,
    Mat4 view,
    SceneLighting lighting,
    bool show_texture,
    bool show_wireframe,
    const RenderObject *objects,
    size_t object_count
);

void renderer_destroy(
    Renderer *renderer,
    SDL_Window *window
);

#endif
