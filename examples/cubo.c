#include <lume/lume.h>

#include <stdio.h>
#include <string.h>

static void mostrar_erro(const char *contexto)
{
    const LumeError *erro = lume_error_last();
    fprintf(stderr, "%s: %s (%s)\n", contexto, erro->message, lume_result_string(erro->code));
}

int main(int argc, char **argv)
{
    bool smoke = argc > 1 && strcmp(argv[1], "--smoke") == 0;
    LumeAppConfig configuracao = lume_app_config_default();
    LumeMaterialConfig material_config = lume_material_config_default(LUME_MATERIAL_PBR);
    LumeApp *aplicativo = NULL;
    LumeScene *cena = NULL;
    LumeNode *camera = NULL, *cubo = NULL, *luz = NULL;
    LumeGeometry *geometria = NULL;
    LumeMaterial *material = NULL;
    int quadros = 0;

    configuracao.title = "Lume3D - Spinning PBR cube";
    configuracao.visible = !smoke;
    configuracao.vsync = !smoke;
    material_config.base_color = (LumeColor){0.12f, 0.48f, 1.0f, 1.0f};
    material_config.metallic = 0.25f;
    material_config.roughness = 0.3f;

    if (lume_app_create(&configuracao, &aplicativo) != LUME_SUCCESS ||
        lume_scene_create(aplicativo, &cena) != LUME_SUCCESS ||
        lume_camera_create_perspective(cena, NULL, &camera) != LUME_SUCCESS ||
        lume_geometry_create_box(aplicativo, 1.5f, 1.5f, 1.5f, &geometria) != LUME_SUCCESS ||
        lume_material_create(aplicativo, &material_config, &material) != LUME_SUCCESS ||
        lume_mesh_create(cena, geometria, material, &cubo) != LUME_SUCCESS ||
        lume_directional_light_create(cena, NULL, &luz) != LUME_SUCCESS)
    {
        mostrar_erro("Could not create the example");
        lume_app_destroy(aplicativo);
        return 1;
    }
    lume_geometry_release(geometria);
    lume_material_release(material);
    lume_node_set_position(camera, (LumeVec3){0.0f, 0.0f, 4.0f});

    while (!lume_app_should_close(aplicativo))
    {
        float delta = lume_app_begin_frame(aplicativo);
        if (lume_key_was_pressed(aplicativo, LUME_KEY_ESCAPE))
            lume_app_request_close(aplicativo);
        lume_node_rotate_x(cubo, delta * 0.7f);
        lume_node_rotate_y(cubo, delta);
        lume_debug_axes(lume_app_renderer(aplicativo), lume_mat4_identity(), 1.5f);
        if (lume_app_render(aplicativo, cena, camera) != LUME_SUCCESS)
        {
            mostrar_erro("Rendering failed");
            break;
        }
        lume_app_end_frame(aplicativo);
        if (smoke && ++quadros >= 2)
            lume_app_request_close(aplicativo);
    }
    lume_app_destroy(aplicativo);
    return 0;
}
