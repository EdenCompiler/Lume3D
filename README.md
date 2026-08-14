# Lume3D — Readable native 3D for C

> **Languages / Idiomas:** [English](#english) · [Português do Brasil](#português-do-brasil)

---

# English

Lume3D is a native C11 3D library that brings an approachable scene, camera, geometry, material, light, asset, animation, and render flow to desktop applications without a browser or JavaScript runtime.

**Current stable release: 1.5.0**

![Lume3D version](https://img.shields.io/badge/Lume3D-1.5.0-blue)
![C](https://img.shields.io/badge/C-11-informational)
![OpenGL](https://img.shields.io/badge/OpenGL-3.3-informational)
![License](https://img.shields.io/badge/license-MIT-green)

## What 1.5.0 means

Lume3D 1.5 is a major API and renderer expansion. It adds modular public headers, structured results and English diagnostics, reference-counted resources, quaternions and spatial queries, glTF/GLB/OBJ assets, transform animation, asynchronous loading, cache/hot reload, PBR/Phong/unlit/custom materials, HDR and post-processing, cascaded directional and spot shadows, instancing, culling, raycasts, frame statistics, and debug primitives.

The public API, runtime errors, build messages, tests, and example output use English. Private implementation identifiers and source comments use Brazilian Portuguese so the engine remains readable to Portuguese-speaking C developers without limiting its international API.

| Subsystem | 1.5.0 status |
| --- | --- |
| Core | C11 runtime, stateful input, structured `LumeResult`/`LumeError`, thread-local diagnostics, leak reporting |
| Scene | Hierarchy, quaternion transforms, perspective/orthographic cameras, four light types, bounds and raycasts |
| Assets | OBJ, glTF, GLB, external/embedded data, immutable cache, asynchronous jobs, change detection |
| Animation | Imported transform clips, once/repeat/ping-pong, pause, seek, speed and crossfade |
| Rendering | Forward HDR, unlit/Phong/PBR/custom materials, transparency, IBL, ACES, bloom, FXAA, custom passes |
| Scale | Instanced meshes, frustum culling, three directional cascades, four spot shadow maps, frame statistics |
| Platforms | Linux GCC/Clang and Windows MSVC/MinGW CMake builds |

## 1.5.0 release highlights

- modular `core`, `math`, `scene`, `render`, `assets`, `animation`, and `debug` headers plus `lume.h`;
- type-safe output parameters and actionable English errors with operation/path/line/column context;
- glTF 2.0, GLB, and OBJ model loading with asynchronous CPU parsing and render-thread upload;
- reference-counted geometry, textures, materials, shaders, pipelines, targets, environments, and models;
- metallic/roughness PBR, Phong, unlit, custom GLSL, HDR targets, ACES, bloom, FXAA, and pass chaining;
- three-cascade sun shadows, up to four shadowed spot lights, hardware instancing, and culling;
- practical shader ocean and numerically integrated Schwarzschild black-hole examples;
- device-free tests, hidden-window rendering tests, example smoke tests, and multi-toolchain CI.

## Installation

Requirements: CMake 3.20+, a C11 compiler, Git, Python 3 with Jinja2, OpenGL development support, and GLFW’s native platform prerequisites. Dependencies are pinned and fetched by CMake.

    python -m pip install -r requirements-build.txt
    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build --parallel
    ctest --test-dir build --output-on-failure

On a headless Linux host:

    xvfb-run -a ctest --test-dir build --output-on-failure

Install and consume the exported target:

    cmake --install build --prefix ./install

```cmake
find_package(Lume3D 1.5 CONFIG REQUIRED)
target_link_libraries(my_application PRIVATE Lume3D::lume3d)
```

| CMake option | Default | Purpose |
| --- | --- | --- |
| `LUME_BUILD_EXAMPLES` | `ON` | Build the two practical shader examples |
| `LUME_BUILD_TESTS` | `ON` | Build unit, renderer, and example smoke tests |
| `LUME_WARNINGS_AS_ERRORS` | `OFF` | Promote Lume3D compiler warnings to errors |
| `BUILD_SHARED_LIBS` | `OFF` | Build shared rather than static libraries |

See [Building Lume3D](doc-en/BUILDING.md) for Linux, Windows, sanitizers, and package consumption.

## Minimal scene

```c
#include <lume/lume.h>
#include <stdio.h>

int main(void)
{
    LumeApp *app = NULL;
    LumeScene *scene = NULL;
    LumeNode *camera = NULL;

    if (lume_app_create(NULL, &app) != LUME_SUCCESS ||
        lume_scene_create(app, &scene) != LUME_SUCCESS ||
        lume_camera_create_perspective(scene, NULL, &camera) != LUME_SUCCESS) {
        fprintf(stderr, "Startup failed: %s\n", lume_error_last()->message);
        lume_app_destroy(app);
        return 1;
    }

    lume_node_set_position(camera, (LumeVec3){0, 0, 3});
    while (!lume_app_should_close(app)) {
        lume_app_begin_frame(app);
        lume_app_render(app, scene, camera);
        lume_app_end_frame(app);
    }
    lume_app_destroy(app);
    return 0;
}
```

## Practical examples

    ./build/lume_example_ocean
    ./build/lume_example_black_hole

The ocean uses true 3D Gerstner displacement, finite-difference normals, Fresnel water, crest foam, sun glitter, and a procedural sky in the low-horizon camera style of the supplied video reference.

The black-hole example adapts the supplied Schwarzschild C ray-tracer into a normalized GLSL geodesic integrator. It uses midpoint stepping, horizon and equatorial-disk intersection tests, a rotating Doppler-beamed disk, stars, and a gravity-grid world. Arrow keys orbit, `W`/`S` zoom, and `R` resets the virtual camera; its limits relative to a full Kerr solver are documented.

See the [example guide](doc-en/EXAMPLES.md) for the implementation and physics boundaries.

## Documentation

- [API guide](doc-en/API.md)
- [Building](doc-en/BUILDING.md)
- [Architecture](doc-en/ARCHITECTURE.md)
- [Rendering](doc-en/RENDERING.md)
- [Assets](doc-en/ASSETS.md)
- [Animation](doc-en/ANIMATION.md)
- [Practical examples](doc-en/EXAMPLES.md)
- [Changelog](CHANGELOG.md)

Brazilian Portuguese mirrors are under [`doc-ptbr`](doc-ptbr/).

## Current boundaries

- OpenGL 3.3 Core is the only rendering backend.
- GPU resource creation and rendering occur on the application thread.
- Joint/weight streams import, but skin deformation and morph-weight evaluation remain experimental.
- MSAA sample fields and GPU time statistics are reserved; 1.5 does not resolve multisampled custom targets or issue timer queries.
- Browser, mobile, audio, physics, and editor tooling are outside this library.

## License

Lume3D is available under the [MIT License](LICENSE).

---

# Português do Brasil

Lume3D é uma biblioteca 3D nativa C11 que leva às aplicações desktop um fluxo acessível de cena, câmera, geometria, material, luz, assets, animação e renderização sem navegador ou runtime JavaScript.

**Versão estável atual: 1.5.0**

## O que 1.5.0 significa

Lume3D 1.5 é uma grande expansão da API e do renderizador. Ela adiciona headers públicos modulares, resultados estruturados e diagnósticos em inglês, recursos com contagem de referências, quaternions e consultas espaciais, assets glTF/GLB/OBJ, animação de transformações, carregamento assíncrono, cache/hot reload, materiais PBR/Phong/unlit/custom, HDR e pós-processamento, sombras direcionais em cascata e spot, instancing, culling, raycasts, estatísticas e primitivas de debug.

A API pública, erros de runtime, mensagens de build, testes e saídas dos exemplos usam inglês. Identificadores privados e comentários do código-fonte usam português brasileiro, mantendo a engine legível para desenvolvedores lusófonos sem limitar sua API internacional.

| Subsistema | Estado na 1.5.0 |
| --- | --- |
| Core | Runtime C11, input stateful, `LumeResult`/`LumeError`, diagnósticos por thread e relatório de leaks |
| Cena | Hierarquia, quaternions, duas câmeras, quatro tipos de luz, bounds e raycasts |
| Assets | OBJ, glTF, GLB, dados externos/embutidos, cache imutável, jobs assíncronos e detecção de mudanças |
| Animação | Clips de transformação, once/repeat/ping-pong, pausa, seek, velocidade e crossfade |
| Renderização | Forward HDR, unlit/Phong/PBR/custom, transparência, IBL, ACES, bloom, FXAA e passes custom |
| Escala | Instancing, frustum culling, três cascatas direcionais, quatro sombras spot e estatísticas |
| Plataformas | Builds CMake Linux GCC/Clang e Windows MSVC/MinGW |

## Instalação

Requisitos: CMake 3.20+, compilador C11, Git, Python 3 com Jinja2, suporte OpenGL de desenvolvimento e pré-requisitos nativos do GLFW. As dependências são fixadas e obtidas pelo CMake.

    python -m pip install -r requirements-build.txt
    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build --parallel
    ctest --test-dir build --output-on-failure

Instale e consuma o target exportado:

    cmake --install build --prefix ./install

```cmake
find_package(Lume3D 1.5 CONFIG REQUIRED)
target_link_libraries(meu_aplicativo PRIVATE Lume3D::lume3d)
```

Consulte [Compilando o Lume3D](doc-ptbr/COMPILACAO.md) para Linux, Windows, sanitizers e consumo do pacote.

## Exemplos práticos

    ./build/lume_example_ocean
    ./build/lume_example_black_hole

O oceano usa deslocamento Gerstner 3D, normais por diferenças finitas, água Fresnel, espuma nas cristas, brilho solar e céu procedural no estilo de câmera de horizonte baixo da referência em vídeo.

O exemplo de buraco negro adapta o ray tracer C de Schwarzschild fornecido para um integrador GLSL de geodésicas normalizado. Ele usa passos de ponto médio, testes de interseção do horizonte e do disco equatorial, disco rotativo com beaming Doppler, estrelas e um mundo de grade gravitacional. As setas orbitam, `W`/`S` aplicam zoom e `R` restaura a câmera virtual; seus limites frente a um solver Kerr completo estão documentados.

Consulte o [guia dos exemplos](doc-ptbr/EXEMPLOS.md) para os detalhes e limites físicos.

## Documentação

- [Guia da API](doc-ptbr/API.md)
- [Compilação](doc-ptbr/COMPILACAO.md)
- [Arquitetura](doc-ptbr/ARQUITETURA.md)
- [Renderização](doc-ptbr/RENDERIZACAO.md)
- [Assets](doc-ptbr/ASSETS.md)
- [Animação](doc-ptbr/ANIMACAO.md)
- [Exemplos práticos](doc-ptbr/EXEMPLOS.md)
- [Changelog](CHANGELOG.md)

## Limites atuais

- OpenGL 3.3 Core é o único backend.
- Criação de recursos GPU e renderização ocorrem na thread do aplicativo.
- Streams de joints/weights são importados; deformação de skin e morph weights permanecem experimentais.
- Campos MSAA e tempo de GPU estão reservados; 1.5 não resolve targets multisample nem emite timer queries.
- Navegador, mobile, áudio, física e editor estão fora do escopo.

## Licença

Lume3D está disponível sob a [Licença MIT](LICENSE).
