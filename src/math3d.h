#ifndef MATH3D_H
#define MATH3D_H

typedef struct {
    float values[16];
} Mat4;

Mat4 mat4_identity(void);
Mat4 mat4_multiply(Mat4 left, Mat4 right);
Mat4 mat4_translation(float x, float y, float z);
Mat4 mat4_rotation_y(float angle);
Mat4 mat4_rotation_x(float angle);
Mat4 mat4_perspective_projection(
    float vertical_fov,
    float aspect,
    float near_plane,
    float far_plane
);

#endif
