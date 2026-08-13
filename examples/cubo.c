#include <lume/lume.h>

#include <stdio.h>
#include <string.h>

int main(int argc, char **argv)
{
    LumeAppConfig app_config = lume_app_config_default();
    bool smoke = argc > 1 && strcmp(argv[1], "--smoke") == 0;
    int frames = 0;
    LumeApp *app;
    LumeScene *scene;
    LumeNode *camera;
    LumeGeometry *geometry;
    LumeMaterial *material;
    LumeNode *cube;

    app_config.title = "Lume3D - Spinning cube";
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
    geometry = lume_geometry_create_box(app, 1.5f, 1.5f, 1.5f);
    material = lume_material_create_basic(app, NULL);
    cube = lume_mesh_create(scene, geometry, material);
    if (!scene || !camera || !geometry || !material || !cube)
    {
        fprintf(stderr, "Could not create the scene: %s\n", lume_get_last_error());
        lume_app_destroy(app);
        return 1;
    }
    lume_node_set_position(camera, (LumeVec3){0.0f, 0.0f, 4.0f});

    while (!lume_app_should_close(app))
    {
        float delta_time = lume_app_begin_frame(app);
        if (lume_key_was_pressed(app, LUME_KEY_ESCAPE))
        {
            lume_app_request_close(app);
        }
        lume_node_rotate_x(cube, delta_time * 0.7f);
        lume_node_rotate_y(cube, delta_time);
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
