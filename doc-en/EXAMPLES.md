# Lume3D practical examples

Lume3D 1.5 ships two focused examples. Both are complete animated scenes built only with the public API and custom GLSL shaders; neither depends on downloaded art assets.

Build and run them:

    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build --parallel

    ./build/lume_example_ocean
    ./build/lume_example_black_hole

Press Escape to close. Pass `--smoke` to create a hidden window, render two frames, and exit; CTest runs this mode for both examples.

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

At each step it tests the event horizon (`r ≤ rₛ`) and detects a sign change through the equatorial plane. A crossing inside the disk radii emits a rotating, radially heated disk color, with special-relativistic Doppler beaming. Escaping rays sample the star field; the gravity-grid world is rendered behind the ray-traced result. Arrow keys orbit the virtual camera, `W`/`S` zoom, `R` resets it, and `Esc` exits.

This reduced solver uses a fixed number of midpoint steps. It is not an adaptive renderer, a full Kerr (spinning-metric) solver, a magnetohydrodynamic disk simulation, or volumetric radiative transfer. The disk rotates visually, but frame dragging requires a Kerr extension rather than this Schwarzschild metric.

This example is useful as a template for:

- fullscreen custom shaders;
- numerically integrated simulations in GLSL;
- physically meaningful constants and stopping conditions;
- resize-aware shader uniforms.
