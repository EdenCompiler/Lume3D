# Lume3D 1.5 API guide

The umbrella header exposes every module:

```c
#include <lume/lume.h>
```

Applications may instead include `core.h`, `math.h`, `scene.h`, `render.h`, `assets.h`, `animation.h`, or `debug.h`. Every public name is English and begins with `lume_`, `Lume`, or `LUME_`. Implementation names and source comments are Brazilian Portuguese; runtime, compiler-facing, test, and example output is English.

## Results and diagnostics

Fallible calls return `LumeResult` and write their output through the final `out_...` parameter:

```c
LumeApp *app = NULL;
LumeResult result = lume_app_create(NULL, &app);
if (result != LUME_SUCCESS) {
    const LumeError *error = lume_error_last();
    fprintf(stderr, "%s: %s\n", lume_result_string(result), error->message);
    return 1;
}
```

`LumeError` contains a result code, operation, optional path, line/column, and an English message. The last error is thread-local. A per-application log callback receives English info, warning, and error messages.

## Application and frames

`lume_app_create` creates the window, OpenGL context, input state, asset services, and renderer. Start from `lume_app_config_default`; the default is a visible, resizable 1280×720 window with VSync.

Each frame has one clear order:

1. `lume_app_begin_frame` polls events and returns elapsed seconds.
2. Update scene, animation, and application state.
3. `lume_app_render` renders the scene with a camera.
4. `lume_app_end_frame` presents the back buffer.

Use `lume_renderer_render` when rendering to a `LumeRenderTarget`. Input queries expose down, pressed, and released state for keyboard and mouse buttons. `lume_mouse_get_position`, `lume_mouse_get_delta`, and `lume_mouse_get_scroll` return cursor coordinates, per-frame motion, and wheel motion; omitted output pointers may be `NULL`.

`lume_renderer_present_target` scales an LDR render target to the application framebuffer with linear filtering. It is useful for expensive software-compatible effects that should render at a smaller internal resolution. HDR targets must be tone-mapped through a pass before presentation.

## Scene and transforms

A scene owns its nodes. Empty nodes, cameras, lights, meshes, and instanced meshes all use the opaque `LumeNode` handle. Nodes have a name, position, quaternion rotation, scale, parent, and children. World matrices and bounds are updated lazily.

```c
LumeNode *pivot = NULL;
LumeNode *mesh = NULL;
lume_node_create(scene, &pivot);
lume_mesh_create(scene, geometry, material, &mesh);
lume_node_add_child(pivot, mesh);
lume_node_set_position(mesh, (LumeVec3){3, 0, 0});
lume_node_rotate_y(pivot, delta_seconds);
```

The coordinate system is right-handed, +Y is up, cameras look along local −Z, angles are radians, and matrices are column-major.

## Cameras, lights, and queries

Perspective and orthographic cameras use configuration structs with default constructors. A zero perspective aspect uses the framebuffer ratio. Lights include ambient, directional, point, and spot. Directional and spot configurations can enable shadow casting.

`lume_scene_raycast` tests visible mesh world AABBs and returns nearest-first `LumeRaycastHit` values. The math module includes vectors, quaternions, matrices, inverse/transform operations, rays, AABBs, frusta, and intersection tests.

## Resources

Geometry, textures, materials, shaders, pipelines, render targets, environments, models, and asset jobs are reference counted. Creation returns one reference. `retain` adds ownership and `release` removes it. Scene mesh nodes retain their geometry and material. The application reports leaked resource handles in English during destruction.

Built-in geometry includes box, plane, and UV sphere. Custom geometry supports positions, normals, UVs, tangents, colors, joint indices/weights, and optional 32-bit indices. Materials include unlit, Phong, PBR, and custom-pipeline variants.

See [Rendering](RENDERING.md), [Assets](ASSETS.md), and [Animation](ANIMATION.md) for subsystem workflows.

## Lifetime order

Destroy animation players before model instances, instances before their scene, scenes before the application, and release application-owned resource handles before `lume_app_destroy`. Passing `NULL` to destroy/release calls is safe unless a function documents otherwise.
