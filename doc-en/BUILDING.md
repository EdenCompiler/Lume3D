# Building Lume3D

## Requirements

- CMake 3.20 or newer;
- C11 compiler;
- Git;
- Python 3;
- OpenGL development support and the native dependencies required by GLFW.

GLFW 3.5.1, GLAD 2.0.8, and stb_image are pinned through CMake `FetchContent`. Normal rebuilds reuse the populated build tree.

## Linux

Install the compiler, CMake, Python, Git, and GLFW's X11 or Wayland development prerequisites using the distribution package manager. Then run:

    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build --parallel
    ctest --test-dir build --output-on-failure

For a headless machine, run the OpenGL tests under Xvfb when no display server is already available:

    xvfb-run -a ctest --test-dir build --output-on-failure

## Windows

Use a Visual Studio developer shell:

    cmake -S . -B build -G "Visual Studio 17 2022" -A x64
    cmake --build build --config Release --parallel
    ctest --test-dir build -C Release --output-on-failure

MinGW is also supported:

    cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
    cmake --build build --parallel

## Build variants

Build without examples or tests:

    cmake -S . -B build -DLUME_BUILD_EXAMPLES=OFF -DLUME_BUILD_TESTS=OFF

Build shared libraries:

    cmake -S . -B build-shared -DBUILD_SHARED_LIBS=ON

Use strict warnings during development:

    cmake -S . -B build-strict -DLUME_WARNINGS_AS_ERRORS=ON

Use AddressSanitizer with GCC or Clang:

    cmake -S . -B build-asan \
      -DCMAKE_BUILD_TYPE=Debug \
      -DCMAKE_C_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer" \
      -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address,undefined"
    cmake --build build-asan --parallel
    ASAN_OPTIONS=detect_leaks=1 ctest --test-dir build-asan -R lume_unit_tests --output-on-failure
    ASAN_OPTIONS=detect_leaks=0 ctest --test-dir build-asan -R lume_render_test --output-on-failure

Leak detection is disabled only for the OpenGL smoke test because display drivers may retain process-lifetime allocations outside Lume3D. Address and undefined-behavior checks remain active for that test.

## Installation and consumers

Install to an isolated prefix:

    cmake --install build --prefix "$PWD/install"

A consumer `CMakeLists.txt` can then use:

```cmake
cmake_minimum_required(VERSION 3.20)
project(example LANGUAGES C)

find_package(Lume3D 1 CONFIG REQUIRED)
add_executable(example main.c)
target_link_libraries(example PRIVATE Lume3D::lume3d)
```

Configure it with:

    cmake -S consumer -B consumer-build -DCMAKE_PREFIX_PATH="$PWD/install"
    cmake --build consumer-build

The package installs its pinned GLFW and GLAD targets and headers with Lume3D so static consumers receive the complete native link interface.

## Tests

`lume_unit_tests` validates vectors, matrix composition/inversion, and hierarchical world transforms without a window. `lume_render_test` creates a hidden context, validates primitive topology, renders a lit box, and reads a non-empty pixel.
