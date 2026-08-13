# Lume3D — Readable 3D for C

> **Languages / Idiomas:** [English](#english) · [Português do Brasil](#português-do-brasil)

---

# English

Lume3D is a small native 3D engine for C11. It brings the approachable scene, camera, geometry, material, light, and render flow popularized by Three.js to desktop applications without a browser or JavaScript runtime.

**Current stable release: 1.0.0**

![Lume3D version](https://img.shields.io/badge/Lume3D-1.0.0-blue)
![C](https://img.shields.io/badge/C-11-informational)
![OpenGL](https://img.shields.io/badge/OpenGL-3.3-informational)
![License](https://img.shields.io/badge/license-MIT-green)

## What 1.0.0 means

Lume3D 1.0.0 provides a complete first rendering path: an integrated GLFW runtime, stateful keyboard and mouse input, a hierarchical scene graph, perspective and orthographic cameras, indexed custom geometry, built-in box/plane/sphere geometry, image textures, basic and Lambert materials, three light types, and an OpenGL 3.3 renderer.

The public API is intentionally English and uses opaque handles such as `LumeApp`, `LumeScene`, and `LumeNode`. The implementation is organized and named in Brazilian Portuguese so Portuguese-speaking C developers can study it without changing the API seen by international users. Runtime errors, logs, build targets, tests, and example output are English.

| Subsystem | 1.0.0 status |
| --- | --- |
| Runtime | Window, OpenGL context, frame loop, resize, VSync, clear color, and diagnostics |
| Input | Keyboard and mouse down/pressed/released state, pointer delta, and scrolling |
| Scene | Owned nodes, parent/child hierarchy, translation, Euler rotation, scale, and look-at |
| Cameras | Perspective with automatic aspect ratio and configurable orthographic projection |
| Geometry | Custom indexed/non-indexed meshes, generated normals, box, plane, and UV sphere |
| Appearance | RGBA textures, stb_image loading, basic material, Lambert material, and wireframe |
| Lighting | Accumulated ambient light and up to four directional plus four point lights |
| Platforms | Linux GCC/Clang and Windows MSVC/MinGW-oriented CMake build |

## 1.0.0 release highlights

- compact C11 API with one public header and opaque implementation types;
- application-owned GPU resources and scene-owned node hierarchies;
- automatic OpenGL entry-point loading through a pinned GLAD generator;
- default cameras that follow framebuffer aspect-ratio changes;
- English runtime diagnostics through a built-in or user-provided log callback;
- three runnable examples, deterministic unit tests, and a hidden-window OpenGL smoke test;
- installable static or shared CMake target named `Lume3D::lume3d`;
- mirrored English and Brazilian Portuguese guides.

## Installation

Lume3D requires CMake 3.20 or newer, a C11 compiler, Python 3 for GLAD generation, Git for pinned dependency retrieval, and development packages required by GLFW on the host platform. CMake retrieves the pinned GLFW, GLAD, and stb_image sources automatically.

Build the library, examples, and tests:

    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build --parallel

Run the tests:

    ctest --test-dir build --output-on-failure

Each example also accepts `--smoke`, which renders two hidden frames and exits for automation.

Install the package:

    cmake --install build --prefix ./install

Use the installed target from another CMake project:

```cmake
find_package(Lume3D 1 CONFIG REQUIRED)
target_link_libraries(my_application PRIVATE Lume3D::lume3d)
```

Options:

| Option | Default | Purpose |
| --- | --- | --- |
| `LUME_BUILD_EXAMPLES` | `ON` | Build the three interactive examples |
| `LUME_BUILD_TESTS` | `ON` | Build unit and OpenGL smoke tests |
| `LUME_WARNINGS_AS_ERRORS` | `OFF` | Promote Lume3D compiler warnings to errors |
| `BUILD_SHARED_LIBS` | `OFF` | Build Lume3D and its bundled targets as shared libraries |

See [Building Lume3D](doc-en/BUILDING.md) for Linux, Windows, sanitizer, and consumer-package workflows.

## Quick tour

### Window, scene, and spinning cube

```c
#include <lume/lume.h>

int main(void)
{
    LumeApp *app = lume_app_create(NULL);
    LumeScene *scene = lume_scene_create(app);
    LumeNode *camera = lume_camera_create_perspective(scene, NULL);
    LumeGeometry *box = lume_geometry_create_box(app, 1.0f, 1.0f, 1.0f);
    LumeMaterial *material = lume_material_create_basic(app, NULL);
    LumeNode *cube = lume_mesh_create(scene, box, material);

    lume_node_set_position(camera, (LumeVec3){0.0f, 0.0f, 3.0f});

    while (!lume_app_should_close(app)) {
        float delta_time = lume_app_begin_frame(app);
        lume_node_rotate_y(cube, delta_time);
        lume_render(app, scene, camera);
        lume_app_end_frame(app);
    }

    lume_app_destroy(app);
}
```

Check a creation call before using its result. `lume_get_last_error()` describes the most recent failure in English. Destroying the application also destroys its scenes, geometry, materials, and textures.

### Scene hierarchy

Empty nodes make useful pivots. A child inherits its parent's transform:

```c
LumeNode *orbit = lume_node_create(scene);
LumeNode *planet = lume_mesh_create(scene, sphere, planet_material);

lume_node_set_position(planet, (LumeVec3){4.0f, 0.0f, 0.0f});
lume_node_add_child(orbit, planet);
lume_node_rotate_y(orbit, delta_time * 0.4f);
```

### Materials, textures, and lights

```c
LumeTexture *texture = lume_texture_load(app, "crate.png", NULL);
LumeLambertMaterialConfig material_config = lume_lambert_material_config_default();
material_config.texture = texture;

LumeMaterial *material = lume_material_create_lambert(app, &material_config);
LumeNode *mesh = lume_mesh_create(scene, box, material);

lume_ambient_light_create(scene, NULL);
lume_directional_light_create(scene, NULL);
```

## Suggested architecture

Applications normally keep one `LumeApp`, one or more scenes, and resource handles shared by meshes:

```text
LumeApp
├── window, input, clock, and OpenGL renderer
├── geometry, materials, and textures
└── LumeScene
    ├── camera
    ├── lights
    └── nodes
        └── child nodes
```

The scene destroys its node hierarchy. The application destroys all scenes and GPU resources still alive. Rendering and resource creation are single-threaded and must occur on the application thread.

## Documentation

- [API guide](doc-en/API.md) — public types, defaults, ownership, and diagnostics;
- [Building](doc-en/BUILDING.md) — toolchains, CMake options, tests, and installation;
- [Architecture](doc-en/ARCHITECTURE.md) — data flow, coordinate system, modules, and renderer limits;
- [Example guide](doc-en/EXAMPLES.md) — what each example demonstrates and how to run it;
- [Changelog](CHANGELOG.md) — release history.

Brazilian Portuguese mirrors are available under [`doc-ptbr`](doc-ptbr/).

## Current limitations

- OpenGL 3.3 Core is the only rendering backend.
- The runtime and renderer are single-threaded.
- A scene renders opaque triangles; transparent sorting is not implemented.
- Custom shaders, glTF/OBJ loaders, skeletal animation, shadows, PBR, post-processing, audio, browser, and mobile support are outside 1.0.0.
- Lambert lighting supports four directional and four point lights per rendered scene; additional lights are ignored with an English warning.

## License

Lume3D is available under the [MIT License](LICENSE).

---

# Português do Brasil

Lume3D é uma pequena engine 3D nativa para C11. Ela leva às aplicações desktop o fluxo acessível de cena, câmera, geometria, material, luz e renderização popularizado pelo Three.js, sem navegador ou runtime JavaScript.

**Versão estável atual: 1.0.0**

## O que 1.0.0 significa

Lume3D 1.0.0 fornece um primeiro caminho completo de renderização: runtime GLFW integrado, input stateful de teclado e mouse, grafo de cena hierárquico, câmeras perspectiva e ortográfica, geometria personalizada indexada, caixa/plano/esfera, texturas de imagem, materiais basic e Lambert, três tipos de luz e renderizador OpenGL 3.3.

A API pública usa inglês deliberadamente e expõe handles opacos como `LumeApp`, `LumeScene` e `LumeNode`. A implementação é organizada e nomeada em português do Brasil para que desenvolvedores C lusófonos possam estudá-la sem alterar a API vista por usuários internacionais. Erros, logs, targets de build, testes e saídas dos exemplos usam inglês.

| Subsistema | Estado na 1.0.0 |
| --- | --- |
| Runtime | Janela, contexto OpenGL, loop de frame, resize, VSync, cor de limpeza e diagnósticos |
| Input | Estados down/pressed/released de teclado e mouse, delta do cursor e rolagem |
| Cena | Nós possuídos, hierarquia pai/filho, translação, rotação Euler, escala e look-at |
| Câmeras | Perspectiva com proporção automática e projeção ortográfica configurável |
| Geometria | Malhas personalizadas, normais geradas, caixa, plano e esfera UV |
| Aparência | Texturas RGBA, carregamento stb_image, materiais basic/Lambert e wireframe |
| Iluminação | Luz ambiente acumulada e até quatro luzes direcionais e quatro pontuais |
| Plataformas | Build CMake orientado a Linux GCC/Clang e Windows MSVC/MinGW |

## Destaques da versão 1.0.0

- API C11 compacta com um header público e tipos de implementação opacos;
- recursos de GPU possuídos pelo aplicativo e hierarquias de nós possuídas pela cena;
- carregamento de funções OpenGL por um gerador GLAD fixado;
- câmeras padrão que acompanham mudanças de proporção do framebuffer;
- diagnósticos de runtime em inglês via callback embutido ou definido pelo usuário;
- três exemplos executáveis, testes unitários determinísticos e smoke test OpenGL;
- target CMake estático ou compartilhado instalável chamado `Lume3D::lume3d`;
- guias espelhados em inglês e português do Brasil.

## Instalação

Lume3D requer CMake 3.20 ou mais recente, compilador C11, Python 3 para gerar o GLAD, Git para obter dependências fixadas e os pacotes de desenvolvimento exigidos pelo GLFW na plataforma. O CMake obtém automaticamente GLFW, GLAD e stb_image.

Compile a biblioteca, os exemplos e os testes:

    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build --parallel

Execute os testes:

    ctest --test-dir build --output-on-failure

Cada exemplo também aceita `--smoke`, que renderiza dois frames ocultos e encerra para automação.

Instale o pacote:

    cmake --install build --prefix ./install

Consuma o target instalado em outro projeto CMake:

```cmake
find_package(Lume3D 1 CONFIG REQUIRED)
target_link_libraries(meu_aplicativo PRIVATE Lume3D::lume3d)
```

Consulte [Compilando o Lume3D](doc-ptbr/COMPILACAO.md) para fluxos Linux, Windows, sanitizers e pacote consumidor.

## Visão rápida

### Janela, cena e cubo giratório

O exemplo mínimo em inglês acima é a forma canônica da API. Cada frame começa com `lume_app_begin_frame`, renderiza com `lume_render` e termina com `lume_app_end_frame`.

Verifique o resultado de uma criação antes de usá-lo. `lume_get_last_error()` descreve em inglês a falha mais recente. Destruir o aplicativo também destrói cenas, geometrias, materiais e texturas pertencentes a ele.

### Hierarquia de cena

Nós vazios funcionam como pivôs. Um filho herda a transformação do pai:

```c
LumeNode *orbita = lume_node_create(scene);
LumeNode *planeta = lume_mesh_create(scene, sphere, planet_material);

lume_node_set_position(planeta, (LumeVec3){4.0f, 0.0f, 0.0f});
lume_node_add_child(orbita, planeta);
lume_node_rotate_y(orbita, delta_time * 0.4f);
```

### Materiais, texturas e luzes

O material `basic` ignora iluminação. O material `lambert` combina cor e textura com luzes ambiente, direcionais e pontuais. Consulte o [Guia da API](doc-ptbr/API.md) para configurações e valores padrão.

## Arquitetura sugerida

Aplicações normalmente mantêm um `LumeApp`, uma ou mais cenas e handles de recursos compartilhados pelas malhas:

```text
LumeApp
├── janela, input, relógio e renderizador OpenGL
├── geometrias, materiais e texturas
└── LumeScene
    ├── câmera
    ├── luzes
    └── nós
        └── nós filhos
```

A cena destrói sua hierarquia de nós. O aplicativo destrói todas as cenas e recursos de GPU ainda existentes. Renderização e criação de recursos são single-threaded e devem ocorrer na thread do aplicativo.

## Documentação

- [Guia da API](doc-ptbr/API.md) — tipos públicos, padrões, ownership e diagnósticos;
- [Compilação](doc-ptbr/COMPILACAO.md) — toolchains, opções CMake, testes e instalação;
- [Arquitetura](doc-ptbr/ARQUITETURA.md) — fluxo de dados, coordenadas, módulos e limites;
- [Guia dos exemplos](doc-ptbr/EXEMPLOS.md) — objetivo e execução de cada exemplo;
- [Changelog](CHANGELOG.md) — histórico de versões em inglês.

Os documentos primários em inglês ficam em [`doc-en`](doc-en/).

## Limitações atuais

- OpenGL 3.3 Core é o único backend de renderização.
- Runtime e renderizador são single-threaded.
- Uma cena renderiza triângulos opacos; ordenação de transparência não está implementada.
- Shaders personalizados, glTF/OBJ, animação esquelética, sombras, PBR, pós-processamento, áudio, browser e mobile estão fora da 1.0.0.
- A iluminação Lambert suporta quatro luzes direcionais e quatro pontuais por cena; luzes adicionais são ignoradas com aviso em inglês.

## Licença

Lume3D está disponível sob a [Licença MIT](LICENSE).
