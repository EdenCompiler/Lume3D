# Lume3D Example Guide

Build examples with the default configuration:

    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build --parallel

Press Escape to close any example.

Pass `--smoke` to hide the window, disable VSync, render two frames, and exit. This is the mode used by the test suite.

## Available examples

| Target | Source | Demonstrates |
| --- | --- | --- |
| `lume_example_cube` | `examples/cubo.c` | Minimal app/scene/camera flow, basic material, and frame-rate-independent rotation |
| `lume_example_solar_system` | `examples/sistema_solar.c` | Empty pivot nodes, nested parent/child transforms, shared geometry, and multiple materials |
| `lume_example_lighting` | `examples/iluminacao.c` | Procedural RGBA texture, Lambert material, ambient/directional/point lights, and look-at camera |

Run from the build directory on Linux:

    ./build/lume_example_cube
    ./build/lume_example_solar_system
    ./build/lume_example_lighting

    ./build/lume_example_cube --smoke

Multi-config generators place executables under a configuration subdirectory such as `build/Release`.

## Spinning cube walkthrough

The cube example follows the smallest useful lifecycle. It creates the application first because scenes and resources belong to it. It then creates a scene, default perspective camera, box geometry, basic material, and mesh node.

The camera moves to +Z and keeps its default local −Z direction. Each frame polls events, rotates the cube using elapsed seconds, renders, and swaps buffers. Every failure path prints `lume_get_last_error()` in English and releases the application.

## Scene hierarchy walkthrough

The solar-system example shares one sphere geometry between three meshes. Empty nodes act as orbit pivots. Rotating the Earth pivot moves both Earth and the Moon pivot, while the Moon pivot provides its independent orbit.

This example uses basic materials so object colors are visible without lights and the transform relationship remains the focus.

## Lighting walkthrough

The lighting example creates a 2×2 checker texture directly from RGBA bytes. A Lambert material combines it with ambient, directional, and warm point lighting. The point light's node position controls its world-space source.
