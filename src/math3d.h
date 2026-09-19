#ifndef MATH3D_H
#define MATH3D_H

typedef struct {
    float x;
    float y;
    float z;
} Vec3;

typedef struct {
    float values[16];
} Mat4;

Vec3 vec3_add(Vec3 left, Vec3 right);
Vec3 vec3_subtract(Vec3 left, Vec3 right);
Vec3 vec3_scale(Vec3 vector, float scale);
float vec3_dot(Vec3 left, Vec3 right);
Vec3 vec3_cross(Vec3 left, Vec3 right);
float vec3_length(Vec3 vector);
Vec3 vec3_normalise(Vec3 vector);

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
Mat4 mat4_look_at(Vec3 eye, Vec3 target, Vec3 world_up);

#endif
