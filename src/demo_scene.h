#ifndef DEMO_SCENE_H
#define DEMO_SCENE_H

#include "renderer.h"

typedef struct {
    World world;
    EntityId rotating_cube;
    RenderObject objects[5];
    size_t object_count;
    SceneLighting lighting;
    float angle_x;
    float angle_y;
} DemoScene;

bool demo_scene_init(
    DemoScene *scene,
    const Mesh *cube_mesh,
    const Mesh *floor_mesh,
    const Mesh *suzanne_mesh,
    const Material *cube_material,
    const Material *floor_material
);

void demo_scene_update(DemoScene *scene, float dt);

#endif
