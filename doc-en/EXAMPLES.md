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

## Spinning Kerr black hole

Source: `examples/buraco_negro.c`

The black-hole fragment shader is a compact numerical relativity demonstration, not a screen-space swirl. In geometric units (`G = c = 1`) it uses a reduced Kerr ray-plane integrator with spin `a = 0.82M`. The radial motion uses the separated Kerr potential:

```text
Δ = r² − 2Mr + a²
R(r) = [r² + a² − aξ]² − Δ[η + (ξ − a)²]
```

It derives the outer event horizon as `r₊ = M + √(M² − a²)`, uses the prograde Kerr ISCO `r ≈ 2.8019M`, and evaluates the physical ZAMO frame-dragging rate while reconstructing each ray in a continuous 3D orbital plane. Rays either cross the horizon or escape to the star field. Bent rays intersect a thin disk; emission uses the Kerr circular-orbit angular velocity, gravitational/Doppler redshift, radial temperature, and invariant-intensity `g³` factor.

This reduced solver preserves the principal Kerr spin effects and continuous primary/secondary disk images, but it is not a full adaptive integration of both Carter radial and polar equations. It does not model magnetohydrodynamic turbulence or volumetric radiative transfer.

This example is useful as a template for:

- fullscreen custom shaders;
- numerically integrated simulations in GLSL;
- physically meaningful constants and stopping conditions;
- resize-aware shader uniforms.
