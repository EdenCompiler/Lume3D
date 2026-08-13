#include "lume_interno.h"

#include <math.h>
#include <stdio.h>

static int falhas = 0;

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
    LumeMatriz4 identidade = lume_matriz_identidade();
    LumeMatriz4 transformacao = lume_matriz_transformacao((LumeVec3){2.0f, 3.0f, 4.0f}, (LumeVec3){0.0f, 0.0f, 0.0f},
                                                          (LumeVec3){1.0f, 1.0f, 1.0f});
    LumeMatriz4 inversa;
    LumeMatriz4 produto;
    LumeVec3 ponto;

    verificar(proximo(identidade.valor[0], 1.0f) && proximo(identidade.valor[15], 1.0f),
              "Identity matrix has the expected diagonal.");
    ponto = lume_matriz_transformar_ponto(transformacao, (LumeVec3){1.0f, 1.0f, 1.0f});
    verificar(proximo(ponto.x, 3.0f) && proximo(ponto.y, 4.0f) && proximo(ponto.z, 5.0f),
              "Transform matrix translates a point.");
    verificar(lume_matriz_inverter(transformacao, &inversa), "Transform matrix can be inverted.");
    produto = lume_matriz_multiplicar(transformacao, inversa);
    verificar(proximo(produto.valor[0], 1.0f) && proximo(produto.valor[5], 1.0f) && proximo(produto.valor[10], 1.0f) &&
                  proximo(produto.valor[15], 1.0f),
              "A matrix multiplied by its inverse is identity.");

    if (falhas == 0)
    {
        puts("All unit tests passed.");
    }
    return falhas == 0 ? 0 : 1;
}
