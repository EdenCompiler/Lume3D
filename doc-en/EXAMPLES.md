# Lume3D practical examples

Lume3D 1.5.1 ships five focused examples. Every scene is built only with the public API and works offline without downloaded art assets.

Build and run them:

    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build --parallel

    ./build/lume_example_ocean
    ./build/lume_example_black_hole
    ./build/lume_example_solar_system
    ./build/lume_example_instanced_city
    ./build/lume_example_lighting_studio

Press Escape to close. Every example accepts `--smoke` to create a hidden window, render two frames, and exit. The three 1.5.1 examples also accept `--low`, which selects an 800×450 window, lighter geometry, 512-pixel shadow maps, and no bloom. CTest runs smoke mode for all five programs.

## Shader ocean

Source: `examples/oceano.c`

The ocean is a dense continuous surface displaced by four directional Gerstner wave trains. The vertex shader changes all three position components, calculates a smooth finite-difference normal, and carries crest energy to the fragment shader. The fragment shader combines Fresnel reflection, sun glitter, depth color, multi-scale noise, and narrow breaking-foam bands. A separate fullscreen procedural sky supplies the low cloudy horizon.

The camera follows the reference’s near-surface, wide-world framing. Use W/A/S/D to travel across the surface while retaining that camera style.

This example is useful as a template for:

- animated vertex displacement;
- finite-difference shader normals;
- multiple custom materials and shared frame uniforms;
- large procedural surfaces and low-horizon composition.

## Schwarzschild black hole and rotating disk

Source: `examples/buraco_negro.c`

The black-hole fragment shader is a compact numerical-relativity demonstration, not a screen-space swirl. It adapts the supplied C ray-tracer structure to a real-time GLSL implementation in normalized Schwarzschild-radius units. A ray stores spherical position and velocity, then the shader advances it with a midpoint integration of the Schwarzschild geodesic differential equations:

```text
f(r) = 1 − rₛ / r
d²r/dλ², d²θ/dλ², d²φ/dλ² = Schwarzschild geodesic RHS
```

At each step it tests the event horizon (`r ≤ rₛ`), the two orbiting bodies, and a sign change through the equatorial plane. A crossing inside the disk radii emits the rotating, radially heated colors used by the supplied program. Escaping rays reveal a perspective gravity grid shaped around the central mass.

The controls mirror the supplied program: left-drag orbits, middle-drag pans, the mouse wheel zooms, `R` resets the camera, `P` pauses or resumes disk motion, `G` toggles the grid, and `Esc` exits. Arrow keys and `W`/`S` remain convenient keyboard alternatives for orbit and zoom.

This reduced solver uses a fixed number of midpoint steps. It is not an adaptive renderer, a full Kerr (spinning-metric) solver, a magnetohydrodynamic disk simulation, or volumetric radiative transfer. Disk rotation is visual; frame dragging requires a Kerr extension rather than this Schwarzschild metric.

For CPU-only systems, the geodesic pass follows the supplied program and renders at `72 × 43`, then `lume_renderer_present_target` scales it to the `500 × 300` window. Camera motion temporarily uses 420 coarse steps per ray; the resting image uses 900. This keeps memory use modest and cuts fragment work by roughly 49 times compared with full-resolution tracing.

This example is useful as a template for:

- fullscreen custom shaders;
- numerically integrated simulations in GLSL;
- physically meaningful constants and stopping conditions;
- resize-aware shader uniforms.

## Procedural solar system

Source: `examples/sistema_solar.c`

The solar system builds planets, moons, and orbital pivots from ordinary scene nodes. A generated equirectangular Earth texture demonstrates runtime texture creation, while a Fibonacci-distributed instanced star field supplies the background. PBR planets, an HDR unlit sun, a point light, ACES, bloom, and debug orbit paths complete the scene without external assets.

Controls: left-drag orbits, middle-drag pans, the wheel zooms, Space pauses, Up/Down changes simulation speed, `O` toggles orbit paths, `R` resets, and Esc exits.

This example is useful as a template for scene hierarchy, parent/child transforms, procedural textures, shared geometry, mixed material types, debug lines, and an orbit camera.

## Instanced procedural city

Source: `examples/cidade_instanciada.c`

The city deterministically generates a street grid and thousands of building transforms. One custom-material instanced mesh renders the skyline in one building draw, with scale-aware procedural windows and distance fog; depth-tested instanced lane markers and the PBR ground receive cascaded directional shadows. A depth-aware LDR pass fills only untouched pixels with a procedural twilight sky, preserving distant geometry. The corrected aggregate instance bounds keep the whole generated layout available to culling and spatial queries.

Controls: WASD moves, Q/E changes altitude, left-drag looks around, the wheel adjusts base speed, Shift boosts speed, `I` toggles periodic frame statistics, `R` resets, and Esc exits.

This example is useful as a template for hardware instancing, deterministic generation, scale-aware custom shaders, depth-aware post-processing, fly cameras, shadows, culling-safe bounds, and `LumeFrameStats`.

## Interactive PBR lighting studio

Source: `examples/estudio_iluminacao.c`

The studio arranges a 7×6 material chart whose columns increase metallic response and whose rows increase roughness. Ambient, directional, moving point, and shadowed spot lights illuminate the chart, floor, and backdrop through the built-in HDR renderer. Right-click selection constructs a camera ray, calls `lume_scene_raycast`, highlights the chosen sphere, and optionally draws its AABB and axes.

Controls: left-drag orbits, middle-drag pans, the wheel zooms, right-click selects, `B` toggles bloom, `F` toggles FXAA, `L` pauses the moving lights, `D` toggles debug drawing, `R` resets, and Esc exits.

This example is useful as a template for PBR parameter studies, all four light types, cascaded and spot shadows, runtime renderer reconfiguration, mouse picking, and debug primitives.
