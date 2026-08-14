# Changelog

## 1.5.0 — 2026-08-13

- redesigned the public API around modular headers, `LumeResult`, structured English errors, and opaque reference-counted resources;
- added quaternion transforms, bounds, frusta, ray/AABB intersection, scene raycasts, instancing, and debug primitives;
- added OBJ, glTF, and GLB loading with immutable caching, asynchronous jobs, cancellation, and source change detection;
- added imported transform animation with loop modes, pause, seek, speed, and crossfades;
- added unlit, Phong, PBR, custom shader materials, typed uniform setters, render targets, and HDR/LDR custom passes;
- added environment lighting, transparency, HDR, ACES, bloom, FXAA, frustum culling, and frame statistics;
- added three-cascade directional shadows and up to four spot shadow maps;
- added practical 3D shader-ocean and spinning Kerr geodesic black-hole examples;
- fixed fullscreen HDR/post-processing texture coordinates so the full source image is sampled;
- expanded tests, GCC/Clang/MSVC/MinGW CI, installable SDK packaging, and mirrored English/Brazilian Portuguese documentation.

## 1.0.0 — 2026-08-13

- added the integrated GLFW window, input, timing, and OpenGL 3.3 runtime;
- added hierarchical scenes, meshes, cameras, procedural geometry, textures, basic materials, and lights;
- added static/shared CMake builds, installation metadata, examples, tests, and bilingual documentation.
