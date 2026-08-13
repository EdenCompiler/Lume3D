#ifndef LUME_LUME_H
#define LUME_LUME_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#if defined(_WIN32) && defined(LUME_SHARED)
#if defined(LUME_BUILDING_LIBRARY)
#define LUME_API __declspec(dllexport)
#else
#define LUME_API __declspec(dllimport)
#endif
#elif defined(__GNUC__) || defined(__clang__)
#define LUME_API __attribute__((visibility("default")))
#else
#define LUME_API
#endif

#ifdef __cplusplus
extern "C"
{
#endif

#define LUME_VERSION_MAJOR 1
#define LUME_VERSION_MINOR 0
#define LUME_VERSION_PATCH 0

typedef struct LumeApp LumeApp;
typedef struct LumeScene LumeScene;
typedef struct LumeNode LumeNode;
typedef struct LumeGeometry LumeGeometry;
typedef struct LumeMaterial LumeMaterial;
typedef struct LumeTexture LumeTexture;

typedef struct LumeVec2
{
    float x;
    float y;
} LumeVec2;

typedef struct LumeVec3
{
    float x;
    float y;
    float z;
} LumeVec3;

typedef struct LumeColor
{
    float r;
    float g;
    float b;
    float a;
} LumeColor;

typedef enum LumeLogLevel
{
    LUME_LOG_INFO,
    LUME_LOG_WARNING,
    LUME_LOG_ERROR
} LumeLogLevel;

typedef void (*LumeLogCallback)(LumeLogLevel level, const char *message, void *user_data);

typedef struct LumeAppConfig
{
    const char *title;
    int width;
    int height;
    bool resizable;
    bool visible;
    bool vsync;
    LumeColor clear_color;
    LumeLogCallback log_callback;
    void *log_user_data;
} LumeAppConfig;

/* Teclas seguem os valores portáteis usados pelo GLFW. */
typedef enum LumeKey
{
    LUME_KEY_UNKNOWN = -1,
    LUME_KEY_SPACE = 32,
    LUME_KEY_APOSTROPHE = 39,
    LUME_KEY_COMMA = 44,
    LUME_KEY_MINUS = 45,
    LUME_KEY_PERIOD = 46,
    LUME_KEY_SLASH = 47,
    LUME_KEY_0 = 48,
    LUME_KEY_1 = 49,
    LUME_KEY_2 = 50,
    LUME_KEY_3 = 51,
    LUME_KEY_4 = 52,
    LUME_KEY_5 = 53,
    LUME_KEY_6 = 54,
    LUME_KEY_7 = 55,
    LUME_KEY_8 = 56,
    LUME_KEY_9 = 57,
    LUME_KEY_A = 65,
    LUME_KEY_B = 66,
    LUME_KEY_C = 67,
    LUME_KEY_D = 68,
    LUME_KEY_E = 69,
    LUME_KEY_F = 70,
    LUME_KEY_G = 71,
    LUME_KEY_H = 72,
    LUME_KEY_I = 73,
    LUME_KEY_J = 74,
    LUME_KEY_K = 75,
    LUME_KEY_L = 76,
    LUME_KEY_M = 77,
    LUME_KEY_N = 78,
    LUME_KEY_O = 79,
    LUME_KEY_P = 80,
    LUME_KEY_Q = 81,
    LUME_KEY_R = 82,
    LUME_KEY_S = 83,
    LUME_KEY_T = 84,
    LUME_KEY_U = 85,
    LUME_KEY_V = 86,
    LUME_KEY_W = 87,
    LUME_KEY_X = 88,
    LUME_KEY_Y = 89,
    LUME_KEY_Z = 90,
    LUME_KEY_ESCAPE = 256,
    LUME_KEY_ENTER = 257,
    LUME_KEY_TAB = 258,
    LUME_KEY_BACKSPACE = 259,
    LUME_KEY_INSERT = 260,
    LUME_KEY_DELETE = 261,
    LUME_KEY_RIGHT = 262,
    LUME_KEY_LEFT = 263,
    LUME_KEY_DOWN = 264,
    LUME_KEY_UP = 265,
    LUME_KEY_PAGE_UP = 266,
    LUME_KEY_PAGE_DOWN = 267,
    LUME_KEY_HOME = 268,
    LUME_KEY_END = 269,
    LUME_KEY_LEFT_SHIFT = 340,
    LUME_KEY_LEFT_CONTROL = 341,
    LUME_KEY_LEFT_ALT = 342,
    LUME_KEY_RIGHT_SHIFT = 344,
    LUME_KEY_RIGHT_CONTROL = 345,
    LUME_KEY_RIGHT_ALT = 346
} LumeKey;

typedef enum LumeMouseButton
{
    LUME_MOUSE_BUTTON_LEFT = 0,
    LUME_MOUSE_BUTTON_RIGHT = 1,
    LUME_MOUSE_BUTTON_MIDDLE = 2
} LumeMouseButton;

/* Ciclo de vida do aplicativo, frame e input stateful. */
LUME_API LumeAppConfig lume_app_config_default(void);
LUME_API LumeApp *lume_app_create(const LumeAppConfig *config);
LUME_API void lume_app_destroy(LumeApp *app);
LUME_API bool lume_app_should_close(const LumeApp *app);
LUME_API void lume_app_request_close(LumeApp *app);
LUME_API float lume_app_begin_frame(LumeApp *app);
LUME_API void lume_app_end_frame(LumeApp *app);
LUME_API void lume_app_set_clear_color(LumeApp *app, LumeColor color);
LUME_API void lume_app_get_framebuffer_size(const LumeApp *app, int *width, int *height);
LUME_API bool lume_key_is_down(const LumeApp *app, LumeKey key);
LUME_API bool lume_key_was_pressed(const LumeApp *app, LumeKey key);
LUME_API bool lume_key_was_released(const LumeApp *app, LumeKey key);
LUME_API bool lume_mouse_button_is_down(const LumeApp *app, LumeMouseButton button);
LUME_API bool lume_mouse_button_was_pressed(const LumeApp *app, LumeMouseButton button);
LUME_API bool lume_mouse_button_was_released(const LumeApp *app, LumeMouseButton button);
LUME_API LumeVec2 lume_mouse_position(const LumeApp *app);
LUME_API LumeVec2 lume_mouse_delta(const LumeApp *app);
LUME_API LumeVec2 lume_mouse_scroll(const LumeApp *app);
LUME_API const char *lume_get_last_error(void);

/* Grafo de cena e transformações hierárquicas. */
LUME_API LumeScene *lume_scene_create(LumeApp *app);
LUME_API void lume_scene_destroy(LumeScene *scene);
LUME_API LumeNode *lume_node_create(LumeScene *scene);
LUME_API void lume_node_destroy(LumeNode *node);
LUME_API bool lume_node_add_child(LumeNode *parent, LumeNode *child);
LUME_API void lume_node_remove_from_parent(LumeNode *node);
LUME_API void lume_node_set_position(LumeNode *node, LumeVec3 position);
LUME_API void lume_node_set_rotation(LumeNode *node, LumeVec3 rotation_radians);
LUME_API void lume_node_set_scale(LumeNode *node, LumeVec3 scale);
LUME_API LumeVec3 lume_node_get_position(const LumeNode *node);
LUME_API LumeVec3 lume_node_get_rotation(const LumeNode *node);
LUME_API LumeVec3 lume_node_get_scale(const LumeNode *node);
LUME_API void lume_node_translate(LumeNode *node, LumeVec3 offset);
LUME_API void lume_node_rotate_x(LumeNode *node, float radians);
LUME_API void lume_node_rotate_y(LumeNode *node, float radians);
LUME_API void lume_node_rotate_z(LumeNode *node, float radians);
LUME_API bool lume_node_look_at(LumeNode *node, LumeVec3 target);

typedef struct LumePerspectiveCameraConfig
{
    float field_of_view_radians;
    float aspect_ratio;
    float near_plane;
    float far_plane;
} LumePerspectiveCameraConfig;

typedef struct LumeOrthographicCameraConfig
{
    float left;
    float right;
    float bottom;
    float top;
    float near_plane;
    float far_plane;
} LumeOrthographicCameraConfig;

/* Câmeras perspectiva e ortográfica. */
LUME_API LumePerspectiveCameraConfig lume_perspective_camera_config_default(void);
LUME_API LumeOrthographicCameraConfig lume_orthographic_camera_config_default(void);
LUME_API LumeNode *lume_camera_create_perspective(LumeScene *scene, const LumePerspectiveCameraConfig *config);
LUME_API LumeNode *lume_camera_create_orthographic(LumeScene *scene, const LumeOrthographicCameraConfig *config);
LUME_API bool lume_camera_set_aspect_ratio(LumeNode *camera, float aspect_ratio);

typedef struct LumeGeometryData
{
    const float *positions;
    const float *normals;
    const float *texture_coordinates;
    size_t vertex_count;
    const uint32_t *indices;
    size_t index_count;
} LumeGeometryData;

/* Geometrias copiadas e pertencentes ao aplicativo. */
LUME_API LumeGeometry *lume_geometry_create_custom(LumeApp *app, const LumeGeometryData *data);
LUME_API LumeGeometry *lume_geometry_create_box(LumeApp *app, float width, float height, float depth);
LUME_API LumeGeometry *lume_geometry_create_plane(LumeApp *app, float width, float height);
LUME_API LumeGeometry *lume_geometry_create_sphere(LumeApp *app, float radius, uint32_t width_segments,
                                                   uint32_t height_segments);

typedef enum LumeTextureFilter
{
    LUME_TEXTURE_FILTER_NEAREST,
    LUME_TEXTURE_FILTER_LINEAR
} LumeTextureFilter;

typedef enum LumeTextureWrap
{
    LUME_TEXTURE_WRAP_CLAMP,
    LUME_TEXTURE_WRAP_REPEAT
} LumeTextureWrap;

typedef struct LumeTextureConfig
{
    LumeTextureFilter min_filter;
    LumeTextureFilter mag_filter;
    LumeTextureWrap wrap_u;
    LumeTextureWrap wrap_v;
    bool generate_mipmaps;
    bool flip_y;
} LumeTextureConfig;

/* Texturas RGBA e imagens carregadas pelo stb_image. */
LUME_API LumeTextureConfig lume_texture_config_default(void);
LUME_API LumeTexture *lume_texture_create(LumeApp *app, const uint8_t *rgba_pixels, int width, int height,
                                          const LumeTextureConfig *config);
LUME_API LumeTexture *lume_texture_load(LumeApp *app, const char *path, const LumeTextureConfig *config);

typedef struct LumeBasicMaterialConfig
{
    LumeColor color;
    LumeTexture *texture;
    bool wireframe;
} LumeBasicMaterialConfig;

typedef struct LumeLambertMaterialConfig
{
    LumeColor color;
    LumeTexture *texture;
    bool wireframe;
} LumeLambertMaterialConfig;

/* Materiais reutilizáveis por várias malhas. */
LUME_API LumeBasicMaterialConfig lume_basic_material_config_default(void);
LUME_API LumeLambertMaterialConfig lume_lambert_material_config_default(void);
LUME_API LumeMaterial *lume_material_create_basic(LumeApp *app, const LumeBasicMaterialConfig *config);
LUME_API LumeMaterial *lume_material_create_lambert(LumeApp *app, const LumeLambertMaterialConfig *config);
LUME_API void lume_material_set_color(LumeMaterial *material, LumeColor color);
LUME_API void lume_material_set_texture(LumeMaterial *material, LumeTexture *texture);

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
} LumeDirectionalLightConfig;

typedef struct LumePointLightConfig
{
    LumeColor color;
    float intensity;
    float range;
} LumePointLightConfig;

/* Luzes, malhas e entrada principal do renderizador. */
LUME_API LumeAmbientLightConfig lume_ambient_light_config_default(void);
LUME_API LumeDirectionalLightConfig lume_directional_light_config_default(void);
LUME_API LumePointLightConfig lume_point_light_config_default(void);
LUME_API LumeNode *lume_ambient_light_create(LumeScene *scene, const LumeAmbientLightConfig *config);
LUME_API LumeNode *lume_directional_light_create(LumeScene *scene, const LumeDirectionalLightConfig *config);
LUME_API LumeNode *lume_point_light_create(LumeScene *scene, const LumePointLightConfig *config);
LUME_API LumeNode *lume_mesh_create(LumeScene *scene, LumeGeometry *geometry, LumeMaterial *material);
LUME_API bool lume_render(LumeApp *app, LumeScene *scene, LumeNode *camera);

#ifdef __cplusplus
}
#endif

#endif
