#ifndef WORLD_H
#define WORLD_H

#include <stdbool.h>
#include <stdint.h>

#include "transform.h"

#define WORLD_MAX_ENTITIES 1024

typedef struct {
    uint32_t index;
    uint32_t generation;
} EntityId;

typedef struct {
    uint32_t mesh_id;
    uint32_t material_id;
} MeshRenderer;

typedef struct {
    bool alive[WORLD_MAX_ENTITIES];
    uint32_t generations[WORLD_MAX_ENTITIES];
    Transform transforms[WORLD_MAX_ENTITIES];
    bool has_mesh_renderer[WORLD_MAX_ENTITIES];
    MeshRenderer mesh_renderers[WORLD_MAX_ENTITIES];
} World;

void world_init(World *world);

EntityId world_create_entity(World *world);

bool world_is_alive(
    const World *world,
    EntityId entity
);

bool world_destroy_entity(
    World *world,
    EntityId entity
);

Transform *world_get_transform(
    World *world,
    EntityId entity
);

const Transform *world_get_transform_const(
    const World *world,
    EntityId entity
);

bool world_set_mesh_renderer(
    World *world,
    EntityId entity,
    MeshRenderer mesh_renderer
);

const MeshRenderer *world_get_mesh_renderer_const(
    const World *world,
    EntityId entity
);

#endif