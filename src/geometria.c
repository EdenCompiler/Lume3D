#include "lume_interno.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

static void lume_destruir_geometria(void *recurso)
{
    LumeGeometry *g = recurso;
    LumeApp *a = g->referencia.aplicativo;
    lume_desregistrar_recurso(a, g);
    if (g->vao)
        glDeleteVertexArrays(1, &g->vao);
    if (g->vbo)
        glDeleteBuffers(1, &g->vbo);
    if (g->ebo)
        glDeleteBuffers(1, &g->ebo);
    free(g->vertices);
    free(g->indices);
    free(g);
}
void lume_geometry_retain(LumeGeometry *g)
{
    if (g)
        lume_referencia_reter(&g->referencia);
}
void lume_geometry_release(LumeGeometry *g)
{
    if (g && lume_referencia_liberar(&g->referencia) == 0)
        lume_destruir_geometria(g);
}
LumeAabb lume_geometry_bounds(const LumeGeometry *g)
{
    return g ? g->limites : lume_aabb_empty();
}

static void lume_calcular_normais(LumeVertice *v, size_t q, const uint32_t *ind, size_t qi)
{
    size_t i;
    for (i = 0; i < q; ++i)
        v[i].normal[0] = v[i].normal[1] = v[i].normal[2] = 0;
    for (i = 0; i + 2 < qi; i += 3)
    {
        uint32_t ia = ind[i], ib = ind[i + 1], ic = ind[i + 2];
        LumeVec3 a = {v[ia].posicao[0], v[ia].posicao[1], v[ia].posicao[2]},
                 b = {v[ib].posicao[0], v[ib].posicao[1], v[ib].posicao[2]},
                 c = {v[ic].posicao[0], v[ic].posicao[1], v[ic].posicao[2]};
        LumeVec3 n = lume_vec3_cross(lume_vec3_subtract(b, a), lume_vec3_subtract(c, a));
        uint32_t t[3] = {ia, ib, ic};
        int k;
        for (k = 0; k < 3; ++k)
        {
            v[t[k]].normal[0] += n.x;
            v[t[k]].normal[1] += n.y;
            v[t[k]].normal[2] += n.z;
        }
    }
    for (i = 0; i < q; ++i)
    {
        LumeVec3 n = lume_vec3_normalize((LumeVec3){v[i].normal[0], v[i].normal[1], v[i].normal[2]});
        v[i].normal[0] = n.x;
        v[i].normal[1] = n.y;
        v[i].normal[2] = n.z;
    }
}

bool lume_enviar_geometria_gpu(LumeGeometry *g)
{
    if (!g || g->enviado_gpu)
        return g != NULL;
    glfwMakeContextCurrent(g->referencia.aplicativo->janela);
    glGenVertexArrays(1, &g->vao);
    glGenBuffers(1, &g->vbo);
    glGenBuffers(1, &g->ebo);
    glBindVertexArray(g->vao);
    glBindBuffer(GL_ARRAY_BUFFER, g->vbo);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(g->quantidade_vertices * sizeof(LumeVertice)), g->vertices,
                 GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g->ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)(g->quantidade_indices * sizeof(uint32_t)), g->indices,
                 GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(LumeVertice), (void *)offsetof(LumeVertice, posicao));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(LumeVertice), (void *)offsetof(LumeVertice, normal));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(LumeVertice), (void *)offsetof(LumeVertice, uv));
    glBindVertexArray(0);
    g->enviado_gpu = true;
    return true;
}

LumeResult lume_geometry_create(LumeApp *a, const LumeGeometryData *d, LumeGeometry **saida)
{
    LumeGeometry *g;
    size_t i;
    bool normais;
    if (!saida)
        return lume_definir_erro(LUME_ERROR_INVALID_ARGUMENT, "geometry.create", NULL, 0, 0,
                                 "out_geometry must not be NULL.");
    *saida = NULL;
    if (!a || !d || !d->positions || !d->vertex_count)
        return lume_definir_erro(LUME_ERROR_INVALID_ARGUMENT, "geometry.create", NULL, 0, 0,
                                 "Geometry requires an application and at least one vertex position.");
    if (d->vertex_count > UINT32_MAX)
        return lume_definir_erro(LUME_ERROR_UNSUPPORTED, "geometry.create", NULL, 0, 0,
                                 "Geometry exceeds the 32-bit vertex limit.");
    if ((d->index_count && !d->indices) || (d->index_count ? d->index_count : d->vertex_count) % 3)
        return lume_definir_erro(LUME_ERROR_INVALID_ARGUMENT, "geometry.create", NULL, 0, 0,
                                 "Triangle index or vertex count must be a multiple of three.");
    g = calloc(1, sizeof(*g));
    if (!g)
        return lume_definir_erro(LUME_ERROR_OUT_OF_MEMORY, "geometry.create", NULL, 0, 0,
                                 "Out of memory while creating geometry.");
    g->referencia = (LumeReferencia){1, a, lume_destruir_geometria, "geometry"};
    g->quantidade_vertices = d->vertex_count;
    g->quantidade_indices = d->index_count ? d->index_count : d->vertex_count;
    g->vertices = calloc(g->quantidade_vertices, sizeof(*g->vertices));
    g->indices = malloc(g->quantidade_indices * sizeof(*g->indices));
    g->limites = lume_aabb_empty();
    if (!g->vertices || !g->indices)
    {
        lume_destruir_geometria(g);
        return lume_definir_erro(LUME_ERROR_OUT_OF_MEMORY, "geometry.create", NULL, 0, 0,
                                 "Out of memory while copying geometry data.");
    }
    normais = d->normals != NULL;
    for (i = 0; i < g->quantidade_vertices; ++i)
    {
        LumeVertice *v = &g->vertices[i];
        memcpy(v->posicao, &d->positions[i * 3], 3 * sizeof(float));
        g->limites = lume_aabb_expand_point(g->limites, (LumeVec3){v->posicao[0], v->posicao[1], v->posicao[2]});
        if (normais)
            memcpy(v->normal, &d->normals[i * 3], 3 * sizeof(float));
        if (d->texture_coordinates)
            memcpy(v->uv, &d->texture_coordinates[i * 2], 2 * sizeof(float));
        if (d->tangents)
            memcpy(v->tangente, &d->tangents[i * 4], 4 * sizeof(float));
        else
            v->tangente[3] = 1;
        if (d->colors)
            memcpy(v->cor, &d->colors[i * 4], 4 * sizeof(float));
        else
            v->cor[0] = v->cor[1] = v->cor[2] = v->cor[3] = 1;
        if (d->joints)
            memcpy(v->juntas, &d->joints[i * 4], 4 * sizeof(uint16_t));
        if (d->joint_weights)
            memcpy(v->pesos, &d->joint_weights[i * 4], 4 * sizeof(float));
    }
    for (i = 0; i < g->quantidade_indices; ++i)
    {
        uint32_t x = d->indices ? d->indices[i] : (uint32_t)i;
        if (x >= g->quantidade_vertices)
        {
            lume_destruir_geometria(g);
            return lume_definir_erro(LUME_ERROR_INVALID_ARGUMENT, "geometry.create", NULL, 0, 0,
                                     "Geometry index %u is outside the vertex range.", x);
        }
        g->indices[i] = x;
    }
    if (!normais)
        lume_calcular_normais(g->vertices, g->quantidade_vertices, g->indices, g->quantidade_indices);
    lume_registrar_recurso(a, g);
    *saida = g;
    return LUME_SUCCESS;
}

LumeResult lume_geometry_create_plane(LumeApp *a, float l, float h, LumeGeometry **s)
{
    float x = l * .5f, y = h * .5f;
    const float p[] = {-x, -y, 0, x, -y, 0, x, y, 0, -x, y, 0}, n[] = {0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1},
                uv[] = {0, 0, 1, 0, 1, 1, 0, 1};
    const uint32_t i[] = {0, 1, 2, 0, 2, 3};
    LumeGeometryData d = {0};
    if (l <= 0 || h <= 0)
        return lume_definir_erro(LUME_ERROR_INVALID_ARGUMENT, "geometry.plane", NULL, 0, 0,
                                 "Plane width and height must be greater than zero.");
    d.positions = p;
    d.normals = n;
    d.texture_coordinates = uv;
    d.vertex_count = 4;
    d.indices = i;
    d.index_count = 6;
    return lume_geometry_create(a, &d, s);
}
LumeResult lume_geometry_create_box(LumeApp *a, float l, float h, float pr, LumeGeometry **s)
{
    float x = l * .5f, y = h * .5f, z = pr * .5f;
    const float p[] = {-x, -y, z, x, -y, z,  x, y, z,  -x, y, z,  x,  -y, -z, -x, -y, -z, -x, y,  -z, x,  y,  -z,
                       -x, y,  z, x, y,  z,  x, y, -z, -x, y, -z, -x, -y, -z, x,  -y, -z, x,  -y, z,  -x, -y, z,
                       x,  -y, z, x, -y, -z, x, y, -z, x,  y, z,  -x, -y, -z, -x, -y, z,  -x, y,  z,  -x, y,  -z};
    const float n[] = {0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0,  0,  -1, 0,  0,  -1, 0,  0,  -1, 0,  0,  -1,
                       0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0,  -1, 0,  0,  -1, 0,  0,  -1, 0,  0,  -1, 0,
                       1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, -1, 0,  0,  -1, 0,  0,  -1, 0,  0,  -1, 0,  0};
    const float uv[] = {0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1,
                        0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1};
    const uint32_t i[] = {0,  1,  2,  0,  2,  3,  4,  5,  6,  4,  6,  7,  8,  9,  10, 8,  10, 11,
                          12, 13, 14, 12, 14, 15, 16, 17, 18, 16, 18, 19, 20, 21, 22, 20, 22, 23};
    LumeGeometryData d = {0};
    if (l <= 0 || h <= 0 || pr <= 0)
        return lume_definir_erro(LUME_ERROR_INVALID_ARGUMENT, "geometry.box", NULL, 0, 0,
                                 "Box dimensions must be greater than zero.");
    d.positions = p;
    d.normals = n;
    d.texture_coordinates = uv;
    d.vertex_count = 24;
    d.indices = i;
    d.index_count = 36;
    return lume_geometry_create(a, &d, s);
}
LumeResult lume_geometry_create_sphere(LumeApp *a, float raio, uint32_t sl, uint32_t sh, LumeGeometry **s)
{
    LumeGeometryData d = {0};
    float *p, *n, *uv;
    uint32_t *ind;
    size_t qv, qi, v = 0, k = 0;
    uint32_t y, x;
    LumeResult r;
    if (raio <= 0 || sl < 3 || sh < 2)
        return lume_definir_erro(LUME_ERROR_INVALID_ARGUMENT, "geometry.sphere", NULL, 0, 0,
                                 "Sphere radius must be positive, with at least 3x2 segments.");
    qv = (size_t)(sl + 1) * (sh + 1);
    qi = (size_t)sl * sh * 6;
    p = malloc(qv * 3 * sizeof(float));
    n = malloc(qv * 3 * sizeof(float));
    uv = malloc(qv * 2 * sizeof(float));
    ind = malloc(qi * sizeof(uint32_t));
    if (!p || !n || !uv || !ind)
    {
        free(p);
        free(n);
        free(uv);
        free(ind);
        return lume_definir_erro(LUME_ERROR_OUT_OF_MEMORY, "geometry.sphere", NULL, 0, 0,
                                 "Out of memory while generating a sphere.");
    }
    for (y = 0; y <= sh; ++y)
    {
        float vv = (float)y / sh, phi = vv * 3.1415926535f;
        for (x = 0; x <= sl; ++x)
        {
            float u = (float)x / sl, theta = u * 6.283185307f, nx = sinf(phi) * cosf(theta), ny = cosf(phi),
                  nz = sinf(phi) * sinf(theta);
            p[v * 3] = nx * raio;
            p[v * 3 + 1] = ny * raio;
            p[v * 3 + 2] = nz * raio;
            n[v * 3] = nx;
            n[v * 3 + 1] = ny;
            n[v * 3 + 2] = nz;
            uv[v * 2] = u;
            uv[v * 2 + 1] = 1 - vv;
            ++v;
        }
    }
    for (y = 0; y < sh; ++y)
        for (x = 0; x < sl; ++x)
        {
            uint32_t z = y * (sl + 1) + x, b = z + sl + 1;
            ind[k++] = z;
            ind[k++] = z + 1;
            ind[k++] = b;
            ind[k++] = z + 1;
            ind[k++] = b + 1;
            ind[k++] = b;
        }
    d.positions = p;
    d.normals = n;
    d.texture_coordinates = uv;
    d.vertex_count = qv;
    d.indices = ind;
    d.index_count = qi;
    r = lume_geometry_create(a, &d, s);
    free(p);
    free(n);
    free(uv);
    free(ind);
    return r;
}
