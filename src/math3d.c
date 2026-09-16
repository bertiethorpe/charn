#include <math.h>

#include "math3d.h"

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
