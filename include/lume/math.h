#ifndef LUME_MATH_H
#define LUME_MATH_H
#include <lume/core.h>

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct LumeVec2
{
    float x, y;
} LumeVec2;
typedef struct LumeVec3
{
    float x, y, z;
} LumeVec3;
typedef struct LumeVec4
{
    float x, y, z, w;
} LumeVec4;
typedef struct LumeQuat
{
    float x, y, z, w;
} LumeQuat;
typedef struct LumeMat4
{
    float values[16];
} LumeMat4;
typedef struct LumeRay
{
    LumeVec3 origin, direction;
} LumeRay;
typedef struct LumeAabb
{
    LumeVec3 min, max;
} LumeAabb;
typedef struct LumeSphere
{
    LumeVec3 center;
    float radius;
} LumeSphere;
typedef struct LumePlane
{
    LumeVec3 normal;
    float distance;
} LumePlane;
typedef struct LumeFrustum
{
    LumePlane planes[6];
} LumeFrustum;

LUME_API LumeVec3 lume_vec3_add(LumeVec3 a, LumeVec3 b);
LUME_API LumeVec3 lume_vec3_subtract(LumeVec3 a, LumeVec3 b);
LUME_API LumeVec3 lume_vec3_scale(LumeVec3 vector, float scalar);
LUME_API float lume_vec3_dot(LumeVec3 a, LumeVec3 b);
LUME_API LumeVec3 lume_vec3_cross(LumeVec3 a, LumeVec3 b);
LUME_API float lume_vec3_length(LumeVec3 vector);
LUME_API LumeVec3 lume_vec3_normalize(LumeVec3 vector);
LUME_API LumeQuat lume_quat_identity(void);
LUME_API LumeQuat lume_quat_from_euler(LumeVec3 radians);
LUME_API LumeQuat lume_quat_slerp(LumeQuat a, LumeQuat b, float amount);
LUME_API LumeMat4 lume_mat4_identity(void);
LUME_API LumeMat4 lume_mat4_multiply(LumeMat4 left, LumeMat4 right);
LUME_API LumeMat4 lume_mat4_transform(LumeVec3 position, LumeQuat rotation, LumeVec3 scale);
LUME_API LumeMat4 lume_mat4_perspective(float fov_radians, float aspect, float near_plane, float far_plane);
LUME_API LumeMat4 lume_mat4_orthographic(float left, float right, float bottom, float top, float near_plane,
                                         float far_plane);
LUME_API bool lume_mat4_inverse(LumeMat4 matrix, LumeMat4 *out_inverse);
LUME_API LumeVec3 lume_mat4_transform_point(LumeMat4 matrix, LumeVec3 point);
LUME_API LumeAabb lume_aabb_empty(void);
LUME_API LumeAabb lume_aabb_expand_point(LumeAabb bounds, LumeVec3 point);
LUME_API LumeAabb lume_aabb_transform(LumeAabb bounds, LumeMat4 transform);
LUME_API bool lume_ray_intersect_aabb(LumeRay ray, LumeAabb bounds, float *out_distance);
LUME_API LumeFrustum lume_frustum_from_matrix(LumeMat4 view_projection);
LUME_API bool lume_frustum_intersects_aabb(LumeFrustum frustum, LumeAabb bounds);

#ifdef __cplusplus
}
#endif
#endif
