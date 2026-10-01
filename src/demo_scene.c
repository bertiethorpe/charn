#include "demo_scene.h"

bool demo_scene_init(
    DemoScene *scene,
    const Mesh *cube_mesh,
    const Mesh *floor_mesh,
    const Material *cube_material,
    const Material *floor_material
) {
    if (!scene || !cube_mesh || !floor_mesh ||
        !cube_material || !floor_material) {
        SDL_Log("Could not initialise demo scene");
        return false;
    }

    *scene = (DemoScene){0};
    world_init(&scene->world);

    scene->rotating_cube = world_create_entity(&scene->world);
    EntityId static_cube = world_create_entity(&scene->world);
    EntityId floor = world_create_entity(&scene->world);
    EntityId back_wall = world_create_entity(&scene->world);

    Transform *rotating_cube_transform =
        world_get_transform(&scene->world, scene->rotating_cube);
    Transform *static_cube_transform =
        world_get_transform(&scene->world, static_cube);
    Transform *floor_transform =
        world_get_transform(&scene->world, floor);
    Transform *back_wall_transform =
        world_get_transform(&scene->world, back_wall);

    if (!rotating_cube_transform || !static_cube_transform ||
        !floor_transform || !back_wall_transform) {
        SDL_Log("Could not create scene entities");
        return false;
    }

    rotating_cube_transform->position = (Vec3){0.0f, 1.0f, 0.0f};
    static_cube_transform->position = (Vec3){1.5f, 0.5f, 1.0f};
    floor_transform->position = (Vec3){0.0f, 0.0f, 0.0f};

    back_wall_transform->position = (Vec3){0.0f, 2.5f, 5.0f};
    back_wall_transform->rotation.x = -1.5707963f;
    back_wall_transform->scale.z = 0.5f;

    scene->objects[0] = (RenderObject){
        .entity = scene->rotating_cube,
        .mesh = cube_mesh,
        .material = cube_material
    };
    scene->objects[1] = (RenderObject){
        .entity = static_cube,
        .mesh = cube_mesh,
        .material = cube_material
    };
    scene->objects[2] = (RenderObject){
        .entity = floor,
        .mesh = floor_mesh,
        .material = floor_material
    };
    scene->objects[3] = (RenderObject){
        .entity = back_wall,
        .mesh = floor_mesh,
        .material = floor_material
    };

    scene->object_count =
        sizeof(scene->objects) / sizeof(scene->objects[0]);

    return true;
}

void demo_scene_update(DemoScene *scene, float dt) {
    const float two_pi = 6.28318530718f;

    scene->angle_x += 1.2f * dt;
    scene->angle_y += 2.0f * dt;

    if (scene->angle_x >= two_pi) {
        scene->angle_x -= two_pi;
    }
    if (scene->angle_y >= two_pi) {
        scene->angle_y -= two_pi;
    }

    Transform *transform =
        world_get_transform(&scene->world, scene->rotating_cube);

    if (transform) {
        transform->rotation.x = scene->angle_x;
        transform->rotation.y = scene->angle_y;
    }
}