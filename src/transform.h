#ifndef TRANSFORM_H
#define TRANSFORM_H

#include "math3d.h"

typedef struct {
    Vec3 position;
    Vec3 rotation;
    Vec3 scale;
} Transform;

Transform transform_identity(void);
Mat4 transform_to_matrix(Transform transform);

#endif