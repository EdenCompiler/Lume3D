#include "lume_interno.h"

#include <math.h>

LumeVec3 lume_vetor3_subtrair(LumeVec3 a, LumeVec3 b)
{
    return (LumeVec3){a.x - b.x, a.y - b.y, a.z - b.z};
}

float lume_vetor3_produto_escalar(LumeVec3 a, LumeVec3 b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

LumeVec3 lume_vetor3_produto_vetorial(LumeVec3 a, LumeVec3 b)
{
    return (LumeVec3){a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

LumeVec3 lume_vetor3_normalizar(LumeVec3 vetor)
{
    float comprimento = sqrtf(lume_vetor3_produto_escalar(vetor, vetor));
    if (comprimento <= 0.000001f)
    {
        return (LumeVec3){0.0f, 0.0f, 0.0f};
    }
    return (LumeVec3){vetor.x / comprimento, vetor.y / comprimento, vetor.z / comprimento};
}

LumeMatriz4 lume_matriz_identidade(void)
{
    LumeMatriz4 matriz = {{0}};
    matriz.valor[0] = 1.0f;
    matriz.valor[5] = 1.0f;
    matriz.valor[10] = 1.0f;
    matriz.valor[15] = 1.0f;
    return matriz;
}

LumeMatriz4 lume_matriz_multiplicar(LumeMatriz4 esquerda, LumeMatriz4 direita)
{
    LumeMatriz4 resultado = {{0}};
    int coluna;
    int linha;
    int indice;

    for (coluna = 0; coluna < 4; ++coluna)
    {
        for (linha = 0; linha < 4; ++linha)
        {
            for (indice = 0; indice < 4; ++indice)
            {
                resultado.valor[coluna * 4 + linha] +=
                    esquerda.valor[indice * 4 + linha] * direita.valor[coluna * 4 + indice];
            }
        }
    }
    return resultado;
}

static LumeMatriz4 lume_matriz_translacao(LumeVec3 posicao)
{
    LumeMatriz4 matriz = lume_matriz_identidade();
    matriz.valor[12] = posicao.x;
    matriz.valor[13] = posicao.y;
    matriz.valor[14] = posicao.z;
    return matriz;
}

static LumeMatriz4 lume_matriz_escala(LumeVec3 escala)
{
    LumeMatriz4 matriz = {{0}};
    matriz.valor[0] = escala.x;
    matriz.valor[5] = escala.y;
    matriz.valor[10] = escala.z;
    matriz.valor[15] = 1.0f;
    return matriz;
}

static LumeMatriz4 lume_matriz_rotacao_x(float angulo)
{
    LumeMatriz4 matriz = lume_matriz_identidade();
    float cosseno = cosf(angulo);
    float seno = sinf(angulo);
    matriz.valor[5] = cosseno;
    matriz.valor[6] = seno;
    matriz.valor[9] = -seno;
    matriz.valor[10] = cosseno;
    return matriz;
}

static LumeMatriz4 lume_matriz_rotacao_y(float angulo)
{
    LumeMatriz4 matriz = lume_matriz_identidade();
    float cosseno = cosf(angulo);
    float seno = sinf(angulo);
    matriz.valor[0] = cosseno;
    matriz.valor[2] = -seno;
    matriz.valor[8] = seno;
    matriz.valor[10] = cosseno;
    return matriz;
}

static LumeMatriz4 lume_matriz_rotacao_z(float angulo)
{
    LumeMatriz4 matriz = lume_matriz_identidade();
    float cosseno = cosf(angulo);
    float seno = sinf(angulo);
    matriz.valor[0] = cosseno;
    matriz.valor[1] = seno;
    matriz.valor[4] = -seno;
    matriz.valor[5] = cosseno;
    return matriz;
}

LumeMatriz4 lume_matriz_transformacao(LumeVec3 posicao, LumeVec3 rotacao, LumeVec3 escala)
{
    LumeMatriz4 matriz = lume_matriz_translacao(posicao);
    matriz = lume_matriz_multiplicar(matriz, lume_matriz_rotacao_z(rotacao.z));
    matriz = lume_matriz_multiplicar(matriz, lume_matriz_rotacao_y(rotacao.y));
    matriz = lume_matriz_multiplicar(matriz, lume_matriz_rotacao_x(rotacao.x));
    return lume_matriz_multiplicar(matriz, lume_matriz_escala(escala));
}

LumeMatriz4 lume_matriz_perspectiva(float campo_visao, float proporcao, float proximo, float distante)
{
    LumeMatriz4 matriz = {{0}};
    float escala = 1.0f / tanf(campo_visao * 0.5f);
    matriz.valor[0] = escala / proporcao;
    matriz.valor[5] = escala;
    matriz.valor[10] = (distante + proximo) / (proximo - distante);
    matriz.valor[11] = -1.0f;
    matriz.valor[14] = (2.0f * distante * proximo) / (proximo - distante);
    return matriz;
}

LumeMatriz4 lume_matriz_ortografica(float esquerda, float direita, float inferior, float superior, float proximo,
                                    float distante)
{
    LumeMatriz4 matriz = lume_matriz_identidade();
    matriz.valor[0] = 2.0f / (direita - esquerda);
    matriz.valor[5] = 2.0f / (superior - inferior);
    matriz.valor[10] = -2.0f / (distante - proximo);
    matriz.valor[12] = -(direita + esquerda) / (direita - esquerda);
    matriz.valor[13] = -(superior + inferior) / (superior - inferior);
    matriz.valor[14] = -(distante + proximo) / (distante - proximo);
    return matriz;
}

bool lume_matriz_inverter(LumeMatriz4 matriz, LumeMatriz4 *resultado)
{
    const float *m = matriz.valor;
    float inversa[16];
    float determinante;
    int indice;

    if (!resultado)
    {
        return false;
    }

    inversa[0] = m[5] * m[10] * m[15] - m[5] * m[11] * m[14] - m[9] * m[6] * m[15] + m[9] * m[7] * m[14] +
                 m[13] * m[6] * m[11] - m[13] * m[7] * m[10];
    inversa[4] = -m[4] * m[10] * m[15] + m[4] * m[11] * m[14] + m[8] * m[6] * m[15] - m[8] * m[7] * m[14] -
                 m[12] * m[6] * m[11] + m[12] * m[7] * m[10];
    inversa[8] = m[4] * m[9] * m[15] - m[4] * m[11] * m[13] - m[8] * m[5] * m[15] + m[8] * m[7] * m[13] +
                 m[12] * m[5] * m[11] - m[12] * m[7] * m[9];
    inversa[12] = -m[4] * m[9] * m[14] + m[4] * m[10] * m[13] + m[8] * m[5] * m[14] - m[8] * m[6] * m[13] -
                  m[12] * m[5] * m[10] + m[12] * m[6] * m[9];
    inversa[1] = -m[1] * m[10] * m[15] + m[1] * m[11] * m[14] + m[9] * m[2] * m[15] - m[9] * m[3] * m[14] -
                 m[13] * m[2] * m[11] + m[13] * m[3] * m[10];
    inversa[5] = m[0] * m[10] * m[15] - m[0] * m[11] * m[14] - m[8] * m[2] * m[15] + m[8] * m[3] * m[14] +
                 m[12] * m[2] * m[11] - m[12] * m[3] * m[10];
    inversa[9] = -m[0] * m[9] * m[15] + m[0] * m[11] * m[13] + m[8] * m[1] * m[15] - m[8] * m[3] * m[13] -
                 m[12] * m[1] * m[11] + m[12] * m[3] * m[9];
    inversa[13] = m[0] * m[9] * m[14] - m[0] * m[10] * m[13] - m[8] * m[1] * m[14] + m[8] * m[2] * m[13] +
                  m[12] * m[1] * m[10] - m[12] * m[2] * m[9];
    inversa[2] = m[1] * m[6] * m[15] - m[1] * m[7] * m[14] - m[5] * m[2] * m[15] + m[5] * m[3] * m[14] +
                 m[13] * m[2] * m[7] - m[13] * m[3] * m[6];
    inversa[6] = -m[0] * m[6] * m[15] + m[0] * m[7] * m[14] + m[4] * m[2] * m[15] - m[4] * m[3] * m[14] -
                 m[12] * m[2] * m[7] + m[12] * m[3] * m[6];
    inversa[10] = m[0] * m[5] * m[15] - m[0] * m[7] * m[13] - m[4] * m[1] * m[15] + m[4] * m[3] * m[13] +
                  m[12] * m[1] * m[7] - m[12] * m[3] * m[5];
    inversa[14] = -m[0] * m[5] * m[14] + m[0] * m[6] * m[13] + m[4] * m[1] * m[14] - m[4] * m[2] * m[13] -
                  m[12] * m[1] * m[6] + m[12] * m[2] * m[5];
    inversa[3] = -m[1] * m[6] * m[11] + m[1] * m[7] * m[10] + m[5] * m[2] * m[11] - m[5] * m[3] * m[10] -
                 m[9] * m[2] * m[7] + m[9] * m[3] * m[6];
    inversa[7] = m[0] * m[6] * m[11] - m[0] * m[7] * m[10] - m[4] * m[2] * m[11] + m[4] * m[3] * m[10] +
                 m[8] * m[2] * m[7] - m[8] * m[3] * m[6];
    inversa[11] = -m[0] * m[5] * m[11] + m[0] * m[7] * m[9] + m[4] * m[1] * m[11] - m[4] * m[3] * m[9] -
                  m[8] * m[1] * m[7] + m[8] * m[3] * m[5];
    inversa[15] = m[0] * m[5] * m[10] - m[0] * m[6] * m[9] - m[4] * m[1] * m[10] + m[4] * m[2] * m[9] +
                  m[8] * m[1] * m[6] - m[8] * m[2] * m[5];

    determinante = m[0] * inversa[0] + m[1] * inversa[4] + m[2] * inversa[8] + m[3] * inversa[12];
    if (fabsf(determinante) <= 0.000001f)
    {
        return false;
    }

    determinante = 1.0f / determinante;
    for (indice = 0; indice < 16; ++indice)
    {
        resultado->valor[indice] = inversa[indice] * determinante;
    }
    return true;
}

LumeVec3 lume_matriz_transformar_ponto(LumeMatriz4 matriz, LumeVec3 ponto)
{
    return (LumeVec3){
        matriz.valor[0] * ponto.x + matriz.valor[4] * ponto.y + matriz.valor[8] * ponto.z + matriz.valor[12],
        matriz.valor[1] * ponto.x + matriz.valor[5] * ponto.y + matriz.valor[9] * ponto.z + matriz.valor[13],
        matriz.valor[2] * ponto.x + matriz.valor[6] * ponto.y + matriz.valor[10] * ponto.z + matriz.valor[14]};
}

LumeVec3 lume_matriz_transformar_direcao(LumeMatriz4 matriz, LumeVec3 direcao)
{
    return (LumeVec3){matriz.valor[0] * direcao.x + matriz.valor[4] * direcao.y + matriz.valor[8] * direcao.z,
                      matriz.valor[1] * direcao.x + matriz.valor[5] * direcao.y + matriz.valor[9] * direcao.z,
                      matriz.valor[2] * direcao.x + matriz.valor[6] * direcao.y + matriz.valor[10] * direcao.z};
}
