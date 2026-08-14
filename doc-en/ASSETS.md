# Assets and model loading

Lume3D 1.5 loads Wavefront OBJ, glTF 2.0 JSON (`.gltf`), and binary glTF (`.glb`). The loader resolves external buffers and images relative to the model, accepts embedded GLB data, imports indexed primitives, and maps unlit and metallic/roughness materials to Lume3D resources.

## Synchronous loading

```c
LumeModel *model = NULL;
LumeModelInstance *instance = NULL;
LumeModelLoadOptions options = lume_model_load_options_default();

options.generate_missing_normals = true;
if (lume_model_load(app, "assets/scene.glb", &options, &model) != LUME_SUCCESS)
    fprintf(stderr, "Load failed: %s\n", lume_error_last()->message);

if (lume_model_instantiate(model, scene, &instance) == LUME_SUCCESS)
    lume_node_set_position(lume_model_instance_root(instance), (LumeVec3){0, 0, -4});

lume_model_release(model);
```

`LumeModel` is immutable and reference counted. An instance owns scene nodes, while geometry, textures, and materials remain shared. Destroy an instance with `lume_model_instance_destroy` before destroying its scene.

## Asynchronous loading

`lume_model_load_async` parses CPU-side data on a worker. Poll `lume_asset_job_state`; when it reaches `LUME_ASSET_JOB_READY_FOR_FINALIZE`, call `lume_asset_job_take_model` from the application/render thread. That call performs the OpenGL upload and moves the job to `LUME_ASSET_JOB_COMPLETE`.

Jobs are reference counted and can be cancelled. `lume_asset_job_progress` returns a value from 0 to 1. Failure details are job-local through `lume_asset_job_error` and are always written in English.

## Cache and hot reload

The default options use the application cache. Repeated loads of the same normalized path reuse an immutable model. `lume_assets_clear_cache` releases cache ownership without invalidating handles retained by application code.

Set `hot_reload = true` and register `lume_assets_set_reload_callback` to watch source modification times. Reload checks run at `LumeAppConfig.hot_reload_interval_seconds`; a successful reload replaces the cached model for later instances and reports the changed path through the callback.

## Current format boundary

OBJ supports positions, texture coordinates, normals, triangulated faces, negative indices, and generated missing normals. glTF supports node hierarchy, indexed triangle primitives, common vertex streams, PBR/unlit material data, embedded/external images, and transform animation clips. Joint and weight streams are imported as geometry attributes; skin deformation and morph-weight evaluation remain experimental in 1.5.
