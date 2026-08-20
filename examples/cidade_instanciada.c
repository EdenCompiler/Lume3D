#include <lume/lume.h>

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct ConfiguracaoExemplo
{
    bool baixa_qualidade;
    bool smoke;
} ConfiguracaoExemplo;

static const char *shader_vertice_predios =
    "#version 330 core\n"
    "layout(location=0)in vec3 aPosition;layout(location=1)in vec3 aNormal;layout(location=3)in mat4 aInstance;"
    "uniform mat4 uModel,uView,uProjection;uniform bool uInstanced;"
    "out vec3 vWorld;out vec3 vNormal;out vec3 vLocal;flat out vec3 vScale;flat out float vSeed;"
    "void main(){mat4 model=uInstanced?uModel*aInstance:uModel;vec4 world=model*vec4(aPosition,1.0);"
    "vWorld=world.xyz;vNormal=normalize(mat3(transpose(inverse(model)))*aNormal);vLocal=aPosition;"
    "vScale=vec3(length(aInstance[0].xyz),length(aInstance[1].xyz),length(aInstance[2].xyz));"
    "vSeed=fract(sin(dot(aInstance[3].xz,vec2(12.9898,78.233)))*43758.5453);"
    "gl_Position=uProjection*uView*world;}";

static const char *shader_fragmento_predios =
    "#version 330 core\n"
    "in vec3 vWorld;in vec3 vNormal;in vec3 vLocal;flat in vec3 vScale;flat in float vSeed;"
    "out vec4 FragColor;uniform vec3 uCamera;uniform float uTime;"
    "float hash(vec2 p){return fract(sin(dot(p,vec2(127.1,311.7)))*43758.5453);}"
    "void main(){vec3 n=normalize(vNormal);vec3 sun=normalize(vec3(-.55,.82,.28));"
    "float diffuse=.16+.84*max(dot(n,sun),0.0);bool faceX=abs(n.x)>.5;"
    "float horizontal=(faceX?(vLocal.z+.5)*vScale.z:(vLocal.x+.5)*vScale.x);"
    "float vertical=(vLocal.y+.5)*vScale.y;vec2 cell=vec2(horizontal/1.18,vertical/1.32);"
    "vec2 within=fract(cell);float facade=step(.16,within.x)*step(within.x,.82)*step(.20,within.y)*step(within.y,.76);"
    "facade*=step(abs(n.y),.55);float windowSeed=hash(floor(cell)+vec2(vSeed*91.0,vSeed*47.0));"
    "float lit=facade*step(.43,windowSeed+.08*sin(uTime*.18+windowSeed*8.0));"
    "vec3 concrete=mix(vec3(.025,.032,.052),vec3(.105,.135,.19),vSeed)*diffuse;"
    "float edge=1.0-smoothstep(.42,.5,max(abs(vLocal.x),abs(vLocal.z)));concrete+=edge*.012;"
    "vec3 windows=mix(vec3(.025,.18,.62),vec3(1.10,.31,.055),windowSeed);vec3 color=concrete+windows*lit*.72;"
    "float distanceFog=length(vWorld-uCamera);color=mix(color,vec3(.018,.028,.065),smoothstep(72.0,155.0,distanceFog));"
    "FragColor=vec4(color,1.0);}";

static const char *shader_vertice_ceu =
    "#version 330 core\nout vec2 vUv;void main(){vec2 p=vec2((gl_VertexID<<1)&2,gl_VertexID&2);"
    "vUv=p;gl_Position=vec4(p*2.0-1.0,0.0,1.0);}";

static const char *shader_fragmento_ceu =
    "#version 330 core\nin vec2 vUv;out vec4 FragColor;uniform sampler2D uColor,uDepth;"
    "uniform float uYaw,uPitch,uTime;"
    "float hash(vec2 p){return fract(sin(dot(p,vec2(127.1,311.7)))*43758.5453);}"
    "void main(){vec3 scene=texture(uColor,vUv).rgb;float depth=texture(uDepth,vUv).r;"
    "vec2 centered=vUv*2.0-1.0;float horizon=clamp(centered.y+uPitch*.55+.10,0.0,1.0);"
    "vec3 low=vec3(.055,.075,.15),high=vec3(.008,.014,.045);vec3 sky=mix(low,high,smoothstep(0.0,1.0,horizon));"
    "float glow=pow(max(1.0-length(centered-vec2(.38-uYaw*.03,.18))*1.35,0.0),5.0);"
    "sky+=vec3(.48,.12,.045)*glow*(.96+.04*sin(uTime*.15));"
    "vec2 starUv=floor((centered+vec2(uYaw*.08,uPitch*.04))*vec2(420.0,230.0));"
    "float star=step(.9965,hash(starUv))*smoothstep(.30,.78,horizon);sky+=vec3(star)*.72;"
    "FragColor=vec4(depth>.999999?sky:scene,1.0);}";

static float limitar(float valor, float minimo, float maximo)
{
    return fmaxf(minimo, fminf(maximo, valor));
}

static bool ler_argumentos(int argc, char **argv, ConfiguracaoExemplo *configuracao)
{
    int indice;
    memset(configuracao, 0, sizeof(*configuracao));
    for (indice = 1; indice < argc; ++indice)
    {
        if (strcmp(argv[indice], "--low") == 0)
            configuracao->baixa_qualidade = true;
        else if (strcmp(argv[indice], "--smoke") == 0)
        {
            configuracao->smoke = true;
            configuracao->baixa_qualidade = true;
        }
        else
        {
            fprintf(stderr, "Unknown option: %s\nUsage: %s [--low] [--smoke]\n", argv[indice], argv[0]);
            return false;
        }
    }
    return true;
}

static uint32_t misturar_hash(uint32_t valor)
{
    valor ^= valor >> 16;
    valor *= 0x7feb352du;
    valor ^= valor >> 15;
    valor *= 0x846ca68bu;
    return valor ^ (valor >> 16);
}

static float aleatorio_celula(int x, int z, uint32_t canal)
{
    uint32_t valor = misturar_hash((uint32_t)x * 73856093u ^ (uint32_t)z * 19349663u ^ canal * 83492791u);
    return (float)(valor & 0xffffu) / 65535.0f;
}

static uint32_t contar_predios(int largura_grade)
{
    int x, z;
    uint32_t quantidade = 0;
    for (z = -largura_grade / 2; z < largura_grade / 2; ++z)
        for (x = -largura_grade / 2; x < largura_grade / 2; ++x)
            if ((abs(x) % 9 > 1) && (abs(z) % 9 > 1))
                ++quantidade;
    return quantidade;
}

static LumeResult preencher_cidade(LumeNode *cidade, int largura_grade)
{
    int x, z;
    uint32_t indice = 0;
    const float espacamento = 2.25f;
    for (z = -largura_grade / 2; z < largura_grade / 2; ++z)
    {
        for (x = -largura_grade / 2; x < largura_grade / 2; ++x)
        {
            float altura, largura, profundidade;
            LumeMat4 transformacao;
            if (abs(x) % 9 <= 1 || abs(z) % 9 <= 1)
                continue;
            altura = 2.4f + powf(aleatorio_celula(x, z, 1), 2.2f) * 19.0f;
            if (abs(x) < 7 && abs(z) < 7)
                altura *= 1.35f;
            largura = 1.25f + aleatorio_celula(x, z, 2) * 0.58f;
            profundidade = 1.25f + aleatorio_celula(x, z, 3) * 0.58f;
            transformacao = lume_mat4_transform(
                (LumeVec3){x * espacamento, altura * 0.5f, z * espacamento}, lume_quat_identity(),
                (LumeVec3){largura, altura, profundidade});
            if (lume_instanced_mesh_set_transform(cidade, indice++, transformacao) != LUME_SUCCESS)
                return lume_error_last()->code;
        }
    }
    return LUME_SUCCESS;
}

static LumeResult criar_material_simples(LumeApp *aplicativo, LumeMaterialType tipo, LumeColor cor, float metalico,
                                         float rugosidade, LumeMaterial **saida)
{
    LumeMaterialConfig configuracao = lume_material_config_default(tipo);
    configuracao.base_color = cor;
    configuracao.metallic = metalico;
    configuracao.roughness = rugosidade;
    return lume_material_create(aplicativo, &configuracao, saida);
}

static uint32_t contar_marcacoes_rua(int largura_grade)
{
    const float espacamento = 2.25f;
    float limite = largura_grade * espacamento * 0.5f;
    uint32_t quantidade_ruas = 0;
    int rua;
    for (rua = -largura_grade / 2; rua < largura_grade / 2; ++rua)
        if (abs(rua) % 9 == 0)
            ++quantidade_ruas;
    return quantidade_ruas * (uint32_t)(((int)limite * 2 + 4) / 5) * 2u + 9u;
}

static LumeResult preencher_marcacoes_rua(LumeNode *marcacoes, int largura_grade)
{
    const float espacamento = 2.25f;
    float limite = largura_grade * espacamento * 0.5f;
    uint32_t indice = 0;
    int rua, trecho;
    /* As caixas muito rasas preservam o teste de profundidade e continuam em uma única draw call. */
    for (rua = -largura_grade / 2; rua < largura_grade / 2; ++rua)
    {
        float coordenada;
        if (abs(rua) % 9 != 0)
            continue;
        coordenada = rua * espacamento;
        for (trecho = -(int)limite; trecho < (int)limite; trecho += 5)
        {
            LumeMat4 vertical = lume_mat4_transform((LumeVec3){coordenada, 0.025f, (float)trecho + 1.1f},
                                                    lume_quat_identity(), (LumeVec3){0.07f, 0.018f, 2.2f});
            LumeMat4 horizontal = lume_mat4_transform((LumeVec3){(float)trecho + 1.1f, 0.027f, coordenada},
                                                      lume_quat_identity(), (LumeVec3){2.2f, 0.018f, 0.07f});
            if (lume_instanced_mesh_set_transform(marcacoes, indice++, vertical) != LUME_SUCCESS ||
                lume_instanced_mesh_set_transform(marcacoes, indice++, horizontal) != LUME_SUCCESS)
                return lume_error_last()->code;
        }
    }
    /* A avenida central continua até a câmera e conduz visualmente à entrada da cidade. */
    for (trecho = (int)limite; trecho < (int)limite + 45; trecho += 5)
    {
        LumeMat4 entrada = lume_mat4_transform((LumeVec3){0, 0.025f, (float)trecho + 1.1f}, lume_quat_identity(),
                                               (LumeVec3){0.07f, 0.018f, 2.2f});
        if (lume_instanced_mesh_set_transform(marcacoes, indice++, entrada) != LUME_SUCCESS)
            return lume_error_last()->code;
    }
    return LUME_SUCCESS;
}

int main(int argc, char **argv)
{
    ConfiguracaoExemplo opcoes;
    LumeAppConfig configuracao = lume_app_config_default();
    LumeRendererConfig renderer_config = lume_renderer_config_default();
    LumePerspectiveCameraConfig camera_config = lume_perspective_camera_config_default();
    LumeShaderConfig shader_config = {shader_vertice_predios, shader_fragmento_predios, NULL, NULL};
    LumeShaderConfig shader_ceu_config = {shader_vertice_ceu, shader_fragmento_ceu, NULL, NULL};
    LumePipelineConfig pipeline_config = lume_pipeline_config_default();
    LumeMaterialConfig material_predios_config = lume_material_config_default(LUME_MATERIAL_CUSTOM);
    LumeApp *aplicativo = NULL;
    LumeScene *cena = NULL;
    LumeNode *camera = NULL, *cidade = NULL, *chao = NULL, *marcacoes = NULL, *luz = NULL;
    LumeGeometry *caixa = NULL, *plano = NULL;
    LumeShader *shader = NULL, *shader_ceu = NULL;
    LumePipeline *pipeline = NULL, *pipeline_ceu = NULL;
    LumeMaterial *material_predios = NULL, *material_chao = NULL, *material_marcacoes = NULL;
    int largura_grade, quadros = 0;
    uint32_t quantidade_predios;
    float guinada = 3.14159265f, inclinacao = -0.10f, tempo = 0.0f, acumulador_estatisticas = 0.0f;
    float velocidade_base = 10.0f;
    LumeVec3 posicao_camera = {0, 10.0f, 80.0f};
    bool mostrar_estatisticas = false;

    if (!ler_argumentos(argc, argv, &opcoes))
        return 2;
    largura_grade = opcoes.baixa_qualidade ? 32 : 64;
    quantidade_predios = contar_predios(largura_grade);
    /* Mantém o mesmo enquadramento inicial, independentemente da extensão da grade. */
    posicao_camera.z = largura_grade * 1.125f + 44.0f;
    configuracao.title = "Lume3D - Instanced procedural city";
    configuracao.width = opcoes.smoke ? 320 : opcoes.baixa_qualidade ? 800 : 1280;
    configuracao.height = opcoes.smoke ? 180 : opcoes.baixa_qualidade ? 450 : 720;
    configuracao.visible = !opcoes.smoke;
    configuracao.vsync = !opcoes.smoke;
    configuracao.clear_color = (LumeColor){0.018f, 0.026f, 0.055f, 1};
    renderer_config.hdr = true;
    renderer_config.tone_mapping = LUME_TONE_MAPPING_ACES;
    renderer_config.bloom = !opcoes.baixa_qualidade;
    renderer_config.fxaa = true;
    renderer_config.exposure = 1.12f;
    renderer_config.directional_shadow_size = opcoes.baixa_qualidade ? 512u : 2048u;
    renderer_config.spot_shadow_size = opcoes.baixa_qualidade ? 512u : 2048u;
    camera_config.field_of_view_radians = 1.12f;
    camera_config.near_plane = 0.08f;
    camera_config.far_plane = 260.0f;

    if (lume_app_create(&configuracao, &aplicativo) != LUME_SUCCESS ||
        lume_renderer_configure(lume_app_renderer(aplicativo), &renderer_config) != LUME_SUCCESS ||
        lume_scene_create(aplicativo, &cena) != LUME_SUCCESS ||
        lume_camera_create_perspective(cena, &camera_config, &camera) != LUME_SUCCESS ||
        lume_geometry_create_box(aplicativo, 1, 1, 1, &caixa) != LUME_SUCCESS ||
        lume_geometry_create_plane(aplicativo, 190, 190, &plano) != LUME_SUCCESS ||
        lume_shader_create(aplicativo, &shader_config, &shader) != LUME_SUCCESS ||
        lume_shader_create(aplicativo, &shader_ceu_config, &shader_ceu) != LUME_SUCCESS)
        goto falha;
    pipeline_config.shader = shader;
    if (lume_pipeline_create(aplicativo, &pipeline_config, &pipeline) != LUME_SUCCESS)
        goto falha;
    pipeline_config.shader = shader_ceu;
    pipeline_config.depth_test = false;
    pipeline_config.depth_write = false;
    pipeline_config.cull_back_faces = false;
    if (lume_pipeline_create(aplicativo, &pipeline_config, &pipeline_ceu) != LUME_SUCCESS)
        goto falha;
    material_predios_config.custom_pipeline = pipeline;
    if (lume_material_create(aplicativo, &material_predios_config, &material_predios) != LUME_SUCCESS ||
        lume_instanced_mesh_create(cena, caixa, material_predios, quantidade_predios, &cidade) != LUME_SUCCESS ||
        preencher_cidade(cidade, largura_grade) != LUME_SUCCESS ||
        criar_material_simples(aplicativo, LUME_MATERIAL_PBR, (LumeColor){0.025f, 0.032f, 0.045f, 1}, 0.2f, 0.84f,
                               &material_chao) != LUME_SUCCESS ||
        lume_mesh_create(cena, plano, material_chao, &chao) != LUME_SUCCESS ||
        criar_material_simples(aplicativo, LUME_MATERIAL_UNLIT, (LumeColor){1.7f, 0.72f, 0.10f, 1}, 0, 1,
                               &material_marcacoes) != LUME_SUCCESS ||
        lume_instanced_mesh_create(cena, caixa, material_marcacoes, contar_marcacoes_rua(largura_grade),
                                   &marcacoes) != LUME_SUCCESS ||
        preencher_marcacoes_rua(marcacoes, largura_grade) != LUME_SUCCESS)
        goto falha;
    {
        LumePassConfig passagem_ceu = {"Procedural city sky", pipeline_ceu, LUME_PASS_LDR, true, true};
        if (lume_renderer_add_pass(lume_app_renderer(aplicativo), &passagem_ceu, NULL) != LUME_SUCCESS)
            goto falha;
    }
    lume_node_set_euler_rotation(chao, (LumeVec3){-1.57079633f, 0, 0});
    {
        LumeDirectionalLightConfig sol = lume_directional_light_config_default();
        LumeAmbientLightConfig ambiente = lume_ambient_light_config_default();
        sol.color = (LumeColor){0.56f, 0.66f, 1.0f, 1};
        sol.intensity = 1.05f;
        sol.direction = (LumeVec3){-0.55f, -0.88f, 0.28f};
        sol.cast_shadows = true;
        ambiente.color = (LumeColor){0.16f, 0.21f, 0.38f, 1};
        ambiente.intensity = 0.16f;
        if (lume_directional_light_create(cena, &sol, &luz) != LUME_SUCCESS ||
            lume_ambient_light_create(cena, &ambiente, &luz) != LUME_SUCCESS)
            goto falha;
    }
    lume_geometry_release(caixa);
    caixa = NULL;
    lume_geometry_release(plano);
    plano = NULL;
    lume_material_release(material_predios);
    material_predios = NULL;
    lume_material_release(material_chao);
    material_chao = NULL;
    lume_material_release(material_marcacoes);
    material_marcacoes = NULL;
    lume_pipeline_release(pipeline);
    pipeline = NULL;
    lume_pipeline_release(pipeline_ceu);
    pipeline_ceu = NULL;

    if (!opcoes.smoke)
        printf("City generated with %u building instances.\nControls: WASD move, Q/E altitude, left-drag look, wheel speed, Shift boost, I statistics, R reset, Esc exit.\n",
               quantidade_predios);
    while (!lume_app_should_close(aplicativo))
    {
        float delta = lume_app_begin_frame(aplicativo);
        float mouse_x, mouse_y, rolagem_x, rolagem_y;
        float velocidade;
        LumeVec3 frente, direita, alvo;
        lume_mouse_get_delta(aplicativo, &mouse_x, &mouse_y);
        lume_mouse_get_scroll(aplicativo, &rolagem_x, &rolagem_y);
        (void)rolagem_x;
        velocidade_base = limitar(velocidade_base + rolagem_y * 1.5f, 3.0f, 24.0f);
        velocidade = lume_key_is_down(aplicativo, LUME_KEY_LEFT_SHIFT) ? velocidade_base * 2.8f : velocidade_base;
        if (lume_key_was_pressed(aplicativo, LUME_KEY_ESCAPE))
            lume_app_request_close(aplicativo);
        if (lume_key_was_pressed(aplicativo, LUME_KEY_I))
            mostrar_estatisticas = !mostrar_estatisticas;
        if (lume_key_was_pressed(aplicativo, LUME_KEY_R))
        {
            posicao_camera = (LumeVec3){0, 10.0f, largura_grade * 1.125f + 44.0f};
            guinada = 3.14159265f;
            inclinacao = -0.10f;
            velocidade_base = 10.0f;
        }
        if (lume_mouse_button_is_down(aplicativo, LUME_MOUSE_BUTTON_LEFT))
        {
            guinada -= mouse_x * 0.0048f;
            inclinacao = limitar(inclinacao - mouse_y * 0.0048f, -1.35f, 1.35f);
        }
        frente = lume_vec3_normalize((LumeVec3){sinf(guinada) * cosf(inclinacao), sinf(inclinacao),
                                                cosf(guinada) * cosf(inclinacao)});
        direita = lume_vec3_normalize(lume_vec3_cross(frente, (LumeVec3){0, 1, 0}));
        if (lume_key_is_down(aplicativo, LUME_KEY_W))
            posicao_camera = lume_vec3_add(posicao_camera, lume_vec3_scale(frente, velocidade * delta));
        if (lume_key_is_down(aplicativo, LUME_KEY_S))
            posicao_camera = lume_vec3_subtract(posicao_camera, lume_vec3_scale(frente, velocidade * delta));
        if (lume_key_is_down(aplicativo, LUME_KEY_D))
            posicao_camera = lume_vec3_add(posicao_camera, lume_vec3_scale(direita, velocidade * delta));
        if (lume_key_is_down(aplicativo, LUME_KEY_A))
            posicao_camera = lume_vec3_subtract(posicao_camera, lume_vec3_scale(direita, velocidade * delta));
        if (lume_key_is_down(aplicativo, LUME_KEY_E))
            posicao_camera.y += velocidade * delta;
        if (lume_key_is_down(aplicativo, LUME_KEY_Q))
            posicao_camera.y -= velocidade * delta;
        posicao_camera.y = limitar(posicao_camera.y, 1.1f, 55.0f);
        lume_node_set_position(camera, posicao_camera);
        alvo = lume_vec3_add(posicao_camera, frente);
        if (lume_node_look_at(camera, alvo) != LUME_SUCCESS)
            goto falha;
        tempo += delta;
        if (lume_shader_set_float(shader, "uTime", tempo) != LUME_SUCCESS ||
            lume_shader_set_float(shader_ceu, "uYaw", guinada) != LUME_SUCCESS ||
            lume_shader_set_float(shader_ceu, "uPitch", inclinacao) != LUME_SUCCESS ||
            lume_shader_set_float(shader_ceu, "uTime", tempo) != LUME_SUCCESS)
            goto falha;
        if (mostrar_estatisticas)
        {
            acumulador_estatisticas += delta;
            if (acumulador_estatisticas >= 1.0f)
            {
                LumeFrameStats estatisticas = lume_renderer_frame_stats(lume_app_renderer(aplicativo));
                printf("Frame %llu: %llu instances, %llu draw calls, %llu culled objects, %.2f ms CPU.\n",
                       (unsigned long long)estatisticas.frame_index, (unsigned long long)estatisticas.instances,
                       (unsigned long long)estatisticas.draw_calls, (unsigned long long)estatisticas.culled_objects,
                       estatisticas.cpu_time_ms);
                acumulador_estatisticas = 0.0f;
            }
        }
        if (lume_app_render(aplicativo, cena, camera) != LUME_SUCCESS)
            goto falha;
        lume_app_end_frame(aplicativo);
        if (opcoes.smoke && ++quadros >= 2)
            lume_app_request_close(aplicativo);
    }
    if (!opcoes.smoke)
        puts("Instanced city finished.");
    lume_shader_release(shader);
    lume_shader_release(shader_ceu);
    lume_app_destroy(aplicativo);
    return 0;

falha:
    fprintf(stderr, "Instanced city example failed: %s\n", lume_error_last()->message);
    lume_geometry_release(caixa);
    lume_geometry_release(plano);
    lume_material_release(material_predios);
    lume_material_release(material_chao);
    lume_material_release(material_marcacoes);
    lume_pipeline_release(pipeline);
    lume_pipeline_release(pipeline_ceu);
    lume_shader_release(shader);
    lume_shader_release(shader_ceu);
    lume_app_destroy(aplicativo);
    return 1;
}
