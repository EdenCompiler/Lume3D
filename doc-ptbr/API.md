# Guia da API Lume3D 1.5

O header agregador expõe todos os módulos:

```c
#include <lume/lume.h>
```

Aplicações também podem incluir `core.h`, `math.h`, `scene.h`, `render.h`, `assets.h`, `animation.h` ou `debug.h`. Todo nome público está em inglês e começa com `lume_`, `Lume` ou `LUME_`. Nomes da implementação e comentários do fonte estão em português brasileiro; saídas de runtime, compilador, testes e exemplos estão em inglês.

## Resultados e diagnósticos

Chamadas falíveis retornam `LumeResult` e escrevem o resultado pelo parâmetro final `out_...`:

```c
LumeApp *app = NULL;
LumeResult result = lume_app_create(NULL, &app);
if (result != LUME_SUCCESS) {
    const LumeError *error = lume_error_last();
    fprintf(stderr, "%s: %s\n", lume_result_string(result), error->message);
    return 1;
}
```

`LumeError` contém código, operação, caminho opcional, linha/coluna e mensagem em inglês. O último erro é thread-local. Um callback por aplicativo recebe mensagens info, warning e error em inglês.

## Aplicativo e frames

`lume_app_create` cria janela, contexto OpenGL, estado de input, serviços de assets e renderizador. Comece por `lume_app_config_default`; o padrão é uma janela 1280×720 visível, redimensionável e com VSync.

Cada frame tem uma ordem direta:

1. `lume_app_begin_frame` processa eventos e retorna os segundos decorridos.
2. Atualize cena, animação e estado do aplicativo.
3. `lume_app_render` renderiza a cena com uma câmera.
4. `lume_app_end_frame` apresenta o back buffer.

Use `lume_renderer_render` para renderizar em `LumeRenderTarget`. O input expõe estados down, pressed e released para teclado e botões do mouse. `lume_mouse_get_position`, `lume_mouse_get_delta` e `lume_mouse_get_scroll` retornam coordenadas do cursor, movimento por frame e movimento da roda; ponteiros de saída dispensáveis podem ser `NULL`.

`lume_renderer_present_target` amplia um render target LDR para o framebuffer do aplicativo com filtragem linear. Ele é útil para efeitos caros e compatíveis com renderização por software que devem usar uma resolução interna menor. Alvos HDR precisam passar por tone mapping antes da apresentação.

## Cena e transformações

Uma cena possui seus nós. Nós vazios, câmeras, luzes, meshes e meshes instanciadas usam o handle opaco `LumeNode`. Nós têm nome, posição, rotação quaternion, escala, pai e filhos. Matrizes mundiais e bounds são atualizados sob demanda.

```c
LumeNode *pivo = NULL;
LumeNode *malha = NULL;
lume_node_create(cena, &pivo);
lume_mesh_create(cena, geometria, material, &malha);
lume_node_add_child(pivo, malha);
lume_node_set_position(malha, (LumeVec3){3, 0, 0});
lume_node_rotate_y(pivo, delta);
```

O sistema é destro, +Y aponta para cima, câmeras olham pelo −Z local, ângulos usam radianos e matrizes são column-major.

## Câmeras, luzes e consultas

Câmeras perspectiva e ortográfica usam structs de configuração com construtores padrão. Aspect zero usa a proporção do framebuffer. Há luzes ambiente, directional, point e spot. Configurações directional e spot podem habilitar sombras.

`lume_scene_raycast` testa AABBs mundiais de meshes visíveis e retorna `LumeRaycastHit` do mais próximo ao distante. O módulo math inclui vetores, quaternions, matrizes, inversão/transformação, rays, AABBs, frusta e interseções.

## Recursos

Geometrias, texturas, materiais, shaders, pipelines, render targets, environments, modelos e asset jobs usam contagem de referências. A criação retorna uma referência; `retain` acrescenta ownership e `release` o remove. Nós mesh retêm geometria e material. O aplicativo relata handles vazados em inglês ao ser destruído.

Geometrias prontas incluem box, plane e UV sphere. Geometria custom aceita posições, normais, UVs, tangentes, cores, joints/weights e índices opcionais de 32 bits. Materiais incluem unlit, Phong, PBR e pipeline custom.

Consulte [Renderização](RENDERIZACAO.md), [Assets](ASSETS.md) e [Animação](ANIMACAO.md) para os fluxos completos.

## Ordem de vida útil

Destrua players antes das instâncias, instâncias antes da cena, cenas antes do aplicativo e libere handles de recursos antes de `lume_app_destroy`. Passar `NULL` a funções destroy/release é seguro salvo documentação contrária.
