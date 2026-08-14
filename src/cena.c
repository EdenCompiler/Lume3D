#include "lume_interno.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

static void lume_marcar_sujo(LumeNode *no)
{
    size_t i;
    if (!no)
        return;
    no->transformacao_suja = true;
    for (i = 0; i < no->quantidade_filhos; ++i)
        lume_marcar_sujo(no->filhos[i]);
}

LumeResult lume_scene_create(LumeApp *a, LumeScene **saida)
{
    LumeScene *c;
    if (!saida)
        return lume_definir_erro(LUME_ERROR_INVALID_ARGUMENT, "scene.create", NULL, 0, 0,
                                 "out_scene must not be NULL.");
    *saida = NULL;
    if (!a)
        return lume_definir_erro(LUME_ERROR_INVALID_ARGUMENT, "scene.create", NULL, 0, 0,
                                 "A valid application is required to create a scene.");
    c = calloc(1, sizeof(*c));
    if (!c)
        return lume_definir_erro(LUME_ERROR_OUT_OF_MEMORY, "scene.create", NULL, 0, 0,
                                 "Out of memory while creating a scene.");
    c->aplicativo = a;
    if (!lume_adicionar_ponteiro((void ***)&a->cenas, &a->quantidade_cenas, &a->capacidade_cenas, c))
    {
        free(c);
        return LUME_ERROR_OUT_OF_MEMORY;
    }
    *saida = c;
    return LUME_SUCCESS;
}

LumeNode *lume_criar_no(LumeScene *c, LumeTipoNo tipo)
{
    LumeNode *n;
    if (!c)
    {
        lume_definir_erro(LUME_ERROR_INVALID_ARGUMENT, "node.create", NULL, 0, 0,
                          "A valid scene is required to create a node.");
        return NULL;
    }
    n = calloc(1, sizeof(*n));
    if (!n)
    {
        lume_definir_erro(LUME_ERROR_OUT_OF_MEMORY, "node.create", NULL, 0, 0,
                          "Out of memory while creating a scene node.");
        return NULL;
    }
    n->cena = c;
    n->tipo = tipo;
    n->escala = (LumeVec3){1, 1, 1};
    n->rotacao = lume_quat_identity();
    n->matriz_local = lume_mat4_identity();
    n->matriz_mundo = lume_mat4_identity();
    n->transformacao_suja = true;
    if (!lume_adicionar_ponteiro((void ***)&c->nos, &c->quantidade_nos, &c->capacidade_nos, n))
    {
        free(n);
        return NULL;
    }
    return n;
}
LumeResult lume_node_create(LumeScene *c, LumeNode **saida)
{
    if (!saida)
        return lume_definir_erro(LUME_ERROR_INVALID_ARGUMENT, "node.create", NULL, 0, 0, "out_node must not be NULL.");
    *saida = lume_criar_no(c, LUME_NO_VAZIO);
    return *saida ? LUME_SUCCESS : lume_error_last()->code;
}

void lume_node_remove_from_parent(LumeNode *n)
{
    if (n && n->pai)
    {
        lume_remover_ponteiro((void **)n->pai->filhos, &n->pai->quantidade_filhos, n);
        n->pai = NULL;
        lume_marcar_sujo(n);
    }
}
static void lume_destruir_no(LumeNode *n)
{
    if (!n)
        return;
    while (n->quantidade_filhos)
        lume_destruir_no(n->filhos[n->quantidade_filhos - 1]);
    lume_node_remove_from_parent(n);
    lume_remover_ponteiro((void **)n->cena->nos, &n->cena->quantidade_nos, n);
    if (n->tipo == LUME_NO_MALHA || n->tipo == LUME_NO_MALHA_INSTANCIADA)
    {
        lume_geometry_release(n->dados.malha.geometria);
        lume_material_release(n->dados.malha.material);
        if (n->dados.malha.vbo_instancias)
            glDeleteBuffers(1, &n->dados.malha.vbo_instancias);
        free(n->dados.malha.instancias);
    }
    free(n->nome);
    free(n->filhos);
    free(n);
}
void lume_node_destroy(LumeNode *n)
{
    lume_destruir_no(n);
}
void lume_scene_destroy(LumeScene *c)
{
    if (!c)
        return;
    while (c->quantidade_nos)
        lume_destruir_no(c->nos[c->quantidade_nos - 1]);
    lume_remover_ponteiro((void **)c->aplicativo->cenas, &c->aplicativo->quantidade_cenas, c);
    free(c->nos);
    free(c);
}

static bool lume_no_ancestral(const LumeNode *n, const LumeNode *possivel)
{
    const LumeNode *p = n;
    while (p)
    {
        if (p == possivel)
            return true;
        p = p->pai;
    }
    return false;
}
LumeResult lume_node_add_child(LumeNode *pai, LumeNode *filho)
{
    if (!pai || !filho || pai->cena != filho->cena)
        return lume_definir_erro(LUME_ERROR_INVALID_ARGUMENT, "node.add_child", NULL, 0, 0,
                                 "Parent and child must belong to the same scene.");
    if (pai == filho || lume_no_ancestral(pai, filho))
        return lume_definir_erro(LUME_ERROR_INVALID_ARGUMENT, "node.add_child", NULL, 0, 0,
                                 "Adding this child would create a scene cycle.");
    if (filho->pai == pai)
        return LUME_SUCCESS;
    lume_node_remove_from_parent(filho);
    if (!lume_adicionar_ponteiro((void ***)&pai->filhos, &pai->quantidade_filhos, &pai->capacidade_filhos, filho))
        return LUME_ERROR_OUT_OF_MEMORY;
    filho->pai = pai;
    lume_marcar_sujo(filho);
    return LUME_SUCCESS;
}

void lume_node_set_name(LumeNode *n, const char *nome)
{
    char *c;
    if (!n)
        return;
    c = lume_copiar_texto(nome ? nome : "");
    if (c)
    {
        free(n->nome);
        n->nome = c;
    }
}
const char *lume_node_name(const LumeNode *n)
{
    return n && n->nome ? n->nome : "";
}
void lume_node_set_position(LumeNode *n, LumeVec3 p)
{
    if (n)
    {
        n->posicao = p;
        lume_marcar_sujo(n);
    }
}
void lume_node_set_rotation(LumeNode *n, LumeQuat q)
{
    if (n)
    {
        n->rotacao = q;
        lume_marcar_sujo(n);
    }
}
void lume_node_set_euler_rotation(LumeNode *n, LumeVec3 r)
{
    if (n)
        lume_node_set_rotation(n, lume_quat_from_euler(r));
}
void lume_node_set_scale(LumeNode *n, LumeVec3 e)
{
    if (n)
    {
        n->escala = e;
        lume_marcar_sujo(n);
    }
}
LumeVec3 lume_node_position(const LumeNode *n)
{
    return n ? n->posicao : (LumeVec3){0, 0, 0};
}
LumeQuat lume_node_rotation(const LumeNode *n)
{
    return n ? n->rotacao : lume_quat_identity();
}
LumeVec3 lume_node_scale(const LumeNode *n)
{
    return n ? n->escala : (LumeVec3){1, 1, 1};
}
void lume_node_translate(LumeNode *n, LumeVec3 o)
{
    if (n)
        lume_node_set_position(n, lume_vec3_add(n->posicao, o));
}
void lume_node_rotate_x(LumeNode *n, float r)
{
    if (n)
        lume_node_set_rotation(n, lume_quat_slerp(n->rotacao, lume_quat_from_euler((LumeVec3){r, 0, 0}), 1.0f));
}
void lume_node_rotate_y(LumeNode *n, float r)
{
    if (n)
    {
        LumeQuat q = n->rotacao, p = lume_quat_from_euler((LumeVec3){0, r, 0});
        lume_node_set_rotation(n, (LumeQuat){q.w * p.x + q.x * p.w + q.y * p.z - q.z * p.y,
                                             q.w * p.y - q.x * p.z + q.y * p.w + q.z * p.x,
                                             q.w * p.z + q.x * p.y - q.y * p.x + q.z * p.w,
                                             q.w * p.w - q.x * p.x - q.y * p.y - q.z * p.z});
    }
}
void lume_node_rotate_z(LumeNode *n, float r)
{
    if (n)
    {
        LumeQuat q = n->rotacao, p = lume_quat_from_euler((LumeVec3){0, 0, r});
        lume_node_set_rotation(n, (LumeQuat){q.w * p.x + q.x * p.w + q.y * p.z - q.z * p.y,
                                             q.w * p.y - q.x * p.z + q.y * p.w + q.z * p.x,
                                             q.w * p.z + q.x * p.y - q.y * p.x + q.z * p.w,
                                             q.w * p.w - q.x * p.x - q.y * p.y - q.z * p.z});
    }
}

static void lume_atualizar_no(LumeNode *n, LumeMat4 pai, bool pai_sujo)
{
    size_t i;
    bool sujo = n->transformacao_suja || pai_sujo;
    if (sujo)
    {
        n->matriz_local = lume_mat4_transform(n->posicao, n->rotacao, n->escala);
        n->matriz_mundo = n->pai ? lume_mat4_multiply(pai, n->matriz_local) : n->matriz_local;
        n->transformacao_suja = false;
    }
    for (i = 0; i < n->quantidade_filhos; ++i)
        lume_atualizar_no(n->filhos[i], n->matriz_mundo, sujo);
}
void lume_atualizar_matrizes_cena(LumeScene *c)
{
    size_t i;
    for (i = 0; i < c->quantidade_nos; ++i)
        if (!c->nos[i]->pai)
            lume_atualizar_no(c->nos[i], lume_mat4_identity(), false);
}
LumeMat4 lume_node_world_matrix(const LumeNode *n)
{
    if (!n)
        return lume_mat4_identity();
    lume_atualizar_matrizes_cena(n->cena);
    return n->matriz_mundo;
}
LumeAabb lume_node_world_bounds(const LumeNode *n)
{
    if (!n || (n->tipo != LUME_NO_MALHA && n->tipo != LUME_NO_MALHA_INSTANCIADA))
        return lume_aabb_empty();
    return lume_aabb_transform(n->dados.malha.geometria->limites, lume_node_world_matrix(n));
}

LumeResult lume_node_look_at(LumeNode *n, LumeVec3 alvo)
{
    LumeVec3 d;
    float guinada, inclinacao;
    if (!n)
        return lume_definir_erro(LUME_ERROR_INVALID_ARGUMENT, "node.look_at", NULL, 0, 0, "A valid node is required.");
    d = lume_vec3_subtract(alvo, n->posicao);
    if (lume_vec3_length(d) < 0.000001f)
        return lume_definir_erro(LUME_ERROR_INVALID_ARGUMENT, "node.look_at", NULL, 0, 0,
                                 "Look-at target must differ from node position.");
    d = lume_vec3_normalize(d);
    guinada = atan2f(d.x, -d.z);
    inclinacao = asinf(d.y);
    lume_node_set_euler_rotation(n, (LumeVec3){inclinacao, guinada, 0});
    return LUME_SUCCESS;
}

LumePerspectiveCameraConfig lume_perspective_camera_config_default(void)
{
    return (LumePerspectiveCameraConfig){1.04719755f, 0, 0.1f, 1000};
}
LumeOrthographicCameraConfig lume_orthographic_camera_config_default(void)
{
    return (LumeOrthographicCameraConfig){-1, 1, -1, 1, 0.1f, 1000};
}
LumeResult lume_camera_create_perspective(LumeScene *c, const LumePerspectiveCameraConfig *cfg, LumeNode **saida)
{
    LumePerspectiveCameraConfig x = cfg ? *cfg : lume_perspective_camera_config_default();
    LumeNode *n;
    if (!saida)
        return lume_definir_erro(LUME_ERROR_INVALID_ARGUMENT, "camera.create", NULL, 0, 0,
                                 "out_camera must not be NULL.");
    *saida = NULL;
    if (x.field_of_view_radians <= 0 || x.near_plane <= 0 || x.far_plane <= x.near_plane)
        return lume_definir_erro(LUME_ERROR_INVALID_ARGUMENT, "camera.create", NULL, 0, 0,
                                 "Perspective camera planes and field of view are invalid.");
    n = lume_criar_no(c, LUME_NO_CAMERA_PERSPECTIVA);
    if (!n)
        return lume_error_last()->code;
    n->dados.perspectiva =
        (LumeDadosCameraPerspectiva){x.field_of_view_radians, x.aspect_ratio, x.near_plane, x.far_plane};
    *saida = n;
    return LUME_SUCCESS;
}
LumeResult lume_camera_create_orthographic(LumeScene *c, const LumeOrthographicCameraConfig *cfg, LumeNode **saida)
{
    LumeOrthographicCameraConfig x = cfg ? *cfg : lume_orthographic_camera_config_default();
    LumeNode *n;
    if (!saida)
        return lume_definir_erro(LUME_ERROR_INVALID_ARGUMENT, "camera.create", NULL, 0, 0,
                                 "out_camera must not be NULL.");
    *saida = NULL;
    if (x.left == x.right || x.bottom == x.top || x.near_plane == x.far_plane)
        return lume_definir_erro(LUME_ERROR_INVALID_ARGUMENT, "camera.create", NULL, 0, 0,
                                 "Orthographic camera bounds are invalid.");
    n = lume_criar_no(c, LUME_NO_CAMERA_ORTOGRAFICA);
    if (!n)
        return lume_error_last()->code;
    n->dados.ortografica = (LumeDadosCameraOrtografica){x.left, x.right, x.bottom, x.top, x.near_plane, x.far_plane};
    *saida = n;
    return LUME_SUCCESS;
}
LumeResult lume_camera_set_aspect_ratio(LumeNode *n, float p)
{
    if (!n || n->tipo != LUME_NO_CAMERA_PERSPECTIVA || p <= 0)
        return lume_definir_erro(LUME_ERROR_INVALID_ARGUMENT, "camera.set_aspect", NULL, 0, 0,
                                 "A perspective camera and positive aspect ratio are required.");
    n->dados.perspectiva.proporcao = p;
    return LUME_SUCCESS;
}

LumeAmbientLightConfig lume_ambient_light_config_default(void)
{
    return (LumeAmbientLightConfig){{1, 1, 1, 1}, 0.15f};
}
LumeDirectionalLightConfig lume_directional_light_config_default(void)
{
    return (LumeDirectionalLightConfig){{1, 1, 1, 1}, 1, (LumeVec3){-1, -1, -1}, true};
}
LumePointLightConfig lume_point_light_config_default(void)
{
    return (LumePointLightConfig){{1, 1, 1, 1}, 1, 10};
}
LumeSpotLightConfig lume_spot_light_config_default(void)
{
    return (LumeSpotLightConfig){{1, 1, 1, 1}, 1, 15, 0.35f, 0.55f, (LumeVec3){0, -1, 0}, true};
}
static LumeResult lume_criar_luz(LumeScene *c, LumeTipoNo tipo, LumeColor cor, float intensidade, float alcance,
                                 LumeVec3 direcao, float interno, float externo, bool sombra, LumeNode **saida)
{
    LumeNode *n;
    if (!saida)
        return lume_definir_erro(LUME_ERROR_INVALID_ARGUMENT, "light.create", NULL, 0, 0,
                                 "out_light must not be NULL.");
    *saida = NULL;
    if (intensidade < 0 || alcance < 0)
        return lume_definir_erro(LUME_ERROR_INVALID_ARGUMENT, "light.create", NULL, 0, 0,
                                 "Light intensity and range must not be negative.");
    n = lume_criar_no(c, tipo);
    if (!n)
        return lume_error_last()->code;
    n->dados.luz = (LumeDadosLuz){cor, intensidade, alcance, interno, externo, lume_vec3_normalize(direcao), sombra};
    *saida = n;
    return LUME_SUCCESS;
}
LumeResult lume_ambient_light_create(LumeScene *c, const LumeAmbientLightConfig *cfg, LumeNode **s)
{
    LumeAmbientLightConfig x = cfg ? *cfg : lume_ambient_light_config_default();
    return lume_criar_luz(c, LUME_NO_LUZ_AMBIENTE, x.color, x.intensity, 0, (LumeVec3){0, 0, 0}, 0, 0, false, s);
}
LumeResult lume_directional_light_create(LumeScene *c, const LumeDirectionalLightConfig *cfg, LumeNode **s)
{
    LumeDirectionalLightConfig x = cfg ? *cfg : lume_directional_light_config_default();
    return lume_criar_luz(c, LUME_NO_LUZ_DIRECIONAL, x.color, x.intensity, 0, x.direction, 0, 0, x.cast_shadows, s);
}
LumeResult lume_point_light_create(LumeScene *c, const LumePointLightConfig *cfg, LumeNode **s)
{
    LumePointLightConfig x = cfg ? *cfg : lume_point_light_config_default();
    return lume_criar_luz(c, LUME_NO_LUZ_PONTUAL, x.color, x.intensity, x.range, (LumeVec3){0, 0, 0}, 0, 0, false, s);
}
LumeResult lume_spot_light_create(LumeScene *c, const LumeSpotLightConfig *cfg, LumeNode **s)
{
    LumeSpotLightConfig x = cfg ? *cfg : lume_spot_light_config_default();
    if (x.inner_angle < 0 || x.outer_angle <= x.inner_angle)
        return lume_definir_erro(LUME_ERROR_INVALID_ARGUMENT, "light.create", NULL, 0, 0,
                                 "Spot light angles are invalid.");
    return lume_criar_luz(c, LUME_NO_LUZ_SPOT, x.color, x.intensity, x.range, x.direction, x.inner_angle, x.outer_angle,
                          x.cast_shadows, s);
}

LumeResult lume_mesh_create(LumeScene *c, LumeGeometry *g, LumeMaterial *m, LumeNode **s)
{
    LumeNode *n;
    if (!s)
        return lume_definir_erro(LUME_ERROR_INVALID_ARGUMENT, "mesh.create", NULL, 0, 0, "out_mesh must not be NULL.");
    *s = NULL;
    if (!c || !g || !m || g->referencia.aplicativo != c->aplicativo || m->referencia.aplicativo != c->aplicativo)
        return lume_definir_erro(LUME_ERROR_INVALID_ARGUMENT, "mesh.create", NULL, 0, 0,
                                 "Scene, geometry, and material must belong to the same application.");
    n = lume_criar_no(c, LUME_NO_MALHA);
    if (!n)
        return lume_error_last()->code;
    lume_geometry_retain(g);
    lume_material_retain(m);
    n->dados.malha.geometria = g;
    n->dados.malha.material = m;
    *s = n;
    return LUME_SUCCESS;
}
LumeResult lume_instanced_mesh_create(LumeScene *c, LumeGeometry *g, LumeMaterial *m, uint32_t cap, LumeNode **s)
{
    LumeResult r = lume_mesh_create(c, g, m, s);
    if (r != LUME_SUCCESS)
        return r;
    (*s)->tipo = LUME_NO_MALHA_INSTANCIADA;
    (*s)->dados.malha.capacidade_instancias = cap;
    (*s)->dados.malha.quantidade_instancias = cap;
    (*s)->dados.malha.instancias = calloc(cap, sizeof(LumeMat4));
    if (cap && !(*s)->dados.malha.instancias)
    {
        lume_node_destroy(*s);
        *s = NULL;
        return lume_definir_erro(LUME_ERROR_OUT_OF_MEMORY, "mesh.create_instanced", NULL, 0, 0,
                                 "Out of memory while creating instance transforms.");
    }
    {
        uint32_t i;
        for (i = 0; i < cap; ++i)
            (*s)->dados.malha.instancias[i] = lume_mat4_identity();
    }
    return LUME_SUCCESS;
}
LumeResult lume_instanced_mesh_set_transform(LumeNode *n, uint32_t i, LumeMat4 t)
{
    if (!n || n->tipo != LUME_NO_MALHA_INSTANCIADA || i >= n->dados.malha.quantidade_instancias)
        return lume_definir_erro(LUME_ERROR_INVALID_ARGUMENT, "mesh.set_instance", NULL, 0, 0,
                                 "Instance index is outside the mesh capacity.");
    n->dados.malha.instancias[i] = t;
    return LUME_SUCCESS;
}

static int lume_comparar_hits(const void *a, const void *b)
{
    float x = ((const LumeRaycastHit *)a)->distance, y = ((const LumeRaycastHit *)b)->distance;
    return x < y ? -1 : x > y ? 1 : 0;
}
size_t lume_scene_raycast(LumeScene *c, LumeRay raio, LumeRaycastHit *hits, size_t cap)
{
    size_t i, q = 0;
    if (!c)
        return 0;
    lume_atualizar_matrizes_cena(c);
    raio.direction = lume_vec3_normalize(raio.direction);
    for (i = 0; i < c->quantidade_nos; ++i)
    {
        LumeNode *n = c->nos[i];
        float d;
        if ((n->tipo == LUME_NO_MALHA || n->tipo == LUME_NO_MALHA_INSTANCIADA) &&
            lume_ray_intersect_aabb(raio, lume_node_world_bounds(n), &d))
        {
            if (hits && q < cap)
                hits[q] = (LumeRaycastHit){n, d, lume_vec3_add(raio.origin, lume_vec3_scale(raio.direction, d)),
                                           (LumeVec3){0, 1, 0}, 0};
            ++q;
        }
    }
    if (hits)
    {
        size_t gravados = q < cap ? q : cap;
        qsort(hits, gravados, sizeof(*hits), lume_comparar_hits);
    }
    return q;
}
