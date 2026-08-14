#include "lume_interno.h"

#include <cgltf.h>
#include <fast_obj.h>
#include <stb_image.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <math.h>

static long long lume_modificacao_arquivo(const char *caminho)
{
    struct stat s;
    return caminho && stat(caminho, &s) == 0 ? (long long)s.st_mtime : 0;
}
static const char *lume_extensao(const char *caminho)
{
    const char *p = caminho ? strrchr(caminho, '.') : NULL;
    return p ? p : "";
}
static bool lume_texto_igual_sem_caixa(const char *a, const char *b)
{
    while (*a && *b)
    {
        char x = *a >= 'A' && *a <= 'Z' ? *a + 32 : *a, y = *b >= 'A' && *b <= 'Z' ? *b + 32 : *b;
        if (x != y)
            return false;
        ++a;
        ++b;
    }
    return *a == *b;
}
static char *lume_caminho_relativo(const char *base, const char *uri)
{
    const char *a = strrchr(base, '/'), *b = strrchr(base, '\\'), *p = a && b ? (a > b ? a : b) : (a ? a : b);
    size_t n = p ? (size_t)(p - base + 1) : 0;
    char *r = malloc(n + strlen(uri) + 1);
    if (!r)
        return NULL;
    if (n)
        memcpy(r, base, n);
    strcpy(r + n, uri);
    return r;
}

LumeModelLoadOptions lume_model_load_options_default(void)
{
    return (LumeModelLoadOptions){true, true, true, true, true};
}

static void lume_liberar_clipe(LumeAnimationClip *c)
{
    size_t i;
    if (!c)
        return;
    for (i = 0; i < c->quantidade_canais; ++i)
    {
        free(c->canais[i].tempos);
        free(c->canais[i].valores);
    }
    free(c->canais);
    free(c->nome);
}
static void lume_destruir_modelo(void *recurso)
{
    LumeModel *m = recurso;
    size_t i;
    lume_desregistrar_recurso(m->referencia.aplicativo, m);
    for (i = 0; i < m->quantidade_primitivas; ++i)
    {
        lume_geometry_release(m->geometrias[i]);
        lume_material_release(m->materiais[i]);
    }
    for (i = 0; i < m->quantidade_nos; ++i)
        free(m->nos[i].nome);
    for (i = 0; i < m->quantidade_clipes; ++i)
        lume_liberar_clipe(&m->clipes[i]);
    free(m->geometrias);
    free(m->materiais);
    free(m->nos);
    free(m->clipes);
    free(m->caminho);
    free(m);
}
void lume_model_retain(LumeModel *m)
{
    if (m)
        lume_referencia_reter(&m->referencia);
}
void lume_model_release(LumeModel *m)
{
    if (m && lume_referencia_liberar(&m->referencia) == 0)
        lume_destruir_modelo(m);
}
const char *lume_model_path(const LumeModel *m)
{
    return m ? m->caminho : NULL;
}
size_t lume_model_animation_count(const LumeModel *m)
{
    return m ? m->quantidade_clipes : 0;
}
LumeAnimationClip *lume_model_animation(const LumeModel *m, size_t i)
{
    return m && i < m->quantidade_clipes ? &m->clipes[i] : NULL;
}

static LumeResult lume_textura_cgltf(LumeApp *a, const char *caminho, cgltf_texture *origem, bool srgb,
                                     LumeTexture **saida)
{
    cgltf_image *img;
    LumeTextureConfig cfg = lume_texture_config_default();
    if (!saida)
        return LUME_ERROR_INVALID_ARGUMENT;
    *saida = NULL;
    if (!origem || !(img = origem->image))
        return LUME_SUCCESS;
    cfg.flip_y = false;
    cfg.srgb = srgb;
    if (img->uri && strncmp(img->uri, "data:", 5) != 0)
    {
        char *p = lume_caminho_relativo(caminho, img->uri);
        LumeResult r;
        if (!p)
            return lume_definir_erro(LUME_ERROR_OUT_OF_MEMORY, "model.texture", caminho, 0, 0,
                                     "Out of memory while resolving a texture path.");
        r = lume_texture_load(a, p, &cfg, saida);
        free(p);
        return r;
    }
    if (img->buffer_view && img->buffer_view->buffer && img->buffer_view->buffer->data)
    {
        unsigned char *base = img->buffer_view->buffer->data;
        unsigned char *pixels;
        int l, h, n;
        pixels = stbi_load_from_memory(base + img->buffer_view->offset, (int)img->buffer_view->size, &l, &h, &n, 4);
        if (!pixels)
            return lume_definir_erro(LUME_ERROR_PARSE, "model.texture", caminho, 0, 0,
                                     "Could not decode an embedded glTF image: %s", stbi_failure_reason());
        {
            LumeResult r = lume_texture_create_rgba8(a, pixels, l, h, &cfg, saida);
            stbi_image_free(pixels);
            return r;
        }
    }
    return lume_definir_erro(LUME_ERROR_UNSUPPORTED, "model.texture", caminho, 0, 0,
                             "This embedded glTF image representation is not supported.");
}

static cgltf_accessor *lume_atributo(cgltf_primitive *p, cgltf_attribute_type tipo, int indice)
{
    cgltf_size i;
    for (i = 0; i < p->attributes_count; ++i)
        if (p->attributes[i].type == tipo && p->attributes[i].index == indice)
            return p->attributes[i].data;
    return NULL;
}
static float *lume_desempacotar(cgltf_accessor *a, size_t componentes)
{
    float *d;
    if (!a)
        return NULL;
    d = malloc(a->count * componentes * sizeof(float));
    if (!d)
        return NULL;
    if (cgltf_accessor_unpack_floats(a, d, a->count * componentes) != a->count * componentes)
    {
        free(d);
        return NULL;
    }
    return d;
}

static LumeResult lume_material_cgltf(LumeApp *a, const char *caminho, cgltf_material *origem, LumeMaterial **saida)
{
    LumeMaterialConfig c =
        lume_material_config_default(origem && origem->unlit ? LUME_MATERIAL_UNLIT : LUME_MATERIAL_PBR);
    LumeResult r;
    if (origem)
    {
        if (origem->has_pbr_metallic_roughness)
        {
            cgltf_pbr_metallic_roughness *p = &origem->pbr_metallic_roughness;
            c.base_color = (LumeColor){p->base_color_factor[0], p->base_color_factor[1], p->base_color_factor[2],
                                       p->base_color_factor[3]};
            c.metallic = p->metallic_factor;
            c.roughness = p->roughness_factor;
            r = lume_textura_cgltf(a, caminho, p->base_color_texture.texture, true, &c.base_color_texture);
            if (r != LUME_SUCCESS)
                return r;
            r = lume_textura_cgltf(a, caminho, p->metallic_roughness_texture.texture, false,
                                   &c.metallic_roughness_texture);
            if (r != LUME_SUCCESS)
            {
                lume_texture_release(c.base_color_texture);
                return r;
            }
        }
        c.emissive_color =
            (LumeColor){origem->emissive_factor[0], origem->emissive_factor[1], origem->emissive_factor[2], 1};
        c.double_sided = origem->double_sided;
        c.alpha_cutoff = origem->alpha_cutoff;
        c.alpha_mode = origem->alpha_mode == cgltf_alpha_mode_mask    ? LUME_ALPHA_MASK
                       : origem->alpha_mode == cgltf_alpha_mode_blend ? LUME_ALPHA_BLEND
                                                                      : LUME_ALPHA_OPAQUE;
        r = lume_textura_cgltf(a, caminho, origem->normal_texture.texture, false, &c.normal_texture);
        if (r != LUME_SUCCESS)
            goto falha;
        r = lume_textura_cgltf(a, caminho, origem->occlusion_texture.texture, false, &c.occlusion_texture);
        if (r != LUME_SUCCESS)
            goto falha;
        r = lume_textura_cgltf(a, caminho, origem->emissive_texture.texture, true, &c.emissive_texture);
        if (r != LUME_SUCCESS)
            goto falha;
    }
    r = lume_material_create(a, &c, saida);
falha:
    lume_texture_release(c.base_color_texture);
    lume_texture_release(c.normal_texture);
    lume_texture_release(c.metallic_roughness_texture);
    lume_texture_release(c.occlusion_texture);
    lume_texture_release(c.emissive_texture);
    return r;
}

static LumeResult lume_primitiva_cgltf(LumeApp *a, const char *caminho, cgltf_primitive *p, LumeGeometry **g,
                                       LumeMaterial **m)
{
    cgltf_accessor *ap = lume_atributo(p, cgltf_attribute_type_position, 0),
                   *an = lume_atributo(p, cgltf_attribute_type_normal, 0),
                   *au = lume_atributo(p, cgltf_attribute_type_texcoord, 0),
                   *at = lume_atributo(p, cgltf_attribute_type_tangent, 0),
                   *aj = lume_atributo(p, cgltf_attribute_type_joints, 0),
                   *aw = lume_atributo(p, cgltf_attribute_type_weights, 0);
    LumeGeometryData d = {0};
    float *pos = NULL, *nor = NULL, *uv = NULL, *tan = NULL, *pes = NULL;
    uint16_t *juntas = NULL;
    uint32_t *indices = NULL;
    size_t i;
    LumeResult r;
    if (p->type != cgltf_primitive_type_triangles)
        return lume_definir_erro(LUME_ERROR_UNSUPPORTED, "model.primitive", caminho, 0, 0,
                                 "Only triangle glTF primitives are supported.");
    if (!ap)
        return lume_definir_erro(LUME_ERROR_PARSE, "model.primitive", caminho, 0, 0,
                                 "A glTF primitive is missing POSITION data.");
    pos = lume_desempacotar(ap, 3);
    nor = lume_desempacotar(an, 3);
    uv = lume_desempacotar(au, 2);
    tan = lume_desempacotar(at, 4);
    pes = lume_desempacotar(aw, 4);
    if (!pos || (an && !nor) || (au && !uv) || (at && !tan) || (aw && !pes))
    {
        r = lume_definir_erro(LUME_ERROR_OUT_OF_MEMORY, "model.primitive", caminho, 0, 0,
                              "Could not unpack glTF vertex attributes.");
        goto fim;
    }
    if (aj)
    {
        juntas = malloc(aj->count * 4 * sizeof(*juntas));
        if (!juntas)
        {
            r = LUME_ERROR_OUT_OF_MEMORY;
            goto fim;
        }
        for (i = 0; i < aj->count; ++i)
        {
            cgltf_uint v[4] = {0};
            cgltf_accessor_read_uint(aj, i, v, 4);
            juntas[i * 4] = (uint16_t)v[0];
            juntas[i * 4 + 1] = (uint16_t)v[1];
            juntas[i * 4 + 2] = (uint16_t)v[2];
            juntas[i * 4 + 3] = (uint16_t)v[3];
        }
    }
    d.positions = pos;
    d.normals = nor;
    d.texture_coordinates = uv;
    d.tangents = tan;
    d.joints = juntas;
    d.joint_weights = pes;
    d.vertex_count = ap->count;
    if (p->indices)
    {
        indices = malloc(p->indices->count * sizeof(*indices));
        if (!indices)
        {
            r = LUME_ERROR_OUT_OF_MEMORY;
            goto fim;
        }
        for (i = 0; i < p->indices->count; ++i)
            indices[i] = (uint32_t)cgltf_accessor_read_index(p->indices, i);
        d.indices = indices;
        d.index_count = p->indices->count;
    }
    r = lume_geometry_create(a, &d, g);
    if (r == LUME_SUCCESS)
    {
        r = lume_material_cgltf(a, caminho, p->material, m);
        if (r != LUME_SUCCESS)
        {
            lume_geometry_release(*g);
            *g = NULL;
        }
    }
fim:
    free(pos);
    free(nor);
    free(uv);
    free(tan);
    free(pes);
    free(juntas);
    free(indices);
    return r;
}

static bool lume_extensao_permitida(const char *n)
{
    return strcmp(n, "KHR_materials_unlit") == 0 || strcmp(n, "KHR_texture_transform") == 0 ||
           strcmp(n, "KHR_lights_punctual") == 0;
}
static void lume_decompor_matriz(const float *m, LumeVec3 *p, LumeQuat *q, LumeVec3 *s)
{
    float tr;
    p->x = m[12];
    p->y = m[13];
    p->z = m[14];
    s->x = sqrtf(m[0] * m[0] + m[1] * m[1] + m[2] * m[2]);
    s->y = sqrtf(m[4] * m[4] + m[5] * m[5] + m[6] * m[6]);
    s->z = sqrtf(m[8] * m[8] + m[9] * m[9] + m[10] * m[10]);
    {
        float a = m[0] / s->x, b = m[5] / s->y, c = m[10] / s->z;
        tr = a + b + c;
        if (tr > 0)
        {
            float z = sqrtf(tr + 1) * 2;
            q->w = .25f * z;
            q->x = (m[6] / s->y - m[9] / s->z) / z;
            q->y = (m[8] / s->z - m[2] / s->x) / z;
            q->z = (m[1] / s->x - m[4] / s->y) / z;
        }
        else
            *q = lume_quat_identity();
    }
}

static LumeResult lume_animacoes_cgltf(LumeModel *m, cgltf_data *d)
{
    size_t i, j;
    m->quantidade_clipes = d->animations_count;
    m->clipes = calloc(m->quantidade_clipes, sizeof(*m->clipes));
    if (m->quantidade_clipes && !m->clipes)
        return LUME_ERROR_OUT_OF_MEMORY;
    for (i = 0; i < m->quantidade_clipes; ++i)
    {
        cgltf_animation *a = &d->animations[i];
        LumeAnimationClip *c = &m->clipes[i];
        c->nome = lume_copiar_texto(a->name ? a->name : "Animation");
        c->quantidade_canais = a->channels_count;
        c->canais = calloc(c->quantidade_canais, sizeof(*c->canais));
        if (c->quantidade_canais && !c->canais)
            return LUME_ERROR_OUT_OF_MEMORY;
        for (j = 0; j < c->quantidade_canais; ++j)
        {
            cgltf_animation_channel *o = &a->channels[j];
            LumeCanalAnimacao *x = &c->canais[j];
            cgltf_accessor *entrada = o->sampler->input, *saida = o->sampler->output;
            size_t total;
            if (!entrada || !saida || !o->target_node)
                return lume_definir_erro(LUME_ERROR_PARSE, "model.animation", m->caminho, 0, 0,
                                         "A glTF animation channel is incomplete.");
            x->indice_no = (uint32_t)(o->target_node - d->nodes);
            x->caminho = o->target_path == cgltf_animation_path_type_rotation  ? LUME_ANIMACAO_ROTACAO
                         : o->target_path == cgltf_animation_path_type_scale   ? LUME_ANIMACAO_ESCALA
                         : o->target_path == cgltf_animation_path_type_weights ? LUME_ANIMACAO_PESOS
                                                                               : LUME_ANIMACAO_TRANSLACAO;
            x->componentes = x->caminho == LUME_ANIMACAO_ROTACAO ? 4
                             : x->caminho == LUME_ANIMACAO_PESOS ? (uint32_t)(saida->count / entrada->count)
                                                                 : 3;
            x->interpolacao = o->sampler->interpolation == cgltf_interpolation_type_step ? LUME_INTERPOLACAO_STEP
                              : o->sampler->interpolation == cgltf_interpolation_type_cubic_spline
                                  ? LUME_INTERPOLACAO_CUBICA
                                  : LUME_INTERPOLACAO_LINEAR;
            x->quantidade_chaves = entrada->count;
            x->tempos = lume_desempacotar(entrada, 1);
            total = saida->count * cgltf_num_components(saida->type);
            x->valores = malloc(total * sizeof(float));
            if (!x->tempos || !x->valores || cgltf_accessor_unpack_floats(saida, x->valores, total) != total)
                return LUME_ERROR_OUT_OF_MEMORY;
            if (x->quantidade_chaves && x->tempos[x->quantidade_chaves - 1] > c->duracao)
                c->duracao = x->tempos[x->quantidade_chaves - 1];
        }
    }
    return LUME_SUCCESS;
}

static LumeResult lume_carregar_gltf(LumeApp *a, const char *caminho, const LumeModelLoadOptions *op, LumeModel *m)
{
    cgltf_options options = {0};
    cgltf_data *d = NULL;
    cgltf_result cr;
    size_t i, j, total = 0, k = 0;
    int *inicio_malha;
    cr = cgltf_parse_file(&options, caminho, &d);
    if (cr != cgltf_result_success)
        return lume_definir_erro(LUME_ERROR_PARSE, "model.parse", caminho, 0, 0,
                                 "Could not parse glTF/GLB file (cgltf result %d).", cr);
    cr = cgltf_load_buffers(&options, d, caminho);
    if (cr != cgltf_result_success)
    {
        cgltf_free(d);
        return lume_definir_erro(LUME_ERROR_IO, "model.buffers", caminho, 0, 0,
                                 "Could not load glTF buffers (cgltf result %d).", cr);
    }
    if (cgltf_validate(d) != cgltf_result_success)
    {
        cgltf_free(d);
        return lume_definir_erro(LUME_ERROR_PARSE, "model.validate", caminho, 0, 0,
                                 "The glTF document failed structural validation.");
    }
    for (i = 0; i < d->extensions_required_count; ++i)
        if (!lume_extensao_permitida(d->extensions_required[i]))
        {
            LumeResult r =
                lume_definir_erro(LUME_ERROR_UNSUPPORTED, "model.extensions", caminho, 0, 0,
                                  "Required glTF extension '%s' is not supported.", d->extensions_required[i]);
            cgltf_free(d);
            return r;
        }
    for (i = 0; i < d->meshes_count; ++i)
        total += d->meshes[i].primitives_count;
    m->quantidade_primitivas = total;
    m->geometrias = calloc(total, sizeof(*m->geometrias));
    m->materiais = calloc(total, sizeof(*m->materiais));
    inicio_malha = malloc(d->meshes_count * sizeof(*inicio_malha));
    if ((total && (!m->geometrias || !m->materiais)) || (d->meshes_count && !inicio_malha))
    {
        cgltf_free(d);
        free(inicio_malha);
        return LUME_ERROR_OUT_OF_MEMORY;
    }
    for (i = 0; i < d->meshes_count; ++i)
    {
        inicio_malha[i] = (int)k;
        for (j = 0; j < d->meshes[i].primitives_count; ++j)
        {
            LumeResult r =
                lume_primitiva_cgltf(a, caminho, &d->meshes[i].primitives[j], &m->geometrias[k], &m->materiais[k]);
            if (r != LUME_SUCCESS)
            {
                free(inicio_malha);
                cgltf_free(d);
                return r;
            }
            ++k;
        }
    }
    m->quantidade_nos = d->nodes_count;
    m->nos = calloc(m->quantidade_nos, sizeof(*m->nos));
    if (m->quantidade_nos && !m->nos)
    {
        free(inicio_malha);
        cgltf_free(d);
        return LUME_ERROR_OUT_OF_MEMORY;
    }
    for (i = 0; i < m->quantidade_nos; ++i)
    {
        cgltf_node *n = &d->nodes[i];
        LumeModeloNo *x = &m->nos[i];
        x->nome = lume_copiar_texto(n->name ? n->name : "Node");
        x->pai = n->parent ? (int)(n->parent - d->nodes) : -1;
        x->primeira_primitiva = n->mesh ? inicio_malha[n->mesh - d->meshes] : -1;
        x->quantidade_primitivas = n->mesh ? (uint32_t)n->mesh->primitives_count : 0;
        x->posicao = (LumeVec3){0, 0, 0};
        x->escala = (LumeVec3){1, 1, 1};
        x->rotacao = lume_quat_identity();
        if (n->has_matrix)
            lume_decompor_matriz(n->matrix, &x->posicao, &x->rotacao, &x->escala);
        else
        {
            if (n->has_translation)
                x->posicao = (LumeVec3){n->translation[0], n->translation[1], n->translation[2]};
            if (n->has_scale)
                x->escala = (LumeVec3){n->scale[0], n->scale[1], n->scale[2]};
            if (n->has_rotation)
                x->rotacao = (LumeQuat){n->rotation[0], n->rotation[1], n->rotation[2], n->rotation[3]};
        }
    }
    free(inicio_malha);
    {
        LumeResult r = lume_animacoes_cgltf(m, d);
        cgltf_free(d);
        (void)op;
        return r;
    }
}

static LumeResult lume_carregar_obj(LumeApp *a, const char *caminho, LumeModel *m)
{
    fastObjMesh *o = fast_obj_read(caminho);
    float *p = NULL, *n = NULL, *uv = NULL;
    uint32_t *ind = NULL;
    size_t triangulos = 0, face, offset = 0, v = 0, k = 0;
    LumeGeometryData d = {0};
    LumeMaterialConfig mc = lume_material_config_default(LUME_MATERIAL_PHONG);
    LumeResult r;
    if (!o)
        return lume_definir_erro(LUME_ERROR_PARSE, "model.parse", caminho, 0, 0, "Could not parse OBJ file.");
    for (face = 0; face < o->face_count; ++face)
        if (o->face_vertices[face] >= 3)
            triangulos += o->face_vertices[face] - 2;
    p = malloc(triangulos * 9 * sizeof(float));
    n = calloc(triangulos * 9, sizeof(float));
    uv = calloc(triangulos * 6, sizeof(float));
    ind = malloc(triangulos * 3 * sizeof(uint32_t));
    if (triangulos && (!p || !n || !uv || !ind))
    {
        r = LUME_ERROR_OUT_OF_MEMORY;
        goto fim;
    }
    for (face = 0; face < o->face_count; ++face)
    {
        unsigned fv = o->face_vertices[face], t;
        for (t = 1; t + 1 < fv; ++t)
        {
            unsigned cantos[3] = {0, t, t + 1}, c;
            for (c = 0; c < 3; ++c)
            {
                fastObjIndex x = o->indices[offset + cantos[c]];
                memcpy(&p[v * 3], &o->positions[x.p * 3], 3 * sizeof(float));
                if (x.n)
                    memcpy(&n[v * 3], &o->normals[x.n * 3], 3 * sizeof(float));
                if (x.t)
                    memcpy(&uv[v * 2], &o->texcoords[x.t * 2], 2 * sizeof(float));
                ind[k++] = (uint32_t)v++;
            }
        }
        offset += fv;
    }
    d.positions = p;
    d.normals = o->normal_count > 1 ? n : NULL;
    d.texture_coordinates = o->texcoord_count > 1 ? uv : NULL;
    d.vertex_count = v;
    d.indices = ind;
    d.index_count = k;
    m->quantidade_primitivas = 1;
    m->geometrias = calloc(1, sizeof(*m->geometrias));
    m->materiais = calloc(1, sizeof(*m->materiais));
    if (!m->geometrias || !m->materiais)
    {
        r = LUME_ERROR_OUT_OF_MEMORY;
        goto fim;
    }
    r = lume_geometry_create(a, &d, &m->geometrias[0]);
    if (r != LUME_SUCCESS)
        goto fim;
    if (o->material_count > 0)
    {
        fastObjMaterial *mat = &o->materials[0];
        mc.base_color = (LumeColor){mat->Kd[0], mat->Kd[1], mat->Kd[2], mat->d};
        mc.shininess = mat->Ns;
        mc.alpha_mode = mat->d < .999f ? LUME_ALPHA_BLEND : LUME_ALPHA_OPAQUE;
        if (mat->map_Kd && mat->map_Kd < o->texture_count)
        {
            LumeTextureConfig tc = lume_texture_config_default();
            tc.flip_y = false;
            (void)lume_texture_load(a, o->textures[mat->map_Kd].path, &tc, &mc.base_color_texture);
        }
    }
    r = lume_material_create(a, &mc, &m->materiais[0]);
    lume_texture_release(mc.base_color_texture);
    if (r != LUME_SUCCESS)
        goto fim;
    m->quantidade_nos = 1;
    m->nos = calloc(1, sizeof(*m->nos));
    if (!m->nos)
    {
        r = LUME_ERROR_OUT_OF_MEMORY;
        goto fim;
    }
    m->nos[0] = (LumeModeloNo){
        lume_copiar_texto("OBJ model"), -1, 0, 1, (LumeVec3){0, 0, 0}, (LumeVec3){1, 1, 1}, lume_quat_identity()};
fim:
    free(p);
    free(n);
    free(uv);
    free(ind);
    fast_obj_destroy(o);
    return r;
}

static LumeModel *lume_buscar_cache(LumeApp *a, const char *caminho)
{
    size_t i;
    for (i = 0; i < a->quantidade_cache_modelos; ++i)
        if (strcmp(a->cache_modelos[i]->caminho, caminho) == 0)
            return a->cache_modelos[i];
    return NULL;
}
LumeResult lume_model_load(LumeApp *a, const char *caminho, const LumeModelLoadOptions *opcoes, LumeModel **saida)
{
    LumeModelLoadOptions op = opcoes ? *opcoes : lume_model_load_options_default();
    LumeModel *m;
    LumeResult r;
    const char *ext;
    if (!saida)
        return lume_definir_erro(LUME_ERROR_INVALID_ARGUMENT, "model.load", caminho, 0, 0,
                                 "out_model must not be NULL.");
    *saida = NULL;
    if (!a || !caminho || !*caminho)
        return lume_definir_erro(LUME_ERROR_INVALID_ARGUMENT, "model.load", caminho, 0, 0,
                                 "An application and model path are required.");
    if (op.use_cache && (m = lume_buscar_cache(a, caminho)) != NULL)
    {
        lume_model_retain(m);
        *saida = m;
        return LUME_SUCCESS;
    }
    m = calloc(1, sizeof(*m));
    if (!m)
        return lume_definir_erro(LUME_ERROR_OUT_OF_MEMORY, "model.load", caminho, 0, 0,
                                 "Out of memory while creating a model.");
    m->referencia = (LumeReferencia){1, a, lume_destruir_modelo, "model"};
    m->caminho = lume_copiar_texto(caminho);
    m->opcoes = op;
    m->modificacao = lume_modificacao_arquivo(caminho);
    ext = lume_extensao(caminho);
    if (lume_texto_igual_sem_caixa(ext, ".gltf") || lume_texto_igual_sem_caixa(ext, ".glb"))
        r = lume_carregar_gltf(a, caminho, &op, m);
    else if (lume_texto_igual_sem_caixa(ext, ".obj"))
        r = lume_carregar_obj(a, caminho, m);
    else
        r = lume_definir_erro(LUME_ERROR_UNSUPPORTED, "model.load", caminho, 0, 0,
                              "Model format '%s' is not supported. Use .gltf, .glb, or .obj.", ext);
    if (r != LUME_SUCCESS)
    {
        lume_destruir_modelo(m);
        return r;
    }
    lume_registrar_recurso(a, m);
    if (op.use_cache)
    {
        lume_model_retain(m);
        if (!lume_adicionar_ponteiro((void ***)&a->cache_modelos, &a->quantidade_cache_modelos,
                                     &a->capacidade_cache_modelos, m))
        {
            lume_model_release(m);
            lume_model_release(m);
            return LUME_ERROR_OUT_OF_MEMORY;
        }
    }
    *saida = m;
    return LUME_SUCCESS;
}

static LumeResult lume_construir_instancia(LumeModelInstance *i)
{
    size_t n, p;
    i->nos = calloc(i->modelo->quantidade_nos, sizeof(*i->nos));
    i->quantidade_nos = i->modelo->quantidade_nos;
    if (i->quantidade_nos && !i->nos)
        return LUME_ERROR_OUT_OF_MEMORY;
    for (n = 0; n < i->quantidade_nos; ++n)
    {
        LumeModeloNo *o = &i->modelo->nos[n];
        LumeNode *no;
        if (lume_node_create(i->cena, &no) != LUME_SUCCESS)
            return lume_error_last()->code;
        i->nos[n] = no;
        lume_node_set_name(no, o->nome);
        lume_node_set_position(no, o->posicao);
        lume_node_set_rotation(no, o->rotacao);
        lume_node_set_scale(no, o->escala);
        if (o->pai >= 0)
            lume_node_add_child(i->nos[o->pai], no);
        else
            lume_node_add_child(i->raiz, no);
        for (p = 0; p < o->quantidade_primitivas; ++p)
        {
            LumeNode *malha;
            if (lume_mesh_create(i->cena, i->modelo->geometrias[o->primeira_primitiva + (int)p],
                                 i->modelo->materiais[o->primeira_primitiva + (int)p], &malha) != LUME_SUCCESS)
                return lume_error_last()->code;
            lume_node_add_child(no, malha);
        }
    }
    return LUME_SUCCESS;
}
LumeResult lume_model_instantiate(LumeModel *m, LumeScene *c, LumeModelInstance **saida)
{
    LumeModelInstance *i;
    LumeResult r;
    if (!saida)
        return lume_definir_erro(LUME_ERROR_INVALID_ARGUMENT, "model.instantiate", NULL, 0, 0,
                                 "out_instance must not be NULL.");
    *saida = NULL;
    if (!m || !c || m->referencia.aplicativo != c->aplicativo)
        return lume_definir_erro(LUME_ERROR_INVALID_ARGUMENT, "model.instantiate", NULL, 0, 0,
                                 "Model and scene must belong to the same application.");
    i = calloc(1, sizeof(*i));
    if (!i)
        return lume_definir_erro(LUME_ERROR_OUT_OF_MEMORY, "model.instantiate", NULL, 0, 0,
                                 "Out of memory while creating a model instance.");
    i->modelo = m;
    i->cena = c;
    lume_model_retain(m);
    if (lume_node_create(c, &i->raiz) != LUME_SUCCESS)
    {
        lume_model_release(m);
        free(i);
        return lume_error_last()->code;
    }
    lume_node_set_name(i->raiz, "Model instance");
    r = lume_construir_instancia(i);
    if (r != LUME_SUCCESS)
    {
        lume_node_destroy(i->raiz);
        lume_model_release(m);
        free(i->nos);
        free(i);
        return r;
    }
    i->proxima = m->instancias;
    m->instancias = i;
    *saida = i;
    return LUME_SUCCESS;
}
LumeNode *lume_model_instance_root(LumeModelInstance *i)
{
    return i ? i->raiz : NULL;
}
void lume_model_instance_destroy(LumeModelInstance *i)
{
    LumeModelInstance **p;
    if (!i)
        return;
    p = &i->modelo->instancias;
    while (*p && *p != i)
        p = &(*p)->proxima;
    if (*p)
        *p = i->proxima;
    lume_node_destroy(i->raiz);
    free(i->nos);
    lume_model_release(i->modelo);
    free(i);
}

static void lume_destruir_trabalho(void *recurso)
{
    LumeAssetJob *j = recurso;
    LumeApp *a = j->referencia.aplicativo;
    if (j->thread_iniciada)
    {
        int resultado;
        thrd_join(j->thread, &resultado);
        j->thread_iniciada = false;
    }
    lume_desregistrar_recurso(a, j);
    lume_remover_ponteiro((void **)a->trabalhos, &a->quantidade_trabalhos, j);
    lume_model_release(j->modelo);
    free(j->dados);
    free(j->caminho);
    free(j);
}
void lume_asset_job_retain(LumeAssetJob *j)
{
    if (j)
        lume_referencia_reter(&j->referencia);
}
void lume_asset_job_release(LumeAssetJob *j)
{
    if (j && lume_referencia_liberar(&j->referencia) == 0)
        lume_destruir_trabalho(j);
}
static void lume_estado_trabalho(LumeAssetJob *j, LumeAssetJobState e)
{
    lume_atomico_escrever(&j->estado, (int)e);
}
static int lume_thread_carregar(void *arg)
{
    LumeAssetJob *j = arg;
    FILE *f;
    long n;
    lume_estado_trabalho(j, LUME_ASSET_JOB_LOADING);
    f = fopen(j->caminho, "rb");
    if (!f)
    {
        j->erro = (LumeError){LUME_ERROR_IO, "model.load_async", j->caminho, 0, 0, "Could not open the model file."};
        lume_estado_trabalho(j, LUME_ASSET_JOB_FAILED);
        return 0;
    }
    fseek(f, 0, SEEK_END);
    n = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (n < 0)
    {
        fclose(f);
        lume_estado_trabalho(j, LUME_ASSET_JOB_FAILED);
        return 0;
    }
    j->dados = malloc((size_t)n);
    if (n && !j->dados)
    {
        fclose(f);
        j->erro = (LumeError){LUME_ERROR_OUT_OF_MEMORY,
                              "model.load_async",
                              j->caminho,
                              0,
                              0,
                              "Out of memory while reading the model file."};
        lume_estado_trabalho(j, LUME_ASSET_JOB_FAILED);
        return 0;
    }
    j->tamanho = (size_t)n;
    if (n && fread(j->dados, 1, (size_t)n, f) != (size_t)n)
    {
        fclose(f);
        j->erro =
            (LumeError){LUME_ERROR_IO, "model.load_async", j->caminho, 0, 0, "Could not read the complete model file."};
        lume_estado_trabalho(j, LUME_ASSET_JOB_FAILED);
        return 0;
    }
    fclose(f);
    if (lume_atomico_ler(&j->cancelar))
        lume_estado_trabalho(j, LUME_ASSET_JOB_CANCELLED);
    else
    {
        j->progresso = .8f;
        lume_estado_trabalho(j, LUME_ASSET_JOB_READY_FOR_FINALIZE);
    }
    return 0;
}
LumeResult lume_model_load_async(LumeApp *a, const char *caminho, const LumeModelLoadOptions *op, LumeAssetJob **saida)
{
    LumeAssetJob *j;
    if (!saida)
        return lume_definir_erro(LUME_ERROR_INVALID_ARGUMENT, "model.load_async", caminho, 0, 0,
                                 "out_job must not be NULL.");
    *saida = NULL;
    if (!a || !caminho)
        return lume_definir_erro(LUME_ERROR_INVALID_ARGUMENT, "model.load_async", caminho, 0, 0,
                                 "An application and path are required.");
    j = calloc(1, sizeof(*j));
    if (!j)
        return lume_definir_erro(LUME_ERROR_OUT_OF_MEMORY, "model.load_async", caminho, 0, 0,
                                 "Out of memory while creating an asset job.");
    j->referencia = (LumeReferencia){2, a, lume_destruir_trabalho, "asset job"};
    j->estado = LUME_ASSET_JOB_QUEUED;
    j->caminho = lume_copiar_texto(caminho);
    j->opcoes = op ? *op : lume_model_load_options_default();
    lume_registrar_recurso(a, j);
    if (!lume_adicionar_ponteiro((void ***)&a->trabalhos, &a->quantidade_trabalhos, &a->capacidade_trabalhos, j))
    {
        j->referencia.contagem = 1;
        lume_asset_job_release(j);
        return LUME_ERROR_OUT_OF_MEMORY;
    }
    if (thrd_create(&j->thread, lume_thread_carregar, j) != thrd_success)
    {
        j->referencia.contagem = 1;
        lume_asset_job_release(j);
        return lume_definir_erro(LUME_ERROR_INTERNAL, "model.load_async", caminho, 0, 0,
                                 "Could not start the asset worker thread.");
    }
    j->thread_iniciada = true;
    *saida = j;
    return LUME_SUCCESS;
}
LumeAssetJobState lume_asset_job_state(const LumeAssetJob *j)
{
    return j ? (LumeAssetJobState)lume_atomico_ler(&j->estado) : LUME_ASSET_JOB_FAILED;
}
float lume_asset_job_progress(const LumeAssetJob *j)
{
    return j ? j->progresso : 0;
}
void lume_asset_job_cancel(LumeAssetJob *j)
{
    if (j)
        lume_atomico_escrever(&j->cancelar, 1);
}
const LumeError *lume_asset_job_error(const LumeAssetJob *j)
{
    return j ? &j->erro : NULL;
}
LumeResult lume_asset_job_take_model(LumeAssetJob *j, LumeModel **saida)
{
    if (!saida)
        return lume_definir_erro(LUME_ERROR_INVALID_ARGUMENT, "asset_job.take_model", NULL, 0, 0,
                                 "out_model must not be NULL.");
    *saida = NULL;
    if (!j)
        return LUME_ERROR_INVALID_ARGUMENT;
    if (lume_asset_job_state(j) == LUME_ASSET_JOB_FAILED)
    {
        lume_definir_erro(j->erro.code, j->erro.operation, j->erro.path, j->erro.line, j->erro.column, "%s",
                          j->erro.message);
        return j->erro.code;
    }
    if (lume_asset_job_state(j) == LUME_ASSET_JOB_CANCELLED)
        return LUME_ERROR_CANCELLED;
    if (lume_asset_job_state(j) != LUME_ASSET_JOB_COMPLETE)
        return LUME_ERROR_NOT_READY;
    *saida = j->modelo;
    j->modelo = NULL;
    return LUME_SUCCESS;
}

void lume_processar_trabalhos(LumeApp *a)
{
    size_t i = 0;
    while (i < a->quantidade_trabalhos)
    {
        LumeAssetJob *j = a->trabalhos[i];
        LumeAssetJobState e = lume_asset_job_state(j);
        if (e == LUME_ASSET_JOB_READY_FOR_FINALIZE)
        {
            if (lume_atomico_ler(&j->cancelar))
                lume_estado_trabalho(j, LUME_ASSET_JOB_CANCELLED);
            else
            {
                LumeResult r = lume_model_load(a, j->caminho, &j->opcoes, &j->modelo);
                if (r == LUME_SUCCESS)
                {
                    j->progresso = 1;
                    lume_estado_trabalho(j, LUME_ASSET_JOB_COMPLETE);
                }
                else
                {
                    j->erro = *lume_error_last();
                    lume_estado_trabalho(j, LUME_ASSET_JOB_FAILED);
                }
            }
            e = lume_asset_job_state(j);
        }
        if (e == LUME_ASSET_JOB_COMPLETE || e == LUME_ASSET_JOB_FAILED || e == LUME_ASSET_JOB_CANCELLED)
        {
            lume_remover_ponteiro((void **)a->trabalhos, &a->quantidade_trabalhos, j);
            lume_asset_job_release(j);
            continue;
        }
        ++i;
    }
}
void lume_assets_set_reload_callback(LumeApp *a, LumeReloadCallback cb, void *dados)
{
    if (a)
    {
        a->retorno_recarga = cb;
        a->dados_recarga = dados;
    }
}
void lume_assets_clear_cache(LumeApp *a)
{
    LumeModel **itens;
    size_t q, i;
    if (!a)
        return;
    itens = a->cache_modelos;
    q = a->quantidade_cache_modelos;
    a->cache_modelos = NULL;
    a->quantidade_cache_modelos = a->capacidade_cache_modelos = 0;
    for (i = 0; i < q; ++i)
        lume_model_release(itens[i]);
    free(itens);
}

static void lume_reconstruir_instancias(LumeModel *m)
{
    LumeModelInstance *i = m->instancias;
    while (i)
    {
        LumeVec3 p = lume_node_position(i->raiz);
        LumeQuat r = lume_node_rotation(i->raiz);
        LumeVec3 s = lume_node_scale(i->raiz);
        size_t n;
        for (n = 0; n < i->quantidade_nos; ++n)
            if (m->nos[n].pai < 0 && i->nos[n])
                lume_node_destroy(i->nos[n]);
        free(i->nos);
        i->nos = NULL;
        i->quantidade_nos = 0;
        (void)lume_construir_instancia(i);
        lume_node_set_position(i->raiz, p);
        lume_node_set_rotation(i->raiz, r);
        lume_node_set_scale(i->raiz, s);
        i = i->proxima;
    }
}
void lume_processar_recarga(LumeApp *a, float delta)
{
    size_t i;
    a->acumulador_hot_reload += delta;
    if (a->acumulador_hot_reload < a->intervalo_hot_reload)
        return;
    a->acumulador_hot_reload = 0;
    for (i = 0; i < a->quantidade_cache_modelos; ++i)
    {
        LumeModel *m = a->cache_modelos[i];
        long long mod;
        if (!m->opcoes.hot_reload || (mod = lume_modificacao_arquivo(m->caminho)) <= m->modificacao)
            continue;
        {
            LumeModelLoadOptions op = m->opcoes;
            LumeModel *novo = NULL;
            LumeResult r;
            LumeGeometry **g;
            LumeMaterial **mat;
            LumeModeloNo *nos;
            LumeAnimationClip *clips;
            size_t qp, qn, qc;
            op.use_cache = false;
            op.hot_reload = false;
            r = lume_model_load(a, m->caminho, &op, &novo);
            if (r == LUME_SUCCESS)
            {
                g = m->geometrias;
                mat = m->materiais;
                nos = m->nos;
                clips = m->clipes;
                qp = m->quantidade_primitivas;
                qn = m->quantidade_nos;
                qc = m->quantidade_clipes;
                m->geometrias = novo->geometrias;
                m->materiais = novo->materiais;
                m->nos = novo->nos;
                m->clipes = novo->clipes;
                m->quantidade_primitivas = novo->quantidade_primitivas;
                m->quantidade_nos = novo->quantidade_nos;
                m->quantidade_clipes = novo->quantidade_clipes;
                m->modificacao = mod;
                novo->geometrias = g;
                novo->materiais = mat;
                novo->nos = nos;
                novo->clipes = clips;
                novo->quantidade_primitivas = qp;
                novo->quantidade_nos = qn;
                novo->quantidade_clipes = qc;
                lume_reconstruir_instancias(m);
                lume_model_release(novo);
            }
            if (a->retorno_recarga)
                a->retorno_recarga(m->caminho, r, r == LUME_SUCCESS ? NULL : lume_error_last(), a->dados_recarga);
        }
    }
}
