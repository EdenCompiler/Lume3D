#include <lume/lume.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define OCEANO_COLUNAS 420u
#define OCEANO_LINHAS 300u
#define OCEANO_LARGURA 180.0f
#define OCEANO_PROFUNDIDADE 150.0f

static const char *shader_vertice_oceano =
    "#version 330 core\n"
    "layout(location=0) in vec3 aPosition;"
    "uniform mat4 uModel;uniform mat4 uView;uniform mat4 uProjection;uniform float uTime;"
    "out vec3 vWorld;out vec3 vNormal;out float vHeight;out float vCrest;"
    "const float PI=3.14159265359;"
    "vec3 gerstner(vec3 p,vec2 direction,float steepness,float wavelength,float speed){"
    "vec2 d=normalize(direction);float k=2.0*PI/wavelength;float c=sqrt(9.8/k)*speed;"
    "float phase=k*(dot(d,p.xz)-c*uTime);float amplitude=steepness/k;"
    "return vec3(d.x*amplitude*cos(phase),amplitude*sin(phase),d.y*amplitude*cos(phase));}"
    "vec3 surface(vec3 p){return p"
    "+gerstner(p,vec2(.16,-1.0),.46,10.5,.82)"
    "+gerstner(p,vec2(-.28,-1.0),.24,6.2,.94)"
    "+gerstner(p,vec2(.72,-.62),.12,2.8,1.12)"
    "+gerstner(p,vec2(-.82,-.34),.06,1.25,1.34);}"
    "float crest(vec3 p){float energy=0.0;vec2 d;float k,c,phase;"
    "d=normalize(vec2(.16,-1.0));k=2.0*PI/10.5;c=sqrt(9.8/k)*.82;phase=k*(dot(d,p.xz)-c*uTime);energy+=smoothstep(.64,.98,sin(phase))*.46;"
    "d=normalize(vec2(-.28,-1.0));k=2.0*PI/6.2;c=sqrt(9.8/k)*.94;phase=k*(dot(d,p.xz)-c*uTime);energy+=smoothstep(.64,.98,sin(phase))*.24;"
    "return energy;}"
    "void main(){vec3 base=aPosition;vec3 p=surface(base);float stepSize=.18;"
    "vec3 px=surface(base+vec3(stepSize,0,0));vec3 pz=surface(base+vec3(0,0,stepSize));"
    "vNormal=normalize(mat3(uModel)*normalize(cross(pz-p,px-p)));"
    "vHeight=p.y;vCrest=crest(base);vWorld=(uModel*vec4(p,1.0)).xyz;"
    "gl_Position=uProjection*uView*vec4(vWorld,1.0);}";

static const char *shader_fragmento_oceano =
    "#version 330 core\n"
    "in vec3 vWorld;in vec3 vNormal;in float vHeight;in float vCrest;out vec4 FragColor;"
    "uniform vec3 uCamera;uniform vec3 uSunDirection;uniform vec3 uDeepColor;uniform vec3 uShallowColor;"
    "float hash(vec2 p){return fract(sin(dot(p,vec2(127.1,311.7)))*43758.5453);}"
    "float noise(vec2 p){vec2 i=floor(p),f=fract(p);f=f*f*(3.0-2.0*f);"
    "return mix(mix(hash(i),hash(i+vec2(1,0)),f.x),mix(hash(i+vec2(0,1)),hash(i+vec2(1)),f.x),f.y);}"
    "void main(){vec3 n=normalize(vNormal);if(n.y<0.0)n=-n;"
    "vec3 viewDirection=normalize(uCamera-vWorld);vec3 lightDirection=normalize(-uSunDirection);"
    "float fresnel=.04+.96*pow(1.0-max(dot(n,viewDirection),0.0),5.0);"
    "float diffuse=max(dot(n,lightDirection),0.0);"
    "float reflection=max(dot(reflect(-lightDirection,n),viewDirection),0.0);"
    "float sparkle=pow(reflection,240.0)*3.2+pow(reflection,32.0)*.30;"
    "float depthMix=smoothstep(.08,.82,vHeight);vec3 water=mix(uDeepColor,uShallowColor,depthMix*.56);"
    "vec3 reflectedDirection=reflect(-viewDirection,n);"
    "vec3 sky=mix(vec3(.72,.55,.56),vec3(.08,.27,.55),clamp(reflectedDirection.y*2.2,0.0,1.0));"
    "float slope=1.0-n.y;float breakup=noise(vWorld.xz*1.45)+.45*noise(vWorld.xz*4.2);"
    "float foamBand=smoothstep(.38,.61,vCrest+slope*.08);"
    "float foam=foamBand*smoothstep(.66,1.02,breakup+vCrest*.10);"
    "vec3 color=water*(.34+.66*diffuse)+sky*fresnel*.78+vec3(1.0,.76,.46)*sparkle;"
    "color=mix(color,vec3(.84,.92,.96),clamp(foam*.82,0.0,1.0));FragColor=vec4(color,1.0);}";

static const char *shader_vertice_ceu =
    "#version 330 core\nlayout(location=0)in vec3 aPosition;layout(location=2)in vec2 aUv;out vec3 vDirection;"
    "void main(){vec2 p=aPosition.xy;vDirection=normalize(vec3(p.x,p.y*.72,-1.0));gl_Position=vec4(p,.9999,1.0);}";

static const char *shader_fragmento_ceu =
    "#version 330 core\nin vec3 vDirection;out vec4 FragColor;uniform float uTime;uniform vec3 uSunDirection;"
    "float hash(vec2 p){return fract(sin(dot(p,vec2(127.1,311.7)))*43758.5453);}"
    "float noise(vec2 p){vec2 i=floor(p),f=fract(p);f=f*f*(3.0-2.0*f);"
    "return mix(mix(hash(i),hash(i+vec2(1,0)),f.x),mix(hash(i+vec2(0,1)),hash(i+vec2(1)),f.x),f.y);}"
    "float fbm(vec2 p){float value=0.0,weight=.55;for(int i=0;i<4;i++){value+=noise(p)*weight;p=p*2.07+vec2(1.7,.9);weight*=.48;}return value;}"
    "void main(){vec3 d=normalize(vDirection);float height=clamp(d.y,0.0,1.0);float horizon=pow(1.0-height,4.0);"
    "vec3 zenith=vec3(.055,.19,.42),low=vec3(.58,.43,.47);vec3 color=mix(low,zenith,smoothstep(0.0,.72,height));"
    "vec2 cloudUv=vec2(atan(d.z,d.x)*1.15,d.y*5.0)+vec2(uTime*.004,0.0);"
    "float cloudShape=fbm(cloudUv*.72);"
    "float billows=.50+.24*sin(cloudUv.x*2.1+cloudShape*5.0)+.18*sin(cloudUv.x*4.7-cloudUv.y*1.3);"
    "float clouds=smoothstep(.57,.73,billows+cloudShape*.22)*smoothstep(.02,.18,height);"
    "float cloudShade=fbm(cloudUv*1.8+3.4);vec3 cloudColor=mix(vec3(.24,.27,.37),vec3(.76,.70,.72),cloudShade);"
    "color=mix(color,cloudColor,clouds*.74);"
    "vec3 sunDirection=normalize(-uSunDirection);float sunAmount=max(dot(d,sunDirection),0.0);"
    "float sun=pow(sunAmount,1100.0);float glow=pow(sunAmount,18.0);"
    "color+=vec3(1.0,.72,.45)*horizon*.18+vec3(1.0,.78,.50)*(sun*2.4+glow*.20);"
    "FragColor=vec4(color,1.0);}";

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
    LumePerspectiveCameraConfig camera_config = lume_perspective_camera_config_default();
    LumeRendererConfig renderer_config = lume_renderer_config_default();
    LumeShaderConfig shader_config = {shader_vertice_oceano, shader_fragmento_oceano, NULL, NULL};
    LumeShaderConfig shader_ceu_config = {shader_vertice_ceu, shader_fragmento_ceu, NULL, NULL};
    LumePipelineConfig pipeline_config = lume_pipeline_config_default();
    LumeMaterialConfig material_config = lume_material_config_default(LUME_MATERIAL_CUSTOM);
    LumeApp *aplicativo = NULL;
    LumeScene *cena = NULL;
    LumeNode *camera = NULL, *oceano = NULL, *ceu = NULL;
    LumeGeometry *superficie = NULL, *esfera_ceu = NULL;
    LumeShader *shader = NULL, *shader_ceu = NULL;
    LumePipeline *pipeline = NULL, *pipeline_ceu = NULL;
    LumeMaterial *material = NULL, *material_ceu = NULL;
    float tempo = 0.0f;
    float camera_x = 0.0f, camera_z = 32.0f;
    int quadros = 0;

    configuracao.title = "Lume3D - Shader ocean waves";
    configuracao.visible = !smoke;
    configuracao.vsync = !smoke;
    configuracao.clear_color = (LumeColor){0.34f, 0.48f, 0.61f, 1.0f};
    camera_config.field_of_view_radians = 1.05f;
    camera_config.near_plane = 0.05f;
    camera_config.far_plane = 500.0f;
    renderer_config.exposure = 0.68f;
    pipeline_config.shader = shader;
    pipeline_config.cull_back_faces = false;

    if (lume_app_create(&configuracao, &aplicativo) != LUME_SUCCESS ||
        lume_scene_create(aplicativo, &cena) != LUME_SUCCESS ||
        lume_camera_create_perspective(cena, &camera_config, &camera) != LUME_SUCCESS ||
        criar_superficie_oceano(aplicativo, &superficie) != LUME_SUCCESS ||
        lume_geometry_create_plane(aplicativo, 2.0f, 2.0f, &esfera_ceu) != LUME_SUCCESS ||
        lume_shader_create(aplicativo, &shader_config, &shader) != LUME_SUCCESS ||
        lume_shader_create(aplicativo, &shader_ceu_config, &shader_ceu) != LUME_SUCCESS)
        goto falha;
    if (lume_renderer_configure(lume_app_renderer(aplicativo), &renderer_config) != LUME_SUCCESS)
        goto falha;
    pipeline_config.shader = shader;
    if (lume_pipeline_create(aplicativo, &pipeline_config, &pipeline) != LUME_SUCCESS)
        goto falha;
    pipeline_config.shader = shader_ceu;
    pipeline_config.cull_back_faces = false;
    if (lume_pipeline_create(aplicativo, &pipeline_config, &pipeline_ceu) != LUME_SUCCESS)
        goto falha;
    material_config.custom_pipeline = pipeline;
    material_config.double_sided = true;
    if (lume_material_create(aplicativo, &material_config, &material) != LUME_SUCCESS ||
        lume_mesh_create(cena, superficie, material, &oceano) != LUME_SUCCESS)
        goto falha;
    material_config.custom_pipeline = pipeline_ceu;
    if (lume_material_create(aplicativo, &material_config, &material_ceu) != LUME_SUCCESS ||
        lume_mesh_create(cena, esfera_ceu, material_ceu, &ceu) != LUME_SUCCESS)
        goto falha;

    lume_shader_set_vec3(shader, "uSunDirection", (LumeVec3){-0.55f, -0.12f, 0.82f});
    lume_shader_set_vec3(shader_ceu, "uSunDirection", (LumeVec3){-0.55f, -0.12f, 0.82f});
    lume_shader_set_vec3(shader, "uDeepColor", (LumeVec3){0.001f, 0.008f, 0.052f});
    lume_shader_set_vec3(shader, "uShallowColor", (LumeVec3){0.004f, 0.12f, 0.30f});
    lume_geometry_release(superficie);
    lume_geometry_release(esfera_ceu);
    lume_material_release(material);
    lume_material_release(material_ceu);
    lume_pipeline_release(pipeline);
    lume_pipeline_release(pipeline_ceu);
    lume_shader_release(shader);
    lume_shader_release(shader_ceu);
    /* A câmera fica quase na superfície, como a tomada aberta usada como referência. */
    lume_node_set_position(camera, (LumeVec3){camera_x, 2.75f, camera_z});

    while (!lume_app_should_close(aplicativo))
    {
        float delta = lume_app_begin_frame(aplicativo);
        tempo += delta;
        if (lume_key_was_pressed(aplicativo, LUME_KEY_ESCAPE))
            lume_app_request_close(aplicativo);
        /* WASD permite explorar o plano aberto sem alterar o enquadramento cinematográfico. */
        if (lume_key_is_down(aplicativo, LUME_KEY_A))
            camera_x -= 8.0f * delta;
        if (lume_key_is_down(aplicativo, LUME_KEY_D))
            camera_x += 8.0f * delta;
        if (lume_key_is_down(aplicativo, LUME_KEY_W))
            camera_z -= 8.0f * delta;
        if (lume_key_is_down(aplicativo, LUME_KEY_S))
            camera_z += 8.0f * delta;
        lume_node_set_position(camera,
                               (LumeVec3){camera_x, 2.75f + sinf(tempo * 0.42f) * 0.035f, camera_z});
        lume_shader_set_float(shader, "uTime", tempo);
        lume_shader_set_float(shader_ceu, "uTime", tempo);
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
