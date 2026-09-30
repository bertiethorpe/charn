#include "transform.h"

Transform transform_identity(void) {
    return (Transform){
          .position = {0.0f, 0.0f, 0.0f},
          .rotation = {0.0f, 0.0f, 0.0f},
          .scale    = {1.0f, 1.0f, 1.0f}
    };
}

static Mat4 transform_rotation_matrix(Transform transform) {
    return mat4_multiply(
        mat4_rotation_z(transform.rotation.z),
        mat4_multiply(
            mat4_rotation_y(transform.rotation.y),
            mat4_rotation_x(transform.rotation.x)
        )
    );
}

Mat4 transform_to_matrix(Transform transform) {
    Mat4 translation = mat4_translation(
        transform.position.x,
        transform.position.y,
        transform.position.z
    );

    Mat4 rotation = transform_rotation_matrix(transform);

    Mat4 scale = mat4_scale(
        transform.scale.x,
        transform.scale.y,
        transform.scale.z
    );

    return mat4_multiply(
        translation,
        mat4_multiply(rotation, scale)
    );
}
