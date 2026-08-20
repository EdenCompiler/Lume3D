#include <lume/lume.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ESTUDIO_COLUNAS 7
#define ESTUDIO_LINHAS 6
#define ESTUDIO_AMOSTRAS (ESTUDIO_COLUNAS * ESTUDIO_LINHAS)

typedef struct ConfiguracaoExemplo
{
    bool baixa_qualidade;
    bool smoke;
} ConfiguracaoExemplo;

typedef struct AmostraMaterial
{
    LumeNode *malha;
    LumeMaterial *material;
    LumeColor cor;
    float metalico;
    float rugosidade;
} AmostraMaterial;

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

static LumeResult criar_material(LumeApp *aplicativo, LumeMaterialType tipo, LumeColor cor, float metalico,
                                 float rugosidade, LumeMaterial **saida)
{
    LumeMaterialConfig configuracao = lume_material_config_default(tipo);
    configuracao.base_color = cor;
    configuracao.metallic = metalico;
    configuracao.roughness = rugosidade;
    return lume_material_create(aplicativo, &configuracao, saida);
}

static LumeResult criar_malha_simples(LumeApp *aplicativo, LumeScene *cena, LumeGeometry *geometria,
                                      LumeMaterialType tipo, LumeColor cor, float metalico, float rugosidade,
                                      LumeNode **saida)
{
    LumeMaterial *material = NULL;
    LumeResult resultado = criar_material(aplicativo, tipo, cor, metalico, rugosidade, &material);
    if (resultado == LUME_SUCCESS)
        resultado = lume_mesh_create(cena, geometria, material, saida);
    lume_material_release(material);
    return resultado;
}

static LumeRay criar_raio_camera(LumeNode *camera, LumeVec3 alvo, float mouse_x, float mouse_y, int largura,
                                 int altura, float campo_visao)
{
    LumeVec3 origem = lume_node_position(camera);
    LumeVec3 frente = lume_vec3_normalize(lume_vec3_subtract(alvo, origem));
    LumeVec3 direita = lume_vec3_normalize(lume_vec3_cross(frente, (LumeVec3){0, 1, 0}));
    LumeVec3 acima = lume_vec3_normalize(lume_vec3_cross(direita, frente));
    float ndc_x = mouse_x / (float)largura * 2.0f - 1.0f;
    float ndc_y = 1.0f - mouse_y / (float)altura * 2.0f;
    float escala = tanf(campo_visao * 0.5f);
    float aspecto = (float)largura / altura;
    LumeVec3 direcao = lume_vec3_add(frente, lume_vec3_add(lume_vec3_scale(direita, ndc_x * escala * aspecto),
                                                           lume_vec3_scale(acima, ndc_y * escala)));
    return (LumeRay){origem, lume_vec3_normalize(direcao)};
}

static int encontrar_amostra(AmostraMaterial *amostras, LumeNode *no)
{
    int indice;
    for (indice = 0; indice < ESTUDIO_AMOSTRAS; ++indice)
        if (amostras[indice].malha == no)
            return indice;
    return -1;
}

int main(int argc, char **argv)
{
    ConfiguracaoExemplo opcoes;
    LumeAppConfig configuracao = lume_app_config_default();
    LumeRendererConfig renderer_config = lume_renderer_config_default();
    LumePerspectiveCameraConfig camera_config = lume_perspective_camera_config_default();
    LumeApp *aplicativo = NULL;
    LumeScene *cena = NULL;
    LumeNode *camera = NULL, *piso = NULL, *parede = NULL, *pedestal = NULL;
    LumeNode *pivo_luz_a = NULL, *pivo_luz_b = NULL, *luz_a = NULL, *luz_b = NULL, *bulbo_a = NULL, *bulbo_b = NULL;
    LumeNode *luz = NULL;
    LumeGeometry *esfera = NULL, *caixa = NULL, *plano = NULL;
    LumeMaterial *material_bulbo_a = NULL, *material_bulbo_b = NULL;
    AmostraMaterial amostras[ESTUDIO_AMOSTRAS] = {0};
    int linha, coluna, selecionada = -1, quadros = 0;
    float guinada = 0.0f, inclinacao = 0.08f, distancia_camera = 17.5f;
    LumeVec3 alvo_camera = {0, 3.35f, 0};
    bool animar_luzes = true, mostrar_debug = true;

    if (!ler_argumentos(argc, argv, &opcoes))
        return 2;
    configuracao.title = "Lume3D - Interactive PBR lighting studio";
    configuracao.width = opcoes.smoke ? 320 : opcoes.baixa_qualidade ? 800 : 1280;
    configuracao.height = opcoes.smoke ? 180 : opcoes.baixa_qualidade ? 450 : 720;
    configuracao.visible = !opcoes.smoke;
    configuracao.vsync = !opcoes.smoke;
    configuracao.clear_color = (LumeColor){0.012f, 0.015f, 0.024f, 1};
    renderer_config.hdr = true;
    renderer_config.tone_mapping = LUME_TONE_MAPPING_ACES;
    renderer_config.bloom = !opcoes.baixa_qualidade;
    renderer_config.fxaa = true;
    renderer_config.exposure = 1.08f;
    renderer_config.directional_shadow_size = opcoes.baixa_qualidade ? 512u : 2048u;
    renderer_config.spot_shadow_size = opcoes.baixa_qualidade ? 512u : 2048u;
    camera_config.field_of_view_radians = 0.94f;
    camera_config.near_plane = 0.05f;
    camera_config.far_plane = 120.0f;

    if (lume_app_create(&configuracao, &aplicativo) != LUME_SUCCESS ||
        lume_renderer_configure(lume_app_renderer(aplicativo), &renderer_config) != LUME_SUCCESS ||
        lume_scene_create(aplicativo, &cena) != LUME_SUCCESS ||
        lume_camera_create_perspective(cena, &camera_config, &camera) != LUME_SUCCESS ||
        lume_geometry_create_sphere(aplicativo, 0.62f, opcoes.baixa_qualidade ? 24u : 64u,
                                    opcoes.baixa_qualidade ? 14u : 36u, &esfera) != LUME_SUCCESS ||
        lume_geometry_create_box(aplicativo, 1, 1, 1, &caixa) != LUME_SUCCESS ||
        lume_geometry_create_plane(aplicativo, 24, 16, &plano) != LUME_SUCCESS)
        goto falha;

    /* Cada coluna aumenta metalicidade; cada linha aumenta rugosidade. */
    for (linha = 0; linha < ESTUDIO_LINHAS; ++linha)
    {
        for (coluna = 0; coluna < ESTUDIO_COLUNAS; ++coluna)
        {
            int indice = linha * ESTUDIO_COLUNAS + coluna;
            char nome[64];
            float metalico = (float)coluna / (ESTUDIO_COLUNAS - 1);
            float rugosidade = 0.08f + (float)linha / (ESTUDIO_LINHAS - 1) * 0.88f;
            float mistura = (float)linha / (ESTUDIO_LINHAS - 1);
            LumeColor cor = {0.12f + 0.55f * metalico, 0.28f + 0.18f * (1.0f - mistura),
                             0.78f - 0.42f * metalico + 0.12f * mistura, 1};
            if (criar_material(aplicativo, LUME_MATERIAL_PBR, cor, metalico, rugosidade,
                               &amostras[indice].material) != LUME_SUCCESS ||
                lume_mesh_create(cena, esfera, amostras[indice].material, &amostras[indice].malha) != LUME_SUCCESS)
                goto falha;
            amostras[indice].cor = cor;
            amostras[indice].metalico = metalico;
            amostras[indice].rugosidade = rugosidade;
            lume_node_set_position(amostras[indice].malha,
                                   (LumeVec3){(coluna - 3) * 1.62f, 0.55f + (ESTUDIO_LINHAS - 1 - linha) * 1.28f, 0});
            snprintf(nome, sizeof(nome), "PBR sample %d:%d", linha + 1, coluna + 1);
            lume_node_set_name(amostras[indice].malha, nome);
        }
    }
    if (criar_malha_simples(aplicativo, cena, plano, LUME_MATERIAL_PBR, (LumeColor){0.07f, 0.075f, 0.09f, 1}, 0.1f,
                            0.78f, &piso) != LUME_SUCCESS ||
        criar_malha_simples(aplicativo, cena, plano, LUME_MATERIAL_PBR, (LumeColor){0.028f, 0.034f, 0.052f, 1},
                            0.02f, 0.92f, &parede) != LUME_SUCCESS ||
        criar_malha_simples(aplicativo, cena, caixa, LUME_MATERIAL_PBR, (LumeColor){0.10f, 0.11f, 0.14f, 1}, 0.32f,
                            0.45f, &pedestal) != LUME_SUCCESS)
        goto falha;
    lume_node_set_euler_rotation(piso, (LumeVec3){-1.57079633f, 0, 0});
    lume_node_set_position(piso, (LumeVec3){0, -0.72f, 2.5f});
    lume_node_set_position(parede, (LumeVec3){0, 3.5f, -0.95f});
    lume_node_set_scale(parede, (LumeVec3){1, 0.62f, 1});
    lume_node_set_position(pedestal, (LumeVec3){0, -0.45f, 0});
    lume_node_set_scale(pedestal, (LumeVec3){12.2f, 0.5f, 2.5f});

    if (lume_node_create(cena, &pivo_luz_a) != LUME_SUCCESS || lume_node_create(cena, &pivo_luz_b) != LUME_SUCCESS)
        goto falha;
    {
        LumeAmbientLightConfig ambiente = lume_ambient_light_config_default();
        LumeDirectionalLightConfig principal = lume_directional_light_config_default();
        LumePointLightConfig azul = lume_point_light_config_default();
        LumePointLightConfig laranja = lume_point_light_config_default();
        LumeSpotLightConfig spot = lume_spot_light_config_default();
        ambiente.color = (LumeColor){0.18f, 0.21f, 0.32f, 1};
        ambiente.intensity = 0.22f;
        principal.color = (LumeColor){1.0f, 0.82f, 0.66f, 1};
        principal.intensity = 1.45f;
        principal.direction = (LumeVec3){-0.48f, -0.88f, -0.32f};
        principal.cast_shadows = true;
        azul.color = (LumeColor){0.18f, 0.48f, 1.0f, 1};
        azul.intensity = 4.6f;
        azul.range = 15.0f;
        laranja.color = (LumeColor){1.0f, 0.26f, 0.08f, 1};
        laranja.intensity = 4.2f;
        laranja.range = 15.0f;
        spot.color = (LumeColor){0.65f, 0.74f, 1.0f, 1};
        spot.intensity = 3.2f;
        spot.range = 20.0f;
        spot.inner_angle = 0.28f;
        spot.outer_angle = 0.52f;
        spot.direction = (LumeVec3){0, -1, -0.18f};
        spot.cast_shadows = true;
        if (lume_ambient_light_create(cena, &ambiente, &luz) != LUME_SUCCESS ||
            lume_directional_light_create(cena, &principal, &luz) != LUME_SUCCESS ||
            lume_point_light_create(cena, &azul, &luz_a) != LUME_SUCCESS ||
            lume_point_light_create(cena, &laranja, &luz_b) != LUME_SUCCESS ||
            lume_spot_light_create(cena, &spot, &luz) != LUME_SUCCESS ||
            lume_node_add_child(pivo_luz_a, luz_a) != LUME_SUCCESS || lume_node_add_child(pivo_luz_b, luz_b) != LUME_SUCCESS)
            goto falha;
        lume_node_set_position(luz_a, (LumeVec3){-7.0f, 4.5f, 3.0f});
        lume_node_set_position(luz_b, (LumeVec3){7.0f, 2.5f, 2.0f});
        lume_node_set_position(luz, (LumeVec3){0, 9.0f, 5.0f});
    }
    if (criar_material(aplicativo, LUME_MATERIAL_UNLIT, (LumeColor){0.2f, 1.0f, 5.0f, 1}, 0, 1,
                       &material_bulbo_a) != LUME_SUCCESS ||
        criar_material(aplicativo, LUME_MATERIAL_UNLIT, (LumeColor){5.0f, 0.45f, 0.08f, 1}, 0, 1,
                       &material_bulbo_b) != LUME_SUCCESS ||
        lume_mesh_create(cena, esfera, material_bulbo_a, &bulbo_a) != LUME_SUCCESS ||
        lume_mesh_create(cena, esfera, material_bulbo_b, &bulbo_b) != LUME_SUCCESS ||
        lume_node_add_child(luz_a, bulbo_a) != LUME_SUCCESS || lume_node_add_child(luz_b, bulbo_b) != LUME_SUCCESS)
        goto falha;
    lume_node_set_scale(bulbo_a, (LumeVec3){0.28f, 0.28f, 0.28f});
    lume_node_set_scale(bulbo_b, (LumeVec3){0.28f, 0.28f, 0.28f});

    lume_geometry_release(esfera);
    esfera = NULL;
    lume_geometry_release(caixa);
    caixa = NULL;
    lume_geometry_release(plano);
    plano = NULL;
    lume_material_release(material_bulbo_a);
    material_bulbo_a = NULL;
    lume_material_release(material_bulbo_b);
    material_bulbo_b = NULL;

    if (!opcoes.smoke)
        puts("Controls: left-drag orbit, middle-drag pan, wheel zoom, right-click select, B bloom, F FXAA, L animate lights, D debug overlay, R reset, Esc exit.");
    while (!lume_app_should_close(aplicativo))
    {
        float delta = lume_app_begin_frame(aplicativo);
        float mouse_x, mouse_y, rolagem_x, rolagem_y;
        lume_mouse_get_delta(aplicativo, &mouse_x, &mouse_y);
        lume_mouse_get_scroll(aplicativo, &rolagem_x, &rolagem_y);
        (void)rolagem_x;
        if (lume_key_was_pressed(aplicativo, LUME_KEY_ESCAPE))
            lume_app_request_close(aplicativo);
        if (lume_key_was_pressed(aplicativo, LUME_KEY_B))
        {
            renderer_config.bloom = !renderer_config.bloom;
            if (lume_renderer_configure(lume_app_renderer(aplicativo), &renderer_config) != LUME_SUCCESS)
                goto falha;
        }
        if (lume_key_was_pressed(aplicativo, LUME_KEY_F))
        {
            renderer_config.fxaa = !renderer_config.fxaa;
            if (lume_renderer_configure(lume_app_renderer(aplicativo), &renderer_config) != LUME_SUCCESS)
                goto falha;
        }
        if (lume_key_was_pressed(aplicativo, LUME_KEY_L))
            animar_luzes = !animar_luzes;
        if (lume_key_was_pressed(aplicativo, LUME_KEY_D))
            mostrar_debug = !mostrar_debug;
        if (lume_key_was_pressed(aplicativo, LUME_KEY_R))
        {
            guinada = 0.0f;
            inclinacao = 0.08f;
            distancia_camera = 17.5f;
            alvo_camera = (LumeVec3){0, 3.35f, 0};
        }
        if (lume_mouse_button_is_down(aplicativo, LUME_MOUSE_BUTTON_LEFT))
        {
            guinada -= mouse_x * 0.006f;
            inclinacao = limitar(inclinacao - mouse_y * 0.006f, -0.85f, 1.15f);
        }
        if (lume_mouse_button_is_down(aplicativo, LUME_MOUSE_BUTTON_MIDDLE))
        {
            alvo_camera.x -= mouse_x * distancia_camera * 0.0011f;
            alvo_camera.y += mouse_y * distancia_camera * 0.0011f;
        }
        distancia_camera = limitar(distancia_camera - rolagem_y * 1.1f, 7.0f, 30.0f);
        lume_node_set_position(camera,
                               (LumeVec3){alvo_camera.x + sinf(guinada) * cosf(inclinacao) * distancia_camera,
                                          alvo_camera.y + sinf(inclinacao) * distancia_camera,
                                          alvo_camera.z + cosf(guinada) * cosf(inclinacao) * distancia_camera});
        if (lume_node_look_at(camera, alvo_camera) != LUME_SUCCESS)
            goto falha;
        if (animar_luzes)
        {
            lume_node_rotate_y(pivo_luz_a, delta * 0.42f);
            lume_node_rotate_y(pivo_luz_b, -delta * 0.31f);
        }
        if (lume_mouse_button_was_pressed(aplicativo, LUME_MOUSE_BUTTON_RIGHT))
        {
            int largura, altura, indice;
            float cursor_x, cursor_y;
            LumeRaycastHit hits[16];
            size_t quantidade, hit;
            lume_app_get_framebuffer_size(aplicativo, &largura, &altura);
            lume_mouse_get_position(aplicativo, &cursor_x, &cursor_y);
            quantidade = lume_scene_raycast(
                cena, criar_raio_camera(camera, alvo_camera, cursor_x, cursor_y, largura, altura,
                                        camera_config.field_of_view_radians),
                hits, sizeof(hits) / sizeof(hits[0]));
            indice = -1;
            for (hit = 0; hit < quantidade && hit < sizeof(hits) / sizeof(hits[0]); ++hit)
            {
                indice = encontrar_amostra(amostras, hits[hit].node);
                if (indice >= 0)
                    break;
            }
            if (indice >= 0)
            {
                if (selecionada >= 0)
                    lume_material_set_base_color(amostras[selecionada].material, amostras[selecionada].cor);
                selecionada = indice;
                lume_material_set_base_color(amostras[selecionada].material, (LumeColor){1.0f, 0.72f, 0.12f, 1});
                printf("Selected PBR sample: metallic %.2f, roughness %.2f.\n", amostras[selecionada].metalico,
                       amostras[selecionada].rugosidade);
            }
        }
        if (mostrar_debug && selecionada >= 0)
        {
            lume_debug_aabb(lume_app_renderer(aplicativo), lume_node_world_bounds(amostras[selecionada].malha),
                            (LumeColor){1, 0.78f, 0.08f, 1});
            lume_debug_axes(lume_app_renderer(aplicativo), lume_node_world_matrix(amostras[selecionada].malha), 0.9f);
        }
        if (lume_app_render(aplicativo, cena, camera) != LUME_SUCCESS)
            goto falha;
        lume_app_end_frame(aplicativo);
        if (opcoes.smoke && ++quadros >= 2)
            lume_app_request_close(aplicativo);
    }
    if (!opcoes.smoke)
        puts("Lighting studio finished.");
    for (linha = 0; linha < ESTUDIO_AMOSTRAS; ++linha)
        lume_material_release(amostras[linha].material);
    lume_app_destroy(aplicativo);
    return 0;

falha:
    fprintf(stderr, "Lighting studio example failed: %s\n", lume_error_last()->message);
    for (linha = 0; linha < ESTUDIO_AMOSTRAS; ++linha)
        lume_material_release(amostras[linha].material);
    lume_geometry_release(esfera);
    lume_geometry_release(caixa);
    lume_geometry_release(plano);
    lume_material_release(material_bulbo_a);
    lume_material_release(material_bulbo_b);
    lume_app_destroy(aplicativo);
    return 1;
}
