#include <math.h>

#include "math3d.h"


Vec3 vec3_add(Vec3 left, Vec3 right) {
    return (Vec3){
        .x = left.x + right.x,
        .y = left.y + right.y,
        .z = left.z + right.z
    };
}

Vec3 vec3_subtract(Vec3 left, Vec3 right) {
    return (Vec3){
        .x = left.x - right.x,
        .y = left.y - right.y,
        .z = left.z - right.z
    };
}

Vec3 vec3_scale(Vec3 vector, float scale) {
    return (Vec3){
        .x = vector.x * scale,
        .y = vector.y * scale,
        .z = vector.z * scale
    };
}

float vec3_dot(Vec3 left, Vec3 right) {
    return
        left.x * right.x +
        left.y * right.y +
        left.z * right.z
    ;
}

Vec3 vec3_cross(Vec3 left, Vec3 right) {
    return (Vec3){
        .x = (left.y * right.z) - (left.z * right.y),
        .y = (left.z * right.x) - (left.x * right.z),
        .z = (left.x * right.y) - (left.y * right.x)
    };
}

float vec3_length(Vec3 vector) {
    return sqrtf(
        vector.x * vector.x +
        vector.y * vector.y +
        vector.z * vector.z
    );
}

Vec3 vec3_normalise(Vec3 vector) {
    float length = vec3_length(vector);

    if (length <= 0.000001f) {
        return (Vec3){0.0f, 0.0f, 0.0f};
    }

    return vec3_scale(vector, 1.0f / length);
}

Mat4 mat4_identity(void) {
    return (Mat4){
        .values = {
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f
        }
    };
}

Mat4 mat4_multiply(Mat4 left, Mat4 right) {
    Mat4 result = {0};

    for (int column = 0; column < 4; ++column) {
        for (int row = 0; row < 4; ++row) {
            for (int index = 0; index < 4; ++index) {
                result.values[column * 4 + row] +=
                    left.values[index * 4 + row] *
                    right.values[column * 4 + index];
            }
        }
    }
    return result;
}

Mat4 mat4_translation(float x, float y, float z) {
    Mat4 result = mat4_identity();

    result.values[12] = x;
    result.values[13] = y;
    result.values[14] = z;

    return result;
}

Mat4 mat4_rotation_y(float angle) {
    float c = cosf(angle);
    float s = sinf(angle);

    return (Mat4){
        .values = {
            c,    0.0f, -s,    0.0f,
            0.0f, 1.0f,  0.0f, 0.0f,
            s,    0.0f,  c,    0.0f,
            0.0f, 0.0f,  0.0f, 1.0f
        }
    };
}

Mat4 mat4_rotation_x(float angle) {
    float c = cosf(angle);
    float s = sinf(angle);

    return (Mat4){
        .values = {
            1.0f,  0.0f, 0.0f, 0.0f,
            0.0f,  c,    s,    0.0f,
            0.0f, -s,    c,    0.0f,
            0.0f,  0.0f, 0.0f, 1.0f
        }
    };
}

Mat4 mat4_perspective_projection(
    float vertical_fov,
    float aspect,
    float near_plane,
    float far_plane
) {
    float focal_length = 1.0f / tanf(vertical_fov * 0.5f);
    float depth_range = far_plane - near_plane;

    return (Mat4){
        .values = {
            focal_length / aspect, 0.0f,         0.0f,                    0.0f,
            0.0f,                  focal_length, 0.0f,                    0.0f,
            0.0f,                  0.0f,         far_plane / depth_range, 1.0f,
            0.0f,                  0.0f,        -near_plane * far_plane /
                                                    depth_range,          0.0f
        }
    };
}

Mat4 mat4_look_at(Vec3 eye, Vec3 target, Vec3 world_up) {
    Vec3 forward = vec3_normalise(vec3_subtract(target, eye));
    Vec3 right = vec3_normalise(vec3_cross(world_up, forward));
    Vec3 up = vec3_cross(forward, right);

    return (Mat4){
        .values = {
            right.x,              up.x,              forward.x,              0.0f,
            right.y,              up.y,              forward.y,              0.0f,
            right.z,              up.z,              forward.z,              0.0f,
            -vec3_dot(right, eye), -vec3_dot(up, eye), -vec3_dot(forward, eye), 1.0f
        }
    };
}
