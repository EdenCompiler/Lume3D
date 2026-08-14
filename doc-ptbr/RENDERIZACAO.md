# Renderização

Lume3D 1.5 usa um renderizador forward OpenGL 3.3 Core. O caminho padrão suporta materiais unlit, Phong e PBR metallic/roughness, luz ambiente por imagem, materiais opacos/masked/blended, instancing de hardware, frustum culling e estatísticas.

## Configuração do renderizador

```c
LumeRendererConfig configuracao = lume_renderer_config_default();
configuracao.hdr = true;
configuracao.tone_mapping = LUME_TONE_MAPPING_ACES;
configuracao.bloom = true;
configuracao.fxaa = true;
configuracao.exposure = 1.1f;
lume_renderer_configure(lume_app_renderer(aplicativo), &configuracao);
```

HDR usa um alvo intermediário de ponto flutuante. Bloom roda antes de ACES; FXAA atua na imagem LDR. Uma luz direcional pode projetar três mapas de sombra em cascata e até quatro spot lights podem projetar um mapa cada. Os tamanhos usam `directional_shadow_size` e `spot_shadow_size`.

## Shaders e passes personalizados

Crie um `LumeShader` com strings GLSL ou caminhos de arquivos, coloque-o em um `LumePipeline` e use o pipeline num material custom. Lume3D fornece `uModel`, `uView`, `uProjection` e `uCamera` aos shaders de malha. Uniformes do aplicativo usam as chamadas tipadas `lume_shader_set_float`, `set_vec2`, `set_vec3`, `set_vec4` e `set_mat4`.

`lume_renderer_add_pass` insere um pass de tela inteira HDR ou LDR. O shader recebe a imagem anterior como `uColorTexture` e, quando `needs_depth` é verdadeiro, a profundidade como `uDepthTexture`. Passes rodam na ordem de inserção dentro da sua fase.

## Ownership e estatísticas

Handles de geometria, textura, material, shader, pipeline, render target e environment têm contagem de referências. Um material retém texturas e pipeline custom; um pipeline retém seu shader. Libere o handle do aplicativo depois de associá-lo a outro recurso.

`lume_renderer_frame_stats` informa objetos submetidos/descartados, draw calls, instâncias, triângulos, draws de sombra, trocas de estado e tempo de CPU. `gpu_time_ms` é reservado e informa zero quando queries de timer da GPU não estão disponíveis.

## Desenho de depuração

`lume_debug_line`, `lume_debug_axes`, `lume_debug_aabb`, `lume_debug_sphere` e `lume_debug_ray` enfileiram primitivas de um frame. Use `lume_debug_clear` para descartá-las explicitamente.
