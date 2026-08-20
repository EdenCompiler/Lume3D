#include <lume/lume.h>

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SOL_PI 3.14159265359f
#define SOL_MAX_CORPOS 8

typedef struct ConfiguracaoExemplo
{
    bool baixa_qualidade;
    bool smoke;
} ConfiguracaoExemplo;

typedef struct CorpoCeleste
{
    LumeNode *pivo_orbita;
    float velocidade_orbita;
    float distancia_orbita;
    LumeColor cor_orbita;
} CorpoCeleste;

static float limitar(float valor, float minimo, float maximo)
{
    return fmaxf(minimo, fminf(maximo, valor));
}

static bool ler_argumentos(int argc, char **argv, ConfiguracaoExemplo *configuracao)
{
    int indice;
    memset(configuracao, 0, sizeof(*configuracao));
    for (indice = 1; indice < argc; ++indice)
    {
        if (strcmp(argv[indice], "--low") == 0)
            configuracao->baixa_qualidade = true;
        else if (strcmp(argv[indice], "--smoke") == 0)
        {
            configuracao->smoke = true;
            configuracao->baixa_qualidade = true;
        }
        else
        {
            fprintf(stderr, "Unknown option: %s\nUsage: %s [--low] [--smoke]\n", argv[indice], argv[0]);
            return false;
        }
    }
    return true;
}

static LumeResult criar_textura_planeta(LumeApp *aplicativo, LumeTexture **saida)
{
    enum
    {
        LARGURA = 192,
        ALTURA = 96
    };
    uint8_t *pixels = malloc(LARGURA * ALTURA * 4u);
    LumeTextureConfig configuracao = lume_texture_config_default();
    int x, y;
    LumeResult resultado;
    if (!pixels)
        return LUME_ERROR_OUT_OF_MEMORY;

    /* O mapa procedural combina oceanos, continentes e calotas sem depender de assets externos. */
    for (y = 0; y < ALTURA; ++y)
    {
        float latitude = ((float)y / (ALTURA - 1) - 0.5f) * SOL_PI;
        for (x = 0; x < LARGURA; ++x)
        {
            float longitude = (float)x / LARGURA * SOL_PI * 2.0f;
            float massa = sinf(longitude * 2.1f + sinf(latitude * 3.0f) * 1.7f) +
                          0.55f * sinf(longitude * 5.3f - latitude * 4.1f) +
                          0.28f * cosf(longitude * 11.0f + latitude * 7.0f);
            float gelo = fabsf(latitude) > 1.18f ? 1.0f : 0.0f;
            bool terra = massa > 0.33f && !gelo;
            size_t pixel = ((size_t)y * LARGURA + x) * 4u;
            pixels[pixel] = gelo ? 220u : terra ? 38u : 8u;
            pixels[pixel + 1] = gelo ? 235u : terra ? (uint8_t)(92 + 35 * sinf(longitude * 3.0f)) : 52u;
            pixels[pixel + 2] = gelo ? 245u : terra ? 34u : (uint8_t)(125 + 35 * cosf(latitude));
            pixels[pixel + 3] = 255u;
        }
    }
    configuracao.srgb = true;
    configuracao.wrap_u = LUME_TEXTURE_WRAP_REPEAT;
    resultado = lume_texture_create_rgba8(aplicativo, pixels, LARGURA, ALTURA, &configuracao, saida);
    free(pixels);
    return resultado;
}

static LumeResult criar_material(LumeApp *aplicativo, LumeMaterialType tipo, LumeColor cor, float metalico,
                                 float rugosidade, LumeTexture *textura, LumeMaterial **saida)
{
    LumeMaterialConfig configuracao = lume_material_config_default(tipo);
    configuracao.base_color = cor;
    configuracao.metallic = metalico;
    configuracao.roughness = rugosidade;
    configuracao.base_color_texture = textura;
    return lume_material_create(aplicativo, &configuracao, saida);
}

static LumeResult criar_anel(LumeApp *aplicativo, uint32_t segmentos, LumeGeometry **saida)
{
    size_t quantidade_vertices = (size_t)(segmentos + 1) * 2u;
    size_t quantidade_indices = (size_t)segmentos * 6u;
    float *posicoes = malloc(quantidade_vertices * 3u * sizeof(float));
    float *normais = malloc(quantidade_vertices * 3u * sizeof(float));
    float *uvs = malloc(quantidade_vertices * 2u * sizeof(float));
    uint32_t *indices = malloc(quantidade_indices * sizeof(uint32_t));
    LumeGeometryData dados = {0};
    uint32_t i;
    LumeResult resultado;
    if (!posicoes || !normais || !uvs || !indices)
    {
        free(posicoes);
        free(normais);
        free(uvs);
        free(indices);
        return LUME_ERROR_OUT_OF_MEMORY;
    }
    for (i = 0; i <= segmentos; ++i)
    {
        float angulo = (float)i / segmentos * SOL_PI * 2.0f;
        uint32_t faixa;
        for (faixa = 0; faixa < 2; ++faixa)
        {
            size_t vertice = (size_t)i * 2u + faixa;
            float raio = faixa ? 2.15f : 1.35f;
            posicoes[vertice * 3u] = cosf(angulo) * raio;
            posicoes[vertice * 3u + 1] = 0.0f;
            posicoes[vertice * 3u + 2] = sinf(angulo) * raio;
            normais[vertice * 3u] = 0.0f;
            normais[vertice * 3u + 1] = 1.0f;
            normais[vertice * 3u + 2] = 0.0f;
            uvs[vertice * 2u] = (float)i / segmentos;
            uvs[vertice * 2u + 1] = (float)faixa;
        }
    }
    for (i = 0; i < segmentos; ++i)
    {
        size_t indice = (size_t)i * 6u;
        uint32_t a = i * 2u;
        indices[indice] = a;
        indices[indice + 1] = a + 1;
        indices[indice + 2] = a + 2;
        indices[indice + 3] = a + 2;
        indices[indice + 4] = a + 1;
        indices[indice + 5] = a + 3;
    }
    dados.positions = posicoes;
    dados.normals = normais;
    dados.texture_coordinates = uvs;
    dados.vertex_count = quantidade_vertices;
    dados.indices = indices;
    dados.index_count = quantidade_indices;
    resultado = lume_geometry_create(aplicativo, &dados, saida);
    free(posicoes);
    free(normais);
    free(uvs);
    free(indices);
    return resultado;
}

static LumeResult criar_corpo(LumeApp *aplicativo, LumeScene *cena, LumeGeometry *esfera, LumeNode *pai,
                              const char *nome, float distancia, float raio, float velocidade, LumeColor cor,
                              float metalico, float rugosidade, LumeTexture *textura, CorpoCeleste *corpo,
                              LumeNode **saida_ancora)
{
    LumeNode *pivo = NULL, *ancora = NULL, *malha = NULL;
    LumeMaterial *material = NULL;
    LumeResult resultado = criar_material(aplicativo, LUME_MATERIAL_PBR, cor, metalico, rugosidade, textura, &material);
    if (resultado != LUME_SUCCESS || lume_node_create(cena, &pivo) != LUME_SUCCESS ||
        lume_node_create(cena, &ancora) != LUME_SUCCESS || lume_mesh_create(cena, esfera, material, &malha) != LUME_SUCCESS ||
        lume_node_add_child(pai, pivo) != LUME_SUCCESS || lume_node_add_child(pivo, ancora) != LUME_SUCCESS ||
        lume_node_add_child(ancora, malha) != LUME_SUCCESS)
    {
        lume_material_release(material);
        return lume_error_last()->code;
    }
    lume_material_release(material);
    lume_node_set_name(malha, nome);
    lume_node_set_position(ancora, (LumeVec3){distancia, 0, 0});
    lume_node_set_scale(malha, (LumeVec3){raio, raio, raio});
    corpo->pivo_orbita = pivo;
    corpo->velocidade_orbita = velocidade;
    corpo->distancia_orbita = distancia;
    corpo->cor_orbita = (LumeColor){cor.r * 0.7f + 0.2f, cor.g * 0.7f + 0.2f, cor.b * 0.7f + 0.2f, 0.7f};
    if (saida_ancora)
        *saida_ancora = ancora;
    return LUME_SUCCESS;
}

static void desenhar_orbita(LumeRenderer *renderizador, float raio, LumeColor cor)
{
    int i;
    const int segmentos = 96;
    for (i = 0; i < segmentos; ++i)
    {
        float a = (float)i / segmentos * SOL_PI * 2.0f;
        float b = (float)(i + 1) / segmentos * SOL_PI * 2.0f;
        lume_debug_line(renderizador, (LumeVec3){cosf(a) * raio, 0, sinf(a) * raio},
                        (LumeVec3){cosf(b) * raio, 0, sinf(b) * raio}, cor);
    }
}

static LumeResult criar_campo_estrelas(LumeApp *aplicativo, LumeScene *cena, bool baixa_qualidade,
                                       LumeGeometry **geometria_saida, LumeMaterial **material_saida,
                                       LumeNode **malha_saida)
{
    uint32_t quantidade = baixa_qualidade ? 180u : 520u;
    uint32_t indice;
    LumeResult resultado;
    resultado = lume_geometry_create_sphere(aplicativo, 1.0f, 6u, 4u, geometria_saida);
    if (resultado != LUME_SUCCESS)
        return resultado;
    resultado = criar_material(aplicativo, LUME_MATERIAL_UNLIT, (LumeColor){2.2f, 2.5f, 3.8f, 1}, 0, 1, NULL,
                               material_saida);
    if (resultado != LUME_SUCCESS)
        return resultado;
    resultado = lume_instanced_mesh_create(cena, *geometria_saida, *material_saida, quantidade, malha_saida);
    if (resultado != LUME_SUCCESS)
        return resultado;
    /* A espiral de Fibonacci distribui estrelas sem agrupamentos ou arquivos de imagem. */
    for (indice = 0; indice < quantidade; ++indice)
    {
        float y = 1.0f - 2.0f * ((float)indice + 0.5f) / quantidade;
        float raio_horizontal = sqrtf(fmaxf(0.0f, 1.0f - y * y));
        float angulo = indice * 2.39996323f;
        float tamanho = 0.055f + (float)(indice % 11u) / 10.0f * 0.105f;
        LumeVec3 posicao = {cosf(angulo) * raio_horizontal * 72.0f, y * 72.0f,
                            sinf(angulo) * raio_horizontal * 72.0f};
        resultado = lume_instanced_mesh_set_transform(
            *malha_saida, indice,
            lume_mat4_transform(posicao, lume_quat_identity(), (LumeVec3){tamanho, tamanho, tamanho}));
        if (resultado != LUME_SUCCESS)
            return resultado;
    }
    return LUME_SUCCESS;
}

int main(int argc, char **argv)
{
    ConfiguracaoExemplo opcoes;
    LumeAppConfig configuracao = lume_app_config_default();
    LumeRendererConfig renderer_config = lume_renderer_config_default();
    LumePerspectiveCameraConfig camera_config = lume_perspective_camera_config_default();
    LumeApp *aplicativo = NULL;
    LumeScene *cena = NULL;
    LumeNode *camera = NULL, *raiz = NULL, *sol = NULL, *terra = NULL, *saturno = NULL, *anel = NULL, *luz = NULL;
    LumeNode *estrelas = NULL;
    LumeGeometry *esfera = NULL, *geometria_anel = NULL, *geometria_estrelas = NULL;
    LumeMaterial *material_sol = NULL, *material_anel = NULL, *material_estrelas = NULL;
    LumeTexture *textura_terra = NULL;
    CorpoCeleste corpos[SOL_MAX_CORPOS] = {0};
    int quantidade_corpos = 0, quadros = 0;
    float escala_tempo = 1.0f, guinada = 0.0f, inclinacao = 0.28f, distancia_camera = 50.0f;
    LumeVec3 alvo_camera = {0, 0, 0};
    bool pausado = false, mostrar_orbitas = true;

    if (!ler_argumentos(argc, argv, &opcoes))
        return 2;
    configuracao.title = "Lume3D - Procedural solar system";
    configuracao.width = opcoes.smoke ? 320 : opcoes.baixa_qualidade ? 800 : 1280;
    configuracao.height = opcoes.smoke ? 180 : opcoes.baixa_qualidade ? 450 : 720;
    configuracao.visible = !opcoes.smoke;
    configuracao.vsync = !opcoes.smoke;
    configuracao.clear_color = (LumeColor){0.001f, 0.002f, 0.012f, 1.0f};
    renderer_config.hdr = true;
    renderer_config.tone_mapping = LUME_TONE_MAPPING_ACES;
    renderer_config.bloom = !opcoes.baixa_qualidade;
    renderer_config.fxaa = true;
    renderer_config.exposure = 1.05f;
    renderer_config.directional_shadow_size = opcoes.baixa_qualidade ? 512u : 2048u;
    renderer_config.spot_shadow_size = opcoes.baixa_qualidade ? 512u : 2048u;
    camera_config.near_plane = 0.05f;
    camera_config.far_plane = 300.0f;

    if (lume_app_create(&configuracao, &aplicativo) != LUME_SUCCESS ||
        lume_renderer_configure(lume_app_renderer(aplicativo), &renderer_config) != LUME_SUCCESS ||
        lume_scene_create(aplicativo, &cena) != LUME_SUCCESS || lume_node_create(cena, &raiz) != LUME_SUCCESS ||
        lume_camera_create_perspective(cena, &camera_config, &camera) != LUME_SUCCESS ||
        lume_geometry_create_sphere(aplicativo, 1.0f, opcoes.baixa_qualidade ? 28u : 64u,
                                    opcoes.baixa_qualidade ? 16u : 36u, &esfera) != LUME_SUCCESS ||
        criar_anel(aplicativo, opcoes.baixa_qualidade ? 64u : 192u, &geometria_anel) != LUME_SUCCESS ||
        criar_textura_planeta(aplicativo, &textura_terra) != LUME_SUCCESS ||
        criar_campo_estrelas(aplicativo, cena, opcoes.baixa_qualidade, &geometria_estrelas, &material_estrelas,
                             &estrelas) != LUME_SUCCESS)
        goto falha;

    if (criar_material(aplicativo, LUME_MATERIAL_UNLIT, (LumeColor){5.0f, 1.6f, 0.22f, 1}, 0, 1, NULL,
                       &material_sol) != LUME_SUCCESS ||
        lume_mesh_create(cena, esfera, material_sol, &sol) != LUME_SUCCESS)
        goto falha;
    lume_node_set_scale(sol, (LumeVec3){2.4f, 2.4f, 2.4f});
    lume_node_set_name(sol, "Sun");
    {
        LumePointLightConfig config_luz = lume_point_light_config_default();
        LumeAmbientLightConfig ambiente = lume_ambient_light_config_default();
        config_luz.color = (LumeColor){1.0f, 0.72f, 0.38f, 1};
        config_luz.intensity = 7.5f;
        config_luz.range = 65.0f;
        ambiente.color = (LumeColor){0.1f, 0.16f, 0.32f, 1};
        ambiente.intensity = 0.16f;
        if (lume_point_light_create(cena, &config_luz, &luz) != LUME_SUCCESS ||
            lume_ambient_light_create(cena, &ambiente, &luz) != LUME_SUCCESS)
            goto falha;
    }

    if (criar_corpo(aplicativo, cena, esfera, raiz, "Mercury", 5.2f, 0.42f, 1.40f,
                    (LumeColor){0.55f, 0.49f, 0.42f, 1}, 0.1f, 0.86f, NULL, &corpos[quantidade_corpos++], NULL) !=
            LUME_SUCCESS ||
        criar_corpo(aplicativo, cena, esfera, raiz, "Venus", 7.3f, 0.72f, 1.05f,
                    (LumeColor){0.84f, 0.52f, 0.19f, 1}, 0.05f, 0.74f, NULL, &corpos[quantidade_corpos++], NULL) !=
            LUME_SUCCESS ||
        criar_corpo(aplicativo, cena, esfera, raiz, "Earth", 10.0f, 0.82f, 0.82f,
                    (LumeColor){1, 1, 1, 1}, 0.08f, 0.52f, textura_terra, &corpos[quantidade_corpos++], &terra) !=
            LUME_SUCCESS ||
        criar_corpo(aplicativo, cena, esfera, terra, "Moon", 1.55f, 0.22f, 2.9f,
                    (LumeColor){0.68f, 0.70f, 0.73f, 1}, 0.0f, 0.91f, NULL, &corpos[quantidade_corpos++], NULL) !=
            LUME_SUCCESS ||
        criar_corpo(aplicativo, cena, esfera, raiz, "Mars", 13.1f, 0.57f, 0.66f,
                    (LumeColor){0.72f, 0.19f, 0.08f, 1}, 0.03f, 0.82f, NULL, &corpos[quantidade_corpos++], NULL) !=
            LUME_SUCCESS ||
        criar_corpo(aplicativo, cena, esfera, raiz, "Jupiter", 17.4f, 1.65f, 0.36f,
                    (LumeColor){0.76f, 0.48f, 0.28f, 1}, 0.05f, 0.67f, NULL, &corpos[quantidade_corpos++], NULL) !=
            LUME_SUCCESS ||
        criar_corpo(aplicativo, cena, esfera, raiz, "Saturn", 22.1f, 1.38f, 0.27f,
                    (LumeColor){0.78f, 0.64f, 0.35f, 1}, 0.12f, 0.61f, NULL, &corpos[quantidade_corpos++], &saturno) !=
            LUME_SUCCESS)
        goto falha;

    if (criar_material(aplicativo, LUME_MATERIAL_PBR, (LumeColor){0.66f, 0.52f, 0.30f, 0.78f}, 0.16f, 0.72f,
                       NULL, &material_anel) != LUME_SUCCESS ||
        lume_mesh_create(cena, geometria_anel, material_anel, &anel) != LUME_SUCCESS ||
        lume_node_add_child(saturno, anel) != LUME_SUCCESS)
        goto falha;
    lume_node_set_scale(anel, (LumeVec3){1.38f, 1.38f, 1.38f});
    lume_node_set_euler_rotation(anel, (LumeVec3){0.14f, 0, 0.08f});

    lume_geometry_release(esfera);
    esfera = NULL;
    lume_geometry_release(geometria_anel);
    geometria_anel = NULL;
    lume_geometry_release(geometria_estrelas);
    geometria_estrelas = NULL;
    lume_material_release(material_sol);
    material_sol = NULL;
    lume_material_release(material_anel);
    material_anel = NULL;
    lume_material_release(material_estrelas);
    material_estrelas = NULL;
    lume_texture_release(textura_terra);
    textura_terra = NULL;

    if (!opcoes.smoke)
        puts("Controls: left-drag orbit, middle-drag pan, wheel zoom, Space pause, Up/Down speed, O orbit paths, R reset, Esc exit.");
    while (!lume_app_should_close(aplicativo))
    {
        float delta = lume_app_begin_frame(aplicativo);
        float mouse_x, mouse_y, rolagem_x, rolagem_y;
        int indice;
        lume_mouse_get_delta(aplicativo, &mouse_x, &mouse_y);
        lume_mouse_get_scroll(aplicativo, &rolagem_x, &rolagem_y);
        (void)rolagem_x;
        if (lume_key_was_pressed(aplicativo, LUME_KEY_ESCAPE))
            lume_app_request_close(aplicativo);
        if (lume_key_was_pressed(aplicativo, LUME_KEY_SPACE))
            pausado = !pausado;
        if (lume_key_was_pressed(aplicativo, LUME_KEY_O))
            mostrar_orbitas = !mostrar_orbitas;
        if (lume_key_is_down(aplicativo, LUME_KEY_UP))
            escala_tempo = limitar(escala_tempo + delta, 0.1f, 5.0f);
        if (lume_key_is_down(aplicativo, LUME_KEY_DOWN))
            escala_tempo = limitar(escala_tempo - delta, 0.1f, 5.0f);
        if (lume_key_was_pressed(aplicativo, LUME_KEY_R))
        {
            guinada = 0.0f;
            inclinacao = 0.28f;
            distancia_camera = 50.0f;
            alvo_camera = (LumeVec3){0, 0, 0};
        }
        if (lume_mouse_button_is_down(aplicativo, LUME_MOUSE_BUTTON_LEFT))
        {
            guinada -= mouse_x * 0.006f;
            inclinacao = limitar(inclinacao - mouse_y * 0.006f, -1.25f, 1.25f);
        }
        if (lume_mouse_button_is_down(aplicativo, LUME_MOUSE_BUTTON_MIDDLE))
        {
            alvo_camera.x -= mouse_x * distancia_camera * 0.0012f;
            alvo_camera.y += mouse_y * distancia_camera * 0.0012f;
        }
        distancia_camera = limitar(distancia_camera - rolagem_y * 2.2f, 9.0f, 80.0f);
        if (!pausado)
        {
            for (indice = 0; indice < quantidade_corpos; ++indice)
                lume_node_rotate_y(corpos[indice].pivo_orbita, delta * corpos[indice].velocidade_orbita * escala_tempo);
            lume_node_rotate_y(sol, delta * 0.18f * escala_tempo);
        }
        lume_node_set_position(camera,
                               (LumeVec3){alvo_camera.x + cosf(inclinacao) * sinf(guinada) * distancia_camera,
                                          alvo_camera.y + sinf(inclinacao) * distancia_camera,
                                          alvo_camera.z + cosf(inclinacao) * cosf(guinada) * distancia_camera});
        if (lume_node_look_at(camera, alvo_camera) != LUME_SUCCESS)
            goto falha;
        if (mostrar_orbitas)
            for (indice = 0; indice < quantidade_corpos; ++indice)
                if (corpos[indice].distancia_orbita > 3.0f)
                    desenhar_orbita(lume_app_renderer(aplicativo), corpos[indice].distancia_orbita,
                                    corpos[indice].cor_orbita);
        if (lume_app_render(aplicativo, cena, camera) != LUME_SUCCESS)
            goto falha;
        lume_app_end_frame(aplicativo);
        if (opcoes.smoke && ++quadros >= 2)
            lume_app_request_close(aplicativo);
    }
    if (!opcoes.smoke)
    {
        LumeFrameStats estatisticas = lume_renderer_frame_stats(lume_app_renderer(aplicativo));
        printf("Solar system finished: %llu objects, %llu draw calls, %.2f ms CPU.\n",
               (unsigned long long)estatisticas.submitted_objects, (unsigned long long)estatisticas.draw_calls,
               estatisticas.cpu_time_ms);
    }
    lume_app_destroy(aplicativo);
    return 0;

falha:
    fprintf(stderr, "Solar system example failed: %s\n", lume_error_last()->message);
    lume_geometry_release(esfera);
    lume_geometry_release(geometria_anel);
    lume_geometry_release(geometria_estrelas);
    lume_material_release(material_sol);
    lume_material_release(material_anel);
    lume_material_release(material_estrelas);
    lume_texture_release(textura_terra);
    lume_app_destroy(aplicativo);
    return 1;
}
