#ifndef LUME_RENDER_H
#define LUME_RENDER_H
#include <lume/scene.h>

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct LumeTexture LumeTexture;
typedef struct LumeShader LumeShader;
typedef struct LumePipeline LumePipeline;
typedef struct LumeRenderTarget LumeRenderTarget;
typedef struct LumeEnvironment LumeEnvironment;

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
typedef enum LumeAlphaMode
{
    LUME_ALPHA_OPAQUE,
    LUME_ALPHA_MASK,
    LUME_ALPHA_BLEND
} LumeAlphaMode;
typedef enum LumeToneMapping
{
    LUME_TONE_MAPPING_NONE,
    LUME_TONE_MAPPING_ACES
} LumeToneMapping;
typedef enum LumeMaterialType
{
    LUME_MATERIAL_UNLIT,
    LUME_MATERIAL_PHONG,
    LUME_MATERIAL_PBR,
    LUME_MATERIAL_CUSTOM
} LumeMaterialType;
typedef enum LumePassPhase
{
    LUME_PASS_HDR,
    LUME_PASS_LDR
} LumePassPhase;

typedef struct LumeGeometryData
{
    const float *positions, *normals, *texture_coordinates, *tangents, *colors, *joint_weights;
    const uint16_t *joints;
    size_t vertex_count;
    const uint32_t *indices;
    size_t index_count;
} LumeGeometryData;
typedef struct LumeTextureConfig
{
    LumeTextureFilter min_filter, mag_filter;
    LumeTextureWrap wrap_u, wrap_v;
    bool generate_mipmaps, flip_y, srgb;
} LumeTextureConfig;
typedef struct LumeMaterialConfig
{
    LumeMaterialType type;
    LumeColor base_color;
    LumeTexture *base_color_texture, *normal_texture, *metallic_roughness_texture;
    LumeTexture *occlusion_texture, *emissive_texture;
    LumeColor emissive_color;
    float metallic, roughness, shininess, alpha_cutoff;
    LumeAlphaMode alpha_mode;
    bool double_sided, wireframe;
    LumePipeline *custom_pipeline;
} LumeMaterialConfig;
typedef struct LumeRendererConfig
{
    bool hdr, fxaa, bloom, frustum_culling;
    float exposure, bloom_threshold, bloom_strength;
    LumeToneMapping tone_mapping;
    uint32_t msaa_samples, directional_shadow_size, spot_shadow_size;
} LumeRendererConfig;
typedef struct LumeShaderConfig
{
    const char *vertex_source, *fragment_source, *vertex_path, *fragment_path;
} LumeShaderConfig;
typedef struct LumePipelineConfig
{
    LumeShader *shader;
    bool depth_test, depth_write, blending, cull_back_faces;
} LumePipelineConfig;
typedef struct LumeRenderTargetConfig
{
    int width, height;
    bool hdr, depth;
    uint32_t samples;
} LumeRenderTargetConfig;
typedef struct LumePassConfig
{
    const char *name;
    LumePipeline *pipeline;
    LumePassPhase phase;
    bool needs_depth;
    bool enabled;
} LumePassConfig;
typedef struct LumeFrameStats
{
    uint64_t frame_index, submitted_objects, culled_objects, draw_calls, instances, triangles;
    uint64_t shadow_draws, texture_switches, shader_switches;
    double cpu_time_ms, gpu_time_ms;
} LumeFrameStats;

LUME_API LumeTextureConfig lume_texture_config_default(void);
LUME_API LumeMaterialConfig lume_material_config_default(LumeMaterialType type);
LUME_API LumeRendererConfig lume_renderer_config_default(void);
LUME_API LumePipelineConfig lume_pipeline_config_default(void);
LUME_API LumeRenderTargetConfig lume_render_target_config_default(void);
LUME_API LumeResult lume_geometry_create(LumeApp *app, const LumeGeometryData *data, LumeGeometry **out_geometry);
LUME_API LumeResult lume_geometry_create_box(LumeApp *app, float width, float height, float depth,
                                             LumeGeometry **out_geometry);
LUME_API LumeResult lume_geometry_create_plane(LumeApp *app, float width, float height, LumeGeometry **out_geometry);
LUME_API LumeResult lume_geometry_create_sphere(LumeApp *app, float radius, uint32_t width_segments,
                                                uint32_t height_segments, LumeGeometry **out_geometry);
LUME_API void lume_geometry_retain(LumeGeometry *geometry);
LUME_API void lume_geometry_release(LumeGeometry *geometry);
LUME_API LumeAabb lume_geometry_bounds(const LumeGeometry *geometry);
LUME_API LumeResult lume_texture_create_rgba8(LumeApp *app, const uint8_t *pixels, int width, int height,
                                              const LumeTextureConfig *config, LumeTexture **out_texture);
LUME_API LumeResult lume_texture_load(LumeApp *app, const char *path, const LumeTextureConfig *config,
                                      LumeTexture **out_texture);
LUME_API void lume_texture_retain(LumeTexture *texture);
LUME_API void lume_texture_release(LumeTexture *texture);
LUME_API LumeResult lume_material_create(LumeApp *app, const LumeMaterialConfig *config, LumeMaterial **out_material);
LUME_API void lume_material_retain(LumeMaterial *material);
LUME_API void lume_material_release(LumeMaterial *material);
LUME_API void lume_material_set_base_color(LumeMaterial *material, LumeColor color);
LUME_API LumeResult lume_material_set_texture(LumeMaterial *material, LumeTexture *texture);
LUME_API LumeResult lume_shader_create(LumeApp *app, const LumeShaderConfig *config, LumeShader **out_shader);
LUME_API void lume_shader_retain(LumeShader *shader);
LUME_API void lume_shader_release(LumeShader *shader);
LUME_API LumeResult lume_pipeline_create(LumeApp *app, const LumePipelineConfig *config, LumePipeline **out_pipeline);
LUME_API void lume_pipeline_retain(LumePipeline *pipeline);
LUME_API void lume_pipeline_release(LumePipeline *pipeline);
LUME_API LumeResult lume_render_target_create(LumeApp *app, const LumeRenderTargetConfig *config,
                                              LumeRenderTarget **out_target);
LUME_API void lume_render_target_retain(LumeRenderTarget *target);
LUME_API void lume_render_target_release(LumeRenderTarget *target);
LUME_API LumeResult lume_environment_load_hdr(LumeApp *app, const char *path, LumeEnvironment **out_environment);
LUME_API void lume_environment_retain(LumeEnvironment *environment);
LUME_API void lume_environment_release(LumeEnvironment *environment);
LUME_API LumeResult lume_renderer_configure(LumeRenderer *renderer, const LumeRendererConfig *config);
LUME_API void lume_renderer_set_environment(LumeRenderer *renderer, LumeEnvironment *environment);
LUME_API LumeResult lume_renderer_add_pass(LumeRenderer *renderer, const LumePassConfig *config,
                                           uint32_t *out_pass_index);
LUME_API void lume_renderer_clear_passes(LumeRenderer *renderer);
LUME_API LumeResult lume_renderer_render(LumeRenderer *renderer, LumeScene *scene, LumeNode *camera,
                                         LumeRenderTarget *target);
LUME_API LumeResult lume_app_render(LumeApp *app, LumeScene *scene, LumeNode *camera);
LUME_API LumeFrameStats lume_renderer_frame_stats(const LumeRenderer *renderer);

#ifdef __cplusplus
}
#endif
#endif
