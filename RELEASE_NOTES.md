# Lume3D 1.5.1

Lume3D 1.5.1 expands the practical, public-API-only example suite and corrects spatial bounds for hardware-instanced scenes.

Highlights:

- modular English C11 API with Brazilian Portuguese implementation and source comments;
- OBJ, glTF, and GLB loading, asynchronous jobs, immutable cache, and hot reload checks;
- unlit, Phong, PBR, and custom GLSL materials plus type-safe uniform setters;
- HDR rendering, ACES, bloom, FXAA, custom HDR/LDR passes, environment lighting, and transparency;
- three-cascade directional shadows and up to four spot shadow maps;
- transform animation, instancing, frustum culling, raycasts, debug primitives, and frame statistics;
- five practical examples: 3D Gerstner ocean, CPU-friendly Schwarzschild black hole, procedural solar system, instanced city, and interactive PBR lighting studio;
- mouse position, per-frame delta, and wheel queries plus direct low-resolution LDR target presentation;
- quality-first rendering with explicit `--low` profiles for the three new examples;
- scale-aware city windows, depth-tested instanced roads, a depth-aware procedural sky, consistent quality-mode framing, and fly-camera speed control;
- cached aggregate world bounds for instanced meshes, including parent transforms and invalidation after updates;
- Linux GCC/Clang, Windows MSVC/MinGW CI and installable SDK archives.

See [README.md](README.md), [CHANGELOG.md](CHANGELOG.md), [`doc-en`](doc-en/), and [`doc-ptbr`](doc-ptbr/) for the full public surface and documented boundaries.
