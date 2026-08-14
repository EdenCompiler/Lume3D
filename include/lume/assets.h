#ifndef LUME_ASSETS_H
#define LUME_ASSETS_H
#include <lume/render.h>

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct LumeModel LumeModel;
typedef struct LumeModelInstance LumeModelInstance;
typedef struct LumeAssetJob LumeAssetJob;
typedef struct LumeAnimationClip LumeAnimationClip;

typedef enum LumeAssetJobState
{
    LUME_ASSET_JOB_QUEUED,
    LUME_ASSET_JOB_LOADING,
    LUME_ASSET_JOB_READY_FOR_FINALIZE,
    LUME_ASSET_JOB_COMPLETE,
    LUME_ASSET_JOB_FAILED,
    LUME_ASSET_JOB_CANCELLED
} LumeAssetJobState;
typedef struct LumeModelLoadOptions
{
    bool generate_missing_normals, load_cameras, load_lights, use_cache, hot_reload;
} LumeModelLoadOptions;
typedef void (*LumeReloadCallback)(const char *path, LumeResult result, const LumeError *error, void *user_data);

LUME_API LumeModelLoadOptions lume_model_load_options_default(void);
LUME_API LumeResult lume_model_load(LumeApp *app, const char *path, const LumeModelLoadOptions *options,
                                    LumeModel **out_model);
LUME_API LumeResult lume_model_load_async(LumeApp *app, const char *path, const LumeModelLoadOptions *options,
                                          LumeAssetJob **out_job);
LUME_API void lume_model_retain(LumeModel *model);
LUME_API void lume_model_release(LumeModel *model);
LUME_API const char *lume_model_path(const LumeModel *model);
LUME_API size_t lume_model_animation_count(const LumeModel *model);
LUME_API LumeAnimationClip *lume_model_animation(const LumeModel *model, size_t index);
LUME_API LumeResult lume_model_instantiate(LumeModel *model, LumeScene *scene, LumeModelInstance **out_instance);
LUME_API LumeNode *lume_model_instance_root(LumeModelInstance *instance);
LUME_API void lume_model_instance_destroy(LumeModelInstance *instance);
LUME_API void lume_asset_job_retain(LumeAssetJob *job);
LUME_API void lume_asset_job_release(LumeAssetJob *job);
LUME_API LumeAssetJobState lume_asset_job_state(const LumeAssetJob *job);
LUME_API float lume_asset_job_progress(const LumeAssetJob *job);
LUME_API void lume_asset_job_cancel(LumeAssetJob *job);
LUME_API const LumeError *lume_asset_job_error(const LumeAssetJob *job);
LUME_API LumeResult lume_asset_job_take_model(LumeAssetJob *job, LumeModel **out_model);
LUME_API void lume_assets_set_reload_callback(LumeApp *app, LumeReloadCallback callback, void *user_data);
LUME_API void lume_assets_clear_cache(LumeApp *app);

#ifdef __cplusplus
}
#endif
#endif
