#include <lume/lume.h>

#include <math.h>
#include <stdio.h>
#include <string.h>

#define OCEANO_COLUNAS 48
#define OCEANO_LINHAS 32
#define OCEANO_INSTANCIAS (OCEANO_COLUNAS * OCEANO_LINHAS)

int main(int argc, char **argv)
{
    bool smoke = argc > 1 && strcmp(argv[1], "--smoke") == 0;
    LumeAppConfig configuracao = lume_app_config_default();
    LumeMaterialConfig material_config = lume_material_config_default(LUME_MATERIAL_PBR);
    LumeDirectionalLightConfig sol_config = lume_directional_light_config_default();
    LumeApp *aplicativo = NULL;
    LumeScene *cena = NULL;
    LumeNode *camera = NULL, *oceano = NULL, *sol = NULL;
    LumeGeometry *bloco = NULL;
    LumeMaterial *material = NULL;
    float tempo = 0.0f;
    int quadros = 0;

    configuracao.title = "Lume3D - Procedural ocean waves";
    configuracao.width = 1280;
    configuracao.height = 720;
    configuracao.visible = !smoke;
    configuracao.vsync = !smoke;
    configuracao.clear_color = (LumeColor){0.01f, 0.04f, 0.09f, 1.0f};
    material_config.base_color = (LumeColor){0.015f, 0.28f, 0.62f, 0.92f};
    material_config.metallic = 0.05f;
    material_config.roughness = 0.18f;
    material_config.alpha_mode = LUME_ALPHA_BLEND;
    sol_config.direction = (LumeVec3){-0.35f, -1.0f, -0.2f};
    sol_config.intensity = 2.2f;

    if (lume_app_create(&configuracao, &aplicativo) != LUME_SUCCESS ||
        lume_scene_create(aplicativo, &cena) != LUME_SUCCESS ||
        lume_camera_create_perspective(cena, NULL, &camera) != LUME_SUCCESS ||
        lume_geometry_create_box(aplicativo, 0.32f, 0.16f, 0.32f, &bloco) != LUME_SUCCESS ||
        lume_material_create(aplicativo, &material_config, &material) != LUME_SUCCESS ||
        lume_instanced_mesh_create(cena, bloco, material, OCEANO_INSTANCIAS, &oceano) != LUME_SUCCESS ||
        lume_directional_light_create(cena, &sol_config, &sol) != LUME_SUCCESS)
    {
        fprintf(stderr, "Could not create the ocean example: %s\n", lume_error_last()->message);
        lume_app_destroy(aplicativo);
        return 1;
    }
    lume_geometry_release(bloco);
    lume_material_release(material);
    lume_node_set_position(camera, (LumeVec3){0.0f, 6.5f, 11.0f});
    lume_node_look_at(camera, (LumeVec3){0.0f, 0.0f, 0.0f});

    while (!lume_app_should_close(aplicativo))
    {
        float delta = lume_app_begin_frame(aplicativo);
        uint32_t linha, coluna;
        tempo += delta;
        if (lume_key_was_pressed(aplicativo, LUME_KEY_ESCAPE))
            lume_app_request_close(aplicativo);

        for (linha = 0; linha < OCEANO_LINHAS; ++linha)
        {
            for (coluna = 0; coluna < OCEANO_COLUNAS; ++coluna)
            {
                float x = ((float)coluna - OCEANO_COLUNAS * 0.5f) * 0.34f;
                float z = ((float)linha - OCEANO_LINHAS * 0.5f) * 0.34f;
                float onda_longa = sinf(x * 0.62f + tempo * 1.7f) * 0.34f;
                float onda_cruzada = cosf(z * 0.9f - tempo * 1.15f) * 0.18f;
                float ondulacao = sinf((x + z) * 1.8f + tempo * 2.4f) * 0.06f;
                LumeMat4 transformacao = lume_mat4_transform((LumeVec3){x, onda_longa + onda_cruzada + ondulacao, z},
                                                             lume_quat_identity(), (LumeVec3){1.0f, 1.0f, 1.0f});
                lume_instanced_mesh_set_transform(oceano, linha * OCEANO_COLUNAS + coluna, transformacao);
            }
        }
        lume_debug_axes(lume_app_renderer(aplicativo), lume_mat4_identity(), 1.0f);
        if (lume_app_render(aplicativo, cena, camera) != LUME_SUCCESS)
        {
            fprintf(stderr, "Ocean rendering failed: %s\n", lume_error_last()->message);
            break;
        }
        lume_app_end_frame(aplicativo);
        if (smoke && ++quadros >= 2)
            lume_app_request_close(aplicativo);
    }

    if (!smoke)
    {
        LumeFrameStats estatisticas = lume_renderer_frame_stats(lume_app_renderer(aplicativo));
        printf("Ocean simulation finished with %llu instances and %llu draw calls in the last frame.\n",
               (unsigned long long)estatisticas.instances, (unsigned long long)estatisticas.draw_calls);
    }
    lume_app_destroy(aplicativo);
    return 0;
}
