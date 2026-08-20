# Lume3D 1.5.0

Lume3D 1.5.0 turns the original readable C renderer into a modular native 3D library with structured diagnostics, reference-counted resources, asset loading, animation, an extensible HDR renderer, shadows, instancing, culling, spatial queries, and bilingual documentation.

Highlights:

- modular English C11 API with Brazilian Portuguese implementation and source comments;
- OBJ, glTF, and GLB loading, asynchronous jobs, immutable cache, and hot reload checks;
- unlit, Phong, PBR, and custom GLSL materials plus type-safe uniform setters;
- HDR rendering, ACES, bloom, FXAA, custom HDR/LDR passes, environment lighting, and transparency;
- three-cascade directional shadows and up to four spot shadow maps;
- transform animation, instancing, frustum culling, raycasts, debug primitives, and frame statistics;
- practical 3D Gerstner ocean and CPU-friendly Schwarzschild ray-traced black-hole examples;
- mouse position, per-frame delta, and wheel queries plus direct low-resolution LDR target presentation;
- Linux GCC/Clang, Windows MSVC/MinGW CI and installable SDK archives.

See [README.md](README.md), [CHANGELOG.md](CHANGELOG.md), [`doc-en`](doc-en/), and [`doc-ptbr`](doc-ptbr/) for the full public surface and documented boundaries.
