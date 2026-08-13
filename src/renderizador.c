#include "lume_interno.h"

#include <stdio.h>
#include <string.h>

static const char *lume_shader_vertice = "#version 330 core\n"
                                         "layout(location = 0) in vec3 aPosition;\n"
                                         "layout(location = 1) in vec3 aNormal;\n"
                                         "layout(location = 2) in vec2 aTexCoord;\n"
                                         "uniform mat4 uModel;\n"
                                         "uniform mat4 uView;\n"
                                         "uniform mat4 uProjection;\n"
                                         "out vec3 vWorldPosition;\n"
                                         "out vec3 vNormal;\n"
                                         "out vec2 vTexCoord;\n"
                                         "void main() {\n"
                                         "  vec4 world = uModel * vec4(aPosition, 1.0);\n"
                                         "  vWorldPosition = world.xyz;\n"
                                         "  vNormal = normalize(transpose(inverse(mat3(uModel))) * aNormal);\n"
                                         "  vTexCoord = aTexCoord;\n"
                                         "  gl_Position = uProjection * uView * world;\n"
                                         "}\n";

static const char *lume_shader_fragmento =
    "#version 330 core\n"
    "#define MAX_DIRECTIONAL_LIGHTS 4\n"
    "#define MAX_POINT_LIGHTS 4\n"
    "struct DirectionalLight { vec3 direction; vec3 color; };\n"
    "struct PointLight { vec3 position; vec3 color; float range; };\n"
    "uniform vec4 uColor;\n"
    "uniform bool uUseLighting;\n"
    "uniform bool uHasTexture;\n"
    "uniform sampler2D uTexture;\n"
    "uniform vec3 uAmbientLight;\n"
    "uniform int uDirectionalLightCount;\n"
    "uniform int uPointLightCount;\n"
    "uniform DirectionalLight uDirectionalLights[MAX_DIRECTIONAL_LIGHTS];\n"
    "uniform PointLight uPointLights[MAX_POINT_LIGHTS];\n"
    "in vec3 vWorldPosition;\n"
    "in vec3 vNormal;\n"
    "in vec2 vTexCoord;\n"
    "out vec4 outColor;\n"
    "void main() {\n"
    "  vec4 base = uColor;\n"
    "  if (uHasTexture) base *= texture(uTexture, vTexCoord);\n"
    "  if (!uUseLighting) { outColor = base; return; }\n"
    "  vec3 normal = normalize(vNormal);\n"
    "  vec3 light = uAmbientLight;\n"
    "  for (int i = 0; i < uDirectionalLightCount; ++i) {\n"
    "    float diffuse = max(dot(normal, normalize(-uDirectionalLights[i].direction)), 0.0);\n"
    "    light += uDirectionalLights[i].color * diffuse;\n"
    "  }\n"
    "  for (int i = 0; i < uPointLightCount; ++i) {\n"
    "    vec3 offset = uPointLights[i].position - vWorldPosition;\n"
    "    float distanceToLight = length(offset);\n"
    "    float attenuation = clamp(1.0 - distanceToLight / uPointLights[i].range, 0.0, 1.0);\n"
    "    attenuation *= attenuation;\n"
    "    float diffuse = max(dot(normal, normalize(offset)), 0.0);\n"
    "    light += uPointLights[i].color * diffuse * attenuation;\n"
    "  }\n"
    "  outColor = vec4(base.rgb * light, base.a);\n"
    "}\n";

static GLuint lume_compilar_shader(LumeApp *aplicativo, GLenum tipo, const char *fonte, const char *nome)
{
    GLuint shader = glCreateShader(tipo);
    GLint sucesso;
    char registro[2048];
    glShaderSource(shader, 1, &fonte, NULL);
    glCompileShader(shader);
    glGetShaderiv(shader, GL_COMPILE_STATUS, &sucesso);
    if (!sucesso)
    {
        glGetShaderInfoLog(shader, sizeof(registro), NULL, registro);
        lume_definir_erro("The built-in %s shader could not be compiled: %s", nome, registro);
        lume_registrar_log(aplicativo, LUME_LOG_ERROR, "%s", lume_get_last_error());
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

bool lume_inicializar_renderizador(LumeApp *aplicativo)
{
    GLuint shader_vertice = lume_compilar_shader(aplicativo, GL_VERTEX_SHADER, lume_shader_vertice, "vertex");
    GLuint shader_fragmento;
    GLint sucesso;
    char registro[2048];
    if (!shader_vertice)
    {
        return false;
    }
    shader_fragmento = lume_compilar_shader(aplicativo, GL_FRAGMENT_SHADER, lume_shader_fragmento, "fragment");
    if (!shader_fragmento)
    {
        glDeleteShader(shader_vertice);
        return false;
    }
    aplicativo->programa_shader = glCreateProgram();
    glAttachShader(aplicativo->programa_shader, shader_vertice);
    glAttachShader(aplicativo->programa_shader, shader_fragmento);
    glLinkProgram(aplicativo->programa_shader);
    glDeleteShader(shader_vertice);
    glDeleteShader(shader_fragmento);
    glGetProgramiv(aplicativo->programa_shader, GL_LINK_STATUS, &sucesso);
    if (!sucesso)
    {
        glGetProgramInfoLog(aplicativo->programa_shader, sizeof(registro), NULL, registro);
        lume_definir_erro("The built-in shader program could not be linked: %s", registro);
        glDeleteProgram(aplicativo->programa_shader);
        aplicativo->programa_shader = 0;
        return false;
    }
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    return true;
}

void lume_destruir_renderizador(LumeApp *aplicativo)
{
    if (aplicativo && aplicativo->programa_shader)
    {
        glDeleteProgram(aplicativo->programa_shader);
        aplicativo->programa_shader = 0;
    }
}

static void lume_uniforme_matriz(GLuint programa, const char *nome, LumeMatriz4 matriz)
{
    GLint local = glGetUniformLocation(programa, nome);
    if (local >= 0)
    {
        glUniformMatrix4fv(local, 1, GL_FALSE, matriz.valor);
    }
}

static void lume_uniforme_cor3(GLuint programa, const char *nome, LumeColor cor, float intensidade)
{
    GLint local = glGetUniformLocation(programa, nome);
    if (local >= 0)
    {
        glUniform3f(local, cor.r * intensidade, cor.g * intensidade, cor.b * intensidade);
    }
}

static void lume_enviar_luzes(LumeApp *aplicativo, LumeScene *cena)
{
    LumeColor ambiente = {0.0f, 0.0f, 0.0f, 1.0f};
    int quantidade_direcionais = 0;
    int quantidade_pontuais = 0;
    bool avisou_direcionais = false;
    bool avisou_pontuais = false;
    size_t indice;
    char nome[96];

    for (indice = 0; indice < cena->quantidade_nos; ++indice)
    {
        LumeNode *no = cena->nos[indice];
        if (no->tipo == LUME_NO_LUZ_AMBIENTE)
        {
            ambiente.r += no->dados.luz.cor.r * no->dados.luz.intensidade;
            ambiente.g += no->dados.luz.cor.g * no->dados.luz.intensidade;
            ambiente.b += no->dados.luz.cor.b * no->dados.luz.intensidade;
        }
        else if (no->tipo == LUME_NO_LUZ_DIRECIONAL)
        {
            LumeVec3 direcao;
            if (quantidade_direcionais >= LUME_MAX_LUZES_DIRECIONAIS)
            {
                if (!avisou_direcionais)
                {
                    lume_registrar_log(
                        aplicativo, LUME_LOG_WARNING,
                        "The scene exceeds the limit of %d directional lights; additional lights were ignored.",
                        LUME_MAX_LUZES_DIRECIONAIS);
                    avisou_direcionais = true;
                }
                continue;
            }
            direcao = lume_vetor3_normalizar(lume_matriz_transformar_direcao(no->matriz_mundo, no->dados.luz.direcao));
            snprintf(nome, sizeof(nome), "uDirectionalLights[%d].direction", quantidade_direcionais);
            glUniform3f(glGetUniformLocation(aplicativo->programa_shader, nome), direcao.x, direcao.y, direcao.z);
            snprintf(nome, sizeof(nome), "uDirectionalLights[%d].color", quantidade_direcionais);
            lume_uniforme_cor3(aplicativo->programa_shader, nome, no->dados.luz.cor, no->dados.luz.intensidade);
            ++quantidade_direcionais;
        }
        else if (no->tipo == LUME_NO_LUZ_PONTUAL)
        {
            LumeVec3 posicao;
            if (quantidade_pontuais >= LUME_MAX_LUZES_PONTUAIS)
            {
                if (!avisou_pontuais)
                {
                    lume_registrar_log(
                        aplicativo, LUME_LOG_WARNING,
                        "The scene exceeds the limit of %d point lights; additional lights were ignored.",
                        LUME_MAX_LUZES_PONTUAIS);
                    avisou_pontuais = true;
                }
                continue;
            }
            posicao = lume_matriz_transformar_ponto(no->matriz_mundo, (LumeVec3){0.0f, 0.0f, 0.0f});
            snprintf(nome, sizeof(nome), "uPointLights[%d].position", quantidade_pontuais);
            glUniform3f(glGetUniformLocation(aplicativo->programa_shader, nome), posicao.x, posicao.y, posicao.z);
            snprintf(nome, sizeof(nome), "uPointLights[%d].color", quantidade_pontuais);
            lume_uniforme_cor3(aplicativo->programa_shader, nome, no->dados.luz.cor, no->dados.luz.intensidade);
            snprintf(nome, sizeof(nome), "uPointLights[%d].range", quantidade_pontuais);
            glUniform1f(glGetUniformLocation(aplicativo->programa_shader, nome), no->dados.luz.alcance);
            ++quantidade_pontuais;
        }
    }
    glUniform3f(glGetUniformLocation(aplicativo->programa_shader, "uAmbientLight"), ambiente.r, ambiente.g, ambiente.b);
    glUniform1i(glGetUniformLocation(aplicativo->programa_shader, "uDirectionalLightCount"), quantidade_direcionais);
    glUniform1i(glGetUniformLocation(aplicativo->programa_shader, "uPointLightCount"), quantidade_pontuais);
}

static bool lume_camera_valida(const LumeScene *cena, const LumeNode *camera)
{
    return camera && camera->cena == cena &&
           (camera->tipo == LUME_NO_CAMERA_PERSPECTIVA || camera->tipo == LUME_NO_CAMERA_ORTOGRAFICA);
}

bool lume_render(LumeApp *aplicativo, LumeScene *cena, LumeNode *camera)
{
    LumeMatriz4 visualizacao;
    LumeMatriz4 projecao;
    int largura;
    int altura;
    size_t indice;

    if (!aplicativo || !cena || cena->aplicativo != aplicativo)
    {
        lume_definir_erro("lume_render requires an application and one of its scenes.");
        return false;
    }
    if (!lume_camera_valida(cena, camera))
    {
        lume_definir_erro("lume_render requires a camera from the rendered scene.");
        return false;
    }
    glfwMakeContextCurrent(aplicativo->janela);
    lume_app_get_framebuffer_size(aplicativo, &largura, &altura);
    if (largura <= 0 || altura <= 0)
    {
        return true;
    }
    lume_atualizar_matrizes_cena(cena);
    if (!lume_matriz_inverter(camera->matriz_mundo, &visualizacao))
    {
        lume_definir_erro("The camera transform is not invertible.");
        return false;
    }
    if (camera->tipo == LUME_NO_CAMERA_PERSPECTIVA)
    {
        LumeDadosCameraPerspectiva *dados = &camera->dados.perspectiva;
        float proporcao = dados->proporcao > 0.0f ? dados->proporcao : (float)largura / (float)altura;
        projecao = lume_matriz_perspectiva(dados->campo_visao, proporcao, dados->plano_proximo, dados->plano_distante);
    }
    else
    {
        LumeDadosCameraOrtografica *dados = &camera->dados.ortografica;
        projecao = lume_matriz_ortografica(dados->esquerda, dados->direita, dados->inferior, dados->superior,
                                           dados->plano_proximo, dados->plano_distante);
    }

    glViewport(0, 0, largura, altura);
    glClearColor(aplicativo->cor_limpeza.r, aplicativo->cor_limpeza.g, aplicativo->cor_limpeza.b,
                 aplicativo->cor_limpeza.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(aplicativo->programa_shader);
    lume_uniforme_matriz(aplicativo->programa_shader, "uView", visualizacao);
    lume_uniforme_matriz(aplicativo->programa_shader, "uProjection", projecao);
    glUniform1i(glGetUniformLocation(aplicativo->programa_shader, "uTexture"), 0);
    lume_enviar_luzes(aplicativo, cena);

    for (indice = 0; indice < cena->quantidade_nos; ++indice)
    {
        LumeNode *no = cena->nos[indice];
        LumeGeometry *geometria;
        LumeMaterial *material;
        if (no->tipo != LUME_NO_MALHA)
        {
            continue;
        }
        geometria = no->dados.malha.geometria;
        material = no->dados.malha.material;
        if (!lume_enviar_geometria_gpu(geometria))
        {
            return false;
        }
        lume_uniforme_matriz(aplicativo->programa_shader, "uModel", no->matriz_mundo);
        glUniform4f(glGetUniformLocation(aplicativo->programa_shader, "uColor"), material->cor.r, material->cor.g,
                    material->cor.b, material->cor.a);
        glUniform1i(glGetUniformLocation(aplicativo->programa_shader, "uUseLighting"),
                    material->tipo == LUME_MATERIAL_LAMBERT);
        glUniform1i(glGetUniformLocation(aplicativo->programa_shader, "uHasTexture"), material->textura != NULL);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, material->textura ? material->textura->identificador : 0);
        glPolygonMode(GL_FRONT_AND_BACK, material->aramado ? GL_LINE : GL_FILL);
        glBindVertexArray(geometria->vao);
        glDrawElements(GL_TRIANGLES, (GLsizei)geometria->quantidade_indices, GL_UNSIGNED_INT, NULL);
    }
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(0);
    return true;
}
