#include <lume/lume.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define OCEANO_COLUNAS 180u
#define OCEANO_LINHAS 120u
#define OCEANO_LARGURA 24.0f
#define OCEANO_PROFUNDIDADE 16.0f

static const char *shader_vertice_oceano =
    "#version 330 core\n"
    "layout(location=0) in vec3 aPosition;"
    "uniform mat4 uModel;uniform mat4 uView;uniform mat4 uProjection;uniform float uTime;"
    "out vec3 vWorld;out float vHeight;"
    "float wave(vec2 p,vec2 direction,float frequency,float speed,float amplitude){"
    "return sin(dot(p,normalize(direction))*frequency+uTime*speed)*amplitude;}"
    "void main(){vec3 p=aPosition;"
    "p.y+=wave(p.xz,vec2(1.0,.25),.72,1.45,.42);"
    "p.y+=wave(p.xz,vec2(-.35,1.0),1.18,1.05,.22);"
    "p.y+=wave(p.xz,vec2(.7,-.65),2.35,2.2,.075);"
    "p.y+=sin((p.x+p.z)*3.1-uTime*2.8)*.025;"
    "vHeight=p.y;vWorld=(uModel*vec4(p,1.0)).xyz;gl_Position=uProjection*uView*vec4(vWorld,1.0);}";

static const char *shader_fragmento_oceano =
    "#version 330 core\n"
    "in vec3 vWorld;in float vHeight;out vec4 FragColor;"
    "uniform vec3 uCamera;uniform vec3 uSunDirection;uniform vec3 uDeepColor;uniform vec3 uShallowColor;"
    "void main(){vec3 dx=dFdx(vWorld),dy=dFdy(vWorld);vec3 n=normalize(cross(dx,dy));if(n.y<0.0)n=-n;"
    "vec3 viewDirection=normalize(uCamera-vWorld);vec3 lightDirection=normalize(-uSunDirection);"
    "float fresnel=pow(1.0-max(dot(n,viewDirection),0.0),4.0);"
    "float diffuse=max(dot(n,lightDirection),0.0);"
    "float sparkle=pow(max(dot(reflect(-lightDirection,n),viewDirection),0.0),180.0);"
    "float depthMix=smoothstep(-.55,.45,vHeight);vec3 water=mix(uDeepColor,uShallowColor,depthMix);"
    "vec3 sky=mix(vec3(.04,.16,.30),vec3(.42,.72,.95),max(n.y,0.0));"
    "float foam=smoothstep(.42,.62,vHeight)*(0.55+0.45*sin(vWorld.x*7.0+vWorld.z*5.0));"
    "vec3 color=water*(.34+.66*diffuse)+sky*fresnel*.72+vec3(1.0,.88,.62)*sparkle*2.8;"
    "color=mix(color,vec3(.82,.94,1.0),clamp(foam,0.0,1.0));FragColor=vec4(color,1.0);}";

static LumeResult criar_superficie_oceano(LumeApp *aplicativo, LumeGeometry **saida)
{
    size_t quantidade_vertices = (size_t)(OCEANO_COLUNAS + 1) * (OCEANO_LINHAS + 1);
    size_t quantidade_indices = (size_t)OCEANO_COLUNAS * OCEANO_LINHAS * 6;
    float *posicoes = malloc(quantidade_vertices * 3 * sizeof(float));
    float *normais = malloc(quantidade_vertices * 3 * sizeof(float));
    float *uvs = malloc(quantidade_vertices * 2 * sizeof(float));
    uint32_t *indices = malloc(quantidade_indices * sizeof(uint32_t));
    LumeGeometryData dados = {0};
    LumeResult resultado;
    uint32_t linha, coluna;
    size_t vertice = 0, indice = 0;

    if (!posicoes || !normais || !uvs || !indices)
    {
        free(posicoes);
        free(normais);
        free(uvs);
        free(indices);
        return LUME_ERROR_OUT_OF_MEMORY;
    }

    /* A malha é contínua; o vertex shader desloca seus vértices sem criar emendas. */
    for (linha = 0; linha <= OCEANO_LINHAS; ++linha)
    {
        for (coluna = 0; coluna <= OCEANO_COLUNAS; ++coluna)
        {
            float u = (float)coluna / OCEANO_COLUNAS;
            float v = (float)linha / OCEANO_LINHAS;
            posicoes[vertice * 3] = (u - 0.5f) * OCEANO_LARGURA;
            posicoes[vertice * 3 + 1] = 0.0f;
            posicoes[vertice * 3 + 2] = (v - 0.5f) * OCEANO_PROFUNDIDADE;
            normais[vertice * 3] = 0.0f;
            normais[vertice * 3 + 1] = 1.0f;
            normais[vertice * 3 + 2] = 0.0f;
            uvs[vertice * 2] = u;
            uvs[vertice * 2 + 1] = v;
            ++vertice;
        }
    }
    for (linha = 0; linha < OCEANO_LINHAS; ++linha)
    {
        for (coluna = 0; coluna < OCEANO_COLUNAS; ++coluna)
        {
            uint32_t a = linha * (OCEANO_COLUNAS + 1) + coluna;
            uint32_t b = a + OCEANO_COLUNAS + 1;
            indices[indice++] = a;
            indices[indice++] = b;
            indices[indice++] = a + 1;
            indices[indice++] = a + 1;
            indices[indice++] = b;
            indices[indice++] = b + 1;
        }
    }
    dados.positions = posicoes;
    dados.normals = normais;
    dados.texture_coordinates = uvs;
    dados.vertex_count = quantidade_vertices;
    dados.indices = indices;
    dados.index_count = quantidade_indices;
    resultado = lume_geometry_create(aplicativo, &dados, saida);
    free(posicoes);
    free(normais);
    free(uvs);
    free(indices);
    return resultado;
}

int main(int argc, char **argv)
{
    bool smoke = argc > 1 && strcmp(argv[1], "--smoke") == 0;
    LumeAppConfig configuracao = lume_app_config_default();
    LumeShaderConfig shader_config = {shader_vertice_oceano, shader_fragmento_oceano, NULL, NULL};
    LumePipelineConfig pipeline_config = lume_pipeline_config_default();
    LumeMaterialConfig material_config = lume_material_config_default(LUME_MATERIAL_CUSTOM);
    LumeApp *aplicativo = NULL;
    LumeScene *cena = NULL;
    LumeNode *camera = NULL, *oceano = NULL;
    LumeGeometry *superficie = NULL;
    LumeShader *shader = NULL;
    LumePipeline *pipeline = NULL;
    LumeMaterial *material = NULL;
    float tempo = 0.0f;
    int quadros = 0;

    configuracao.title = "Lume3D - Shader ocean waves";
    configuracao.visible = !smoke;
    configuracao.vsync = !smoke;
    configuracao.clear_color = (LumeColor){0.015f, 0.08f, 0.16f, 1.0f};
    pipeline_config.shader = shader;
    pipeline_config.cull_back_faces = false;

    if (lume_app_create(&configuracao, &aplicativo) != LUME_SUCCESS ||
        lume_scene_create(aplicativo, &cena) != LUME_SUCCESS ||
        lume_camera_create_perspective(cena, NULL, &camera) != LUME_SUCCESS ||
        criar_superficie_oceano(aplicativo, &superficie) != LUME_SUCCESS ||
        lume_shader_create(aplicativo, &shader_config, &shader) != LUME_SUCCESS)
        goto falha;
    pipeline_config.shader = shader;
    if (lume_pipeline_create(aplicativo, &pipeline_config, &pipeline) != LUME_SUCCESS)
        goto falha;
    material_config.custom_pipeline = pipeline;
    material_config.double_sided = true;
    if (lume_material_create(aplicativo, &material_config, &material) != LUME_SUCCESS ||
        lume_mesh_create(cena, superficie, material, &oceano) != LUME_SUCCESS)
        goto falha;

    lume_shader_set_vec3(shader, "uSunDirection", (LumeVec3){-0.35f, -1.0f, -0.2f});
    lume_shader_set_vec3(shader, "uDeepColor", (LumeVec3){0.005f, 0.055f, 0.16f});
    lume_shader_set_vec3(shader, "uShallowColor", (LumeVec3){0.01f, 0.42f, 0.62f});
    lume_geometry_release(superficie);
    lume_material_release(material);
    lume_pipeline_release(pipeline);
    lume_shader_release(shader);
    lume_node_set_position(camera, (LumeVec3){0.0f, 5.2f, 9.5f});
    lume_node_look_at(camera, (LumeVec3){0.0f, -0.15f, -1.0f});

    while (!lume_app_should_close(aplicativo))
    {
        float delta = lume_app_begin_frame(aplicativo);
        tempo += delta;
        if (lume_key_was_pressed(aplicativo, LUME_KEY_ESCAPE))
            lume_app_request_close(aplicativo);
        lume_shader_set_float(shader, "uTime", tempo);
        if (lume_app_render(aplicativo, cena, camera) != LUME_SUCCESS)
            goto falha;
        lume_app_end_frame(aplicativo);
        if (smoke && ++quadros >= 2)
            lume_app_request_close(aplicativo);
    }
    if (!smoke)
    {
        LumeFrameStats estatisticas = lume_renderer_frame_stats(lume_app_renderer(aplicativo));
        printf("Ocean simulation finished with %llu triangles in %llu draw call(s).\n",
               (unsigned long long)estatisticas.triangles, (unsigned long long)estatisticas.draw_calls);
    }
    lume_app_destroy(aplicativo);
    return 0;

falha:
    fprintf(stderr, "Ocean example failed: %s\n", lume_error_last()->message);
    lume_app_destroy(aplicativo);
    return 1;
}
