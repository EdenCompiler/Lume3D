#ifndef LUME_SCENE_H
#define LUME_SCENE_H
#include <lume/math.h>

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct LumeScene LumeScene;
typedef struct LumeNode LumeNode;
typedef struct LumeGeometry LumeGeometry;
typedef struct LumeMaterial LumeMaterial;

typedef struct LumePerspectiveCameraConfig
{
    float field_of_view_radians, aspect_ratio, near_plane, far_plane;
} LumePerspectiveCameraConfig;
typedef struct LumeOrthographicCameraConfig
{
    float left, right, bottom, top, near_plane, far_plane;
} LumeOrthographicCameraConfig;
typedef struct LumeAmbientLightConfig
{
    LumeColor color;
    float intensity;
} LumeAmbientLightConfig;
typedef struct LumeDirectionalLightConfig
{
    LumeColor color;
    float intensity;
    LumeVec3 direction;
    bool cast_shadows;
} LumeDirectionalLightConfig;
typedef struct LumePointLightConfig
{
    LumeColor color;
    float intensity, range;
} LumePointLightConfig;
typedef struct LumeSpotLightConfig
{
    LumeColor color;
    float intensity, range, inner_angle, outer_angle;
    LumeVec3 direction;
    bool cast_shadows;
} LumeSpotLightConfig;
typedef struct LumeRaycastHit
{
    LumeNode *node;
    float distance;
    LumeVec3 position, normal;
    uint32_t triangle_index;
} LumeRaycastHit;

LUME_API LumeResult lume_scene_create(LumeApp *app, LumeScene **out_scene);
LUME_API void lume_scene_destroy(LumeScene *scene);
LUME_API LumeResult lume_node_create(LumeScene *scene, LumeNode **out_node);
LUME_API void lume_node_destroy(LumeNode *node);
LUME_API LumeResult lume_node_add_child(LumeNode *parent, LumeNode *child);
LUME_API void lume_node_remove_from_parent(LumeNode *node);
LUME_API void lume_node_set_name(LumeNode *node, const char *name);
LUME_API const char *lume_node_name(const LumeNode *node);
LUME_API void lume_node_set_position(LumeNode *node, LumeVec3 position);
LUME_API void lume_node_set_rotation(LumeNode *node, LumeQuat rotation);
LUME_API void lume_node_set_euler_rotation(LumeNode *node, LumeVec3 radians);
LUME_API void lume_node_set_scale(LumeNode *node, LumeVec3 scale);
LUME_API LumeVec3 lume_node_position(const LumeNode *node);
LUME_API LumeQuat lume_node_rotation(const LumeNode *node);
LUME_API LumeVec3 lume_node_scale(const LumeNode *node);
LUME_API LumeMat4 lume_node_world_matrix(const LumeNode *node);
LUME_API LumeAabb lume_node_world_bounds(const LumeNode *node);
LUME_API void lume_node_translate(LumeNode *node, LumeVec3 offset);
LUME_API void lume_node_rotate_x(LumeNode *node, float radians);
LUME_API void lume_node_rotate_y(LumeNode *node, float radians);
LUME_API void lume_node_rotate_z(LumeNode *node, float radians);
LUME_API LumeResult lume_node_look_at(LumeNode *node, LumeVec3 target);
LUME_API LumePerspectiveCameraConfig lume_perspective_camera_config_default(void);
LUME_API LumeOrthographicCameraConfig lume_orthographic_camera_config_default(void);
LUME_API LumeResult lume_camera_create_perspective(LumeScene *scene, const LumePerspectiveCameraConfig *config,
                                                   LumeNode **out_camera);
LUME_API LumeResult lume_camera_create_orthographic(LumeScene *scene, const LumeOrthographicCameraConfig *config,
                                                    LumeNode **out_camera);
LUME_API LumeResult lume_camera_set_aspect_ratio(LumeNode *camera, float aspect_ratio);
LUME_API LumeAmbientLightConfig lume_ambient_light_config_default(void);
LUME_API LumeDirectionalLightConfig lume_directional_light_config_default(void);
LUME_API LumePointLightConfig lume_point_light_config_default(void);
LUME_API LumeSpotLightConfig lume_spot_light_config_default(void);
LUME_API LumeResult lume_ambient_light_create(LumeScene *scene, const LumeAmbientLightConfig *config,
                                              LumeNode **out_light);
LUME_API LumeResult lume_directional_light_create(LumeScene *scene, const LumeDirectionalLightConfig *config,
                                                  LumeNode **out_light);
LUME_API LumeResult lume_point_light_create(LumeScene *scene, const LumePointLightConfig *config, LumeNode **out_light);
LUME_API LumeResult lume_spot_light_create(LumeScene *scene, const LumeSpotLightConfig *config, LumeNode **out_light);
LUME_API LumeResult lume_mesh_create(LumeScene *scene, LumeGeometry *geometry, LumeMaterial *material,
                                     LumeNode **out_mesh);
LUME_API LumeResult lume_instanced_mesh_create(LumeScene *scene, LumeGeometry *geometry, LumeMaterial *material,
                                               uint32_t capacity, LumeNode **out_mesh);
LUME_API LumeResult lume_instanced_mesh_set_transform(LumeNode *mesh, uint32_t index, LumeMat4 transform);
LUME_API size_t lume_scene_raycast(LumeScene *scene, LumeRay ray, LumeRaycastHit *hits, size_t capacity);

#ifdef __cplusplus
}
#endif
#endif
