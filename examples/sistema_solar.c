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
    LumeNode *sun;
    LumeNode *earth_orbit;
    LumeNode *earth;
    LumeNode *moon_orbit;
    LumeNode *moon;
    LumeGeometry *sphere;
    LumeBasicMaterialConfig sun_config = lume_basic_material_config_default();
    LumeBasicMaterialConfig earth_config = lume_basic_material_config_default();
    LumeBasicMaterialConfig moon_config = lume_basic_material_config_default();

    app_config.title = "Lume3D - Scene hierarchy";
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
    sphere = lume_geometry_create_sphere(app, 1.0f, 32, 20);
    sun_config.color = (LumeColor){1.0f, 0.65f, 0.08f, 1.0f};
    earth_config.color = (LumeColor){0.08f, 0.35f, 1.0f, 1.0f};
    moon_config.color = (LumeColor){0.7f, 0.72f, 0.76f, 1.0f};
    sun = lume_mesh_create(scene, sphere, lume_material_create_basic(app, &sun_config));
    earth_orbit = lume_node_create(scene);
    earth = lume_mesh_create(scene, sphere, lume_material_create_basic(app, &earth_config));
    moon_orbit = lume_node_create(scene);
    moon = lume_mesh_create(scene, sphere, lume_material_create_basic(app, &moon_config));
    if (!scene || !camera || !sphere || !sun || !earth_orbit || !earth || !moon_orbit || !moon)
    {
        fprintf(stderr, "Could not create the scene: %s\n", lume_get_last_error());
        lume_app_destroy(app);
        return 1;
    }

    lume_node_set_position(camera, (LumeVec3){0.0f, 4.5f, 11.0f});
    lume_node_look_at(camera, (LumeVec3){0.0f, 0.0f, 0.0f});
    lume_node_set_scale(sun, (LumeVec3){1.4f, 1.4f, 1.4f});
    lume_node_set_position(earth, (LumeVec3){4.0f, 0.0f, 0.0f});
    lume_node_set_scale(earth, (LumeVec3){0.55f, 0.55f, 0.55f});
    lume_node_set_position(moon_orbit, (LumeVec3){4.0f, 0.0f, 0.0f});
    lume_node_set_position(moon, (LumeVec3){1.1f, 0.0f, 0.0f});
    lume_node_set_scale(moon, (LumeVec3){0.18f, 0.18f, 0.18f});
    lume_node_add_child(earth_orbit, earth);
    lume_node_add_child(earth_orbit, moon_orbit);
    lume_node_add_child(moon_orbit, moon);

    while (!lume_app_should_close(app))
    {
        float delta_time = lume_app_begin_frame(app);
        if (lume_key_was_pressed(app, LUME_KEY_ESCAPE))
            lume_app_request_close(app);
        lume_node_rotate_y(sun, delta_time * 0.25f);
        lume_node_rotate_y(earth_orbit, delta_time * 0.35f);
        lume_node_rotate_y(earth, delta_time * 1.5f);
        lume_node_rotate_y(moon_orbit, delta_time * 1.6f);
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
