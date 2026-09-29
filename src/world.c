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