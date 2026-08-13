#include "lume_interno.h"

#include <math.h>
#include <stdlib.h>

static void lume_marcar_transformacao_suja(LumeNode *no)
{
    size_t indice;
    if (!no)
    {
        return;
    }
    no->transformacao_suja = true;
    for (indice = 0; indice < no->quantidade_filhos; ++indice)
    {
        lume_marcar_transformacao_suja(no->filhos[indice]);
    }
}

LumeScene *lume_scene_create(LumeApp *aplicativo)
{
    LumeScene *cena;
    if (!aplicativo)
    {
        lume_definir_erro("A valid application is required to create a scene.");
        return NULL;
    }
    cena = calloc(1, sizeof(*cena));
    if (!cena)
    {
        lume_definir_erro("Out of memory while creating a scene.");
        return NULL;
    }
    cena->aplicativo = aplicativo;
    if (!lume_adicionar_ponteiro((void ***)&aplicativo->cenas, &aplicativo->quantidade_cenas,
                                 &aplicativo->capacidade_cenas, cena))
    {
        free(cena);
        return NULL;
    }
    return cena;
}

LumeNode *lume_criar_no(LumeScene *cena, LumeTipoNo tipo)
{
    LumeNode *no;
    if (!cena)
    {
        lume_definir_erro("A valid scene is required to create a node.");
        return NULL;
    }
    no = calloc(1, sizeof(*no));
    if (!no)
    {
        lume_definir_erro("Out of memory while creating a scene node.");
        return NULL;
    }
    no->cena = cena;
    no->tipo = tipo;
    no->escala = (LumeVec3){1.0f, 1.0f, 1.0f};
    no->matriz_local = lume_matriz_identidade();
    no->matriz_mundo = lume_matriz_identidade();
    no->transformacao_suja = true;
    if (!lume_adicionar_ponteiro((void ***)&cena->nos, &cena->quantidade_nos, &cena->capacidade_nos, no))
    {
        free(no);
        return NULL;
    }
    return no;
}

LumeNode *lume_node_create(LumeScene *cena)
{
    return lume_criar_no(cena, LUME_NO_VAZIO);
}

static void lume_destruir_no_recursivo(LumeNode *no)
{
    if (!no)
    {
        return;
    }
    while (no->quantidade_filhos > 0)
    {
        lume_destruir_no_recursivo(no->filhos[no->quantidade_filhos - 1]);
    }
    lume_node_remove_from_parent(no);
    lume_remover_ponteiro((void **)no->cena->nos, &no->cena->quantidade_nos, no);
    free(no->filhos);
    free(no);
}

void lume_node_destroy(LumeNode *no)
{
    lume_destruir_no_recursivo(no);
}

void lume_scene_destroy(LumeScene *cena)
{
    if (!cena)
    {
        return;
    }
    while (cena->quantidade_nos > 0)
    {
        lume_destruir_no_recursivo(cena->nos[cena->quantidade_nos - 1]);
    }
    lume_remover_ponteiro((void **)cena->aplicativo->cenas, &cena->aplicativo->quantidade_cenas, cena);
    free(cena->nos);
    free(cena);
}

static bool lume_no_e_ancestral(const LumeNode *possivel_ancestral, const LumeNode *no)
{
    const LumeNode *atual = no;
    while (atual)
    {
        if (atual == possivel_ancestral)
        {
            return true;
        }
        atual = atual->pai;
    }
    return false;
}

bool lume_node_add_child(LumeNode *pai, LumeNode *filho)
{
    if (!pai || !filho)
    {
        lume_definir_erro("Both parent and child nodes must be valid.");
        return false;
    }
    if (pai->cena != filho->cena)
    {
        lume_definir_erro("Parent and child nodes must belong to the same scene.");
        return false;
    }
    if (pai == filho || lume_no_e_ancestral(filho, pai))
    {
        lume_definir_erro("A node cannot be parented to itself or one of its descendants.");
        return false;
    }
    if (filho->pai == pai)
    {
        return true;
    }
    lume_node_remove_from_parent(filho);
    if (!lume_adicionar_ponteiro((void ***)&pai->filhos, &pai->quantidade_filhos, &pai->capacidade_filhos, filho))
    {
        return false;
    }
    filho->pai = pai;
    lume_marcar_transformacao_suja(filho);
    return true;
}

void lume_node_remove_from_parent(LumeNode *no)
{
    if (!no || !no->pai)
    {
        return;
    }
    lume_remover_ponteiro((void **)no->pai->filhos, &no->pai->quantidade_filhos, no);
    no->pai = NULL;
    lume_marcar_transformacao_suja(no);
}

void lume_node_set_position(LumeNode *no, LumeVec3 posicao)
{
    if (no)
    {
        no->posicao = posicao;
        lume_marcar_transformacao_suja(no);
    }
}

void lume_node_set_rotation(LumeNode *no, LumeVec3 rotacao)
{
    if (no)
    {
        no->rotacao = rotacao;
        lume_marcar_transformacao_suja(no);
    }
}

void lume_node_set_scale(LumeNode *no, LumeVec3 escala)
{
    if (no)
    {
        no->escala = escala;
        lume_marcar_transformacao_suja(no);
    }
}

LumeVec3 lume_node_get_position(const LumeNode *no)
{
    return no ? no->posicao : (LumeVec3){0.0f, 0.0f, 0.0f};
}

LumeVec3 lume_node_get_rotation(const LumeNode *no)
{
    return no ? no->rotacao : (LumeVec3){0.0f, 0.0f, 0.0f};
}

LumeVec3 lume_node_get_scale(const LumeNode *no)
{
    return no ? no->escala : (LumeVec3){1.0f, 1.0f, 1.0f};
}

void lume_node_translate(LumeNode *no, LumeVec3 deslocamento)
{
    if (no)
    {
        no->posicao.x += deslocamento.x;
        no->posicao.y += deslocamento.y;
        no->posicao.z += deslocamento.z;
        lume_marcar_transformacao_suja(no);
    }
}

void lume_node_rotate_x(LumeNode *no, float angulo)
{
    if (no)
    {
        no->rotacao.x += angulo;
        lume_marcar_transformacao_suja(no);
    }
}

void lume_node_rotate_y(LumeNode *no, float angulo)
{
    if (no)
    {
        no->rotacao.y += angulo;
        lume_marcar_transformacao_suja(no);
    }
}

void lume_node_rotate_z(LumeNode *no, float angulo)
{
    if (no)
    {
        no->rotacao.z += angulo;
        lume_marcar_transformacao_suja(no);
    }
}

bool lume_node_look_at(LumeNode *no, LumeVec3 alvo)
{
    LumeVec3 direcao;
    LumeVec3 alvo_local = alvo;
    float comprimento_xz;
    if (!no)
    {
        lume_definir_erro("A valid node is required by lume_node_look_at.");
        return false;
    }
    /* O alvo público está no espaço do mundo; convertemos para o espaço do pai. */
    if (no->pai)
    {
        LumeMatriz4 inversa_pai;
        lume_atualizar_matrizes_cena(no->cena);
        if (!lume_matriz_inverter(no->pai->matriz_mundo, &inversa_pai))
        {
            lume_definir_erro("The parent transform is not invertible.");
            return false;
        }
        alvo_local = lume_matriz_transformar_ponto(inversa_pai, alvo);
    }
    direcao = lume_vetor3_subtrair(alvo_local, no->posicao);
    comprimento_xz = sqrtf(direcao.x * direcao.x + direcao.z * direcao.z);
    if (comprimento_xz <= 0.000001f && fabsf(direcao.y) <= 0.000001f)
    {
        lume_definir_erro("A node cannot look at its own position.");
        return false;
    }
    no->rotacao.x = atan2f(direcao.y, comprimento_xz);
    no->rotacao.y = atan2f(-direcao.x, -direcao.z);
    no->rotacao.z = 0.0f;
    lume_marcar_transformacao_suja(no);
    return true;
}

static void lume_atualizar_no(LumeNode *no, const LumeMatriz4 *matriz_pai, bool pai_sujo)
{
    size_t indice;
    bool precisa_atualizar = no->transformacao_suja || pai_sujo;
    if (precisa_atualizar)
    {
        no->matriz_local = lume_matriz_transformacao(no->posicao, no->rotacao, no->escala);
        no->matriz_mundo = matriz_pai ? lume_matriz_multiplicar(*matriz_pai, no->matriz_local) : no->matriz_local;
        no->transformacao_suja = false;
    }
    for (indice = 0; indice < no->quantidade_filhos; ++indice)
    {
        lume_atualizar_no(no->filhos[indice], &no->matriz_mundo, precisa_atualizar);
    }
}

void lume_atualizar_matrizes_cena(LumeScene *cena)
{
    size_t indice;
    for (indice = 0; indice < cena->quantidade_nos; ++indice)
    {
        if (!cena->nos[indice]->pai)
        {
            lume_atualizar_no(cena->nos[indice], NULL, false);
        }
    }
}

LumePerspectiveCameraConfig lume_perspective_camera_config_default(void)
{
    return (LumePerspectiveCameraConfig){1.0471975512f, 0.0f, 0.1f, 1000.0f};
}

LumeOrthographicCameraConfig lume_orthographic_camera_config_default(void)
{
    return (LumeOrthographicCameraConfig){-5.0f, 5.0f, -5.0f, 5.0f, 0.1f, 1000.0f};
}

LumeNode *lume_camera_create_perspective(LumeScene *cena, const LumePerspectiveCameraConfig *config)
{
    LumePerspectiveCameraConfig configuracao = config ? *config : lume_perspective_camera_config_default();
    LumeNode *camera;
    if (configuracao.field_of_view_radians <= 0.0f || configuracao.field_of_view_radians >= 3.1415926535f ||
        configuracao.aspect_ratio < 0.0f || configuracao.near_plane <= 0.0f ||
        configuracao.far_plane <= configuracao.near_plane)
    {
        lume_definir_erro("Perspective camera parameters are invalid.");
        return NULL;
    }
    camera = lume_criar_no(cena, LUME_NO_CAMERA_PERSPECTIVA);
    if (camera)
    {
        camera->dados.perspectiva =
            (LumeDadosCameraPerspectiva){configuracao.field_of_view_radians, configuracao.aspect_ratio,
                                         configuracao.near_plane, configuracao.far_plane};
    }
    return camera;
}

LumeNode *lume_camera_create_orthographic(LumeScene *cena, const LumeOrthographicCameraConfig *config)
{
    LumeOrthographicCameraConfig configuracao = config ? *config : lume_orthographic_camera_config_default();
    LumeNode *camera;
    if (configuracao.right == configuracao.left || configuracao.top == configuracao.bottom ||
        configuracao.far_plane <= configuracao.near_plane)
    {
        lume_definir_erro("Orthographic camera parameters are invalid.");
        return NULL;
    }
    camera = lume_criar_no(cena, LUME_NO_CAMERA_ORTOGRAFICA);
    if (camera)
    {
        camera->dados.ortografica =
            (LumeDadosCameraOrtografica){configuracao.left, configuracao.right,      configuracao.bottom,
                                         configuracao.top,  configuracao.near_plane, configuracao.far_plane};
    }
    return camera;
}

bool lume_camera_set_aspect_ratio(LumeNode *camera, float proporcao)
{
    if (!camera || camera->tipo != LUME_NO_CAMERA_PERSPECTIVA)
    {
        lume_definir_erro("lume_camera_set_aspect_ratio requires a perspective camera.");
        return false;
    }
    if (proporcao < 0.0f)
    {
        lume_definir_erro("Camera aspect ratio cannot be negative; use zero for automatic sizing.");
        return false;
    }
    camera->dados.perspectiva.proporcao = proporcao;
    return true;
}

LumeNode *lume_mesh_create(LumeScene *cena, LumeGeometry *geometria, LumeMaterial *material)
{
    LumeNode *malha;
    if (!cena || !geometria || !material)
    {
        lume_definir_erro("A scene, geometry, and material are required to create a mesh.");
        return NULL;
    }
    if (geometria->aplicativo != cena->aplicativo || material->aplicativo != cena->aplicativo)
    {
        lume_definir_erro("Mesh resources must belong to the same application as the scene.");
        return NULL;
    }
    malha = lume_criar_no(cena, LUME_NO_MALHA);
    if (malha)
    {
        malha->dados.malha.geometria = geometria;
        malha->dados.malha.material = material;
    }
    return malha;
}

LumeAmbientLightConfig lume_ambient_light_config_default(void)
{
    return (LumeAmbientLightConfig){{1.0f, 1.0f, 1.0f, 1.0f}, 0.25f};
}

LumeDirectionalLightConfig lume_directional_light_config_default(void)
{
    return (LumeDirectionalLightConfig){{1.0f, 1.0f, 1.0f, 1.0f}, 1.0f, {-1.0f, -1.0f, -1.0f}};
}

LumePointLightConfig lume_point_light_config_default(void)
{
    return (LumePointLightConfig){{1.0f, 1.0f, 1.0f, 1.0f}, 1.0f, 10.0f};
}

LumeNode *lume_ambient_light_create(LumeScene *cena, const LumeAmbientLightConfig *config)
{
    LumeAmbientLightConfig configuracao = config ? *config : lume_ambient_light_config_default();
    LumeNode *luz = lume_criar_no(cena, LUME_NO_LUZ_AMBIENTE);
    if (luz)
    {
        luz->dados.luz.cor = configuracao.color;
        luz->dados.luz.intensidade = configuracao.intensity;
    }
    return luz;
}

LumeNode *lume_directional_light_create(LumeScene *cena, const LumeDirectionalLightConfig *config)
{
    LumeDirectionalLightConfig configuracao = config ? *config : lume_directional_light_config_default();
    LumeNode *luz = lume_criar_no(cena, LUME_NO_LUZ_DIRECIONAL);
    if (luz)
    {
        luz->dados.luz.cor = configuracao.color;
        luz->dados.luz.intensidade = configuracao.intensity;
        luz->dados.luz.direcao = lume_vetor3_normalizar(configuracao.direction);
    }
    return luz;
}

LumeNode *lume_point_light_create(LumeScene *cena, const LumePointLightConfig *config)
{
    LumePointLightConfig configuracao = config ? *config : lume_point_light_config_default();
    LumeNode *luz;
    if (configuracao.range <= 0.0f)
    {
        lume_definir_erro("Point light range must be greater than zero.");
        return NULL;
    }
    luz = lume_criar_no(cena, LUME_NO_LUZ_PONTUAL);
    if (luz)
    {
        luz->dados.luz.cor = configuracao.color;
        luz->dados.luz.intensidade = configuracao.intensity;
        luz->dados.luz.alcance = configuracao.range;
    }
    return luz;
}
