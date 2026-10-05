#ifndef CAMERA_H
#define CAMERA_H

#include "math3d.h"

typedef struct {
    Vec3 position;
    float yaw;
    float pitch;
} Camera;

Vec3 camera_forward_direction(const Camera *camera);
Mat4 camera_view_matrix(const Camera *camera);
void camera_rotate_fps(Camera *camera, float yaw_delta, float pitch_delta);

#endif
