#include "lume_interno.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_MSC_VER)
#define LUME_LOCAL_THREAD __declspec(thread)
#else
#define LUME_LOCAL_THREAD _Thread_local
#endif

static LUME_LOCAL_THREAD LumeError lume_ultimo_erro = {LUME_SUCCESS, "none", NULL, 0, 0, "No error."};
static int lume_contagem_glfw = 0;

const char *lume_version_string(void)
{
    return LUME_VERSION_STRING;
}
const LumeError *lume_error_last(void)
{
    return &lume_ultimo_erro;
}
void lume_error_clear(void)
{
    lume_ultimo_erro = (LumeError){LUME_SUCCESS, "none", NULL, 0, 0, "No error."};
}

const char *lume_result_string(LumeResult resultado)
{
    static const char *nomes[] = {"Success",     "Invalid argument", "Out of memory", "I/O error", "Parse error",
                                  "Unsupported", "GPU error",        "Cancelled",     "Not ready", "Internal error"};
    return resultado >= LUME_SUCCESS && resultado <= LUME_ERROR_INTERNAL ? nomes[resultado] : "Unknown result";
}

LumeResult lume_definir_erro(LumeResult codigo, const char *operacao, const char *caminho, int linha, int coluna,
                             const char *formato, ...)
{
    va_list argumentos;
    lume_ultimo_erro.code = codigo;
    lume_ultimo_erro.operation = operacao ? operacao : "unknown";
    lume_ultimo_erro.path = caminho;
    lume_ultimo_erro.line = linha;
    lume_ultimo_erro.column = coluna;
    va_start(argumentos, formato);
    vsnprintf(lume_ultimo_erro.message, sizeof(lume_ultimo_erro.message), formato, argumentos);
    va_end(argumentos);
    return codigo;
}

void lume_registrar_log(LumeApp *aplicativo, LumeLogLevel nivel, const char *formato, ...)
{
    char mensagem[1024];
    va_list argumentos;
    va_start(argumentos, formato);
    vsnprintf(mensagem, sizeof(mensagem), formato, argumentos);
    va_end(argumentos);
    if (aplicativo && aplicativo->retorno_log)
        aplicativo->retorno_log(nivel, mensagem, aplicativo->dados_log);
    else
        fprintf(nivel == LUME_LOG_ERROR ? stderr : stdout, "[Lume3D] %s\n", mensagem);
}

bool lume_adicionar_ponteiro(void ***itens, size_t *quantidade, size_t *capacidade, void *item)
{
    void **novos;
    size_t nova;
    if (*quantidade < *capacidade)
    {
        (*itens)[(*quantidade)++] = item;
        return true;
    }
    nova = *capacidade ? *capacidade * 2 : 8;
    novos = realloc(*itens, nova * sizeof(void *));
    if (!novos)
    {
        lume_definir_erro(LUME_ERROR_OUT_OF_MEMORY, "collection.grow", NULL, 0, 0,
                          "Out of memory while growing an internal collection.");
        return false;
    }
    *itens = novos;
    *capacidade = nova;
    (*itens)[(*quantidade)++] = item;
    return true;
}
void lume_remover_ponteiro(void **itens, size_t *quantidade, const void *item)
{
    size_t i;
    for (i = 0; i < *quantidade; ++i)
        if (itens[i] == item)
        {
            itens[i] = itens[*quantidade - 1];
            --*quantidade;
            return;
        }
}
char *lume_copiar_texto(const char *texto)
{
    char *copia;
    size_t tamanho;
    if (!texto)
        return NULL;
    tamanho = strlen(texto) + 1;
    copia = malloc(tamanho);
    if (copia)
        memcpy(copia, texto, tamanho);
    return copia;
}
int lume_referencia_reter(LumeReferencia *r)
{
#if defined(_WIN32)
    return (int)InterlockedIncrement((volatile LONG *)&r->contagem);
#else
    return __atomic_add_fetch(&r->contagem, 1, __ATOMIC_RELAXED);
#endif
}
int lume_referencia_liberar(LumeReferencia *r)
{
#if defined(_WIN32)
    return (int)InterlockedDecrement((volatile LONG *)&r->contagem);
#else
    return __atomic_sub_fetch(&r->contagem, 1, __ATOMIC_ACQ_REL);
#endif
}
void lume_registrar_recurso(LumeApp *a, void *r)
{
    if (a)
        (void)lume_adicionar_ponteiro(&a->recursos, &a->quantidade_recursos, &a->capacidade_recursos, r);
}
void lume_desregistrar_recurso(LumeApp *a, void *r)
{
    if (a)
        lume_remover_ponteiro(a->recursos, &a->quantidade_recursos, r);
}

static void lume_retorno_erro_glfw(int codigo, const char *descricao)
{
    lume_definir_erro(LUME_ERROR_INTERNAL, "glfw", NULL, 0, 0, "GLFW error %d: %s", codigo,
                      descricao ? descricao : "Unknown GLFW error.");
}
static void lume_retorno_tecla(GLFWwindow *j, int t, int cv, int acao, int mods)
{
    LumeApp *a = glfwGetWindowUserPointer(j);
    (void)cv;
    (void)mods;
    if (a && t >= 0 && t < LUME_TOTAL_TECLAS)
        a->teclas[t] = acao != GLFW_RELEASE;
}
static void lume_retorno_botao(GLFWwindow *j, int b, int acao, int mods)
{
    LumeApp *a = glfwGetWindowUserPointer(j);
    (void)mods;
    if (a && b >= 0 && b < LUME_TOTAL_BOTOES_MOUSE)
        a->botoes_mouse[b] = acao != GLFW_RELEASE;
}
static void lume_retorno_cursor(GLFWwindow *j, double x, double y)
{
    LumeApp *a = glfwGetWindowUserPointer(j);
    if (a)
        a->posicao_mouse = (LumeVec2){(float)x, (float)y};
}
static void lume_retorno_rolagem(GLFWwindow *j, double x, double y)
{
    LumeApp *a = glfwGetWindowUserPointer(j);
    if (a)
    {
        a->rolagem_mouse.x += (float)x;
        a->rolagem_mouse.y += (float)y;
    }
}

LumeAppConfig lume_app_config_default(void)
{
    LumeAppConfig c = {0};
    c.title = "Lume3D";
    c.width = 1280;
    c.height = 720;
    c.resizable = true;
    c.visible = true;
    c.vsync = true;
    c.clear_color = (LumeColor){0.04f, 0.05f, 0.08f, 1.0f};
    c.hot_reload_interval_seconds = 0.5f;
    return c;
}

LumeResult lume_app_create(const LumeAppConfig *config, LumeApp **saida)
{
    LumeAppConfig c = config ? *config : lume_app_config_default();
    LumeApp *a;
    double x, y;
    LumeResult resultado;
    if (!saida)
        return lume_definir_erro(LUME_ERROR_INVALID_ARGUMENT, "app.create", NULL, 0, 0, "out_app must not be NULL.");
    *saida = NULL;
    if (c.width <= 0 || c.height <= 0)
        return lume_definir_erro(LUME_ERROR_INVALID_ARGUMENT, "app.create", NULL, 0, 0,
                                 "Window width and height must be greater than zero.");
    glfwSetErrorCallback(lume_retorno_erro_glfw);
    if (lume_contagem_glfw == 0 && !glfwInit())
        return lume_definir_erro(LUME_ERROR_INTERNAL, "app.create", NULL, 0, 0, "GLFW could not be initialized.");
    ++lume_contagem_glfw;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, c.resizable ? GLFW_TRUE : GLFW_FALSE);
    glfwWindowHint(GLFW_VISIBLE, c.visible ? GLFW_TRUE : GLFW_FALSE);
    a = calloc(1, sizeof(*a));
    if (!a)
    {
        resultado = LUME_ERROR_OUT_OF_MEMORY;
        goto falha_memoria;
    }
    a->janela = glfwCreateWindow(c.width, c.height, c.title ? c.title : "Lume3D", NULL, NULL);
    if (!a->janela)
    {
        free(a);
        resultado = LUME_ERROR_INTERNAL;
        goto falha;
    }
    a->cor_limpeza = c.clear_color;
    a->retorno_log = c.log_callback;
    a->dados_log = c.log_user_data;
    a->intervalo_hot_reload = c.hot_reload_interval_seconds > 0 ? c.hot_reload_interval_seconds : 0.5f;
    a->quantidade_trabalhadores = c.worker_count ? c.worker_count : 1;
    glfwMakeContextCurrent(a->janela);
    glfwSwapInterval(c.vsync ? 1 : 0);
    if (!gladLoadGL((GLADloadfunc)glfwGetProcAddress))
    {
        glfwDestroyWindow(a->janela);
        free(a);
        resultado = LUME_ERROR_GPU;
        goto falha;
    }
    glfwSetWindowUserPointer(a->janela, a);
    glfwSetKeyCallback(a->janela, lume_retorno_tecla);
    glfwSetMouseButtonCallback(a->janela, lume_retorno_botao);
    glfwSetCursorPosCallback(a->janela, lume_retorno_cursor);
    glfwSetScrollCallback(a->janela, lume_retorno_rolagem);
    glfwGetCursorPos(a->janela, &x, &y);
    a->posicao_mouse = (LumeVec2){(float)x, (float)y};
    a->posicao_mouse_anterior = a->posicao_mouse;
    a->tempo_anterior = glfwGetTime();
    resultado = lume_inicializar_renderizador(a);
    if (resultado != LUME_SUCCESS)
    {
        glfwDestroyWindow(a->janela);
        free(a);
        goto falha;
    }
    *saida = a;
    lume_error_clear();
    lume_registrar_log(a, LUME_LOG_INFO, "Lume3D %s initialized with OpenGL %s.", LUME_VERSION_STRING,
                       glGetString(GL_VERSION));
    return LUME_SUCCESS;
falha_memoria:
    lume_definir_erro(resultado, "app.create", NULL, 0, 0, "Out of memory while creating the application.");
falha:
    --lume_contagem_glfw;
    if (lume_contagem_glfw == 0)
        glfwTerminate();
    return resultado;
}

void lume_app_destroy(LumeApp *a)
{
    if (!a)
        return;
    glfwMakeContextCurrent(a->janela);
    while (a->quantidade_trabalhos > 0)
        lume_asset_job_release(a->trabalhos[a->quantidade_trabalhos - 1]);
    lume_assets_clear_cache(a);
    while (a->quantidade_cenas > 0)
        lume_scene_destroy(a->cenas[a->quantidade_cenas - 1]);
    while (a->quantidade_recursos > 0)
    {
        LumeReferencia *r = a->recursos[a->quantidade_recursos - 1];
        lume_registrar_log(a, LUME_LOG_WARNING,
                           "Leaked %s resource with %d reference(s); releasing it during shutdown.",
                           r->nome_tipo ? r->nome_tipo : "unknown", r->contagem);
        r->contagem = 1;
        if (r->destruir)
            r->destruir(r);
        else
            --a->quantidade_recursos;
    }
    free(a->trabalhos);
    free(a->cache_modelos);
    free(a->cenas);
    free(a->recursos);
    lume_destruir_renderizador(a);
    glfwDestroyWindow(a->janela);
    free(a);
    --lume_contagem_glfw;
    if (lume_contagem_glfw == 0)
        glfwTerminate();
}

LumeRenderer *lume_app_renderer(LumeApp *a)
{
    return a ? a->renderizador : NULL;
}
bool lume_app_should_close(const LumeApp *a)
{
    return !a || glfwWindowShouldClose(a->janela) != 0;
}
void lume_app_request_close(LumeApp *a)
{
    if (a)
        glfwSetWindowShouldClose(a->janela, GLFW_TRUE);
}
float lume_app_begin_frame(LumeApp *a)
{
    double agora;
    if (!a)
        return 0;
    memcpy(a->teclas_anteriores, a->teclas, sizeof(a->teclas));
    memcpy(a->botoes_mouse_anteriores, a->botoes_mouse, sizeof(a->botoes_mouse));
    a->posicao_mouse_anterior = a->posicao_mouse;
    a->rolagem_mouse = (LumeVec2){0, 0};
    glfwPollEvents();
    a->delta_mouse =
        (LumeVec2){a->posicao_mouse.x - a->posicao_mouse_anterior.x, a->posicao_mouse.y - a->posicao_mouse_anterior.y};
    agora = glfwGetTime();
    a->delta_tempo = (float)(agora - a->tempo_anterior);
    a->tempo_anterior = agora;
    lume_processar_trabalhos(a);
    lume_processar_recarga(a, a->delta_tempo);
    return a->delta_tempo;
}
void lume_app_end_frame(LumeApp *a)
{
    if (a)
        glfwSwapBuffers(a->janela);
}
void lume_app_set_clear_color(LumeApp *a, LumeColor c)
{
    if (a)
        a->cor_limpeza = c;
}
void lume_app_get_framebuffer_size(const LumeApp *a, int *l, int *h)
{
    int x = 0, y = 0;
    if (a)
        glfwGetFramebufferSize(a->janela, &x, &y);
    if (l)
        *l = x;
    if (h)
        *h = y;
}
static bool lume_tecla_valida(int t)
{
    return t >= 0 && t < LUME_TOTAL_TECLAS;
}
bool lume_key_is_down(const LumeApp *a, LumeKey t)
{
    return a && lume_tecla_valida(t) && a->teclas[t];
}
bool lume_key_was_pressed(const LumeApp *a, LumeKey t)
{
    return a && lume_tecla_valida(t) && a->teclas[t] && !a->teclas_anteriores[t];
}
bool lume_key_was_released(const LumeApp *a, LumeKey t)
{
    return a && lume_tecla_valida(t) && !a->teclas[t] && a->teclas_anteriores[t];
}
static bool lume_botao_valido(int b)
{
    return b >= 0 && b < LUME_TOTAL_BOTOES_MOUSE;
}
bool lume_mouse_button_is_down(const LumeApp *a, LumeMouseButton b)
{
    return a && lume_botao_valido(b) && a->botoes_mouse[b];
}
bool lume_mouse_button_was_pressed(const LumeApp *a, LumeMouseButton b)
{
    return a && lume_botao_valido(b) && a->botoes_mouse[b] && !a->botoes_mouse_anteriores[b];
}
bool lume_mouse_button_was_released(const LumeApp *a, LumeMouseButton b)
{
    return a && lume_botao_valido(b) && !a->botoes_mouse[b] && a->botoes_mouse_anteriores[b];
}
