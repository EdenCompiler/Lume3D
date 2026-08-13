#include "lume_interno.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char lume_ultimo_erro[1024] = "No error.";
static int lume_contagem_glfw = 0;

void lume_definir_erro(const char *formato, ...)
{
    va_list argumentos;
    va_start(argumentos, formato);
    vsnprintf(lume_ultimo_erro, sizeof(lume_ultimo_erro), formato, argumentos);
    va_end(argumentos);
}

const char *lume_get_last_error(void)
{
    return lume_ultimo_erro;
}

void lume_registrar_log(LumeApp *aplicativo, LumeLogLevel nivel, const char *formato, ...)
{
    char mensagem[1024];
    va_list argumentos;
    va_start(argumentos, formato);
    vsnprintf(mensagem, sizeof(mensagem), formato, argumentos);
    va_end(argumentos);

    if (aplicativo && aplicativo->retorno_log)
    {
        aplicativo->retorno_log(nivel, mensagem, aplicativo->dados_log);
        return;
    }

    fprintf(nivel == LUME_LOG_ERROR ? stderr : stdout, "[Lume3D] %s\n", mensagem);
}

bool lume_adicionar_ponteiro(void ***itens, size_t *quantidade, size_t *capacidade, void *item)
{
    void **novos_itens;
    size_t nova_capacidade;

    if (*quantidade < *capacidade)
    {
        (*itens)[(*quantidade)++] = item;
        return true;
    }

    nova_capacidade = *capacidade == 0 ? 8 : *capacidade * 2;
    novos_itens = realloc(*itens, nova_capacidade * sizeof(void *));
    if (!novos_itens)
    {
        lume_definir_erro("Out of memory while growing an internal collection.");
        return false;
    }
    *itens = novos_itens;
    *capacidade = nova_capacidade;
    (*itens)[(*quantidade)++] = item;
    return true;
}

void lume_remover_ponteiro(void **itens, size_t *quantidade, const void *item)
{
    size_t indice;
    for (indice = 0; indice < *quantidade; ++indice)
    {
        if (itens[indice] == item)
        {
            itens[indice] = itens[*quantidade - 1];
            --(*quantidade);
            return;
        }
    }
}

static void lume_retorno_erro_glfw(int codigo, const char *descricao)
{
    lume_definir_erro("GLFW error %d: %s", codigo, descricao ? descricao : "Unknown GLFW error.");
}

static void lume_retorno_tecla(GLFWwindow *janela, int tecla, int codigo_varredura, int acao, int modificadores)
{
    LumeApp *aplicativo = glfwGetWindowUserPointer(janela);
    (void)codigo_varredura;
    (void)modificadores;
    if (aplicativo && tecla >= 0 && tecla < LUME_TOTAL_TECLAS)
    {
        aplicativo->teclas[tecla] = acao != GLFW_RELEASE;
    }
}

static void lume_retorno_botao_mouse(GLFWwindow *janela, int botao, int acao, int modificadores)
{
    LumeApp *aplicativo = glfwGetWindowUserPointer(janela);
    (void)modificadores;
    if (aplicativo && botao >= 0 && botao < LUME_TOTAL_BOTOES_MOUSE)
    {
        aplicativo->botoes_mouse[botao] = acao != GLFW_RELEASE;
    }
}

static void lume_retorno_cursor(GLFWwindow *janela, double x, double y)
{
    LumeApp *aplicativo = glfwGetWindowUserPointer(janela);
    if (aplicativo)
    {
        aplicativo->posicao_mouse = (LumeVec2){(float)x, (float)y};
    }
}

static void lume_retorno_rolagem(GLFWwindow *janela, double x, double y)
{
    LumeApp *aplicativo = glfwGetWindowUserPointer(janela);
    if (aplicativo)
    {
        aplicativo->rolagem_mouse.x += (float)x;
        aplicativo->rolagem_mouse.y += (float)y;
    }
}

LumeAppConfig lume_app_config_default(void)
{
    return (LumeAppConfig){"Lume3D", 1280, 720, true, true, true, {0.04f, 0.05f, 0.08f, 1.0f}, NULL, NULL};
}

LumeApp *lume_app_create(const LumeAppConfig *config)
{
    LumeAppConfig configuracao = config ? *config : lume_app_config_default();
    LumeApp *aplicativo;
    double mouse_x;
    double mouse_y;

    if (configuracao.width <= 0 || configuracao.height <= 0)
    {
        lume_definir_erro("Window width and height must be greater than zero.");
        return NULL;
    }

    glfwSetErrorCallback(lume_retorno_erro_glfw);
    if (lume_contagem_glfw == 0 && !glfwInit())
    {
        if (strcmp(lume_ultimo_erro, "No error.") == 0)
        {
            lume_definir_erro("GLFW could not be initialized.");
        }
        return NULL;
    }
    ++lume_contagem_glfw;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#endif
    glfwWindowHint(GLFW_RESIZABLE, configuracao.resizable ? GLFW_TRUE : GLFW_FALSE);
    glfwWindowHint(GLFW_VISIBLE, configuracao.visible ? GLFW_TRUE : GLFW_FALSE);

    aplicativo = calloc(1, sizeof(*aplicativo));
    if (!aplicativo)
    {
        lume_definir_erro("Out of memory while creating the application.");
        --lume_contagem_glfw;
        if (lume_contagem_glfw == 0)
        {
            glfwTerminate();
        }
        return NULL;
    }

    aplicativo->janela = glfwCreateWindow(configuracao.width, configuracao.height,
                                          configuracao.title ? configuracao.title : "Lume3D", NULL, NULL);
    if (!aplicativo->janela)
    {
        free(aplicativo);
        --lume_contagem_glfw;
        if (lume_contagem_glfw == 0)
        {
            glfwTerminate();
        }
        return NULL;
    }

    aplicativo->cor_limpeza = configuracao.clear_color;
    aplicativo->retorno_log = configuracao.log_callback;
    aplicativo->dados_log = configuracao.log_user_data;
    glfwMakeContextCurrent(aplicativo->janela);
    glfwSwapInterval(configuracao.vsync ? 1 : 0);

    if (!gladLoadGL((GLADloadfunc)glfwGetProcAddress))
    {
        lume_definir_erro("OpenGL functions could not be loaded.");
        glfwDestroyWindow(aplicativo->janela);
        free(aplicativo);
        --lume_contagem_glfw;
        if (lume_contagem_glfw == 0)
        {
            glfwTerminate();
        }
        return NULL;
    }

    glfwSetWindowUserPointer(aplicativo->janela, aplicativo);
    glfwSetKeyCallback(aplicativo->janela, lume_retorno_tecla);
    glfwSetMouseButtonCallback(aplicativo->janela, lume_retorno_botao_mouse);
    glfwSetCursorPosCallback(aplicativo->janela, lume_retorno_cursor);
    glfwSetScrollCallback(aplicativo->janela, lume_retorno_rolagem);
    glfwGetCursorPos(aplicativo->janela, &mouse_x, &mouse_y);
    aplicativo->posicao_mouse = (LumeVec2){(float)mouse_x, (float)mouse_y};
    aplicativo->posicao_mouse_anterior = aplicativo->posicao_mouse;
    aplicativo->tempo_anterior = glfwGetTime();

    if (!lume_inicializar_renderizador(aplicativo))
    {
        glfwDestroyWindow(aplicativo->janela);
        free(aplicativo);
        --lume_contagem_glfw;
        if (lume_contagem_glfw == 0)
        {
            glfwTerminate();
        }
        return NULL;
    }

    lume_registrar_log(aplicativo, LUME_LOG_INFO, "Lume3D 1.0.0 initialized with OpenGL %s.", glGetString(GL_VERSION));
    return aplicativo;
}

void lume_app_destroy(LumeApp *aplicativo)
{
    size_t indice;
    if (!aplicativo)
    {
        return;
    }

    glfwMakeContextCurrent(aplicativo->janela);
    while (aplicativo->quantidade_cenas > 0)
    {
        lume_scene_destroy(aplicativo->cenas[aplicativo->quantidade_cenas - 1]);
    }
    for (indice = 0; indice < aplicativo->quantidade_geometrias; ++indice)
    {
        LumeGeometry *geometria = aplicativo->geometrias[indice];
        if (geometria->vao)
            glDeleteVertexArrays(1, &geometria->vao);
        if (geometria->vbo)
            glDeleteBuffers(1, &geometria->vbo);
        if (geometria->ebo)
            glDeleteBuffers(1, &geometria->ebo);
        free(geometria->vertices);
        free(geometria->indices);
        free(geometria);
    }
    for (indice = 0; indice < aplicativo->quantidade_materiais; ++indice)
    {
        free(aplicativo->materiais[indice]);
    }
    for (indice = 0; indice < aplicativo->quantidade_texturas; ++indice)
    {
        glDeleteTextures(1, &aplicativo->texturas[indice]->identificador);
        free(aplicativo->texturas[indice]);
    }
    free(aplicativo->cenas);
    free(aplicativo->geometrias);
    free(aplicativo->materiais);
    free(aplicativo->texturas);
    lume_destruir_renderizador(aplicativo);
    glfwDestroyWindow(aplicativo->janela);
    free(aplicativo);

    --lume_contagem_glfw;
    if (lume_contagem_glfw == 0)
    {
        glfwTerminate();
    }
}

bool lume_app_should_close(const LumeApp *aplicativo)
{
    return !aplicativo || glfwWindowShouldClose(aplicativo->janela) != 0;
}

void lume_app_request_close(LumeApp *aplicativo)
{
    if (aplicativo)
    {
        glfwSetWindowShouldClose(aplicativo->janela, GLFW_TRUE);
    }
}

float lume_app_begin_frame(LumeApp *aplicativo)
{
    double tempo_atual;
    if (!aplicativo)
    {
        return 0.0f;
    }
    memcpy(aplicativo->teclas_anteriores, aplicativo->teclas, sizeof(aplicativo->teclas));
    memcpy(aplicativo->botoes_mouse_anteriores, aplicativo->botoes_mouse, sizeof(aplicativo->botoes_mouse));
    aplicativo->posicao_mouse_anterior = aplicativo->posicao_mouse;
    aplicativo->rolagem_mouse = (LumeVec2){0.0f, 0.0f};
    glfwPollEvents();
    aplicativo->delta_mouse = (LumeVec2){aplicativo->posicao_mouse.x - aplicativo->posicao_mouse_anterior.x,
                                         aplicativo->posicao_mouse.y - aplicativo->posicao_mouse_anterior.y};
    tempo_atual = glfwGetTime();
    aplicativo->delta_tempo = (float)(tempo_atual - aplicativo->tempo_anterior);
    aplicativo->tempo_anterior = tempo_atual;
    return aplicativo->delta_tempo;
}

void lume_app_end_frame(LumeApp *aplicativo)
{
    if (aplicativo)
    {
        glfwSwapBuffers(aplicativo->janela);
    }
}

void lume_app_set_clear_color(LumeApp *aplicativo, LumeColor cor)
{
    if (aplicativo)
    {
        aplicativo->cor_limpeza = cor;
    }
}

void lume_app_get_framebuffer_size(const LumeApp *aplicativo, int *largura, int *altura)
{
    int largura_local = 0;
    int altura_local = 0;
    if (aplicativo)
    {
        glfwGetFramebufferSize(aplicativo->janela, &largura_local, &altura_local);
    }
    if (largura)
        *largura = largura_local;
    if (altura)
        *altura = altura_local;
}

static bool lume_tecla_valida(LumeKey tecla)
{
    return (int)tecla >= 0 && (int)tecla < LUME_TOTAL_TECLAS;
}

bool lume_key_is_down(const LumeApp *aplicativo, LumeKey tecla)
{
    return aplicativo && lume_tecla_valida(tecla) && aplicativo->teclas[tecla];
}

bool lume_key_was_pressed(const LumeApp *aplicativo, LumeKey tecla)
{
    return aplicativo && lume_tecla_valida(tecla) && aplicativo->teclas[tecla] && !aplicativo->teclas_anteriores[tecla];
}

bool lume_key_was_released(const LumeApp *aplicativo, LumeKey tecla)
{
    return aplicativo && lume_tecla_valida(tecla) && !aplicativo->teclas[tecla] && aplicativo->teclas_anteriores[tecla];
}

static bool lume_botao_valido(LumeMouseButton botao)
{
    return (int)botao >= 0 && (int)botao < LUME_TOTAL_BOTOES_MOUSE;
}

bool lume_mouse_button_is_down(const LumeApp *aplicativo, LumeMouseButton botao)
{
    return aplicativo && lume_botao_valido(botao) && aplicativo->botoes_mouse[botao];
}

bool lume_mouse_button_was_pressed(const LumeApp *aplicativo, LumeMouseButton botao)
{
    return aplicativo && lume_botao_valido(botao) && aplicativo->botoes_mouse[botao] &&
           !aplicativo->botoes_mouse_anteriores[botao];
}

bool lume_mouse_button_was_released(const LumeApp *aplicativo, LumeMouseButton botao)
{
    return aplicativo && lume_botao_valido(botao) && !aplicativo->botoes_mouse[botao] &&
           aplicativo->botoes_mouse_anteriores[botao];
}

LumeVec2 lume_mouse_position(const LumeApp *aplicativo)
{
    return aplicativo ? aplicativo->posicao_mouse : (LumeVec2){0.0f, 0.0f};
}

LumeVec2 lume_mouse_delta(const LumeApp *aplicativo)
{
    return aplicativo ? aplicativo->delta_mouse : (LumeVec2){0.0f, 0.0f};
}

LumeVec2 lume_mouse_scroll(const LumeApp *aplicativo)
{
    return aplicativo ? aplicativo->rolagem_mouse : (LumeVec2){0.0f, 0.0f};
}
