#include "lume_interno.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

static int falhas;
static void verificar(bool condicao, const char *mensagem)
{
    if (!condicao)
    {
        fprintf(stderr, "FAIL: %s\n", mensagem);
        ++falhas;
    }
}
static bool proximo(float a, float b)
{
    return fabsf(a - b) < 0.0001f;
}

int main(void)
{
    LumeMat4 identidade = lume_mat4_identity();
    LumeMat4 transformacao = lume_mat4_transform((LumeVec3){2, 3, 4}, lume_quat_identity(), (LumeVec3){1, 1, 1});
    LumeMat4 inversa;
    LumeVec3 ponto = lume_mat4_transform_point(transformacao, (LumeVec3){1, 1, 1});
    LumeAabb caixa = {{-1, -1, -1}, {1, 1, 1}};
    LumeRay raio = {{0, 0, 4}, {0, 0, -1}};
    float distancia = 0;
    float mouse_x = 1, mouse_y = 1;
    const LumeError *erro;

    verificar(strcmp(lume_version_string(), "1.5.1") == 0, "Version string reports 1.5.1.");
    verificar(proximo(identidade.values[0], 1) && proximo(identidade.values[15], 1),
              "Identity matrix has the expected diagonal.");
    verificar(proximo(ponto.x, 3) && proximo(ponto.y, 4) && proximo(ponto.z, 5),
              "Transform matrix translates a point.");
    verificar(lume_mat4_inverse(transformacao, &inversa), "Transform matrix can be inverted.");
    verificar(lume_ray_intersect_aabb(raio, caixa, &distancia) && proximo(distancia, 3),
              "Ray intersects an AABB at the expected distance.");
    verificar(!lume_ray_intersect_aabb((LumeRay){{4, 4, 4}, {1, 0, 0}}, caixa, NULL), "Ray correctly misses an AABB.");
    verificar(lume_result_string(LUME_ERROR_PARSE) && strcmp(lume_result_string(LUME_ERROR_PARSE), "Parse error") == 0,
              "Result descriptions are English.");
    lume_mouse_get_position(NULL, &mouse_x, &mouse_y);
    verificar(proximo(mouse_x, 0) && proximo(mouse_y, 0), "Mouse queries safely return zero without an application.");
    lume_definir_erro(LUME_ERROR_PARSE, "test.parse", "asset.gltf", 7, 3, "Invalid test document.");
    erro = lume_error_last();
    verificar(erro->code == LUME_ERROR_PARSE && erro->line == 7 && strcmp(erro->operation, "test.parse") == 0 &&
                  strstr(erro->message, "Invalid test document") != NULL,
              "Structured diagnostics preserve code, location, operation, and English message.");

    if (!falhas)
        puts("All unit tests passed.");
    return falhas ? 1 : 0;
}
