# Compilando o Lume3D

## Requisitos

- CMake 3.20 ou mais recente;
- compilador C11;
- Git;
- Python 3 com Jinja2;
- suporte de desenvolvimento OpenGL e dependências nativas exigidas pelo GLFW.

GLFW 3.5.1, GLAD 2.0.8 e stb_image são fixados via CMake `FetchContent`. Rebuilds normais reutilizam a árvore de build preenchida.

Instale o requisito do gerador GLAD antes de configurar:

    python -m pip install -r requirements-build.txt

O CMake verifica esse requisito durante a configuração e mostra um comando de instalação em inglês quando ele não está disponível.

## Linux

Instale compilador, CMake, Python, Git e pré-requisitos X11 ou Wayland do GLFW pelo gerenciador da distribuição. Execute:

    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build --parallel
    ctest --test-dir build --output-on-failure

Em máquina headless, execute testes OpenGL sob Xvfb quando não houver display:

    xvfb-run -a ctest --test-dir build --output-on-failure

## Windows

Use um terminal de desenvolvimento do Visual Studio:

    cmake -S . -B build -G "Visual Studio 17 2022" -A x64
    cmake --build build --config Release --parallel
    ctest --test-dir build -C Release --output-on-failure

MinGW também é suportado:

    cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
    cmake --build build --parallel

## Variantes de build

Compile sem exemplos ou testes:

    cmake -S . -B build -DLUME_BUILD_EXAMPLES=OFF -DLUME_BUILD_TESTS=OFF

Compile bibliotecas compartilhadas:

    cmake -S . -B build-shared -DBUILD_SHARED_LIBS=ON

Ative warnings estritos no desenvolvimento:

    cmake -S . -B build-strict -DLUME_WARNINGS_AS_ERRORS=ON

Use AddressSanitizer com GCC ou Clang:

    cmake -S . -B build-asan \
      -DCMAKE_BUILD_TYPE=Debug \
      -DCMAKE_C_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer" \
      -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address,undefined"
    cmake --build build-asan --parallel
    ASAN_OPTIONS=detect_leaks=1 ctest --test-dir build-asan -R lume_unit_tests --output-on-failure
    ASAN_OPTIONS=detect_leaks=0 ctest --test-dir build-asan -R lume_render_test --output-on-failure

A detecção de leaks fica desativada somente no smoke test OpenGL porque drivers de display podem reter alocações até o fim do processo fora do Lume3D. As verificações de endereço e comportamento indefinido continuam ativas nesse teste.

## Instalação e consumidores

Instale em prefixo isolado:

    cmake --install build --prefix "$PWD/install"

Um `CMakeLists.txt` consumidor pode usar:

```cmake
cmake_minimum_required(VERSION 3.20)
project(exemplo LANGUAGES C)

find_package(Lume3D 1 CONFIG REQUIRED)
add_executable(exemplo main.c)
target_link_libraries(exemplo PRIVATE Lume3D::lume3d)
```

Configure com:

    cmake -S consumer -B consumer-build -DCMAKE_PREFIX_PATH="$PWD/install"
    cmake --build consumer-build

O pacote instala seus targets e headers GLFW e GLAD fixados junto ao Lume3D, fornecendo a interface nativa completa a consumidores estáticos.

## Testes

`lume_unit_tests` valida vetores, composição/inversão de matrizes e transformações mundiais hierárquicas sem janela. `lume_render_test` cria contexto oculto, valida topologia das primitivas, renderiza caixa iluminada e lê um pixel não vazio.
