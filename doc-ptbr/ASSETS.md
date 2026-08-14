# Assets e carregamento de modelos

Lume3D 1.5 carrega Wavefront OBJ, glTF 2.0 JSON (`.gltf`) e glTF binário (`.glb`). O loader resolve buffers e imagens externos em relação ao modelo, aceita dados embutidos no GLB, importa primitivas indexadas e converte materiais unlit e metallic/roughness em recursos Lume3D.

## Carregamento síncrono

```c
LumeModel *modelo = NULL;
LumeModelInstance *instancia = NULL;
LumeModelLoadOptions opcoes = lume_model_load_options_default();

opcoes.generate_missing_normals = true;
if (lume_model_load(aplicativo, "assets/scene.glb", &opcoes, &modelo) != LUME_SUCCESS)
    fprintf(stderr, "Load failed: %s\n", lume_error_last()->message);

if (lume_model_instantiate(modelo, cena, &instancia) == LUME_SUCCESS)
    lume_node_set_position(lume_model_instance_root(instancia), (LumeVec3){0, 0, -4});

lume_model_release(modelo);
```

`LumeModel` é imutável e contado por referências. Uma instância possui os nós da cena; geometria, texturas e materiais continuam compartilhados. Destrua a instância com `lume_model_instance_destroy` antes de destruir a cena.

## Carregamento assíncrono

`lume_model_load_async` faz o parsing dos dados de CPU em um worker. Consulte `lume_asset_job_state`; ao chegar em `LUME_ASSET_JOB_READY_FOR_FINALIZE`, chame `lume_asset_job_take_model` na thread do aplicativo/renderização. Essa chamada envia os dados ao OpenGL e move o job para `LUME_ASSET_JOB_COMPLETE`.

Jobs têm contagem de referências e podem ser cancelados. `lume_asset_job_progress` retorna um valor de 0 a 1. Detalhes de falha pertencem ao job por `lume_asset_job_error` e sempre são escritos em inglês.

## Cache e hot reload

As opções padrão usam o cache do aplicativo. Carregamentos repetidos do mesmo caminho normalizado reutilizam um modelo imutável. `lume_assets_clear_cache` libera a referência do cache sem invalidar handles retidos pelo aplicativo.

Ative `hot_reload` e registre `lume_assets_set_reload_callback` para observar a data de modificação. A verificação usa `LumeAppConfig.hot_reload_interval_seconds`; um reload bem-sucedido substitui o modelo em cache para instâncias futuras e informa o caminho alterado pelo callback.

## Limite atual dos formatos

OBJ suporta posições, UVs, normais, faces trianguladas, índices negativos e geração de normais ausentes. glTF suporta hierarquia de nós, primitivas triangulares indexadas, streams comuns de vértices, materiais PBR/unlit, imagens embutidas/externas e clips de animação de transformação. Streams de joints e weights são importados como atributos; deformação de skin e avaliação de morph weights ainda são experimentais na 1.5.
