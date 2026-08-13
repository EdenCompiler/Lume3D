#include <lume/lume.h>

#include <stdio.h>
#include <string.h>

int main(int argc, char **argv)
{
    static const uint8_t checker[] = {245, 245, 245, 255, 40, 75, 150, 255, 40, 75, 150, 255, 245, 245, 245, 255};
    LumeAppConfig app_config = lume_app_config_default();
    bool smoke = argc > 1 && strcmp(argv[1], "--smoke") == 0;
    int frames = 0;
    LumeApp *app;
    LumeScene *scene;
    LumeNode *camera;
    LumeNode *cube;
    LumeNode *point_light;
    LumeTexture *texture;
    LumeLambertMaterialConfig material_config = lume_lambert_material_config_default();
    LumeDirectionalLightConfig directional_config = lume_directional_light_config_default();
    LumePointLightConfig point_config = lume_point_light_config_default();

    app_config.title = "Lume3D - Lighting and texture";
    app_config.clear_color = (LumeColor){0.015f, 0.02f, 0.04f, 1.0f};
    app_config.visible = !smoke;
    app_config.vsync = !smoke;
    app = lume_app_create(&app_config);
    if (!app)
    {
        fprintf(stderr, "Could not start Lume3D: %s\n", lume_get_last_error());
        return 1;
    }
    scene = lume_scene_create(app);
    camera = lume_camera_create_perspective(scene, NULL);
    texture = lume_texture_create(app, checker, 2, 2, NULL);
    material_config.texture = texture;
    material_config.color = (LumeColor){0.9f, 0.95f, 1.0f, 1.0f};
    cube = lume_mesh_create(scene, lume_geometry_create_box(app, 2.0f, 2.0f, 2.0f),
                            lume_material_create_lambert(app, &material_config));
    directional_config.intensity = 0.7f;
    lume_ambient_light_create(scene, NULL);
    lume_directional_light_create(scene, &directional_config);
    point_config.color = (LumeColor){1.0f, 0.25f, 0.08f, 1.0f};
    point_config.intensity = 2.0f;
    point_config.range = 8.0f;
    point_light = lume_point_light_create(scene, &point_config);
    if (!scene || !camera || !texture || !cube || !point_light)
    {
        fprintf(stderr, "Could not create the scene: %s\n", lume_get_last_error());
        lume_app_destroy(app);
        return 1;
    }
    lume_node_set_position(camera, (LumeVec3){0.0f, 1.0f, 5.5f});
    lume_node_look_at(camera, (LumeVec3){0.0f, 0.0f, 0.0f});
    lume_node_set_position(point_light, (LumeVec3){2.5f, 1.5f, 2.0f});

    while (!lume_app_should_close(app))
    {
        float delta_time = lume_app_begin_frame(app);
        if (lume_key_was_pressed(app, LUME_KEY_ESCAPE))
            lume_app_request_close(app);
        lume_node_rotate_x(cube, delta_time * 0.35f);
        lume_node_rotate_y(cube, delta_time * 0.7f);
        if (!lume_render(app, scene, camera))
        {
            fprintf(stderr, "Rendering failed: %s\n", lume_get_last_error());
            break;
        }
        lume_app_end_frame(app);
        if (smoke && ++frames >= 2)
            lume_app_request_close(app);
    }
    lume_app_destroy(app);
    return 0;
}
