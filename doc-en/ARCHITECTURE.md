# Lume3D architecture

## Purpose and module shape

Lume3D is a readable native scene renderer for C11. It offers a Three.js-like object vocabulary while keeping allocation, ownership, threading, and failure explicit.

The public surface is modular: `core`, `math`, `scene`, `render`, `assets`, `animation`, and `debug`, with `lume.h` as the umbrella. Private contracts live in `src/lume_interno.h`; implementation identifiers and comments are Brazilian Portuguese. GLFW provides native windows, GLAD provides OpenGL 3.3 declarations, stb_image decodes images, cgltf parses glTF, fast_obj parses OBJ, and tinycthread provides portable worker primitives. Dependency revisions are pinned by CMake.

## Runtime data flow

```text
input/events ──→ application update ──→ animation/node transforms
                                              │
model job ──→ CPU parse ──→ render-thread upload/cache
                                              │
scene ──→ culling ──→ shadow maps ──→ forward HDR ──→ custom HDR passes
                                                   ──→ bloom/ACES/FXAA
                                                   ──→ custom LDR passes ──→ present
```

The OpenGL context and all GPU creation/finalization belong to the application thread. Asynchronous jobs only parse and stage CPU-side data. The renderer traverses a flat scene registry while resolving parent transforms recursively.

## Coordinates and visibility

Lume3D is right-handed, +Y up, camera forward −Z, radians, and column-major matrices. Transform composition is translation × rotation × scale. A zero camera aspect selects the current framebuffer ratio.

Geometry stores local AABBs. Scene nodes derive world AABBs from their matrices. The renderer extracts a camera frustum, removes non-intersecting meshes when culling is enabled, then submits opaque/masked objects before blended objects. Raycasts reuse the world bounds.

## Ownership

`LumeApp` owns the window, renderer, asset cache, jobs, and the registry used for leak diagnostics. `LumeScene` owns its nodes. GPU-facing resources and immutable models use intrusive reference counts, allowing geometry/material/texture sharing between nodes and model instances.

The useful ownership chain is:

```text
application → scene → nodes
application registry → reference-counted resources
mesh node → geometry + material → textures/custom pipeline → shader
model instance → scene nodes; model → immutable imported data/clips
```

## Rendering limits

Version 1.5 has one OpenGL 3.3 Core backend and a forward renderer. Directional shadows use three practical cascades and spot shadows support four casting lights. Custom fullscreen passes use a color ping-pong chain. MSAA sample configuration and GPU time are public forward-compatible fields; this release does not resolve multisampled render targets or issue GPU timer queries. Skin and morph streams are imported but deformation remains experimental.
