#include "lume_interno.h"

#include <math.h>
#include <stdlib.h>

const char *lume_animation_clip_name(const LumeAnimationClip *c)
{
    return c && c->nome ? c->nome : "";
}
float lume_animation_clip_duration(const LumeAnimationClip *c)
{
    return c ? c->duracao : 0;
}

LumeResult lume_animation_player_create(LumeModelInstance *i, LumeAnimationPlayer **saida)
{
    LumeAnimationPlayer *p;
    if (!saida)
        return lume_definir_erro(LUME_ERROR_INVALID_ARGUMENT, "animation_player.create", NULL, 0, 0,
                                 "out_player must not be NULL.");
    *saida = NULL;
    if (!i)
        return lume_definir_erro(LUME_ERROR_INVALID_ARGUMENT, "animation_player.create", NULL, 0, 0,
                                 "A model instance is required.");
    p = calloc(1, sizeof(*p));
    if (!p)
        return lume_definir_erro(LUME_ERROR_OUT_OF_MEMORY, "animation_player.create", NULL, 0, 0,
                                 "Out of memory while creating an animation player.");
    p->instancia = i;
    p->velocidade = 1;
    *saida = p;
    return LUME_SUCCESS;
}
void lume_animation_player_destroy(LumeAnimationPlayer *p)
{
    free(p);
}
LumeResult lume_animation_player_play(LumeAnimationPlayer *p, LumeAnimationClip *c, LumeLoopMode modo)
{
    if (!p || !c)
        return lume_definir_erro(LUME_ERROR_INVALID_ARGUMENT, "animation_player.play", NULL, 0, 0,
                                 "An animation player and clip are required.");
    p->atual = c;
    p->destino = NULL;
    p->repeticao = modo;
    p->tempo = 0;
    p->tempo_transicao = 0;
    return LUME_SUCCESS;
}
LumeResult lume_animation_player_crossfade(LumeAnimationPlayer *p, LumeAnimationClip *c, float duracao)
{
    if (!p || !c || duracao < 0)
        return lume_definir_erro(LUME_ERROR_INVALID_ARGUMENT, "animation_player.crossfade", NULL, 0, 0,
                                 "An animation player, clip, and non-negative duration are required.");
    if (!p->atual || duracao == 0)
        return lume_animation_player_play(p, c, p->repeticao);
    p->destino = c;
    p->tempo_transicao = 0;
    p->duracao_transicao = duracao;
    return LUME_SUCCESS;
}
void lume_animation_player_pause(LumeAnimationPlayer *p, bool pausado)
{
    if (p)
        p->pausado = pausado;
}
void lume_animation_player_set_speed(LumeAnimationPlayer *p, float v)
{
    if (p)
        p->velocidade = v;
}
float lume_animation_player_time(const LumeAnimationPlayer *p)
{
    return p ? p->tempo : 0;
}

static float lume_tempo_clipe(float tempo, float duracao, LumeLoopMode modo)
{
    if (duracao <= 0)
        return 0;
    if (modo == LUME_LOOP_ONCE)
        return fmaxf(0, fminf(duracao, tempo));
    if (modo == LUME_LOOP_REPEAT)
    {
        tempo = fmodf(tempo, duracao);
        return tempo < 0 ? tempo + duracao : tempo;
    }
    {
        float ciclo = fmodf(tempo, duracao * 2);
        if (ciclo < 0)
            ciclo += duracao * 2;
        return ciclo > duracao ? duracao * 2 - ciclo : ciclo;
    }
}

static void lume_localizar_chaves(const LumeCanalAnimacao *c, float tempo, size_t *a, size_t *b, float *t)
{
    size_t i;
    if (c->quantidade_chaves < 2 || tempo <= c->tempos[0])
    {
        *a = *b = 0;
        *t = 0;
        return;
    }
    for (i = 0; i + 1 < c->quantidade_chaves; ++i)
        if (tempo < c->tempos[i + 1])
        {
            *a = i;
            *b = i + 1;
            *t = (tempo - c->tempos[i]) / (c->tempos[i + 1] - c->tempos[i]);
            return;
        }
    *a = *b = c->quantidade_chaves - 1;
    *t = 0;
}
static float lume_hermite(float p0, float m0, float p1, float m1, float t, float intervalo)
{
    float t2 = t * t, t3 = t2 * t;
    return (2 * t3 - 3 * t2 + 1) * p0 + (t3 - 2 * t2 + t) * m0 * intervalo + (-2 * t3 + 3 * t2) * p1 +
           (t3 - t2) * m1 * intervalo;
}
static void lume_amostrar_canal(const LumeCanalAnimacao *c, float tempo, float *saida)
{
    size_t a, b;
    float t;
    uint32_t i;
    size_t passo = c->componentes * (c->interpolacao == LUME_INTERPOLACAO_CUBICA ? 3 : 1);
    lume_localizar_chaves(c, tempo, &a, &b, &t);
    if (c->interpolacao == LUME_INTERPOLACAO_STEP)
        t = 0;
    for (i = 0; i < c->componentes; ++i)
    {
        if (c->interpolacao == LUME_INTERPOLACAO_CUBICA)
        {
            float intervalo = c->tempos[b] - c->tempos[a];
            float p0 = c->valores[a * passo + c->componentes + i], m0 = c->valores[a * passo + 2 * c->componentes + i],
                  p1 = c->valores[b * passo + c->componentes + i], m1 = c->valores[b * passo + i];
            saida[i] = lume_hermite(p0, m0, p1, m1, t, intervalo);
        }
        else
        {
            float p0 = c->valores[a * passo + i], p1 = c->valores[b * passo + i];
            saida[i] = p0 + (p1 - p0) * t;
        }
    }
    if (c->caminho == LUME_ANIMACAO_ROTACAO)
    {
        LumeQuat q = {saida[0], saida[1], saida[2], saida[3]}, id = lume_quat_identity();
        q = lume_quat_slerp(id, q, 1);
        saida[0] = q.x;
        saida[1] = q.y;
        saida[2] = q.z;
        saida[3] = q.w;
    }
}

static bool lume_amostrar_propriedade(LumeAnimationClip *c, uint32_t no, LumeCaminhoAnimacao caminho, float tempo,
                                      float *saida)
{
    size_t i;
    for (i = 0; i < c->quantidade_canais; ++i)
        if (c->canais[i].indice_no == no && c->canais[i].caminho == caminho)
        {
            lume_amostrar_canal(&c->canais[i], tempo, saida);
            return true;
        }
    return false;
}
static void lume_aplicar_clipe(LumeAnimationPlayer *p, LumeAnimationClip *c, float tempo, float mistura)
{
    size_t i;
    float valor[16];
    for (i = 0; i < p->instancia->quantidade_nos; ++i)
    {
        LumeNode *n = p->instancia->nos[i];
        if (lume_amostrar_propriedade(c, (uint32_t)i, LUME_ANIMACAO_TRANSLACAO, tempo, valor))
        {
            LumeVec3 a = lume_node_position(n), b = {valor[0], valor[1], valor[2]};
            lume_node_set_position(n, lume_vec3_add(lume_vec3_scale(a, 1 - mistura), lume_vec3_scale(b, mistura)));
        }
        if (lume_amostrar_propriedade(c, (uint32_t)i, LUME_ANIMACAO_ROTACAO, tempo, valor))
        {
            LumeQuat b = {valor[0], valor[1], valor[2], valor[3]};
            lume_node_set_rotation(n, lume_quat_slerp(lume_node_rotation(n), b, mistura));
        }
        if (lume_amostrar_propriedade(c, (uint32_t)i, LUME_ANIMACAO_ESCALA, tempo, valor))
        {
            LumeVec3 a = lume_node_scale(n), b = {valor[0], valor[1], valor[2]};
            lume_node_set_scale(n, lume_vec3_add(lume_vec3_scale(a, 1 - mistura), lume_vec3_scale(b, mistura)));
        }
    }
}

void lume_animation_player_seek(LumeAnimationPlayer *p, float segundos)
{
    if (!p)
        return;
    p->tempo = segundos;
    if (p->atual)
        lume_aplicar_clipe(p, p->atual, lume_tempo_clipe(p->tempo, p->atual->duracao, p->repeticao), 1);
}
void lume_animation_player_update(LumeAnimationPlayer *p, float delta)
{
    float t;
    if (!p || !p->atual || p->pausado)
        return;
    p->tempo += delta * p->velocidade;
    t = lume_tempo_clipe(p->tempo, p->atual->duracao, p->repeticao);
    lume_aplicar_clipe(p, p->atual, t, 1);
    if (p->destino)
    {
        float mistura;
        p->tempo_transicao += fabsf(delta);
        mistura = p->duracao_transicao > 0 ? fminf(1, p->tempo_transicao / p->duracao_transicao) : 1;
        lume_aplicar_clipe(p, p->destino, lume_tempo_clipe(p->tempo, p->destino->duracao, p->repeticao), mistura);
        if (mistura >= 1)
        {
            p->atual = p->destino;
            p->destino = NULL;
            p->tempo_transicao = 0;
        }
    }
}
