#ifndef LUME_INTERNO_H
#define LUME_INTERNO_H

#include <lume/lume.h>

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <stdarg.h>

#define LUME_MAX_LUZES_DIRECIONAIS 4
#define LUME_MAX_LUZES_PONTUAIS 4
#define LUME_TOTAL_TECLAS (GLFW_KEY_LAST + 1)
#define LUME_TOTAL_BOTOES_MOUSE (GLFW_MOUSE_BUTTON_LAST + 1)

typedef struct LumeMatriz4
{
    float valor[16];
} LumeMatriz4;

typedef enum LumeTipoNo
{
    LUME_NO_VAZIO,
    LUME_NO_MALHA,
    LUME_NO_CAMERA_PERSPECTIVA,
    LUME_NO_CAMERA_ORTOGRAFICA,
    LUME_NO_LUZ_AMBIENTE,
    LUME_NO_LUZ_DIRECIONAL,
    LUME_NO_LUZ_PONTUAL
} LumeTipoNo;

typedef enum LumeTipoMaterial
{
    LUME_MATERIAL_BASICO,
    LUME_MATERIAL_LAMBERT
} LumeTipoMaterial;

typedef struct LumeVertice
{
    float posicao[3];
    float normal[3];
    float uv[2];
} LumeVertice;

struct LumeGeometry
{
    LumeApp *aplicativo;
    LumeVertice *vertices;
    uint32_t *indices;
    size_t quantidade_vertices;
    size_t quantidade_indices;
    GLuint vao;
    GLuint vbo;
    GLuint ebo;
    bool enviado_gpu;
};

struct LumeTexture
{
    LumeApp *aplicativo;
    GLuint identificador;
    int largura;
    int altura;
};

struct LumeMaterial
{
    LumeApp *aplicativo;
    LumeTipoMaterial tipo;
    LumeColor cor;
    LumeTexture *textura;
    bool aramado;
};

typedef struct LumeDadosCameraPerspectiva
{
    float campo_visao;
    float proporcao;
    float plano_proximo;
    float plano_distante;
} LumeDadosCameraPerspectiva;

typedef struct LumeDadosCameraOrtografica
{
    float esquerda;
    float direita;
    float inferior;
    float superior;
    float plano_proximo;
    float plano_distante;
} LumeDadosCameraOrtografica;

typedef struct LumeDadosLuz
{
    LumeColor cor;
    float intensidade;
    float alcance;
    LumeVec3 direcao;
} LumeDadosLuz;

struct LumeNode
{
    LumeScene *cena;
    LumeTipoNo tipo;
    LumeVec3 posicao;
    LumeVec3 rotacao;
    LumeVec3 escala;
    LumeMatriz4 matriz_local;
    LumeMatriz4 matriz_mundo;
    bool transformacao_suja;
    LumeNode *pai;
    LumeNode **filhos;
    size_t quantidade_filhos;
    size_t capacidade_filhos;
    union
    {
        struct
        {
            LumeGeometry *geometria;
            LumeMaterial *material;
        } malha;
        LumeDadosCameraPerspectiva perspectiva;
        LumeDadosCameraOrtografica ortografica;
        LumeDadosLuz luz;
    } dados;
};

struct LumeScene
{
    LumeApp *aplicativo;
    LumeNode **nos;
    size_t quantidade_nos;
    size_t capacidade_nos;
};

struct LumeApp
{
    GLFWwindow *janela;
    LumeColor cor_limpeza;
    LumeLogCallback retorno_log;
    void *dados_log;
    double tempo_anterior;
    float delta_tempo;
    bool teclas[LUME_TOTAL_TECLAS];
    bool teclas_anteriores[LUME_TOTAL_TECLAS];
    bool botoes_mouse[LUME_TOTAL_BOTOES_MOUSE];
    bool botoes_mouse_anteriores[LUME_TOTAL_BOTOES_MOUSE];
    LumeVec2 posicao_mouse;
    LumeVec2 posicao_mouse_anterior;
    LumeVec2 delta_mouse;
    LumeVec2 rolagem_mouse;
    LumeScene **cenas;
    size_t quantidade_cenas;
    size_t capacidade_cenas;
    LumeGeometry **geometrias;
    size_t quantidade_geometrias;
    size_t capacidade_geometrias;
    LumeMaterial **materiais;
    size_t quantidade_materiais;
    size_t capacidade_materiais;
    LumeTexture **texturas;
    size_t quantidade_texturas;
    size_t capacidade_texturas;
    GLuint programa_shader;
};

void lume_definir_erro(const char *formato, ...);
void lume_registrar_log(LumeApp *aplicativo, LumeLogLevel nivel, const char *formato, ...);
bool lume_adicionar_ponteiro(void ***itens, size_t *quantidade, size_t *capacidade, void *item);
void lume_remover_ponteiro(void **itens, size_t *quantidade, const void *item);

LumeMatriz4 lume_matriz_identidade(void);
LumeMatriz4 lume_matriz_multiplicar(LumeMatriz4 esquerda, LumeMatriz4 direita);
LumeMatriz4 lume_matriz_transformacao(LumeVec3 posicao, LumeVec3 rotacao, LumeVec3 escala);
LumeMatriz4 lume_matriz_perspectiva(float campo_visao, float proporcao, float proximo, float distante);
LumeMatriz4 lume_matriz_ortografica(float esquerda, float direita, float inferior, float superior, float proximo,
                                    float distante);
bool lume_matriz_inverter(LumeMatriz4 matriz, LumeMatriz4 *resultado);
LumeVec3 lume_matriz_transformar_ponto(LumeMatriz4 matriz, LumeVec3 ponto);
LumeVec3 lume_matriz_transformar_direcao(LumeMatriz4 matriz, LumeVec3 direcao);
LumeVec3 lume_vetor3_normalizar(LumeVec3 vetor);
LumeVec3 lume_vetor3_subtrair(LumeVec3 a, LumeVec3 b);
LumeVec3 lume_vetor3_produto_vetorial(LumeVec3 a, LumeVec3 b);
float lume_vetor3_produto_escalar(LumeVec3 a, LumeVec3 b);

LumeNode *lume_criar_no(LumeScene *cena, LumeTipoNo tipo);
void lume_atualizar_matrizes_cena(LumeScene *cena);
bool lume_enviar_geometria_gpu(LumeGeometry *geometria);
bool lume_inicializar_renderizador(LumeApp *aplicativo);
void lume_destruir_renderizador(LumeApp *aplicativo);

#endif
