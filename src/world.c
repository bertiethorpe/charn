#include <stddef.h>

#include "world.h"

void world_init(World *world) {
    *world = (World){0};
}

EntityId world_create_entity(World *world) {
    for (uint32_t index = 0; index < WORLD_MAX_ENTITIES; ++index) {
        if (world->alive[index]) {
            continue;
        }

        if (world->generations[index] == 0) {
            world->generations[index] = 1;
        }

        world->alive[index] = true;
        world->transforms[index] = transform_identity();
        world->has_mesh_renderer[index] = false;

        return (EntityId){
            .index      = index,
            .generation = world->generations[index]
        };
    }

    return (EntityId){
        .index      = WORLD_MAX_ENTITIES,
        .generation = 0
    };
}

bool world_is_alive(
    const World *world,
    EntityId entity
) {
    return
        entity.index < WORLD_MAX_ENTITIES &&
        world->alive[entity.index] &&
        world->generations[entity.index] == entity.generation;
}

bool world_destroy_entity(
    World *world,
    EntityId entity
) {
    if (!world_is_alive(world, entity)) {
        return false;
    }

    world->alive[entity.index] = false;
    world->has_mesh_renderer[entity.index] = false;
    ++world->generations[entity.index];

    if (world->generations[entity.index] == 0) {
        world->generations[entity.index] = 1;
    }

    return true;
}

Transform *world_get_transform(
    World *world,
    EntityId entity
) {
    if (!world_is_alive(world, entity)) {
        return NULL;
    }

    return &world->transforms[entity.index];
}

const Transform *world_get_transform_const(
    const World *world,
    EntityId entity
) {
    if (!world_is_alive(world, entity)) {
        return NULL;
    }

    return &world->transforms[entity.index];
}

bool world_set_mesh_renderer(
    World *world,
    EntityId entity,
    MeshRenderer mesh_renderer
) {
    if (!world_is_alive(world, entity)) {
        return false;
    }

    world->mesh_renderers[entity.index] = mesh_renderer;
    world->has_mesh_renderer[entity.index] = true;
    return true;
}

const MeshRenderer *world_get_mesh_renderer_const(
    const World *world,
    EntityId entity
) {
    if (!world_is_alive(world, entity) ||
        !world->has_mesh_renderer[entity.index]) {
        return NULL;
    }

    return &world->mesh_renderers[entity.index];
}
