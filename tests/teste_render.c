#include "lume_interno.h"

#include <stdio.h>
#include <string.h>

static float orientacao_face_esfera(const LumeGeometry *geometria, size_t inicio)
{
    const LumeVertice *a = &geometria->vertices[geometria->indices[inicio]];
    const LumeVertice *b = &geometria->vertices[geometria->indices[inicio + 1]];
    const LumeVertice *c = &geometria->vertices[geometria->indices[inicio + 2]];
    float ab_x = b->posicao[0] - a->posicao[0];
    float ab_y = b->posicao[1] - a->posicao[1];
    float ab_z = b->posicao[2] - a->posicao[2];
    float ac_x = c->posicao[0] - a->posicao[0];
    float ac_y = c->posicao[1] - a->posicao[1];
    float ac_z = c->posicao[2] - a->posicao[2];
    float normal_x = ab_y * ac_z - ab_z * ac_y;
    float normal_y = ab_z * ac_x - ab_x * ac_z;
    float normal_z = ab_x * ac_y - ab_y * ac_x;
    return normal_x * a->posicao[0] + normal_y * a->posicao[1] + normal_z * a->posicao[2];
}

int main(void)
{
    LumeAppConfig config = lume_app_config_default();
    LumeApp *app;
    LumeScene *scene;
    LumeNode *camera;
    LumeGeometry *box;
    LumeGeometry *plane;
    LumeGeometry *sphere;
    LumeNode *mesh;
    LumeNode *parent;
    LumeNode *child;
    LumeMaterial *material;
    LumeTexture *texture;
    LumeLambertMaterialConfig material_config = lume_lambert_material_config_default();
    const uint8_t pixels[] = {255, 255, 255, 255, 30, 80, 200, 255, 30, 80, 200, 255, 255, 255, 255, 255};
    const float invalid_positions[] = {0, 0, 0, 1, 0, 0, 0, 1, 0};
    const uint32_t invalid_indices[] = {0, 1, 3};
    LumeGeometryData invalid_data = {invalid_positions, NULL, NULL, 3, invalid_indices, 3};
    unsigned char pixel[4] = {0};

    config.width = 128;
    config.height = 128;
    config.visible = false;
    config.vsync = false;
    config.clear_color = (LumeColor){0.1f, 0.2f, 0.3f, 1.0f};
    app = lume_app_create(&config);
    if (!app)
    {
        fprintf(stderr, "FAIL: Could not create a hidden OpenGL context: %s\n", lume_get_last_error());
        return 1;
    }
    scene = lume_scene_create(app);
    camera = lume_camera_create_perspective(scene, NULL);
    box = lume_geometry_create_box(app, 1.0f, 1.0f, 1.0f);
    plane = lume_geometry_create_plane(app, 1.0f, 1.0f);
    sphere = lume_geometry_create_sphere(app, 1.0f, 8, 6);
    texture = lume_texture_create(app, pixels, 2, 2, NULL);
    material_config.texture = texture;
    material = lume_material_create_lambert(app, &material_config);
    mesh = lume_mesh_create(scene, box, material);
    parent = lume_node_create(scene);
    child = lume_node_create(scene);
    lume_node_set_position(parent, (LumeVec3){2.0f, 0.0f, 0.0f});
    lume_node_set_position(child, (LumeVec3){0.0f, 3.0f, 0.0f});
    lume_node_add_child(parent, child);
    lume_ambient_light_create(scene, NULL);
    lume_directional_light_create(scene, NULL);
    lume_node_set_position(camera, (LumeVec3){0.0f, 0.0f, 3.0f});

    if (!scene || !camera || !box || !plane || !sphere || !texture || !material || !mesh || !parent || !child ||
        box->quantidade_vertices != 24 || box->quantidade_indices != 36 || plane->quantidade_vertices != 4 ||
        sphere->quantidade_indices != 288)
    {
        fprintf(stderr, "FAIL: Primitive geometry data is invalid: %s\n", lume_get_last_error());
        lume_app_destroy(app);
        return 1;
    }
    if (orientacao_face_esfera(sphere, 48) <= 0.0f)
    {
        fprintf(stderr, "FAIL: Sphere triangle winding faces inward.\n");
        lume_app_destroy(app);
        return 1;
    }
    if (lume_geometry_create_custom(app, &invalid_data) != NULL ||
        strstr(lume_get_last_error(), "outside the vertex range") == NULL)
    {
        fprintf(stderr, "FAIL: Invalid custom geometry was not rejected with an English diagnostic.\n");
        lume_app_destroy(app);
        return 1;
    }
    if (!lume_render(app, scene, camera))
    {
        fprintf(stderr, "FAIL: Rendering failed: %s\n", lume_get_last_error());
        lume_app_destroy(app);
        return 1;
    }
    if (child->matriz_mundo.valor[12] < 1.999f || child->matriz_mundo.valor[13] < 2.999f)
    {
        fprintf(stderr, "FAIL: Child world transform does not include its parent transform.\n");
        lume_app_destroy(app);
        return 1;
    }
    glfwSetWindowSize(app->janela, 160, 96);
    glfwPollEvents();
    if (!lume_render(app, scene, camera))
    {
        fprintf(stderr, "FAIL: Rendering after resize failed: %s\n", lume_get_last_error());
        lume_app_destroy(app);
        return 1;
    }
    glFinish();
    glReadPixels(64, 64, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
    if (pixel[0] == 0 && pixel[1] == 0 && pixel[2] == 0)
    {
        fprintf(stderr, "FAIL: Rendered center pixel is empty.\n");
        lume_app_destroy(app);
        return 1;
    }
    puts("OpenGL smoke test passed.");
    lume_app_destroy(app);
    return 0;
}
