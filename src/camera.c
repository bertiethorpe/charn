#include "camera.h"

#include <math.h>

Vec3 camera_forward_direction(const Camera *camera) {
    return (Vec3){
        .x = cosf(camera->pitch) * sinf(camera->yaw),
        .y = sinf(camera->pitch),
        .z = cosf(camera->pitch) * cosf(camera->yaw)
    };
}

Mat4 camera_view_matrix(const Camera *camera) {
    Vec3 target = vec3_add(
        camera->position,
        camera_forward_direction(camera)
    );

    return mat4_look_at(
        camera->position,
        target,
        (Vec3){0.0f, 1.0f, 0.0f}
    );
}

void camera_rotate_fps(Camera *camera, float yaw_delta, float pitch_delta) {
    const float maximum_pitch = 1.553343f;

    camera->yaw += yaw_delta;
    camera->pitch += pitch_delta;

    if (camera->pitch < -maximum_pitch) {
        camera->pitch = -maximum_pitch;
    }
    if (camera->pitch > maximum_pitch) {
        camera->pitch = maximum_pitch;
    }
}
