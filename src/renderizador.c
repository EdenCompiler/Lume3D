#include "lume_interno.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *lume_vertice_padrao =
    "#version 330 core\n"
    "layout(location=0)in vec3 aPosition;layout(location=1)in vec3 aNormal;layout(location=2)in vec2 aUv;"
    "layout(location=3)in mat4 aInstance;uniform mat4 uModel,uView,uProjection;uniform bool uInstanced;"
    "uniform mat4 uDirShadowMatrices[3],uSpotShadowMatrices[4];"
    "out vec3 vWorld;out vec3 vNormal;out vec2 vUv;out vec4 vDirShadow[3];out vec4 vSpotShadow[4];"
    "void main(){mat4 model=uInstanced?uModel*aInstance:uModel;vec4 world=model*vec4(aPosition,1);"
    "vWorld=world.xyz;vNormal=mat3(transpose(inverse(model)))*aNormal;vUv=aUv;gl_Position=uProjection*uView*world;"
    "for(int i=0;i<3;i++)vDirShadow[i]=uDirShadowMatrices[i]*world;"
    "for(int i=0;i<4;i++)vSpotShadow[i]=uSpotShadowMatrices[i]*world;}";
static const char *lume_fragmento_padrao =
    "#version 330 core\n"
    "struct Dir{vec3 direction,color;};struct Point{vec3 position,color;float range;};struct Spot{vec3 "
    "position,direction,color;float range,innerCos,outerCos;};"
    "in vec3 vWorld,vNormal;in vec2 vUv;in vec4 vDirShadow[3];in vec4 vSpotShadow[4];out vec4 FragColor;"
    "uniform vec4 uBaseColor;uniform sampler2D "
    "uBaseTexture;uniform bool uHasTexture;"
    "uniform int uMaterialType;uniform float uMetallic,uRoughness,uAlphaCutoff;uniform int uAlphaMode;uniform vec3 "
    "uAmbient;uniform vec3 uCamera;uniform sampler2D uEnvironment;uniform bool uHasEnvironment;"
    "uniform sampler2D uDirShadow0,uDirShadow1,uDirShadow2,uSpotShadow0,uSpotShadow1,uSpotShadow2,uSpotShadow3;"
    "uniform bool uHasDirShadows;uniform int uSpotShadowCount;uniform vec3 uCascadeSplits;"
    "uniform int uDirCount,uPointCount,uSpotCount;uniform Dir uDirs[4];uniform Point uPoints[32];uniform Spot "
    "uSpots[8];"
    "const float PI=3.14159265359;vec2 envUv(vec3 d){return vec2(atan(d.z,d.x)/(2.*PI)+.5,"
    "asin(clamp(d.y,-1.,1.))/PI+.5);}"
    "vec3 brdf(vec3 n,vec3 l,vec3 v,vec3 color){float ndl=max(dot(n,l),0);if(uMaterialType==0)return color;"
    "if(uMaterialType==1){vec3 h=normalize(l+v);return color*ndl+pow(max(dot(n,h),0),32)*.25;}"
    "vec3 f0=mix(vec3(.04),color,uMetallic);vec3 h=normalize(l+v);float "
    "spec=pow(max(dot(n,h),0),mix(128.,4.,uRoughness));return color*(1-uMetallic)*ndl+f0*spec*ndl;}"
    "float shadowSample(sampler2D map,vec4 lp){vec3 p=lp.xyz/lp.w;p=p*.5+.5;"
    "if(p.z>1.||p.x<0.||p.x>1.||p.y<0.||p.y>1.)return 1.;vec2 texel=1./vec2(textureSize(map,0));float s=0.;"
    "for(int x=-1;x<=1;x++)for(int y=-1;y<=1;y++){float d=texture(map,p.xy+vec2(x,y)*texel).r;"
    "s+=p.z-.0018<=d?1.:0.;}return s/9.;}"
    "float dirShadow(){if(!uHasDirShadows)return 1.;float d=length(vWorld-uCamera);"
    "if(d<uCascadeSplits.x)return shadowSample(uDirShadow0,vDirShadow[0]);"
    "if(d<uCascadeSplits.y)return shadowSample(uDirShadow1,vDirShadow[1]);"
    "return shadowSample(uDirShadow2,vDirShadow[2]);}"
    "float spotShadow(int i){if(i==0)return shadowSample(uSpotShadow0,vSpotShadow[0]);"
    "if(i==1)return shadowSample(uSpotShadow1,vSpotShadow[1]);if(i==2)return shadowSample(uSpotShadow2,vSpotShadow[2]);"
    "return shadowSample(uSpotShadow3,vSpotShadow[3]);}"
    "void main(){vec4 "
    "base=uBaseColor*(uHasTexture?texture(uBaseTexture,vUv):vec4(1));if(uAlphaMode==1&&base.a<uAlphaCutoff)discard;"
    "vec3 n=normalize(vNormal),v=normalize(uCamera-vWorld),lit=uMaterialType==0?base.rgb:base.rgb*uAmbient;"
    "if(uMaterialType==2&&uHasEnvironment){vec3 env=texture(uEnvironment,envUv(reflect(-v,n))).rgb;"
    "lit+=env*mix(vec3(.04),base.rgb,uMetallic)*mix(.35,.08,uRoughness);}int i;"
    "for(i=0;i<uDirCount;i++){vec3 l=normalize(-uDirs[i].direction);float sh=i==0?dirShadow():1.;"
    "lit+=brdf(n,l,v,base.rgb)*uDirs[i].color*sh;}"
    "for(i=0;i<uPointCount;i++){vec3 d=uPoints[i].position-vWorld;float "
    "z=length(d),a=pow(max(1-z/uPoints[i].range,0),2);lit+=brdf(n,normalize(d),v,base.rgb)*uPoints[i].color*a;}"
    "for(i=0;i<uSpotCount;i++){vec3 d=uSpots[i].position-vWorld;float "
    "z=length(d),cone=dot(normalize(-d),normalize(uSpots[i].direction));float "
    "a=smoothstep(uSpots[i].outerCos,uSpots[i].innerCos,cone)*pow(max(1-z/"
    "uSpots[i].range,0),2);float sh=i<uSpotShadowCount?spotShadow(i):1.;"
    "lit+=brdf(n,normalize(d),v,base.rgb)*uSpots[i].color*a*sh;}"
    "FragColor=vec4(lit,base.a);}";
static const char *lume_vertice_tela =
    "#version 330 core\nout vec2 vUv;void main(){vec2 "
    "p=vec2((gl_VertexID<<1)&2,gl_VertexID&2);vUv=p*.5;gl_Position=vec4(p*2-1,0,1);}";
static const char *lume_fragmento_tom =
    "#version 330 core\nin vec2 vUv;out vec4 FragColor;uniform sampler2D uColor;uniform float uExposure;uniform int "
    "uTone;uniform bool uFxaa;uniform bool uBloom;"
    "vec3 aces(vec3 x){return clamp((x*(2.51*x+.03))/(x*(2.43*x+.59)+.14),0,1);}void main(){vec3 "
    "c=texture(uColor,vUv).rgb;"
    "if(uFxaa){vec2 "
    "s=1./"
    "vec2(textureSize(uColor,0));c=(c*4.+texture(uColor,vUv+vec2(s.x,0)).rgb+texture(uColor,vUv-vec2(s.x,0)).rgb+"
    "texture(uColor,vUv+vec2(0,s.y)).rgb+texture(uColor,vUv-vec2(0,s.y)).rgb)/8.;}"
    "if(uBloom){float "
    "b=max(max(c.r,c.g),c.b);c+=c*max(b-1.,0.)*.08;}c*=uExposure;if(uTone==1)c=aces(c);FragColor=vec4(pow(c,vec3(1./"
    "2.2)),1);}";
static const char *lume_fragmento_copia = "#version 330 core\nin vec2 vUv;out vec4 FragColor;uniform sampler2D uColor;"
                                          "void main(){FragColor=texture(uColor,vUv);}";
static const char *lume_vertice_sombra =
    "#version 330 core\nlayout(location=0)in vec3 aPosition;layout(location=3)in mat4 aInstance;"
    "uniform mat4 uModel,uLightViewProjection;uniform bool uInstanced;"
    "void main(){mat4 model=uInstanced?uModel*aInstance:uModel;"
    "gl_Position=uLightViewProjection*model*vec4(aPosition,1);}";
static const char *lume_fragmento_sombra = "#version 330 core\nvoid main(){}";
static const char *lume_vertice_debug =
    "#version 330 core\nlayout(location=0)in vec3 aPosition;layout(location=1)in vec4 aColor;uniform mat4 "
    "uViewProjection;out vec4 vColor;void main(){vColor=aColor;gl_Position=uViewProjection*vec4(aPosition,1);}";
static const char *lume_fragmento_debug =
    "#version 330 core\nin vec4 vColor;out vec4 FragColor;void main(){FragColor=vColor;}";

static GLuint lume_compilar(GLenum tipo, const char *fonte)
{
    GLuint s = glCreateShader(tipo);
    GLint ok;
    glShaderSource(s, 1, &fonte, NULL);
    glCompileShader(s);
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok)
    {
        char log[2048];
        glGetShaderInfoLog(s, sizeof(log), NULL, log);
        lume_definir_erro(LUME_ERROR_GPU, "renderer.shader", NULL, 0, 0, "Built-in shader compilation failed: %s", log);
        glDeleteShader(s);
        return 0;
    }
    return s;
}
static GLuint lume_programa(const char *v, const char *f)
{
    GLuint vs = lume_compilar(GL_VERTEX_SHADER, v), fs, p;
    GLint ok;
    if (!vs)
        return 0;
    fs = lume_compilar(GL_FRAGMENT_SHADER, f);
    if (!fs)
    {
        glDeleteShader(vs);
        return 0;
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
        lume_definir_erro(LUME_ERROR_GPU, "renderer.shader", NULL, 0, 0, "Built-in shader linking failed: %s", log);
        glDeleteProgram(p);
        return 0;
    }
    return p;
}

LumeResult lume_inicializar_renderizador(LumeApp *a)
{
    LumeRenderer *r = calloc(1, sizeof(*r));
    if (!r)
        return lume_definir_erro(LUME_ERROR_OUT_OF_MEMORY, "renderer.create", NULL, 0, 0,
                                 "Out of memory while creating the renderer.");
    r->aplicativo = a;
    r->configuracao = lume_renderer_config_default();
    r->programa_padrao = lume_programa(lume_vertice_padrao, lume_fragmento_padrao);
    r->programa_tom = lume_programa(lume_vertice_tela, lume_fragmento_tom);
    r->programa_copia = lume_programa(lume_vertice_tela, lume_fragmento_copia);
    r->programa_debug = lume_programa(lume_vertice_debug, lume_fragmento_debug);
    r->programa_sombra = lume_programa(lume_vertice_sombra, lume_fragmento_sombra);
    if (!r->programa_padrao || !r->programa_tom || !r->programa_copia || !r->programa_debug || !r->programa_sombra)
    {
        free(r);
        return LUME_ERROR_GPU;
    }
    glGenVertexArrays(1, &r->vao_tela);
    glGenVertexArrays(1, &r->vao_debug);
    glGenBuffers(1, &r->vbo_debug);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glEnable(GL_FRAMEBUFFER_SRGB);
    a->renderizador = r;
    return LUME_SUCCESS;
}
void lume_destruir_renderizador(LumeApp *a)
{
    LumeRenderer *r = a ? a->renderizador : NULL;
    size_t i;
    if (!r)
        return;
    lume_environment_release(r->ambiente);
    for (i = 0; i < r->quantidade_passagens; ++i)
    {
        free(r->passagens[i].nome);
        lume_pipeline_release(r->passagens[i].pipeline);
    }
    free(r->passagens);
    free(r->linhas);
    if (r->programa_padrao)
        glDeleteProgram(r->programa_padrao);
    if (r->programa_tom)
        glDeleteProgram(r->programa_tom);
    if (r->programa_copia)
        glDeleteProgram(r->programa_copia);
    if (r->programa_debug)
        glDeleteProgram(r->programa_debug);
    if (r->programa_sombra)
        glDeleteProgram(r->programa_sombra);
    glDeleteFramebuffers(3, r->framebuffers_sombra_direcional);
    glDeleteTextures(3, r->texturas_sombra_direcional);
    glDeleteFramebuffers(4, r->framebuffers_sombra_spot);
    glDeleteTextures(4, r->texturas_sombra_spot);
    if (r->vao_tela)
        glDeleteVertexArrays(1, &r->vao_tela);
    if (r->vao_debug)
        glDeleteVertexArrays(1, &r->vao_debug);
    if (r->vbo_debug)
        glDeleteBuffers(1, &r->vbo_debug);
    if (r->framebuffer_hdr)
        glDeleteFramebuffers(1, &r->framebuffer_hdr);
    if (r->framebuffer_ping)
        glDeleteFramebuffers(1, &r->framebuffer_ping);
    if (r->textura_hdr)
        glDeleteTextures(1, &r->textura_hdr);
    if (r->textura_ping)
        glDeleteTextures(1, &r->textura_ping);
    if (r->profundidade_hdr)
        glDeleteTextures(1, &r->profundidade_hdr);
    free(r);
    a->renderizador = NULL;
}

LumeResult lume_renderer_configure(LumeRenderer *r, const LumeRendererConfig *c)
{
    if (!r || !c)
        return lume_definir_erro(LUME_ERROR_INVALID_ARGUMENT, "renderer.configure", NULL, 0, 0,
                                 "A renderer and configuration are required.");
    if (c->msaa_samples != 1 && c->msaa_samples != 2 && c->msaa_samples != 4 && c->msaa_samples != 8)
        return lume_definir_erro(LUME_ERROR_INVALID_ARGUMENT, "renderer.configure", NULL, 0, 0,
                                 "MSAA samples must be 1, 2, 4, or 8.");
    if (c->directional_shadow_size == 0 || c->spot_shadow_size == 0)
        return lume_definir_erro(LUME_ERROR_INVALID_ARGUMENT, "renderer.configure", NULL, 0, 0,
                                 "Shadow map sizes must be greater than zero.");
    r->configuracao = *c;
    return LUME_SUCCESS;
}
void lume_renderer_set_environment(LumeRenderer *r, LumeEnvironment *e)
{
    if (!r || (e && e->referencia.aplicativo != r->aplicativo))
        return;
    lume_environment_retain(e);
    lume_environment_release(r->ambiente);
    r->ambiente = e;
}
LumeResult lume_renderer_add_pass(LumeRenderer *r, const LumePassConfig *c, uint32_t *indice)
{
    LumePassagemInterna *p;
    LumePassagemInterna *novas;
    size_t nova_capacidade;
    if (!r || !c || !c->pipeline || c->pipeline->referencia.aplicativo != r->aplicativo)
        return lume_definir_erro(LUME_ERROR_INVALID_ARGUMENT, "renderer.add_pass", NULL, 0, 0,
                                 "A pass requires a renderer and pipeline from the same application.");
    if (r->quantidade_passagens == r->capacidade_passagens)
    {
        nova_capacidade = r->capacidade_passagens ? r->capacidade_passagens * 2 : 4;
        novas = realloc(r->passagens, nova_capacidade * sizeof(*novas));
        if (!novas)
            return lume_definir_erro(LUME_ERROR_OUT_OF_MEMORY, "renderer.add_pass", NULL, 0, 0,
                                     "Out of memory while growing the render pass list.");
        r->passagens = novas;
        r->capacidade_passagens = nova_capacidade;
    }
    p = &r->passagens[r->quantidade_passagens++];
    *p = (LumePassagemInterna){lume_copiar_texto(c->name ? c->name : "Custom pass"), c->pipeline, c->phase,
                               c->needs_depth, c->enabled};
    lume_pipeline_retain(c->pipeline);
    if (indice)
        *indice = (uint32_t)(r->quantidade_passagens - 1);
    return LUME_SUCCESS;
}
void lume_renderer_clear_passes(LumeRenderer *r)
{
    size_t i;
    if (!r)
        return;
    for (i = 0; i < r->quantidade_passagens; ++i)
    {
        free(r->passagens[i].nome);
        lume_pipeline_release(r->passagens[i].pipeline);
    }
    r->quantidade_passagens = 0;
}

static void lume_uniforme_mat(GLuint p, const char *n, LumeMat4 m)
{
    glUniformMatrix4fv(glGetUniformLocation(p, n), 1, GL_FALSE, m.values);
}
static void lume_uniforme_cor(GLuint p, const char *n, LumeColor c, float i)
{
    glUniform3f(glGetUniformLocation(p, n), c.r * i, c.g * i, c.b * i);
}
static bool lume_camera_valida(const LumeScene *c, const LumeNode *n)
{
    return n && n->cena == c && (n->tipo == LUME_NO_CAMERA_PERSPECTIVA || n->tipo == LUME_NO_CAMERA_ORTOGRAFICA);
}

static LumeMat4 lume_matriz_olhar(LumeVec3 olho, LumeVec3 alvo, LumeVec3 acima)
{
    LumeVec3 frente = lume_vec3_normalize(lume_vec3_subtract(alvo, olho));
    LumeVec3 lado = lume_vec3_normalize(lume_vec3_cross(frente, acima));
    LumeVec3 topo = lume_vec3_cross(lado, frente);
    LumeMat4 matriz = lume_mat4_identity();
    matriz.values[0] = lado.x;
    matriz.values[4] = lado.y;
    matriz.values[8] = lado.z;
    matriz.values[1] = topo.x;
    matriz.values[5] = topo.y;
    matriz.values[9] = topo.z;
    matriz.values[2] = -frente.x;
    matriz.values[6] = -frente.y;
    matriz.values[10] = -frente.z;
    matriz.values[12] = -lume_vec3_dot(lado, olho);
    matriz.values[13] = -lume_vec3_dot(topo, olho);
    matriz.values[14] = lume_vec3_dot(frente, olho);
    return matriz;
}

static bool lume_preparar_mapa_sombra(GLuint *framebuffer, GLuint *textura, uint32_t tamanho)
{
    GLenum estado;
    if (*framebuffer && *textura)
        return true;
    glGenFramebuffers(1, framebuffer);
    glGenTextures(1, textura);
    glBindTexture(GL_TEXTURE_2D, *textura);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, (GLsizei)tamanho, (GLsizei)tamanho, 0, GL_DEPTH_COMPONENT,
                 GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    {
        const float borda[] = {1, 1, 1, 1};
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borda);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, *framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, *textura, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    estado = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return estado == GL_FRAMEBUFFER_COMPLETE;
}

static void lume_configurar_instancias_sombra(LumeNode *no)
{
    int coluna;
    uint32_t quantidade = no->dados.malha.quantidade_instancias;
    if (!no->dados.malha.vbo_instancias)
        glGenBuffers(1, &no->dados.malha.vbo_instancias);
    glBindBuffer(GL_ARRAY_BUFFER, no->dados.malha.vbo_instancias);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(quantidade * sizeof(LumeMat4)), no->dados.malha.instancias,
                 GL_STREAM_DRAW);
    for (coluna = 0; coluna < 4; ++coluna)
    {
        glEnableVertexAttribArray(3 + coluna);
        glVertexAttribPointer(3 + coluna, 4, GL_FLOAT, GL_FALSE, sizeof(LumeMat4),
                              (void *)(size_t)(coluna * 4 * sizeof(float)));
        glVertexAttribDivisor(3 + coluna, 1);
    }
}

static void lume_desenhar_mapa_sombra(LumeRenderer *renderizador, LumeScene *cena, LumeMat4 matriz, GLuint framebuffer,
                                      uint32_t tamanho)
{
    size_t indice;
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glViewport(0, 0, (GLsizei)tamanho, (GLsizei)tamanho);
    glClear(GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);
    glUseProgram(renderizador->programa_sombra);
    lume_uniforme_mat(renderizador->programa_sombra, "uLightViewProjection", matriz);
    for (indice = 0; indice < cena->quantidade_nos; ++indice)
    {
        LumeNode *no = cena->nos[indice];
        LumeGeometry *geometria;
        if (no->tipo != LUME_NO_MALHA && no->tipo != LUME_NO_MALHA_INSTANCIADA)
            continue;
        geometria = no->dados.malha.geometria;
        if (!lume_enviar_geometria_gpu(geometria))
            continue;
        lume_uniforme_mat(renderizador->programa_sombra, "uModel", no->matriz_mundo);
        glUniform1i(glGetUniformLocation(renderizador->programa_sombra, "uInstanced"),
                    no->tipo == LUME_NO_MALHA_INSTANCIADA);
        glBindVertexArray(geometria->vao);
        if (no->tipo == LUME_NO_MALHA_INSTANCIADA)
        {
            lume_configurar_instancias_sombra(no);
            glDrawElementsInstanced(GL_TRIANGLES, (GLsizei)geometria->quantidade_indices, GL_UNSIGNED_INT, NULL,
                                    (GLsizei)no->dados.malha.quantidade_instancias);
        }
        else
            glDrawElements(GL_TRIANGLES, (GLsizei)geometria->quantidade_indices, GL_UNSIGNED_INT, NULL);
        ++renderizador->estatisticas.shadow_draws;
    }
    glCullFace(GL_BACK);
}

static bool lume_renderizar_sombras(LumeRenderer *renderizador, LumeScene *cena, LumeNode *camera)
{
    LumeNode *sol = NULL;
    LumeVec3 posicao_camera = lume_mat4_transform_point(camera->matriz_mundo, (LumeVec3){0, 0, 0});
    LumeVec3 frente_camera =
        lume_vec3_normalize(lume_matriz_transformar_direcao(camera->matriz_mundo, (LumeVec3){0, 0, -1}));
    const float extensoes[3] = {12.0f, 35.0f, 90.0f};
    size_t indice;
    int cascata;

    renderizador->quantidade_sombras_spot = 0;
    for (indice = 0; indice < cena->quantidade_nos; ++indice)
        if (cena->nos[indice]->tipo == LUME_NO_LUZ_DIRECIONAL && cena->nos[indice]->dados.luz.projeta_sombra)
        {
            sol = cena->nos[indice];
            break;
        }
    if (sol)
    {
        LumeVec3 direcao = lume_vec3_normalize(sol->dados.luz.direcao);
        for (cascata = 0; cascata < 3; ++cascata)
        {
            float extensao = extensoes[cascata];
            LumeVec3 centro = lume_vec3_add(posicao_camera, lume_vec3_scale(frente_camera, extensao * 0.38f));
            LumeVec3 olho = lume_vec3_subtract(centro, lume_vec3_scale(direcao, extensao * 2.0f));
            LumeMat4 vista =
                lume_matriz_olhar(olho, centro, fabsf(direcao.y) > 0.95f ? (LumeVec3){0, 0, 1} : (LumeVec3){0, 1, 0});
            LumeMat4 projecao = lume_mat4_orthographic(-extensao, extensao, -extensao, extensao, 0.1f, extensao * 5.0f);
            renderizador->matrizes_sombra_direcional[cascata] = lume_mat4_multiply(projecao, vista);
            if (lume_preparar_mapa_sombra(&renderizador->framebuffers_sombra_direcional[cascata],
                                          &renderizador->texturas_sombra_direcional[cascata],
                                          renderizador->configuracao.directional_shadow_size))
                lume_desenhar_mapa_sombra(renderizador, cena, renderizador->matrizes_sombra_direcional[cascata],
                                          renderizador->framebuffers_sombra_direcional[cascata],
                                          renderizador->configuracao.directional_shadow_size);
        }
        renderizador->divisoes_sombra = (LumeVec3){extensoes[0], extensoes[1], extensoes[2]};
    }
    for (indice = 0; indice < cena->quantidade_nos && renderizador->quantidade_sombras_spot < 4; ++indice)
    {
        LumeNode *no = cena->nos[indice];
        int mapa;
        if (no->tipo != LUME_NO_LUZ_SPOT || !no->dados.luz.projeta_sombra)
            continue;
        mapa = renderizador->quantidade_sombras_spot++;
        {
            LumeVec3 posicao = lume_mat4_transform_point(no->matriz_mundo, (LumeVec3){0, 0, 0});
            LumeVec3 direcao =
                lume_vec3_normalize(lume_matriz_transformar_direcao(no->matriz_mundo, no->dados.luz.direcao));
            LumeMat4 vista = lume_matriz_olhar(posicao, lume_vec3_add(posicao, direcao),
                                               fabsf(direcao.y) > 0.95f ? (LumeVec3){0, 0, 1} : (LumeVec3){0, 1, 0});
            LumeMat4 projecao =
                lume_mat4_perspective(no->dados.luz.angulo_externo * 2.0f, 1.0f, 0.1f, no->dados.luz.alcance);
            renderizador->matrizes_sombra_spot[mapa] = lume_mat4_multiply(projecao, vista);
        }
        if (lume_preparar_mapa_sombra(&renderizador->framebuffers_sombra_spot[mapa],
                                      &renderizador->texturas_sombra_spot[mapa],
                                      renderizador->configuracao.spot_shadow_size))
            lume_desenhar_mapa_sombra(renderizador, cena, renderizador->matrizes_sombra_spot[mapa],
                                      renderizador->framebuffers_sombra_spot[mapa],
                                      renderizador->configuracao.spot_shadow_size);
    }
    return sol != NULL;
}

static void lume_enviar_luzes(GLuint p, LumeScene *c)
{
    size_t i;
    int nd = 0, np = 0, ns = 0;
    LumeColor ambiente = {0, 0, 0, 1};
    char nome[80];
    for (i = 0; i < c->quantidade_nos; ++i)
    {
        LumeNode *n = c->nos[i];
        LumeDadosLuz *l = &n->dados.luz;
        if (n->tipo == LUME_NO_LUZ_AMBIENTE)
        {
            ambiente.r += l->cor.r * l->intensidade;
            ambiente.g += l->cor.g * l->intensidade;
            ambiente.b += l->cor.b * l->intensidade;
        }
        else if (n->tipo == LUME_NO_LUZ_DIRECIONAL && nd < LUME_MAX_LUZES_DIRECIONAIS)
        {
            snprintf(nome, sizeof(nome), "uDirs[%d].direction", nd);
            glUniform3f(glGetUniformLocation(p, nome), l->direcao.x, l->direcao.y, l->direcao.z);
            snprintf(nome, sizeof(nome), "uDirs[%d].color", nd);
            lume_uniforme_cor(p, nome, l->cor, l->intensidade);
            ++nd;
        }
        else if (n->tipo == LUME_NO_LUZ_PONTUAL && np < LUME_MAX_LUZES_PONTUAIS)
        {
            LumeVec3 pos = lume_mat4_transform_point(n->matriz_mundo, (LumeVec3){0, 0, 0});
            snprintf(nome, sizeof(nome), "uPoints[%d].position", np);
            glUniform3f(glGetUniformLocation(p, nome), pos.x, pos.y, pos.z);
            snprintf(nome, sizeof(nome), "uPoints[%d].color", np);
            lume_uniforme_cor(p, nome, l->cor, l->intensidade);
            snprintf(nome, sizeof(nome), "uPoints[%d].range", np);
            glUniform1f(glGetUniformLocation(p, nome), l->alcance);
            ++np;
        }
        else if (n->tipo == LUME_NO_LUZ_SPOT && ns < LUME_MAX_LUZES_SPOT)
        {
            LumeVec3 pos = lume_mat4_transform_point(n->matriz_mundo, (LumeVec3){0, 0, 0}),
                     dir = lume_matriz_transformar_direcao(n->matriz_mundo, l->direcao);
            snprintf(nome, sizeof(nome), "uSpots[%d].position", ns);
            glUniform3f(glGetUniformLocation(p, nome), pos.x, pos.y, pos.z);
            snprintf(nome, sizeof(nome), "uSpots[%d].direction", ns);
            glUniform3f(glGetUniformLocation(p, nome), dir.x, dir.y, dir.z);
            snprintf(nome, sizeof(nome), "uSpots[%d].color", ns);
            lume_uniforme_cor(p, nome, l->cor, l->intensidade);
            snprintf(nome, sizeof(nome), "uSpots[%d].range", ns);
            glUniform1f(glGetUniformLocation(p, nome), l->alcance);
            snprintf(nome, sizeof(nome), "uSpots[%d].innerCos", ns);
            glUniform1f(glGetUniformLocation(p, nome), cosf(l->angulo_interno));
            snprintf(nome, sizeof(nome), "uSpots[%d].outerCos", ns);
            glUniform1f(glGetUniformLocation(p, nome), cosf(l->angulo_externo));
            ++ns;
        }
    }
    glUniform3f(glGetUniformLocation(p, "uAmbient"), ambiente.r, ambiente.g, ambiente.b);
    glUniform1i(glGetUniformLocation(p, "uDirCount"), nd);
    glUniform1i(glGetUniformLocation(p, "uPointCount"), np);
    glUniform1i(glGetUniformLocation(p, "uSpotCount"), ns);
}

typedef struct LumeItemDesenho
{
    LumeNode *no;
    float distancia;
    bool transparente;
} LumeItemDesenho;
static int lume_comparar_desenho(const void *a, const void *b)
{
    const LumeItemDesenho *x = a, *y = b;
    if (x->transparente != y->transparente)
        return x->transparente ? 1 : -1;
    if (x->transparente)
        return x->distancia > y->distancia ? -1 : x->distancia < y->distancia ? 1 : 0;
    return 0;
}

static LumeResult lume_preparar_hdr(LumeRenderer *r, int l, int h)
{
    GLenum estado;
    if (r->largura_hdr == l && r->altura_hdr == h && r->framebuffer_hdr)
        return LUME_SUCCESS;
    if (r->framebuffer_hdr)
        glDeleteFramebuffers(1, &r->framebuffer_hdr);
    if (r->framebuffer_ping)
        glDeleteFramebuffers(1, &r->framebuffer_ping);
    if (r->textura_hdr)
        glDeleteTextures(1, &r->textura_hdr);
    if (r->textura_ping)
        glDeleteTextures(1, &r->textura_ping);
    if (r->profundidade_hdr)
        glDeleteTextures(1, &r->profundidade_hdr);
    glGenFramebuffers(1, &r->framebuffer_hdr);
    glBindFramebuffer(GL_FRAMEBUFFER, r->framebuffer_hdr);
    glGenTextures(1, &r->textura_hdr);
    glBindTexture(GL_TEXTURE_2D, r->textura_hdr);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, l, h, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, r->textura_hdr, 0);
    glGenTextures(1, &r->profundidade_hdr);
    glBindTexture(GL_TEXTURE_2D, r->profundidade_hdr);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, l, h, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, r->profundidade_hdr, 0);
    estado = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    if (estado != GL_FRAMEBUFFER_COMPLETE)
        return lume_definir_erro(LUME_ERROR_GPU, "renderer.hdr", NULL, 0, 0,
                                 "OpenGL could not create the HDR framebuffer (status 0x%X).", estado);
    glGenFramebuffers(1, &r->framebuffer_ping);
    glBindFramebuffer(GL_FRAMEBUFFER, r->framebuffer_ping);
    glGenTextures(1, &r->textura_ping);
    glBindTexture(GL_TEXTURE_2D, r->textura_ping);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, l, h, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, r->textura_ping, 0);
    estado = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    if (estado != GL_FRAMEBUFFER_COMPLETE)
        return lume_definir_erro(LUME_ERROR_GPU, "renderer.hdr", NULL, 0, 0,
                                 "OpenGL could not create the post-processing framebuffer (status 0x%X).", estado);
    r->largura_hdr = l;
    r->altura_hdr = h;
    return LUME_SUCCESS;
}

static bool lume_possui_passes(const LumeRenderer *renderizador, LumePassPhase fase)
{
    size_t indice;
    for (indice = 0; indice < renderizador->quantidade_passagens; ++indice)
        if (renderizador->passagens[indice].habilitada && renderizador->passagens[indice].fase == fase)
            return true;
    return false;
}

static GLuint lume_executar_passes(LumeRenderer *renderizador, LumePassPhase fase, GLuint textura_inicial,
                                   GLuint framebuffer_inicial, int largura, int altura, GLuint *framebuffer_final)
{
    GLuint textura_atual = textura_inicial;
    GLuint framebuffer_atual = framebuffer_inicial;
    size_t indice;

    for (indice = 0; indice < renderizador->quantidade_passagens; ++indice)
    {
        LumePassagemInterna *passagem = &renderizador->passagens[indice];
        LumePipeline *pipeline = passagem->pipeline;
        GLuint destino;
        GLuint programa;
        if (!passagem->habilitada || passagem->fase != fase)
            continue;

        destino = framebuffer_atual == renderizador->framebuffer_hdr ? renderizador->framebuffer_ping
                                                                     : renderizador->framebuffer_hdr;
        programa = pipeline->shader->programa;
        glBindFramebuffer(GL_FRAMEBUFFER, destino);
        glViewport(0, 0, largura, altura);
        if (pipeline->teste_profundidade)
            glEnable(GL_DEPTH_TEST);
        else
            glDisable(GL_DEPTH_TEST);
        glDepthMask(pipeline->escrita_profundidade ? GL_TRUE : GL_FALSE);
        if (pipeline->mistura)
        {
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        }
        else
            glDisable(GL_BLEND);
        glUseProgram(programa);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textura_atual);
        glUniform1i(glGetUniformLocation(programa, "uColor"), 0);
        if (passagem->precisa_profundidade)
        {
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, renderizador->profundidade_hdr);
            glUniform1i(glGetUniformLocation(programa, "uDepth"), 1);
        }
        glBindVertexArray(renderizador->vao_tela);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        ++renderizador->estatisticas.draw_calls;
        framebuffer_atual = destino;
        textura_atual =
            destino == renderizador->framebuffer_hdr ? renderizador->textura_hdr : renderizador->textura_ping;
    }
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    *framebuffer_final = framebuffer_atual;
    return textura_atual;
}

static void lume_aplicar_tom(LumeRenderer *renderizador, GLuint textura, GLuint destino, int largura, int altura)
{
    glBindFramebuffer(GL_FRAMEBUFFER, destino);
    glViewport(0, 0, largura, altura);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);
    glUseProgram(renderizador->programa_tom);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textura);
    glUniform1i(glGetUniformLocation(renderizador->programa_tom, "uColor"), 0);
    glUniform1f(glGetUniformLocation(renderizador->programa_tom, "uExposure"), renderizador->configuracao.exposure);
    glUniform1i(glGetUniformLocation(renderizador->programa_tom, "uTone"), renderizador->configuracao.tone_mapping);
    glUniform1i(glGetUniformLocation(renderizador->programa_tom, "uFxaa"), renderizador->configuracao.fxaa);
    glUniform1i(glGetUniformLocation(renderizador->programa_tom, "uBloom"), renderizador->configuracao.bloom);
    glBindVertexArray(renderizador->vao_tela);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    ++renderizador->estatisticas.draw_calls;
}

static void lume_apresentar_textura(LumeRenderer *renderizador, GLuint textura, int largura, int altura)
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, largura, altura);
    glDisable(GL_DEPTH_TEST);
    glUseProgram(renderizador->programa_copia);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textura);
    glUniform1i(glGetUniformLocation(renderizador->programa_copia, "uColor"), 0);
    glBindVertexArray(renderizador->vao_tela);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    ++renderizador->estatisticas.draw_calls;
}

static void lume_desenhar_debug(LumeRenderer *r, LumeMat4 vp, int l, int h)
{
    typedef struct
    {
        float p[3], c[4];
    } V;
    V *vertices;
    size_t i;
    if (!r->quantidade_linhas)
        return;
    vertices = malloc(r->quantidade_linhas * 2 * sizeof(*vertices));
    if (!vertices)
        return;
    for (i = 0; i < r->quantidade_linhas; ++i)
    {
        LumeLinhaDepuracao *x = &r->linhas[i];
        vertices[i * 2] = (V){{x->inicio.x, x->inicio.y, x->inicio.z}, {x->cor.r, x->cor.g, x->cor.b, x->cor.a}};
        vertices[i * 2 + 1] = (V){{x->fim.x, x->fim.y, x->fim.z}, {x->cor.r, x->cor.g, x->cor.b, x->cor.a}};
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, l, h);
    glUseProgram(r->programa_debug);
    lume_uniforme_mat(r->programa_debug, "uViewProjection", vp);
    glBindVertexArray(r->vao_debug);
    glBindBuffer(GL_ARRAY_BUFFER, r->vbo_debug);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(r->quantidade_linhas * 2 * sizeof(*vertices)), vertices, GL_STREAM_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(V), (void *)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(V), (void *)(3 * sizeof(float)));
    glDisable(GL_DEPTH_TEST);
    glDrawArrays(GL_LINES, 0, (GLsizei)(r->quantidade_linhas * 2));
    glEnable(GL_DEPTH_TEST);
    glBindVertexArray(0);
    free(vertices);
    r->quantidade_linhas = 0;
}

LumeResult lume_renderer_render(LumeRenderer *r, LumeScene *c, LumeNode *camera, LumeRenderTarget *alvo)
{
    LumeMat4 vista, proj, vp;
    int l, h;
    size_t i, q = 0;
    LumeItemDesenho *itens;
    LumeVec3 cam;
    LumeFrustum fr;
    bool possui_sombra_direcional;
    double inicio;
    if (!r || !c || c->aplicativo != r->aplicativo)
        return lume_definir_erro(LUME_ERROR_INVALID_ARGUMENT, "renderer.render", NULL, 0, 0,
                                 "Renderer and scene must belong to the same application.");
    if (!lume_camera_valida(c, camera))
        return lume_definir_erro(LUME_ERROR_INVALID_ARGUMENT, "renderer.render", NULL, 0, 0,
                                 "A camera from the rendered scene is required.");
    if (alvo && alvo->referencia.aplicativo != r->aplicativo)
        return lume_definir_erro(LUME_ERROR_INVALID_ARGUMENT, "renderer.render", NULL, 0, 0,
                                 "Render target must belong to the renderer application.");
    inicio = glfwGetTime();
    glfwMakeContextCurrent(r->aplicativo->janela);
    if (alvo)
    {
        l = alvo->largura;
        h = alvo->altura;
    }
    else
        lume_app_get_framebuffer_size(r->aplicativo, &l, &h);
    if (l <= 0 || h <= 0)
        return LUME_SUCCESS;
    lume_atualizar_matrizes_cena(c);
    if (!lume_mat4_inverse(camera->matriz_mundo, &vista))
        return lume_definir_erro(LUME_ERROR_INVALID_ARGUMENT, "renderer.render", NULL, 0, 0,
                                 "The camera transform is not invertible.");
    if (camera->tipo == LUME_NO_CAMERA_PERSPECTIVA)
    {
        LumeDadosCameraPerspectiva *d = &camera->dados.perspectiva;
        proj = lume_mat4_perspective(d->campo_visao, d->proporcao > 0 ? d->proporcao : (float)l / h, d->plano_proximo,
                                     d->plano_distante);
    }
    else
    {
        LumeDadosCameraOrtografica *d = &camera->dados.ortografica;
        proj = lume_mat4_orthographic(d->esquerda, d->direita, d->inferior, d->superior, d->plano_proximo,
                                      d->plano_distante);
    }
    vp = lume_mat4_multiply(proj, vista);
    fr = lume_frustum_from_matrix(vp);
    cam = lume_mat4_transform_point(camera->matriz_mundo, (LumeVec3){0, 0, 0});
    {
        uint64_t proximo_quadro = r->estatisticas.frame_index + 1;
        memset(&r->estatisticas, 0, sizeof(r->estatisticas));
        r->estatisticas.frame_index = proximo_quadro;
    }
    possui_sombra_direcional = lume_renderizar_sombras(r, c, camera);
    itens = malloc(c->quantidade_nos * sizeof(*itens));
    if (!itens)
        return lume_definir_erro(LUME_ERROR_OUT_OF_MEMORY, "renderer.render", NULL, 0, 0,
                                 "Out of memory while building the draw queue.");
    for (i = 0; i < c->quantidade_nos; ++i)
    {
        LumeNode *n = c->nos[i];
        if (n->tipo != LUME_NO_MALHA && n->tipo != LUME_NO_MALHA_INSTANCIADA)
            continue;
        ++r->estatisticas.submitted_objects;
        if (r->configuracao.frustum_culling && !lume_frustum_intersects_aabb(fr, lume_node_world_bounds(n)))
        {
            ++r->estatisticas.culled_objects;
            continue;
        }
        itens[q++] = (LumeItemDesenho){
            n,
            lume_vec3_length(lume_vec3_subtract(lume_mat4_transform_point(n->matriz_mundo, (LumeVec3){0, 0, 0}), cam)),
            n->dados.malha.material->modo_alpha == LUME_ALPHA_BLEND};
    }
    qsort(itens, q, sizeof(*itens), lume_comparar_desenho);
    if (!alvo && r->configuracao.hdr)
    {
        LumeResult rr = lume_preparar_hdr(r, l, h);
        if (rr != LUME_SUCCESS)
        {
            free(itens);
            return rr;
        }
        glBindFramebuffer(GL_FRAMEBUFFER, r->framebuffer_hdr);
    }
    else
        glBindFramebuffer(GL_FRAMEBUFFER, alvo ? alvo->framebuffer : 0);
    glViewport(0, 0, l, h);
    glClearColor(r->aplicativo->cor_limpeza.r, r->aplicativo->cor_limpeza.g, r->aplicativo->cor_limpeza.b,
                 r->aplicativo->cor_limpeza.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    for (i = 0; i < q; ++i)
    {
        LumeNode *n = itens[i].no;
        LumeGeometry *g = n->dados.malha.geometria;
        LumeMaterial *m = n->dados.malha.material;
        LumePipeline *pipeline = m->tipo == LUME_MATERIAL_CUSTOM ? m->pipeline : NULL;
        GLuint programa = pipeline ? pipeline->shader->programa : r->programa_padrao;
        uint32_t instancias = n->tipo == LUME_NO_MALHA_INSTANCIADA ? n->dados.malha.quantidade_instancias : 1;
        if (!lume_enviar_geometria_gpu(g))
        {
            free(itens);
            return LUME_ERROR_GPU;
        }
        glUseProgram(programa);
        lume_uniforme_mat(programa, "uModel", n->matriz_mundo);
        lume_uniforme_mat(programa, "uView", vista);
        lume_uniforme_mat(programa, "uProjection", proj);
        glUniform3f(glGetUniformLocation(programa, "uCamera"), cam.x, cam.y, cam.z);
        glUniform1i(glGetUniformLocation(programa, "uInstanced"), n->tipo == LUME_NO_MALHA_INSTANCIADA);
        if (!pipeline)
        {
            int sombra;
            char nome_uniforme[64];
            glUniform4f(glGetUniformLocation(programa, "uBaseColor"), m->cor_base.r, m->cor_base.g, m->cor_base.b,
                        m->cor_base.a);
            glUniform1i(glGetUniformLocation(programa, "uMaterialType"), m->tipo);
            glUniform1f(glGetUniformLocation(programa, "uMetallic"), m->metalico);
            glUniform1f(glGetUniformLocation(programa, "uRoughness"), m->rugosidade);
            glUniform1f(glGetUniformLocation(programa, "uAlphaCutoff"), m->corte_alpha);
            glUniform1i(glGetUniformLocation(programa, "uAlphaMode"), m->modo_alpha);
            glUniform1i(glGetUniformLocation(programa, "uHasTexture"), m->textura_base != NULL);
            glUniform1i(glGetUniformLocation(programa, "uBaseTexture"), 0);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, m->textura_base ? m->textura_base->identificador : 0);
            glUniform1i(glGetUniformLocation(programa, "uEnvironment"), 1);
            glUniform1i(glGetUniformLocation(programa, "uHasEnvironment"), r->ambiente != NULL);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, r->ambiente ? r->ambiente->equiretangular->identificador : 0);
            glUniform1i(glGetUniformLocation(programa, "uHasDirShadows"), possui_sombra_direcional);
            glUniform3f(glGetUniformLocation(programa, "uCascadeSplits"), r->divisoes_sombra.x, r->divisoes_sombra.y,
                        r->divisoes_sombra.z);
            glUniform1i(glGetUniformLocation(programa, "uSpotShadowCount"), r->quantidade_sombras_spot);
            for (sombra = 0; sombra < 3; ++sombra)
            {
                snprintf(nome_uniforme, sizeof(nome_uniforme), "uDirShadowMatrices[%d]", sombra);
                lume_uniforme_mat(programa, nome_uniforme, r->matrizes_sombra_direcional[sombra]);
                snprintf(nome_uniforme, sizeof(nome_uniforme), "uDirShadow%d", sombra);
                glUniform1i(glGetUniformLocation(programa, nome_uniforme), 2 + sombra);
                glActiveTexture(GL_TEXTURE2 + sombra);
                glBindTexture(GL_TEXTURE_2D, r->texturas_sombra_direcional[sombra]);
            }
            for (sombra = 0; sombra < 4; ++sombra)
            {
                snprintf(nome_uniforme, sizeof(nome_uniforme), "uSpotShadowMatrices[%d]", sombra);
                lume_uniforme_mat(programa, nome_uniforme, r->matrizes_sombra_spot[sombra]);
                snprintf(nome_uniforme, sizeof(nome_uniforme), "uSpotShadow%d", sombra);
                glUniform1i(glGetUniformLocation(programa, nome_uniforme), 5 + sombra);
                glActiveTexture(GL_TEXTURE5 + sombra);
                glBindTexture(GL_TEXTURE_2D, r->texturas_sombra_spot[sombra]);
            }
            lume_enviar_luzes(programa, c);
        }
        if (pipeline ? pipeline->mistura : m->modo_alpha == LUME_ALPHA_BLEND)
        {
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glDepthMask(GL_FALSE);
        }
        else
        {
            glDisable(GL_BLEND);
            glDepthMask(GL_TRUE);
        }
        if (pipeline ? !pipeline->descartar_costas : m->dupla_face)
            glDisable(GL_CULL_FACE);
        else
            glEnable(GL_CULL_FACE);
        if (pipeline ? pipeline->teste_profundidade : true)
            glEnable(GL_DEPTH_TEST);
        else
            glDisable(GL_DEPTH_TEST);
        if (pipeline)
            glDepthMask(pipeline->escrita_profundidade ? GL_TRUE : GL_FALSE);
        glPolygonMode(GL_FRONT_AND_BACK, m->aramado ? GL_LINE : GL_FILL);
        glBindVertexArray(g->vao);
        if (n->tipo == LUME_NO_MALHA_INSTANCIADA)
        {
            int k;
            if (!n->dados.malha.vbo_instancias)
                glGenBuffers(1, &n->dados.malha.vbo_instancias);
            glBindBuffer(GL_ARRAY_BUFFER, n->dados.malha.vbo_instancias);
            glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(instancias * sizeof(LumeMat4)), n->dados.malha.instancias,
                         GL_STREAM_DRAW);
            for (k = 0; k < 4; ++k)
            {
                glEnableVertexAttribArray(3 + k);
                glVertexAttribPointer(3 + k, 4, GL_FLOAT, GL_FALSE, sizeof(LumeMat4),
                                      (void *)(size_t)(k * 4 * sizeof(float)));
                glVertexAttribDivisor(3 + k, 1);
            }
            glDrawElementsInstanced(GL_TRIANGLES, (GLsizei)g->quantidade_indices, GL_UNSIGNED_INT, NULL,
                                    (GLsizei)instancias);
        }
        else
            glDrawElements(GL_TRIANGLES, (GLsizei)g->quantidade_indices, GL_UNSIGNED_INT, NULL);
        ++r->estatisticas.draw_calls;
        r->estatisticas.instances += instancias;
        r->estatisticas.triangles += (g->quantidade_indices / 3) * instancias;
    }
    free(itens);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glBindVertexArray(0);
    if (!alvo && r->configuracao.hdr)
    {
        GLuint framebuffer_atual = r->framebuffer_hdr;
        GLuint textura_atual =
            lume_executar_passes(r, LUME_PASS_HDR, r->textura_hdr, r->framebuffer_hdr, l, h, &framebuffer_atual);
        if (lume_possui_passes(r, LUME_PASS_LDR))
        {
            GLuint destino_tom = framebuffer_atual == r->framebuffer_hdr ? r->framebuffer_ping : r->framebuffer_hdr;
            lume_aplicar_tom(r, textura_atual, destino_tom, l, h);
            framebuffer_atual = destino_tom;
            textura_atual = destino_tom == r->framebuffer_hdr ? r->textura_hdr : r->textura_ping;
            textura_atual =
                lume_executar_passes(r, LUME_PASS_LDR, textura_atual, framebuffer_atual, l, h, &framebuffer_atual);
            lume_apresentar_textura(r, textura_atual, l, h);
        }
        else
            lume_aplicar_tom(r, textura_atual, 0, l, h);
        glEnable(GL_DEPTH_TEST);
    }
    lume_desenhar_debug(r, vp, l, h);
    r->estatisticas.cpu_time_ms = (glfwGetTime() - inicio) * 1000.0;
    return LUME_SUCCESS;
}
LumeResult lume_app_render(LumeApp *a, LumeScene *c, LumeNode *camera)
{
    return a ? lume_renderer_render(a->renderizador, c, camera, NULL)
             : lume_definir_erro(LUME_ERROR_INVALID_ARGUMENT, "app.render", NULL, 0, 0,
                                 "A valid application is required.");
}
LumeFrameStats lume_renderer_frame_stats(const LumeRenderer *r)
{
    return r ? r->estatisticas : (LumeFrameStats){0};
}

void lume_debug_line(LumeRenderer *r, LumeVec3 a, LumeVec3 b, LumeColor c)
{
    if (!r)
        return;
    if (r->quantidade_linhas == r->capacidade_linhas)
    {
        size_t nova = r->capacidade_linhas ? r->capacidade_linhas * 2 : 64;
        void *p = realloc(r->linhas, nova * sizeof(*r->linhas));
        if (!p)
            return;
        r->linhas = p;
        r->capacidade_linhas = nova;
    }
    r->linhas[r->quantidade_linhas++] = (LumeLinhaDepuracao){a, b, c};
}
void lume_debug_axes(LumeRenderer *r, LumeMat4 m, float s)
{
    LumeVec3 o = lume_mat4_transform_point(m, (LumeVec3){0, 0, 0});
    lume_debug_line(r, o, lume_mat4_transform_point(m, (LumeVec3){s, 0, 0}), (LumeColor){1, 0, 0, 1});
    lume_debug_line(r, o, lume_mat4_transform_point(m, (LumeVec3){0, s, 0}), (LumeColor){0, 1, 0, 1});
    lume_debug_line(r, o, lume_mat4_transform_point(m, (LumeVec3){0, 0, s}), (LumeColor){0, 0, 1, 1});
}
void lume_debug_aabb(LumeRenderer *r, LumeAabb b, LumeColor c)
{
    LumeVec3 p[8];
    int i;
    static const int e[24] = {0, 1, 0, 2, 0, 4, 1, 3, 1, 5, 2, 3, 2, 6, 3, 7, 4, 5, 4, 6, 5, 7, 6, 7};
    for (i = 0; i < 8; ++i)
        p[i] = (LumeVec3){i & 1 ? b.max.x : b.min.x, i & 2 ? b.max.y : b.min.y, i & 4 ? b.max.z : b.min.z};
    for (i = 0; i < 24; i += 2)
        lume_debug_line(r, p[e[i]], p[e[i + 1]], c);
}
void lume_debug_sphere(LumeRenderer *r, LumeSphere s, LumeColor c)
{
    int i;
    for (i = 0; i < 32; ++i)
    {
        float a = (float)i * 6.283185307f / 32, b = (float)(i + 1) * 6.283185307f / 32;
        lume_debug_line(r, lume_vec3_add(s.center, (LumeVec3){cosf(a) * s.radius, sinf(a) * s.radius, 0}),
                        lume_vec3_add(s.center, (LumeVec3){cosf(b) * s.radius, sinf(b) * s.radius, 0}), c);
        lume_debug_line(r, lume_vec3_add(s.center, (LumeVec3){cosf(a) * s.radius, 0, sinf(a) * s.radius}),
                        lume_vec3_add(s.center, (LumeVec3){cosf(b) * s.radius, 0, sinf(b) * s.radius}), c);
    }
}
void lume_debug_ray(LumeRenderer *r, LumeRay raio, float l, LumeColor c)
{
    lume_debug_line(r, raio.origin, lume_vec3_add(raio.origin, lume_vec3_scale(lume_vec3_normalize(raio.direction), l)),
                    c);
}
void lume_debug_clear(LumeRenderer *r)
{
    if (r)
        r->quantidade_linhas = 0;
}
