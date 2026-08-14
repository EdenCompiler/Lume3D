#ifndef LUME_INTERNO_H
#define LUME_INTERNO_H

#include <lume/lume.h>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <stdarg.h>
#include <tinycthread.h>

#define LUME_MAX_LUZES_DIRECIONAIS 4
#define LUME_MAX_LUZES_PONTUAIS 32
#define LUME_MAX_LUZES_SPOT 8
#define LUME_TOTAL_TECLAS (GLFW_KEY_LAST + 1)
#define LUME_TOTAL_BOTOES_MOUSE (GLFW_MOUSE_BUTTON_LAST + 1)

typedef enum LumeTipoNo
{
    LUME_NO_VAZIO,
    LUME_NO_MALHA,
    LUME_NO_MALHA_INSTANCIADA,
    LUME_NO_CAMERA_PERSPECTIVA,
    LUME_NO_CAMERA_ORTOGRAFICA,
    LUME_NO_LUZ_AMBIENTE,
    LUME_NO_LUZ_DIRECIONAL,
    LUME_NO_LUZ_PONTUAL,
    LUME_NO_LUZ_SPOT
} LumeTipoNo;

typedef struct LumeReferencia
{
    volatile int contagem;
    LumeApp *aplicativo;
    void (*destruir)(void *recurso);
    const char *nome_tipo;
} LumeReferencia;

typedef struct LumeVertice
{
    float posicao[3];
    float normal[3];
    float uv[2];
    float tangente[4];
    float cor[4];
    uint16_t juntas[4];
    float pesos[4];
} LumeVertice;

struct LumeGeometry
{
    LumeReferencia referencia;
    LumeVertice *vertices;
    uint32_t *indices;
    size_t quantidade_vertices;
    size_t quantidade_indices;
    LumeAabb limites;
    GLuint vao, vbo, ebo;
    bool enviado_gpu;
};

struct LumeTexture
{
    LumeReferencia referencia;
    GLuint identificador;
    int largura, altura;
    bool srgb;
    char *caminho;
};

struct LumeShader
{
    LumeReferencia referencia;
    GLuint programa;
    char *caminho_vertice;
    char *caminho_fragmento;
};

struct LumePipeline
{
    LumeReferencia referencia;
    LumeShader *shader;
    bool teste_profundidade, escrita_profundidade, mistura, descartar_costas;
};

struct LumeMaterial
{
    LumeReferencia referencia;
    LumeMaterialType tipo;
    LumeColor cor_base, cor_emissiva;
    LumeTexture *textura_base, *textura_normal, *textura_metal_rugosidade;
    LumeTexture *textura_oclusao, *textura_emissiva;
    LumePipeline *pipeline;
    float metalico, rugosidade, brilho, corte_alpha;
    LumeAlphaMode modo_alpha;
    bool dupla_face, aramado;
};

struct LumeRenderTarget
{
    LumeReferencia referencia;
    GLuint framebuffer, cor, profundidade;
    int largura, altura;
    bool hdr;
};

struct LumeEnvironment
{
    LumeReferencia referencia;
    LumeTexture *equiretangular;
    float intensidade;
};

typedef struct LumeDadosCameraPerspectiva
{
    float campo_visao, proporcao, plano_proximo, plano_distante;
} LumeDadosCameraPerspectiva;
typedef struct LumeDadosCameraOrtografica
{
    float esquerda, direita, inferior, superior, plano_proximo, plano_distante;
} LumeDadosCameraOrtografica;
typedef struct LumeDadosLuz
{
    LumeColor cor;
    float intensidade, alcance, angulo_interno, angulo_externo;
    LumeVec3 direcao;
    bool projeta_sombra;
} LumeDadosLuz;

struct LumeNode
{
    LumeScene *cena;
    LumeTipoNo tipo;
    char *nome;
    LumeVec3 posicao, escala;
    LumeQuat rotacao;
    LumeMat4 matriz_local, matriz_mundo;
    bool transformacao_suja;
    LumeNode *pai;
    LumeNode **filhos;
    size_t quantidade_filhos, capacidade_filhos;
    union
    {
        struct
        {
            LumeGeometry *geometria;
            LumeMaterial *material;
            LumeMat4 *instancias;
            uint32_t quantidade_instancias, capacidade_instancias;
            GLuint vbo_instancias;
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
    size_t quantidade_nos, capacidade_nos;
};

typedef struct LumePassagemInterna
{
    char *nome;
    LumePipeline *pipeline;
    LumePassPhase fase;
    bool precisa_profundidade, habilitada;
} LumePassagemInterna;

typedef struct LumeLinhaDepuracao
{
    LumeVec3 inicio, fim;
    LumeColor cor;
} LumeLinhaDepuracao;

struct LumeRenderer
{
    LumeApp *aplicativo;
    LumeRendererConfig configuracao;
    GLuint programa_padrao;
    GLuint programa_tom, programa_copia, programa_debug, vao_tela, vao_debug, vbo_debug;
    GLuint programa_sombra;
    GLuint framebuffers_sombra_direcional[3], texturas_sombra_direcional[3];
    GLuint framebuffers_sombra_spot[4], texturas_sombra_spot[4];
    LumeMat4 matrizes_sombra_direcional[3], matrizes_sombra_spot[4];
    LumeVec3 divisoes_sombra;
    int quantidade_sombras_spot;
    GLuint framebuffer_hdr, textura_hdr, framebuffer_ping, textura_ping, profundidade_hdr;
    int largura_hdr, altura_hdr;
    LumeEnvironment *ambiente;
    LumePassagemInterna *passagens;
    size_t quantidade_passagens, capacidade_passagens;
    LumeLinhaDepuracao *linhas;
    size_t quantidade_linhas, capacidade_linhas;
    LumeFrameStats estatisticas;
};

struct LumeApp
{
    GLFWwindow *janela;
    LumeColor cor_limpeza;
    LumeLogCallback retorno_log;
    void *dados_log;
    double tempo_anterior;
    float delta_tempo, intervalo_hot_reload, acumulador_hot_reload;
    uint32_t quantidade_trabalhadores;
    bool teclas[LUME_TOTAL_TECLAS], teclas_anteriores[LUME_TOTAL_TECLAS];
    bool botoes_mouse[LUME_TOTAL_BOTOES_MOUSE], botoes_mouse_anteriores[LUME_TOTAL_BOTOES_MOUSE];
    LumeVec2 posicao_mouse, posicao_mouse_anterior, delta_mouse, rolagem_mouse;
    LumeScene **cenas;
    size_t quantidade_cenas, capacidade_cenas;
    void **recursos;
    size_t quantidade_recursos, capacidade_recursos;
    LumeRenderer *renderizador;
    LumeReloadCallback retorno_recarga;
    void *dados_recarga;
    LumeAssetJob **trabalhos;
    size_t quantidade_trabalhos, capacidade_trabalhos;
    LumeModel **cache_modelos;
    size_t quantidade_cache_modelos, capacidade_cache_modelos;
    unsigned long id_thread_principal;
};

typedef struct LumeModeloNo
{
    char *nome;
    int pai;
    int primeira_primitiva;
    uint32_t quantidade_primitivas;
    LumeVec3 posicao, escala;
    LumeQuat rotacao;
} LumeModeloNo;

typedef enum LumeCaminhoAnimacao
{
    LUME_ANIMACAO_TRANSLACAO,
    LUME_ANIMACAO_ROTACAO,
    LUME_ANIMACAO_ESCALA,
    LUME_ANIMACAO_PESOS
} LumeCaminhoAnimacao;

typedef enum LumeInterpolacaoAnimacao
{
    LUME_INTERPOLACAO_STEP,
    LUME_INTERPOLACAO_LINEAR,
    LUME_INTERPOLACAO_CUBICA
} LumeInterpolacaoAnimacao;

typedef struct LumeCanalAnimacao
{
    uint32_t indice_no;
    LumeCaminhoAnimacao caminho;
    LumeInterpolacaoAnimacao interpolacao;
    float *tempos;
    float *valores;
    size_t quantidade_chaves;
    uint32_t componentes;
} LumeCanalAnimacao;

struct LumeAnimationClip
{
    char *nome;
    float duracao;
    LumeCanalAnimacao *canais;
    size_t quantidade_canais;
};

struct LumeModelInstance
{
    LumeModel *modelo;
    LumeScene *cena;
    LumeNode *raiz;
    LumeNode **nos;
    size_t quantidade_nos;
    LumeModelInstance *proxima;
};

struct LumeModel
{
    LumeReferencia referencia;
    char *caminho;
    LumeModelLoadOptions opcoes;
    LumeGeometry **geometrias;
    LumeMaterial **materiais;
    size_t quantidade_primitivas;
    LumeModeloNo *nos;
    size_t quantidade_nos;
    LumeAnimationClip *clipes;
    size_t quantidade_clipes;
    long long modificacao;
    LumeModelInstance *instancias;
};

struct LumeAssetJob
{
    LumeReferencia referencia;
    volatile int estado;
    volatile int cancelar;
    float progresso;
    char *caminho;
    LumeModelLoadOptions opcoes;
    unsigned char *dados;
    size_t tamanho;
    LumeError erro;
    LumeModel *modelo;
    thrd_t thread;
    bool thread_iniciada;
};

struct LumeAnimationPlayer
{
    LumeModelInstance *instancia;
    LumeAnimationClip *atual, *destino;
    LumeLoopMode repeticao;
    float tempo, velocidade, tempo_transicao, duracao_transicao;
    bool pausado;
};

LumeResult lume_definir_erro(LumeResult codigo, const char *operacao, const char *caminho, int linha, int coluna,
                             const char *formato, ...);
void lume_registrar_log(LumeApp *aplicativo, LumeLogLevel nivel, const char *formato, ...);
bool lume_adicionar_ponteiro(void ***itens, size_t *quantidade, size_t *capacidade, void *item);
void lume_remover_ponteiro(void **itens, size_t *quantidade, const void *item);
char *lume_copiar_texto(const char *texto);
int lume_referencia_reter(LumeReferencia *referencia);
int lume_referencia_liberar(LumeReferencia *referencia);
int lume_atomico_ler(const volatile int *valor);
void lume_atomico_escrever(volatile int *destino, int valor);
void lume_registrar_recurso(LumeApp *aplicativo, void *recurso);
void lume_desregistrar_recurso(LumeApp *aplicativo, void *recurso);

LumeMat4 lume_matriz_transformacao_euler(LumeVec3 posicao, LumeVec3 rotacao, LumeVec3 escala);
LumeVec3 lume_matriz_transformar_direcao(LumeMat4 matriz, LumeVec3 direcao);
LumeNode *lume_criar_no(LumeScene *cena, LumeTipoNo tipo);
void lume_atualizar_matrizes_cena(LumeScene *cena);
bool lume_enviar_geometria_gpu(LumeGeometry *geometria);
LumeResult lume_inicializar_renderizador(LumeApp *aplicativo);
void lume_destruir_renderizador(LumeApp *aplicativo);
void lume_processar_trabalhos(LumeApp *aplicativo);
void lume_processar_recarga(LumeApp *aplicativo, float delta);

#endif
