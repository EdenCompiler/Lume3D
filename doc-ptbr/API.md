# Guia da API Lume3D 1.0

Lume3D expõe um único header público C11:

```c
#include <lume/lume.h>
```

Todo símbolo público começa com `lume_` ou `Lume`. Os handles são opacos; aplicações usam funções em vez de acessar campos internos.

## Aplicativo e diagnósticos

`lume_app_create` cria janela GLFW, contexto OpenGL 3.3 Core, estado de input, relógio, renderizador e proprietário dos recursos. Passar `NULL` seleciona `lume_app_config_default()`.

A configuração padrão cria uma janela visível e redimensionável de 1280×720 com VSync. Use `visible = false` em testes automatizados de renderização.

Funções de criação retornam `NULL` em falha. Operações booleanas retornam `false`. `lume_get_last_error()` devolve o último diagnóstico global do processo em inglês. Configure `log_callback` para receber inicialização e avisos não fatais sem usar a saída padrão.

Cada frame segue esta ordem:

1. `lume_app_begin_frame` registra o input anterior, processa eventos e devolve segundos decorridos.
2. Atualize o aplicativo e a cena.
3. `lume_render` limpa e renderiza uma cena.
4. `lume_app_end_frame` apresenta o back buffer.

## Input

Consultas de teclado e mouse oferecem variantes `is_down`, `was_pressed` e `was_released`. As transições são válidas depois de `lume_app_begin_frame`.

`lume_mouse_position` usa coordenadas da janela. `lume_mouse_delta` informa a mudança desde o frame anterior. `lume_mouse_scroll` informa a rolagem acumulada no frame atual.

## Cenas e nós

`lume_scene_create` registra uma cena no aplicativo. `lume_node_create` adiciona um nó vazio de transformação. Câmeras, malhas e luzes são nós especializados e participam da mesma hierarquia.

Transformações contêm posição, rotação Euler XYZ em radianos e escala. `lume_node_add_child` rejeita relações entre cenas e ciclos. Reparenting remove automaticamente o filho do pai anterior. `lume_node_look_at` recebe um alvo no espaço do mundo.

`lume_node_destroy` destrói o nó escolhido e seus descendentes. `lume_scene_destroy` destrói todos os nós restantes. `lume_app_destroy` destrói com segurança todas as cenas restantes.

## Câmeras

`lume_camera_create_perspective` recebe campo de visão em radianos, proporção e planos de corte. A proporção padrão é zero, fazendo a projeção acompanhar o framebuffer atual. Use valor positivo para projeção fixa; passe zero a `lume_camera_set_aspect_ratio` para restaurar o modo automático.

`lume_camera_create_orthographic` recebe planos esquerdo, direito, inferior, superior, próximo e distante.

O sistema de coordenadas é destro, +Y aponta para cima e câmeras olham ao longo de −Z local.

## Geometria

Construtores embutidos criam caixa centralizada, plano XY voltado para +Z e esfera UV. Dimensões e raio devem ser positivos. A esfera exige pelo menos 3×2 segmentos.

`lume_geometry_create_custom` copia todos os arrays fornecidos. Posições possuem três floats por vértice. Normais são opcionais e geradas acumulando normais das faces indexadas. Coordenadas de textura são opcionais e possuem dois floats por vértice. Índices são opcionais; sem eles, cada grupo sequencial de três vértices forma um triângulo. Todos os índices são validados.

A geometria é enviada à GPU no primeiro render e pode ser compartilhada por várias malhas.

## Texturas e materiais

`lume_texture_create` copia pixels RGBA8 para uma textura OpenGL. `lume_texture_load` aceita formatos do stb_image e os expande para RGBA8. Os padrões usam filtragem linear, repetição, mipmaps e inversão vertical da imagem carregada.

`lume_material_create_basic` combina cor e textura opcional sem luzes da cena. `lume_material_create_lambert` aplica luz difusa ambiente, direcional e pontual. Ambos aceitam modo wireframe na criação.

Materiais e texturas devem pertencer ao mesmo aplicativo. `lume_material_set_color` e `lume_material_set_texture` atualizam um material reutilizável.

## Luzes e renderização

Luz ambiente contribui com uma cor constante. Luz direcional usa direção local transformada pelo nó. Luz pontual usa a posição mundial do nó e um alcance finito com queda quadrática suave.

Contribuições ambiente acumulam sem contagem fixa. O shader aceita quatro luzes direcionais e quatro pontuais. Luzes adicionais são ignoradas e produzem aviso em inglês.

`lume_render` valida ownership de aplicativo/cena/câmera, atualiza matrizes mundiais e viewport, limpa cor e profundidade, envia luzes e desenha cada malha. Retorna `false` em ownership inválido, câmera não inversível ou falha do renderizador.

## Ownership de recursos

Cenas possuem nós. Aplicativos possuem cenas, geometrias, materiais, texturas, janela e objetos OpenGL. Handles de recurso permanecem válidos até `lume_app_destroy`; destruição individual de recursos não existe na 1.0 para manter regras de compartilhamento previsíveis.

Não use um handle com outro aplicativo. Não use nós depois de destruir sua cena ou ancestral. Toda chamada que toca janela ou GPU deve ocorrer na thread do aplicativo.
