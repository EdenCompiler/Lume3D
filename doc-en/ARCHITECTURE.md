# Lume3D Architecture

## Purpose

Lume3D is an approachable scene renderer rather than a full game engine. Its public API favors short creation/configuration calls, while its C implementation keeps explicit ownership and a small OpenGL boundary.

## Module shape

The public surface lives in `include/lume/lume.h`. The implementation is split into application/input, math, scene graph, geometry, resources, and renderer modules. Shared private types and contracts live in `src/lume_interno.h`.

GLFW owns native window integration. GLAD supplies generated OpenGL 3.3 declarations. stb_image decodes image files. Vector, matrix, transform, scene, and geometry code is native Lume3D code.

## Frame data flow

```text
poll input → update nodes → update world matrices → collect lights
           → bind camera → bind each mesh/material → draw → present
```

Geometry CPU data is copied at creation and uploaded to VAO/VBO/EBO objects on first use. Materials reference shared textures. Each render traverses the scene's flat owned-node registry; hierarchical transforms are updated recursively from roots.

## Coordinate and matrix conventions

Lume3D uses a right-handed coordinate system, +Y up, and local −Z as camera forward. Matrices are column-major for OpenGL. Transform composition is translation × Z rotation × Y rotation × X rotation × scale. Public Euler rotations use radians.

The view matrix is the inverse camera world transform. Perspective projection uses the OpenGL −1..1 clip-depth convention. A zero perspective aspect ratio selects the framebuffer ratio every frame.

## Ownership

An application owns renderer state and all registered resources. A scene belongs to exactly one application and owns every node created through it. A node may have one parent and multiple children within the same scene.

Destroying a node recursively destroys descendants. Destroying a scene destroys its nodes. Destroying an application first destroys scenes, then GPU resources, renderer state, and the window.

## Renderer boundary

All direct OpenGL calls are confined to application initialization/destruction, geometry upload, texture upload, and rendering modules. This keeps scene and math behavior independently testable and leaves room for a future renderer backend interface without changing scene ownership.

The 1.0 shader implements vertex transforms, texture/color composition, unlit basic materials, and diffuse Lambert lighting. Transparency, shadows, custom pipelines, and render queues are intentionally outside this boundary.

## Language convention

Public symbols, runtime diagnostics, shader identifiers, build targets, test output, and user-facing examples use English. Private C identifiers and source comments use Brazilian Portuguese. Documentation is mirrored in English and Brazilian Portuguese.
