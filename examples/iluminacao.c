#include <lume/lume.h>

#include <stdio.h>
#include <string.h>

int main(int argc, char **argv)
{
    static const uint8_t quadriculado[] = {245, 245, 245, 255, 40, 75, 150, 255, 40, 75, 150, 255, 245, 245, 245, 255};
    bool smoke = argc > 1 && strcmp(argv[1], "--smoke") == 0;
    LumeAppConfig configuracao = lume_app_config_default();
    LumeMaterialConfig material_config = lume_material_config_default(LUME_MATERIAL_PHONG);
    LumePointLightConfig ponto_config = lume_point_light_config_default();
    LumeTextureConfig textura_config = lume_texture_config_default();
    LumeApp *aplicativo = NULL;
    LumeScene *cena = NULL;
    LumeNode *camera = NULL, *cubo = NULL, *luz = NULL;
    LumeTexture *textura = NULL;
    LumeGeometry *geometria = NULL;
    LumeMaterial *material = NULL;
    int quadros = 0;

    configuracao.title = "Lume3D - Lighting and texture";
    configuracao.clear_color = (LumeColor){0.015f, 0.02f, 0.04f, 1.0f};
    configuracao.visible = !smoke;
    configuracao.vsync = !smoke;
    textura_config.srgb = true;
    ponto_config.color = (LumeColor){1.0f, 0.25f, 0.08f, 1.0f};
    ponto_config.intensity = 2.0f;
    ponto_config.range = 8.0f;

    if (lume_app_create(&configuracao, &aplicativo) != LUME_SUCCESS ||
        lume_scene_create(aplicativo, &cena) != LUME_SUCCESS ||
        lume_camera_create_perspective(cena, NULL, &camera) != LUME_SUCCESS ||
        lume_texture_create_rgba8(aplicativo, quadriculado, 2, 2, &textura_config, &textura) != LUME_SUCCESS ||
        lume_geometry_create_box(aplicativo, 2.0f, 2.0f, 2.0f, &geometria) != LUME_SUCCESS)
        goto falha;
    material_config.base_color_texture = textura;
    if (lume_material_create(aplicativo, &material_config, &material) != LUME_SUCCESS ||
        lume_mesh_create(cena, geometria, material, &cubo) != LUME_SUCCESS ||
        lume_ambient_light_create(cena, NULL, &luz) != LUME_SUCCESS ||
        lume_directional_light_create(cena, NULL, &luz) != LUME_SUCCESS ||
        lume_point_light_create(cena, &ponto_config, &luz) != LUME_SUCCESS)
        goto falha;
    lume_texture_release(textura);
    lume_geometry_release(geometria);
    lume_material_release(material);
    lume_node_set_position(camera, (LumeVec3){0.0f, 1.0f, 5.5f});
    lume_node_look_at(camera, (LumeVec3){0.0f, 0.0f, 0.0f});
    lume_node_set_position(luz, (LumeVec3){2.5f, 1.5f, 2.0f});

    while (!lume_app_should_close(aplicativo))
    {
        float delta = lume_app_begin_frame(aplicativo);
        lume_node_rotate_x(cubo, delta * 0.35f);
        lume_node_rotate_y(cubo, delta * 0.7f);
        if (lume_app_render(aplicativo, cena, camera) != LUME_SUCCESS)
            goto falha;
        lume_app_end_frame(aplicativo);
        if (smoke && ++quadros >= 2)
            lume_app_request_close(aplicativo);
    }
    lume_app_destroy(aplicativo);
    return 0;

falha:
    fprintf(stderr, "Example failed: %s\n", lume_error_last()->message);
    lume_app_destroy(aplicativo);
    return 1;
}
