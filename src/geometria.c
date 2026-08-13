#include "lume_interno.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

static void lume_calcular_normais(LumeVertice *vertices, size_t quantidade_vertices, const uint32_t *indices,
                                  size_t quantidade_indices)
{
    size_t indice;
    for (indice = 0; indice < quantidade_vertices; ++indice)
    {
        vertices[indice].normal[0] = 0.0f;
        vertices[indice].normal[1] = 0.0f;
        vertices[indice].normal[2] = 0.0f;
    }

    for (indice = 0; indice + 2 < quantidade_indices; indice += 3)
    {
        uint32_t ia = indices[indice];
        uint32_t ib = indices[indice + 1];
        uint32_t ic = indices[indice + 2];
        LumeVec3 a = {vertices[ia].posicao[0], vertices[ia].posicao[1], vertices[ia].posicao[2]};
        LumeVec3 b = {vertices[ib].posicao[0], vertices[ib].posicao[1], vertices[ib].posicao[2]};
        LumeVec3 c = {vertices[ic].posicao[0], vertices[ic].posicao[1], vertices[ic].posicao[2]};
        LumeVec3 normal = lume_vetor3_produto_vetorial(lume_vetor3_subtrair(b, a), lume_vetor3_subtrair(c, a));
        uint32_t vertices_triangulo[3] = {ia, ib, ic};
        int vertice;
        for (vertice = 0; vertice < 3; ++vertice)
        {
            vertices[vertices_triangulo[vertice]].normal[0] += normal.x;
            vertices[vertices_triangulo[vertice]].normal[1] += normal.y;
            vertices[vertices_triangulo[vertice]].normal[2] += normal.z;
        }
    }

    for (indice = 0; indice < quantidade_vertices; ++indice)
    {
        LumeVec3 normal = lume_vetor3_normalizar(
            (LumeVec3){vertices[indice].normal[0], vertices[indice].normal[1], vertices[indice].normal[2]});
        vertices[indice].normal[0] = normal.x;
        vertices[indice].normal[1] = normal.y;
        vertices[indice].normal[2] = normal.z;
    }
}

bool lume_enviar_geometria_gpu(LumeGeometry *geometria)
{
    if (!geometria || geometria->enviado_gpu)
    {
        return geometria != NULL;
    }
    glfwMakeContextCurrent(geometria->aplicativo->janela);
    glGenVertexArrays(1, &geometria->vao);
    glGenBuffers(1, &geometria->vbo);
    glGenBuffers(1, &geometria->ebo);
    glBindVertexArray(geometria->vao);
    glBindBuffer(GL_ARRAY_BUFFER, geometria->vbo);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(geometria->quantidade_vertices * sizeof(LumeVertice)),
                 geometria->vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, geometria->ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)(geometria->quantidade_indices * sizeof(uint32_t)),
                 geometria->indices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(LumeVertice), (void *)offsetof(LumeVertice, posicao));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(LumeVertice), (void *)offsetof(LumeVertice, normal));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(LumeVertice), (void *)offsetof(LumeVertice, uv));
    glBindVertexArray(0);
    geometria->enviado_gpu = true;
    return true;
}

LumeGeometry *lume_geometry_create_custom(LumeApp *aplicativo, const LumeGeometryData *dados)
{
    LumeGeometry *geometria;
    size_t indice;
    bool possui_normais;

    if (!aplicativo || !dados || !dados->positions || dados->vertex_count == 0)
    {
        lume_definir_erro("Custom geometry requires an application and at least one vertex position.");
        return NULL;
    }
    if (dados->vertex_count > UINT32_MAX)
    {
        lume_definir_erro("Custom geometry exceeds the 32-bit vertex limit.");
        return NULL;
    }
    if (dados->index_count > 0 && !dados->indices)
    {
        lume_definir_erro("Custom geometry index data is missing.");
        return NULL;
    }
    if (dados->index_count > 0 && dados->index_count % 3 != 0)
    {
        lume_definir_erro("Custom geometry index count must be a multiple of three.");
        return NULL;
    }

    geometria = calloc(1, sizeof(*geometria));
    if (!geometria)
    {
        lume_definir_erro("Out of memory while creating geometry.");
        return NULL;
    }
    geometria->aplicativo = aplicativo;
    geometria->quantidade_vertices = dados->vertex_count;
    geometria->quantidade_indices = dados->index_count > 0 ? dados->index_count : dados->vertex_count;
    if (geometria->quantidade_indices % 3 != 0)
    {
        lume_definir_erro("Non-indexed custom geometry vertex count must be a multiple of three.");
        free(geometria);
        return NULL;
    }
    geometria->vertices = calloc(geometria->quantidade_vertices, sizeof(LumeVertice));
    geometria->indices = malloc(geometria->quantidade_indices * sizeof(uint32_t));
    if (!geometria->vertices || !geometria->indices)
    {
        lume_definir_erro("Out of memory while copying geometry data.");
        free(geometria->vertices);
        free(geometria->indices);
        free(geometria);
        return NULL;
    }

    possui_normais = dados->normals != NULL;
    for (indice = 0; indice < geometria->quantidade_vertices; ++indice)
    {
        memcpy(geometria->vertices[indice].posicao, &dados->positions[indice * 3], 3 * sizeof(float));
        if (possui_normais)
        {
            memcpy(geometria->vertices[indice].normal, &dados->normals[indice * 3], 3 * sizeof(float));
        }
        if (dados->texture_coordinates)
        {
            memcpy(geometria->vertices[indice].uv, &dados->texture_coordinates[indice * 2], 2 * sizeof(float));
        }
    }
    for (indice = 0; indice < geometria->quantidade_indices; ++indice)
    {
        uint32_t valor = dados->indices ? dados->indices[indice] : (uint32_t)indice;
        if (valor >= geometria->quantidade_vertices)
        {
            lume_definir_erro("Geometry index %u is outside the vertex range.", valor);
            free(geometria->vertices);
            free(geometria->indices);
            free(geometria);
            return NULL;
        }
        geometria->indices[indice] = valor;
    }
    if (!possui_normais)
    {
        lume_calcular_normais(geometria->vertices, geometria->quantidade_vertices, geometria->indices,
                              geometria->quantidade_indices);
    }

    if (!lume_adicionar_ponteiro((void ***)&aplicativo->geometrias, &aplicativo->quantidade_geometrias,
                                 &aplicativo->capacidade_geometrias, geometria))
    {
        free(geometria->vertices);
        free(geometria->indices);
        free(geometria);
        return NULL;
    }
    return geometria;
}

LumeGeometry *lume_geometry_create_plane(LumeApp *aplicativo, float largura, float altura)
{
    float x = largura * 0.5f;
    float y = altura * 0.5f;
    const float posicoes[] = {-x, -y, 0, x, -y, 0, x, y, 0, -x, y, 0};
    const float normais[] = {0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1};
    const float uvs[] = {0, 0, 1, 0, 1, 1, 0, 1};
    const uint32_t indices[] = {0, 1, 2, 0, 2, 3};
    LumeGeometryData dados = {posicoes, normais, uvs, 4, indices, 6};
    if (largura <= 0.0f || altura <= 0.0f)
    {
        lume_definir_erro("Plane width and height must be greater than zero.");
        return NULL;
    }
    return lume_geometry_create_custom(aplicativo, &dados);
}

LumeGeometry *lume_geometry_create_box(LumeApp *aplicativo, float largura, float altura, float profundidade)
{
    float x = largura * 0.5f;
    float y = altura * 0.5f;
    float z = profundidade * 0.5f;
    const float posicoes[] = {-x, -y, z,  x, -y, z,  x,  y,  z,  -x, y,  z, x,  -y, -z, -x, -y, -z,
                              -x, y,  -z, x, y,  -z, -x, y,  z,  x,  y,  z, x,  y,  -z, -x, y,  -z,
                              -x, -y, -z, x, -y, -z, x,  -y, z,  -x, -y, z, x,  -y, z,  x,  -y, -z,
                              x,  y,  -z, x, y,  z,  -x, -y, -z, -x, -y, z, -x, y,  z,  -x, y,  -z};
    const float normais[] = {0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0,  0,  -1, 0,  0,  -1, 0,  0,  -1, 0,  0,  -1,
                             0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0,  -1, 0,  0,  -1, 0,  0,  -1, 0,  0,  -1, 0,
                             1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, -1, 0,  0,  -1, 0,  0,  -1, 0,  0,  -1, 0,  0};
    const float uvs[] = {0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1,
                         0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1};
    const uint32_t indices[] = {0,  1,  2,  0,  2,  3,  4,  5,  6,  4,  6,  7,  8,  9,  10, 8,  10, 11,
                                12, 13, 14, 12, 14, 15, 16, 17, 18, 16, 18, 19, 20, 21, 22, 20, 22, 23};
    LumeGeometryData dados = {posicoes, normais, uvs, 24, indices, 36};
    if (largura <= 0.0f || altura <= 0.0f || profundidade <= 0.0f)
    {
        lume_definir_erro("Box dimensions must be greater than zero.");
        return NULL;
    }
    return lume_geometry_create_custom(aplicativo, &dados);
}

LumeGeometry *lume_geometry_create_sphere(LumeApp *aplicativo, float raio, uint32_t segmentos_largura,
                                          uint32_t segmentos_altura)
{
    LumeGeometry *geometria;
    LumeGeometryData dados;
    float *posicoes;
    float *normais;
    float *uvs;
    uint32_t *indices;
    size_t quantidade_vertices;
    size_t quantidade_indices;
    size_t vertice = 0;
    size_t indice = 0;
    uint32_t y;
    uint32_t x;

    if (raio <= 0.0f || segmentos_largura < 3 || segmentos_altura < 2)
    {
        lume_definir_erro("Sphere radius must be positive, with at least 3x2 segments.");
        return NULL;
    }
    quantidade_vertices = (size_t)(segmentos_largura + 1) * (segmentos_altura + 1);
    quantidade_indices = (size_t)segmentos_largura * segmentos_altura * 6;
    posicoes = malloc(quantidade_vertices * 3 * sizeof(float));
    normais = malloc(quantidade_vertices * 3 * sizeof(float));
    uvs = malloc(quantidade_vertices * 2 * sizeof(float));
    indices = malloc(quantidade_indices * sizeof(uint32_t));
    if (!posicoes || !normais || !uvs || !indices)
    {
        lume_definir_erro("Out of memory while generating a sphere.");
        free(posicoes);
        free(normais);
        free(uvs);
        free(indices);
        return NULL;
    }

    for (y = 0; y <= segmentos_altura; ++y)
    {
        float v = (float)y / (float)segmentos_altura;
        float phi = v * 3.1415926535f;
        for (x = 0; x <= segmentos_largura; ++x)
        {
            float u = (float)x / (float)segmentos_largura;
            float theta = u * 6.283185307f;
            float nx = sinf(phi) * cosf(theta);
            float ny = cosf(phi);
            float nz = sinf(phi) * sinf(theta);
            posicoes[vertice * 3] = nx * raio;
            posicoes[vertice * 3 + 1] = ny * raio;
            posicoes[vertice * 3 + 2] = nz * raio;
            normais[vertice * 3] = nx;
            normais[vertice * 3 + 1] = ny;
            normais[vertice * 3 + 2] = nz;
            uvs[vertice * 2] = u;
            uvs[vertice * 2 + 1] = 1.0f - v;
            ++vertice;
        }
    }
    for (y = 0; y < segmentos_altura; ++y)
    {
        for (x = 0; x < segmentos_largura; ++x)
        {
            uint32_t a = y * (segmentos_largura + 1) + x;
            uint32_t b = a + segmentos_largura + 1;
            /* A ordem anti-horária mantém a face externa voltada para fora. */
            indices[indice++] = a;
            indices[indice++] = a + 1;
            indices[indice++] = b;
            indices[indice++] = a + 1;
            indices[indice++] = b + 1;
            indices[indice++] = b;
        }
    }
    dados = (LumeGeometryData){posicoes, normais, uvs, quantidade_vertices, indices, quantidade_indices};
    geometria = lume_geometry_create_custom(aplicativo, &dados);
    free(posicoes);
    free(normais);
    free(uvs);
    free(indices);
    return geometria;
}
