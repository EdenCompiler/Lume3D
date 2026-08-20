#include "lume_interno.h"

#include <stdio.h>
#include <string.h>

#ifndef LUME_TEST_FIXTURES
#define LUME_TEST_FIXTURES "tests/fixtures"
#endif

int main(void)
{
    static const char *vertice_passagem =
        "#version 330 core\nout vec2 vUv;void main(){vec2 p=vec2((gl_VertexID<<1)&2,gl_VertexID&2);"
        "vUv=p*.5;gl_Position=vec4(p*2-1,0,1);}";
    static const char *fragmento_passagem =
        "#version 330 core\nin vec2 vUv;out vec4 FragColor;uniform sampler2D uColor;"
        "void main(){FragColor=texture(uColor,vUv);}";
    LumeAppConfig configuracao = lume_app_config_default();
    LumeMaterialConfig material_config = lume_material_config_default(LUME_MATERIAL_PBR);
    LumeRenderTargetConfig configuracao_alvo = lume_render_target_config_default();
    LumeApp *aplicativo = NULL;
    LumeScene *cena = NULL;
    LumeNode *camera = NULL, *malha = NULL, *luz = NULL;
    LumeGeometry *caixa = NULL;
    LumeMaterial *material = NULL;
    LumeModel *gltf = NULL, *obj = NULL, *nao_suportado = NULL;
    LumeModelInstance *instancia = NULL;
    LumeAnimationPlayer *player = NULL;
    LumeShader *shader_passagem = NULL;
    LumePipeline *pipeline_passagem = NULL;
    LumeRenderTarget *alvo = NULL;
    unsigned char pixel[4] = {0};
    char caminho[1024];

    configuracao.width = 128;
    configuracao.height = 128;
    configuracao.visible = false;
    configuracao.vsync = false;
    configuracao_alvo.width = 32;
    configuracao_alvo.height = 32;
    configuracao_alvo.hdr = false;
    if (lume_app_create(&configuracao, &aplicativo) != LUME_SUCCESS ||
        lume_scene_create(aplicativo, &cena) != LUME_SUCCESS ||
        lume_camera_create_perspective(cena, NULL, &camera) != LUME_SUCCESS ||
        lume_geometry_create_box(aplicativo, 1, 1, 1, &caixa) != LUME_SUCCESS ||
        lume_material_create(aplicativo, &material_config, &material) != LUME_SUCCESS ||
        lume_mesh_create(cena, caixa, material, &malha) != LUME_SUCCESS ||
        lume_directional_light_create(cena, NULL, &luz) != LUME_SUCCESS ||
        lume_render_target_create(aplicativo, &configuracao_alvo, &alvo) != LUME_SUCCESS)
        goto falha;
    lume_geometry_release(caixa);
    lume_material_release(material);
    lume_node_set_position(camera, (LumeVec3){0, 0, 3});

    snprintf(caminho, sizeof(caminho), "%s/triangle.gltf", LUME_TEST_FIXTURES);
    if (lume_model_load(aplicativo, caminho, NULL, &gltf) != LUME_SUCCESS || lume_model_animation_count(gltf) != 1 ||
        strcmp(lume_animation_clip_name(lume_model_animation(gltf, 0)), "Move") != 0 ||
        lume_model_instantiate(gltf, cena, &instancia) != LUME_SUCCESS ||
        lume_animation_player_create(instancia, &player) != LUME_SUCCESS ||
        lume_animation_player_play(player, lume_model_animation(gltf, 0), LUME_LOOP_REPEAT) != LUME_SUCCESS)
        goto falha;
    lume_animation_player_update(player, 0.5f);

    snprintf(caminho, sizeof(caminho), "%s/triangle.obj", LUME_TEST_FIXTURES);
    if (lume_model_load(aplicativo, caminho, NULL, &obj) != LUME_SUCCESS)
        goto falha;
    snprintf(caminho, sizeof(caminho), "%s/unsupported.gltf", LUME_TEST_FIXTURES);
    if (lume_model_load(aplicativo, caminho, NULL, &nao_suportado) != LUME_ERROR_UNSUPPORTED ||
        strstr(lume_error_last()->message, "not supported") == NULL)
    {
        fprintf(stderr, "FAIL: Unsupported required glTF extension was not rejected in English.\n");
        goto falha;
    }

    lume_debug_aabb(lume_app_renderer(aplicativo), lume_node_world_bounds(malha), (LumeColor){1, 1, 0, 1});
    {
        LumeShaderConfig shader_config = {vertice_passagem, fragmento_passagem, NULL, NULL};
        LumePipelineConfig pipeline_config = lume_pipeline_config_default();
        LumePassConfig passagem_hdr = {"Test HDR pass", NULL, LUME_PASS_HDR, false, true};
        LumePassConfig passagem_ldr = {"Test LDR pass", NULL, LUME_PASS_LDR, false, true};
        pipeline_config.depth_test = false;
        pipeline_config.depth_write = false;
        pipeline_config.cull_back_faces = false;
        if (lume_shader_create(aplicativo, &shader_config, &shader_passagem) != LUME_SUCCESS)
            goto falha;
        pipeline_config.shader = shader_passagem;
        if (lume_pipeline_create(aplicativo, &pipeline_config, &pipeline_passagem) != LUME_SUCCESS)
            goto falha;
        passagem_hdr.pipeline = pipeline_passagem;
        passagem_ldr.pipeline = pipeline_passagem;
        if (lume_renderer_add_pass(lume_app_renderer(aplicativo), &passagem_hdr, NULL) != LUME_SUCCESS ||
            lume_renderer_add_pass(lume_app_renderer(aplicativo), &passagem_ldr, NULL) != LUME_SUCCESS)
            goto falha;
        lume_shader_release(shader_passagem);
        lume_pipeline_release(pipeline_passagem);
    }
    if (lume_renderer_render(lume_app_renderer(aplicativo), cena, camera, alvo) != LUME_SUCCESS ||
        lume_renderer_present_target(lume_app_renderer(aplicativo), alvo) != LUME_SUCCESS ||
        lume_app_render(aplicativo, cena, camera) != LUME_SUCCESS)
        goto falha;
    glFinish();
    glReadPixels(64, 64, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
    if (pixel[0] == 0 && pixel[1] == 0 && pixel[2] == 0)
    {
        fprintf(stderr, "FAIL: Rendered center pixel is empty.\n");
        goto falha;
    }
verificar_fim:
    lume_animation_player_destroy(player);
    lume_model_instance_destroy(instancia);
    lume_model_release(gltf);
    lume_model_release(obj);
    lume_render_target_release(alvo);
    lume_app_destroy(aplicativo);
    if (!aplicativo)
        return 1;
    puts("OpenGL and asset smoke test passed.");
    return 0;

falha:
    fprintf(stderr, "FAIL: %s\n", lume_error_last()->message);
    lume_animation_player_destroy(player);
    lume_model_instance_destroy(instancia);
    lume_model_release(gltf);
    lume_model_release(obj);
    lume_render_target_release(alvo);
    lume_app_destroy(aplicativo);
    return 1;
}
