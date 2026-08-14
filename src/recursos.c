#include "lume_interno.h"

#include <stb_image.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void lume_destruir_textura(void *recurso)
{
    LumeTexture *t = recurso;
    lume_desregistrar_recurso(t->referencia.aplicativo, t);
    if (t->identificador)
        glDeleteTextures(1, &t->identificador);
    free(t->caminho);
    free(t);
}
static void lume_destruir_shader(void *recurso)
{
    LumeShader *s = recurso;
    lume_desregistrar_recurso(s->referencia.aplicativo, s);
    if (s->programa)
        glDeleteProgram(s->programa);
    free(s->caminho_vertice);
    free(s->caminho_fragmento);
    free(s);
}
static void lume_destruir_pipeline(void *recurso)
{
    LumePipeline *p = recurso;
    lume_desregistrar_recurso(p->referencia.aplicativo, p);
    lume_shader_release(p->shader);
    free(p);
}
static void lume_destruir_material(void *recurso)
{
    LumeMaterial *m = recurso;
    lume_desregistrar_recurso(m->referencia.aplicativo, m);
    lume_texture_release(m->textura_base);
    lume_texture_release(m->textura_normal);
    lume_texture_release(m->textura_metal_rugosidade);
    lume_texture_release(m->textura_oclusao);
    lume_texture_release(m->textura_emissiva);
    lume_pipeline_release(m->pipeline);
    free(m);
}
static void lume_destruir_alvo(void *recurso)
{
    LumeRenderTarget *t = recurso;
    lume_desregistrar_recurso(t->referencia.aplicativo, t);
    if (t->framebuffer)
        glDeleteFramebuffers(1, &t->framebuffer);
    if (t->cor)
        glDeleteTextures(1, &t->cor);
    if (t->profundidade)
        glDeleteRenderbuffers(1, &t->profundidade);
    free(t);
}
static void lume_destruir_ambiente(void *recurso)
{
    LumeEnvironment *a = recurso;
    lume_desregistrar_recurso(a->referencia.aplicativo, a);
    lume_texture_release(a->equiretangular);
    free(a);
}

void lume_texture_retain(LumeTexture *t)
{
    if (t)
        lume_referencia_reter(&t->referencia);
}
void lume_texture_release(LumeTexture *t)
{
    if (t && lume_referencia_liberar(&t->referencia) == 0)
        lume_destruir_textura(t);
}
void lume_material_retain(LumeMaterial *m)
{
    if (m)
        lume_referencia_reter(&m->referencia);
}
void lume_material_release(LumeMaterial *m)
{
    if (m && lume_referencia_liberar(&m->referencia) == 0)
        lume_destruir_material(m);
}
void lume_shader_retain(LumeShader *s)
{
    if (s)
        lume_referencia_reter(&s->referencia);
}
void lume_shader_release(LumeShader *s)
{
    if (s && lume_referencia_liberar(&s->referencia) == 0)
        lume_destruir_shader(s);
}
void lume_pipeline_retain(LumePipeline *p)
{
    if (p)
        lume_referencia_reter(&p->referencia);
}
void lume_pipeline_release(LumePipeline *p)
{
    if (p && lume_referencia_liberar(&p->referencia) == 0)
        lume_destruir_pipeline(p);
}
void lume_render_target_retain(LumeRenderTarget *t)
{
    if (t)
        lume_referencia_reter(&t->referencia);
}
void lume_render_target_release(LumeRenderTarget *t)
{
    if (t && lume_referencia_liberar(&t->referencia) == 0)
        lume_destruir_alvo(t);
}
void lume_environment_retain(LumeEnvironment *a)
{
    if (a)
        lume_referencia_reter(&a->referencia);
}
void lume_environment_release(LumeEnvironment *a)
{
    if (a && lume_referencia_liberar(&a->referencia) == 0)
        lume_destruir_ambiente(a);
}

LumeTextureConfig lume_texture_config_default(void)
{
    return (LumeTextureConfig){LUME_TEXTURE_FILTER_LINEAR,
                               LUME_TEXTURE_FILTER_LINEAR,
                               LUME_TEXTURE_WRAP_REPEAT,
                               LUME_TEXTURE_WRAP_REPEAT,
                               true,
                               true,
                               true};
}
LumeMaterialConfig lume_material_config_default(LumeMaterialType tipo)
{
    LumeMaterialConfig c = {0};
    c.type = tipo;
    c.base_color = (LumeColor){1, 1, 1, 1};
    c.emissive_color = (LumeColor){0, 0, 0, 1};
    c.metallic = 0;
    c.roughness = 1;
    c.shininess = 32;
    c.alpha_cutoff = .5f;
    c.alpha_mode = LUME_ALPHA_OPAQUE;
    return c;
}
LumeRendererConfig lume_renderer_config_default(void)
{
    return (LumeRendererConfig){true, true, false, true, 1.0f, 1.0f, .15f, LUME_TONE_MAPPING_ACES, 1, 2048, 1024};
}
LumePipelineConfig lume_pipeline_config_default(void)
{
    return (LumePipelineConfig){NULL, true, true, false, true};
}
LumeRenderTargetConfig lume_render_target_config_default(void)
{
    return (LumeRenderTargetConfig){1280, 720, true, true, 1};
}

LumeResult lume_texture_create_rgba8(LumeApp *a, const uint8_t *p, int l, int h, const LumeTextureConfig *cfg,
                                     LumeTexture **saida)
{
    LumeTextureConfig c = cfg ? *cfg : lume_texture_config_default();
    LumeTexture *t;
    GLint min;
    if (!saida)
        return lume_definir_erro(LUME_ERROR_INVALID_ARGUMENT, "texture.create", NULL, 0, 0,
                                 "out_texture must not be NULL.");
    *saida = NULL;
    if (!a || !p || l <= 0 || h <= 0)
        return lume_definir_erro(LUME_ERROR_INVALID_ARGUMENT, "texture.create", NULL, 0, 0,
                                 "Texture creation requires an application, RGBA pixels, and positive dimensions.");
    t = calloc(1, sizeof(*t));
    if (!t)
        return lume_definir_erro(LUME_ERROR_OUT_OF_MEMORY, "texture.create", NULL, 0, 0,
                                 "Out of memory while creating a texture.");
    t->referencia = (LumeReferencia){1, a, lume_destruir_textura, "texture"};
    t->largura = l;
    t->altura = h;
    t->srgb = c.srgb;
    min = c.min_filter == LUME_TEXTURE_FILTER_NEAREST ? (c.generate_mipmaps ? GL_NEAREST_MIPMAP_NEAREST : GL_NEAREST)
                                                      : (c.generate_mipmaps ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
    glGenTextures(1, &t->identificador);
    glBindTexture(GL_TEXTURE_2D, t->identificador);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                    c.mag_filter == LUME_TEXTURE_FILTER_NEAREST ? GL_NEAREST : GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
                    c.wrap_u == LUME_TEXTURE_WRAP_REPEAT ? GL_REPEAT : GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
                    c.wrap_v == LUME_TEXTURE_WRAP_REPEAT ? GL_REPEAT : GL_CLAMP_TO_EDGE);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, c.srgb ? GL_SRGB8_ALPHA8 : GL_RGBA8, l, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, p);
    if (c.generate_mipmaps)
        glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);
    lume_registrar_recurso(a, t);
    *saida = t;
    return LUME_SUCCESS;
}

static void lume_inverter_linhas(unsigned char *p, int l, int h, int canais)
{
    size_t passo = (size_t)l * canais;
    unsigned char *tmp = malloc(passo);
    int y;
    if (!tmp)
        return;
    for (y = 0; y < h / 2; ++y)
    {
        unsigned char *a = p + (size_t)y * passo, *b = p + (size_t)(h - 1 - y) * passo;
        memcpy(tmp, a, passo);
        memcpy(a, b, passo);
        memcpy(b, tmp, passo);
    }
    free(tmp);
}
LumeResult lume_texture_load(LumeApp *a, const char *caminho, const LumeTextureConfig *cfg, LumeTexture **saida)
{
    LumeTextureConfig c = cfg ? *cfg : lume_texture_config_default();
    unsigned char *p;
    int l, h, n;
    LumeResult r;
    if (!caminho)
        return lume_definir_erro(LUME_ERROR_INVALID_ARGUMENT, "texture.load", NULL, 0, 0,
                                 "A file path is required to load a texture.");
    p = stbi_load(caminho, &l, &h, &n, 4);
    if (!p)
        return lume_definir_erro(LUME_ERROR_IO, "texture.load", caminho, 0, 0, "Could not load texture '%s': %s",
                                 caminho, stbi_failure_reason());
    if (c.flip_y)
        lume_inverter_linhas(p, l, h, 4);
    r = lume_texture_create_rgba8(a, p, l, h, &c, saida);
    stbi_image_free(p);
    if (r == LUME_SUCCESS)
        (*saida)->caminho = lume_copiar_texto(caminho);
    return r;
}

LumeResult lume_material_create(LumeApp *a, const LumeMaterialConfig *cfg, LumeMaterial **saida)
{
    LumeMaterialConfig c = cfg ? *cfg : lume_material_config_default(LUME_MATERIAL_PBR);
    LumeMaterial *m;
    LumeTexture *ts[] = {c.base_color_texture, c.normal_texture, c.metallic_roughness_texture, c.occlusion_texture,
                         c.emissive_texture};
    int i;
    if (!saida)
        return lume_definir_erro(LUME_ERROR_INVALID_ARGUMENT, "material.create", NULL, 0, 0,
                                 "out_material must not be NULL.");
    *saida = NULL;
    if (!a)
        return lume_definir_erro(LUME_ERROR_INVALID_ARGUMENT, "material.create", NULL, 0, 0,
                                 "A valid application is required to create a material.");
    for (i = 0; i < 5; ++i)
        if (ts[i] && ts[i]->referencia.aplicativo != a)
            return lume_definir_erro(LUME_ERROR_INVALID_ARGUMENT, "material.create", NULL, 0, 0,
                                     "Material textures must belong to the same application.");
    if (c.type == LUME_MATERIAL_CUSTOM && (!c.custom_pipeline || c.custom_pipeline->referencia.aplicativo != a))
        return lume_definir_erro(LUME_ERROR_INVALID_ARGUMENT, "material.create", NULL, 0, 0,
                                 "A custom material requires a pipeline from the same application.");
    m = calloc(1, sizeof(*m));
    if (!m)
        return lume_definir_erro(LUME_ERROR_OUT_OF_MEMORY, "material.create", NULL, 0, 0,
                                 "Out of memory while creating a material.");
    m->referencia = (LumeReferencia){1, a, lume_destruir_material, "material"};
    m->tipo = c.type;
    m->cor_base = c.base_color;
    m->cor_emissiva = c.emissive_color;
    m->textura_base = c.base_color_texture;
    m->textura_normal = c.normal_texture;
    m->textura_metal_rugosidade = c.metallic_roughness_texture;
    m->textura_oclusao = c.occlusion_texture;
    m->textura_emissiva = c.emissive_texture;
    m->pipeline = c.custom_pipeline;
    m->metalico = c.metallic;
    m->rugosidade = c.roughness;
    m->brilho = c.shininess;
    m->corte_alpha = c.alpha_cutoff;
    m->modo_alpha = c.alpha_mode;
    m->dupla_face = c.double_sided;
    m->aramado = c.wireframe;
    for (i = 0; i < 5; ++i)
        lume_texture_retain(ts[i]);
    lume_pipeline_retain(m->pipeline);
    lume_registrar_recurso(a, m);
    *saida = m;
    return LUME_SUCCESS;
}
void lume_material_set_base_color(LumeMaterial *m, LumeColor c)
{
    if (m)
        m->cor_base = c;
}
LumeResult lume_material_set_texture(LumeMaterial *m, LumeTexture *t)
{
    if (!m || (t && t->referencia.aplicativo != m->referencia.aplicativo))
        return lume_definir_erro(LUME_ERROR_INVALID_ARGUMENT, "material.set_texture", NULL, 0, 0,
                                 "Material and texture must belong to the same application.");
    lume_texture_retain(t);
    lume_texture_release(m->textura_base);
    m->textura_base = t;
    return LUME_SUCCESS;
}

static char *lume_ler_arquivo(const char *caminho)
{
    FILE *f;
    long tamanho;
    char *d;
    if (!caminho)
        return NULL;
    f = fopen(caminho, "rb");
    if (!f)
        return NULL;
    if (fseek(f, 0, SEEK_END) != 0 || (tamanho = ftell(f)) < 0 || fseek(f, 0, SEEK_SET) != 0)
    {
        fclose(f);
        return NULL;
    }
    d = malloc((size_t)tamanho + 1);
    if (!d)
    {
        fclose(f);
        return NULL;
    }
    if (fread(d, 1, (size_t)tamanho, f) != (size_t)tamanho)
    {
        free(d);
        fclose(f);
        return NULL;
    }
    d[tamanho] = 0;
    fclose(f);
    return d;
}
static LumeResult lume_compilar_estagio(GLenum tipo, const char *fonte, const char *nome, GLuint *saida)
{
    GLuint s = glCreateShader(tipo);
    GLint ok = 0, tamanho = 0;
    char *log;
    glShaderSource(s, 1, &fonte, NULL);
    glCompileShader(s);
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (ok)
    {
        *saida = s;
        return LUME_SUCCESS;
    }
    glGetShaderiv(s, GL_INFO_LOG_LENGTH, &tamanho);
    log = malloc((size_t)(tamanho > 1 ? tamanho : 2));
    if (log)
        glGetShaderInfoLog(s, tamanho, NULL, log);
    lume_definir_erro(LUME_ERROR_GPU, "shader.compile", nome, 0, 0, "Shader compilation failed for '%s': %s",
                      nome ? nome : "inline source", log ? log : "No driver log was provided.");
    free(log);
    glDeleteShader(s);
    return LUME_ERROR_GPU;
}
LumeResult lume_shader_create(LumeApp *a, const LumeShaderConfig *cfg, LumeShader **saida)
{
    char *fv = NULL, *ff = NULL;
    const char *v, *f;
    GLuint vs = 0, fs = 0, p = 0;
    GLint ok;
    LumeShader *s;
    LumeResult r;
    if (!saida)
        return lume_definir_erro(LUME_ERROR_INVALID_ARGUMENT, "shader.create", NULL, 0, 0,
                                 "out_shader must not be NULL.");
    *saida = NULL;
    if (!a || !cfg)
        return lume_definir_erro(LUME_ERROR_INVALID_ARGUMENT, "shader.create", NULL, 0, 0,
                                 "An application and shader configuration are required.");
    v = cfg->vertex_source;
    f = cfg->fragment_source;
    if (!v && cfg->vertex_path)
        v = fv = lume_ler_arquivo(cfg->vertex_path);
    if (!f && cfg->fragment_path)
        f = ff = lume_ler_arquivo(cfg->fragment_path);
    if (!v || !f)
    {
        free(fv);
        free(ff);
        return lume_definir_erro(LUME_ERROR_IO, "shader.create", !v ? cfg->vertex_path : cfg->fragment_path, 0, 0,
                                 "Could not read the configured shader source.");
    }
    r = lume_compilar_estagio(GL_VERTEX_SHADER, v, cfg->vertex_path, &vs);
    if (r == LUME_SUCCESS)
        r = lume_compilar_estagio(GL_FRAGMENT_SHADER, f, cfg->fragment_path, &fs);
    free(fv);
    free(ff);
    if (r != LUME_SUCCESS)
    {
        if (vs)
            glDeleteShader(vs);
        return r;
    }
    p = glCreateProgram();
    glAttachShader(p, vs);
    glAttachShader(p, fs);
    glLinkProgram(p);
    glDeleteShader(vs);
    glDeleteShader(fs);
    glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok)
    {
        char log[2048];
        glGetProgramInfoLog(p, sizeof(log), NULL, log);
        glDeleteProgram(p);
        return lume_definir_erro(LUME_ERROR_GPU, "shader.link", NULL, 0, 0, "Shader program linking failed: %s", log);
    }
    s = calloc(1, sizeof(*s));
    if (!s)
    {
        glDeleteProgram(p);
        return lume_definir_erro(LUME_ERROR_OUT_OF_MEMORY, "shader.create", NULL, 0, 0,
                                 "Out of memory while creating a shader.");
    }
    s->referencia = (LumeReferencia){1, a, lume_destruir_shader, "shader"};
    s->programa = p;
    s->caminho_vertice = lume_copiar_texto(cfg->vertex_path);
    s->caminho_fragmento = lume_copiar_texto(cfg->fragment_path);
    lume_registrar_recurso(a, s);
    *saida = s;
    return LUME_SUCCESS;
}

static LumeResult lume_localizar_uniforme(LumeShader *shader, const char *nome, GLint *saida)
{
    if (!shader || !nome || !saida)
        return lume_definir_erro(LUME_ERROR_INVALID_ARGUMENT, "shader.set_uniform", NULL, 0, 0,
                                 "A shader, uniform name, and value are required.");
    *saida = glGetUniformLocation(shader->programa, nome);
    if (*saida < 0)
        return lume_definir_erro(LUME_ERROR_INVALID_ARGUMENT, "shader.set_uniform", NULL, 0, 0,
                                 "Shader uniform '%s' does not exist or was optimized out.", nome);
    glUseProgram(shader->programa);
    return LUME_SUCCESS;
}

LumeResult lume_shader_set_float(LumeShader *shader, const char *nome, float valor)
{
    GLint local;
    LumeResult resultado = lume_localizar_uniforme(shader, nome, &local);
    if (resultado == LUME_SUCCESS)
        glUniform1f(local, valor);
    return resultado;
}

LumeResult lume_shader_set_vec2(LumeShader *shader, const char *nome, LumeVec2 valor)
{
    GLint local;
    LumeResult resultado = lume_localizar_uniforme(shader, nome, &local);
    if (resultado == LUME_SUCCESS)
        glUniform2f(local, valor.x, valor.y);
    return resultado;
}

LumeResult lume_shader_set_vec3(LumeShader *shader, const char *nome, LumeVec3 valor)
{
    GLint local;
    LumeResult resultado = lume_localizar_uniforme(shader, nome, &local);
    if (resultado == LUME_SUCCESS)
        glUniform3f(local, valor.x, valor.y, valor.z);
    return resultado;
}

LumeResult lume_shader_set_vec4(LumeShader *shader, const char *nome, LumeVec4 valor)
{
    GLint local;
    LumeResult resultado = lume_localizar_uniforme(shader, nome, &local);
    if (resultado == LUME_SUCCESS)
        glUniform4f(local, valor.x, valor.y, valor.z, valor.w);
    return resultado;
}

LumeResult lume_shader_set_mat4(LumeShader *shader, const char *nome, LumeMat4 valor)
{
    GLint local;
    LumeResult resultado = lume_localizar_uniforme(shader, nome, &local);
    if (resultado == LUME_SUCCESS)
        glUniformMatrix4fv(local, 1, GL_FALSE, valor.values);
    return resultado;
}
LumeResult lume_pipeline_create(LumeApp *a, const LumePipelineConfig *cfg, LumePipeline **saida)
{
    LumePipelineConfig c = cfg ? *cfg : lume_pipeline_config_default();
    LumePipeline *p;
    if (!saida)
        return lume_definir_erro(LUME_ERROR_INVALID_ARGUMENT, "pipeline.create", NULL, 0, 0,
                                 "out_pipeline must not be NULL.");
    *saida = NULL;
    if (!a || !c.shader || c.shader->referencia.aplicativo != a)
        return lume_definir_erro(LUME_ERROR_INVALID_ARGUMENT, "pipeline.create", NULL, 0, 0,
                                 "A pipeline requires a shader from the same application.");
    p = calloc(1, sizeof(*p));
    if (!p)
        return lume_definir_erro(LUME_ERROR_OUT_OF_MEMORY, "pipeline.create", NULL, 0, 0,
                                 "Out of memory while creating a pipeline.");
    p->referencia = (LumeReferencia){1, a, lume_destruir_pipeline, "pipeline"};
    p->shader = c.shader;
    p->teste_profundidade = c.depth_test;
    p->escrita_profundidade = c.depth_write;
    p->mistura = c.blending;
    p->descartar_costas = c.cull_back_faces;
    lume_shader_retain(c.shader);
    lume_registrar_recurso(a, p);
    *saida = p;
    return LUME_SUCCESS;
}
LumeResult lume_render_target_create(LumeApp *a, const LumeRenderTargetConfig *cfg, LumeRenderTarget **saida)
{
    LumeRenderTargetConfig c = cfg ? *cfg : lume_render_target_config_default();
    LumeRenderTarget *t;
    GLenum estado;
    if (!saida)
        return lume_definir_erro(LUME_ERROR_INVALID_ARGUMENT, "render_target.create", NULL, 0, 0,
                                 "out_target must not be NULL.");
    *saida = NULL;
    if (!a || c.width <= 0 || c.height <= 0)
        return lume_definir_erro(LUME_ERROR_INVALID_ARGUMENT, "render_target.create", NULL, 0, 0,
                                 "Render target dimensions must be positive.");
    if (c.samples != 0 && c.samples != 1)
        return lume_definir_erro(LUME_ERROR_UNSUPPORTED, "render_target.create", NULL, 0, 0,
                                 "Multisampled custom render targets are not supported in this release.");
    t = calloc(1, sizeof(*t));
    if (!t)
        return lume_definir_erro(LUME_ERROR_OUT_OF_MEMORY, "render_target.create", NULL, 0, 0,
                                 "Out of memory while creating a render target.");
    t->referencia = (LumeReferencia){1, a, lume_destruir_alvo, "render target"};
    t->largura = c.width;
    t->altura = c.height;
    t->hdr = c.hdr;
    glGenFramebuffers(1, &t->framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, t->framebuffer);
    glGenTextures(1, &t->cor);
    glBindTexture(GL_TEXTURE_2D, t->cor);
    glTexImage2D(GL_TEXTURE_2D, 0, c.hdr ? GL_RGBA16F : GL_RGBA8, c.width, c.height, 0, GL_RGBA,
                 c.hdr ? GL_FLOAT : GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, t->cor, 0);
    if (c.depth)
    {
        glGenRenderbuffers(1, &t->profundidade);
        glBindRenderbuffer(GL_RENDERBUFFER, t->profundidade);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, c.width, c.height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, t->profundidade);
    }
    estado = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    if (estado != GL_FRAMEBUFFER_COMPLETE)
    {
        lume_destruir_alvo(t);
        return lume_definir_erro(LUME_ERROR_GPU, "render_target.create", NULL, 0, 0,
                                 "OpenGL could not create a complete render target (status 0x%X).", estado);
    }
    lume_registrar_recurso(a, t);
    *saida = t;
    return LUME_SUCCESS;
}
LumeResult lume_environment_load_hdr(LumeApp *a, const char *caminho, LumeEnvironment **saida)
{
    float *p;
    int l, h, n;
    LumeTexture *t;
    LumeEnvironment *e;
    if (!saida)
        return lume_definir_erro(LUME_ERROR_INVALID_ARGUMENT, "environment.load", NULL, 0, 0,
                                 "out_environment must not be NULL.");
    *saida = NULL;
    if (!a || !caminho)
        return lume_definir_erro(LUME_ERROR_INVALID_ARGUMENT, "environment.load", NULL, 0, 0,
                                 "An application and HDR path are required.");
    p = stbi_loadf(caminho, &l, &h, &n, 3);
    if (!p)
        return lume_definir_erro(LUME_ERROR_IO, "environment.load", caminho, 0, 0,
                                 "Could not load HDR environment '%s': %s", caminho, stbi_failure_reason());
    t = calloc(1, sizeof(*t));
    if (!t)
    {
        stbi_image_free(p);
        return lume_definir_erro(LUME_ERROR_OUT_OF_MEMORY, "environment.load", caminho, 0, 0,
                                 "Out of memory while loading an HDR environment.");
    }
    t->referencia = (LumeReferencia){1, a, lume_destruir_textura, "HDR texture"};
    t->largura = l;
    t->altura = h;
    t->caminho = lume_copiar_texto(caminho);
    glGenTextures(1, &t->identificador);
    glBindTexture(GL_TEXTURE_2D, t->identificador);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, l, h, 0, GL_RGB, GL_FLOAT, p);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);
    stbi_image_free(p);
    lume_registrar_recurso(a, t);
    e = calloc(1, sizeof(*e));
    if (!e)
    {
        lume_texture_release(t);
        return lume_definir_erro(LUME_ERROR_OUT_OF_MEMORY, "environment.load", caminho, 0, 0,
                                 "Out of memory while creating an environment.");
    }
    e->referencia = (LumeReferencia){1, a, lume_destruir_ambiente, "environment"};
    e->equiretangular = t;
    e->intensidade = 1;
    lume_registrar_recurso(a, e);
    *saida = e;
    return LUME_SUCCESS;
}
