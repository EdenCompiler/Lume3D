# Arquitetura do Lume3D

## Objetivo e módulos

Lume3D é um renderizador nativo de cenas legível para C11. Ele oferece um vocabulário de objetos semelhante ao Three.js, mantendo alocação, ownership, threads e falhas explícitos.

A API é modular: `core`, `math`, `scene`, `render`, `assets`, `animation` e `debug`, com `lume.h` como agregador. Contratos privados ficam em `src/lume_interno.h`; identificadores e comentários internos usam português brasileiro. GLFW fornece janelas, GLAD as declarações OpenGL 3.3, stb_image decodifica imagens, cgltf analisa glTF, fast_obj analisa OBJ e tinycthread oferece primitivas portáveis de workers. Revisões são fixadas pelo CMake.

## Fluxo de dados

```text
input/eventos ──→ update do aplicativo ──→ animação/transformações
                                                │
job de modelo ──→ parsing CPU ──→ upload/cache na thread de renderização
                                                │
cena ──→ culling ──→ mapas de sombra ──→ forward HDR ──→ passes HDR custom
                                                    ──→ bloom/ACES/FXAA
                                                    ──→ passes LDR custom ──→ apresentar
```

O contexto OpenGL e toda criação/finalização na GPU pertencem à thread do aplicativo. Jobs assíncronos apenas analisam e preparam dados na CPU. O renderizador percorre um registro plano da cena e resolve transformações de pais recursivamente.

## Coordenadas e visibilidade

Lume3D é destro, +Y para cima, frente da câmera em −Z, radianos e matrizes column-major. A composição é translação × rotação × escala. Aspect zero usa a proporção atual do framebuffer.

Geometrias guardam AABBs locais. Nós derivam AABBs mundiais das matrizes. O renderizador extrai o frustum, descarta meshes sem interseção quando culling está ativo e envia objetos opacos/masked antes dos blended. Raycasts reutilizam os bounds mundiais.

## Ownership

`LumeApp` possui janela, renderizador, cache, jobs e o registro de diagnóstico de vazamentos. `LumeScene` possui seus nós. Recursos de GPU e modelos imutáveis usam contagem de referências, permitindo compartilhar geometria/material/textura.

```text
aplicativo → cena → nós
registro do aplicativo → recursos com contagem de referências
nó mesh → geometria + material → texturas/pipeline custom → shader
instância → nós da cena; modelo → dados/clips importados imutáveis
```

## Limites de renderização

A versão 1.5 tem um backend OpenGL 3.3 Core e renderizador forward. Sombras direcionais usam três cascatas práticas e sombras spot aceitam quatro luzes. Passes custom de tela inteira usam uma cadeia ping-pong. A configuração de amostras MSAA e o tempo de GPU são campos públicos preparados para evolução; esta versão não resolve targets multisample nem emite timer queries. Streams de skin e morph são importados, mas a deformação permanece experimental.
