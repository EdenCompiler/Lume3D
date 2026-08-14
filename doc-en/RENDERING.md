# Rendering

Lume3D 1.5 uses an OpenGL 3.3 Core forward renderer. The default path supports unlit, Phong, metallic/roughness PBR, image-based environment light, opaque/masked/blended materials, hardware instancing, frustum culling, and render statistics.

## Renderer configuration

```c
LumeRendererConfig config = lume_renderer_config_default();
config.hdr = true;
config.tone_mapping = LUME_TONE_MAPPING_ACES;
config.bloom = true;
config.fxaa = true;
config.exposure = 1.1f;
lume_renderer_configure(lume_app_renderer(app), &config);
```

HDR rendering uses a floating-point intermediate target. Bloom operates before ACES; FXAA operates on the LDR image. A directional light may cast three cascade shadow maps, and up to four spot lights may cast one shadow map each. Shadow sizes are configured through `directional_shadow_size` and `spot_shadow_size`.

## Custom shaders and passes

Create `LumeShader` from GLSL source strings or file paths, place it in a `LumePipeline`, and use that pipeline from a custom material. Lume3D supplies `uModel`, `uView`, `uProjection`, and `uCamera` to mesh shaders. Application uniforms use the type-safe `lume_shader_set_float`, `set_vec2`, `set_vec3`, `set_vec4`, and `set_mat4` calls.

`lume_renderer_add_pass` inserts a fullscreen HDR or LDR pass. The pass shader receives the previous color image as `uColorTexture` and, when `needs_depth` is true, the scene depth image as `uDepthTexture`. Passes execute in insertion order within their phase.

## Ownership and statistics

Geometry, texture, material, shader, pipeline, render-target, and environment handles are reference counted. A material retains its textures and custom pipeline; a pipeline retains its shader. Release the application handle after assigning it to another resource.

`lume_renderer_frame_stats` reports submitted/culled objects, draw calls, instances, triangles, shadow draws, state switches, and CPU frame time. `gpu_time_ms` is reserved and reports zero when GPU timer queries are unavailable.

## Debug drawing

`lume_debug_line`, `lume_debug_axes`, `lume_debug_aabb`, `lume_debug_sphere`, and `lume_debug_ray` enqueue one-frame primitives. Call `lume_debug_clear` to discard queued primitives explicitly.
