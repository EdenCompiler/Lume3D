#include <lume/lume.h>

#include <stdio.h>
#include <string.h>

int main(int argc, char **argv)
{
    bool smoke = argc > 1 && strcmp(argv[1], "--smoke") == 0;
    LumeAppConfig configuracao = lume_app_config_default();
    LumeApp *aplicativo = NULL;
    LumeScene *cena = NULL;
    LumeNode *camera = NULL, *sol = NULL, *orbita_terra = NULL, *terra = NULL, *orbita_lua = NULL, *lua = NULL;
    LumeGeometry *esfera = NULL;
    int quadros = 0;

    configuracao.title = "Lume3D - Scene hierarchy";
    configuracao.visible = !smoke;
    configuracao.vsync = !smoke;
    if (lume_app_create(&configuracao, &aplicativo) != LUME_SUCCESS ||
        lume_scene_create(aplicativo, &cena) != LUME_SUCCESS ||
        lume_camera_create_perspective(cena, NULL, &camera) != LUME_SUCCESS ||
        lume_geometry_create_sphere(aplicativo, 1.0f, 32, 20, &esfera) != LUME_SUCCESS ||
        lume_node_create(cena, &orbita_terra) != LUME_SUCCESS || lume_node_create(cena, &orbita_lua) != LUME_SUCCESS)
        goto falha;

    /* Materiais são criados localmente e as malhas retêm suas referências. */
    {
        LumeMaterialConfig material_config = lume_material_config_default(LUME_MATERIAL_UNLIT);
        LumeMaterial *material = NULL;
        material_config.base_color = (LumeColor){1.0f, 0.65f, 0.08f, 1.0f};
        if (lume_material_create(aplicativo, &material_config, &material) != LUME_SUCCESS ||
            lume_mesh_create(cena, esfera, material, &sol) != LUME_SUCCESS)
            goto falha;
        lume_material_release(material);
        material_config.base_color = (LumeColor){0.08f, 0.35f, 1.0f, 1.0f};
        if (lume_material_create(aplicativo, &material_config, &material) != LUME_SUCCESS ||
            lume_mesh_create(cena, esfera, material, &terra) != LUME_SUCCESS)
            goto falha;
        lume_material_release(material);
        material_config.base_color = (LumeColor){0.7f, 0.72f, 0.76f, 1.0f};
        if (lume_material_create(aplicativo, &material_config, &material) != LUME_SUCCESS ||
            lume_mesh_create(cena, esfera, material, &lua) != LUME_SUCCESS)
            goto falha;
        lume_material_release(material);
    }
    lume_geometry_release(esfera);
    lume_node_set_position(camera, (LumeVec3){0.0f, 4.5f, 11.0f});
    lume_node_look_at(camera, (LumeVec3){0.0f, 0.0f, 0.0f});
    lume_node_set_scale(sol, (LumeVec3){1.4f, 1.4f, 1.4f});
    lume_node_set_position(terra, (LumeVec3){4.0f, 0.0f, 0.0f});
    lume_node_set_scale(terra, (LumeVec3){0.55f, 0.55f, 0.55f});
    lume_node_set_position(orbita_lua, (LumeVec3){4.0f, 0.0f, 0.0f});
    lume_node_set_position(lua, (LumeVec3){1.1f, 0.0f, 0.0f});
    lume_node_set_scale(lua, (LumeVec3){0.18f, 0.18f, 0.18f});
    lume_node_add_child(orbita_terra, terra);
    lume_node_add_child(orbita_terra, orbita_lua);
    lume_node_add_child(orbita_lua, lua);

    while (!lume_app_should_close(aplicativo))
    {
        float delta = lume_app_begin_frame(aplicativo);
        lume_node_rotate_y(sol, delta * 0.25f);
        lume_node_rotate_y(orbita_terra, delta * 0.35f);
        lume_node_rotate_y(terra, delta * 1.5f);
        lume_node_rotate_y(orbita_lua, delta * 1.6f);
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
