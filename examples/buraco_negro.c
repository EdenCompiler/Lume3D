#include <lume/lume.h>

#include <math.h>
#include <stdio.h>
#include <string.h>

static const char *shader_vertice =
    "#version 330 core\n"
    "layout(location=0)in vec3 aPosition;layout(location=2)in vec2 aUv;"
    "out vec2 vUv;void main(){vUv=aUv;gl_Position=vec4(aPosition.xy,0.0,1.0);}";

static const char *shader_fragmento =
    /* O traçado usa o raio de Schwarzschild como unidade de comprimento. */
    "#version 330 core\n"
    "in vec2 vUv;out vec4 FragColor;uniform float uTime;uniform vec2 uResolution;uniform float uAzimuth;uniform float uElevation;uniform float uDistance;"
    "float linhaGrade(vec2 p){vec2 largura=fwidth(p);vec2 celula=abs(fract(p-.5)-.5)/largura;"
    "return 1.0-min(min(celula.x,celula.y),1.0);}"
    "float aleatorio(vec3 p){return fract(sin(dot(p,vec3(12.9898,78.233,151.7182)))*43758.5453);}"
    "vec3 estrelas(vec3 direction){float s=aleatorio(floor(direction*540.0));return s>.9975?vec3((s-.9975)*400.0):vec3(0.0);}"
    "struct Raio{float r;float theta;float phi;float dr;float dtheta;float dphi;float energia;};"
    "vec3 posicao(Raio raio){float seno=sin(raio.theta);return vec3(raio.r*seno*cos(raio.phi),raio.r*cos(raio.theta),raio.r*seno*sin(raio.phi));}"
    "Raio criarRaio(vec3 origem,vec3 direcao){Raio raio;raio.r=length(origem);raio.theta=acos(clamp(origem.y/raio.r,-1.0,1.0));raio.phi=atan(origem.z,origem.x);"
    "float seno=max(sin(raio.theta),.0001);raio.dr=dot(normalize(origem),direcao);"
    "raio.dtheta=(cos(raio.theta)*cos(raio.phi)*direcao.x-seno*direcao.y+cos(raio.theta)*sin(raio.phi)*direcao.z)/raio.r;"
    "raio.dphi=(-sin(raio.phi)*direcao.x+cos(raio.phi)*direcao.z)/(raio.r*seno);"
    "float lapse=max(1.0-1.0/raio.r,.001);raio.energia=lapse*sqrt(raio.dr*raio.dr/lapse+raio.r*raio.r*(raio.dtheta*raio.dtheta+seno*seno*raio.dphi*raio.dphi));return raio;}"
    "void derivadas(Raio raio,out vec3 velocidade,out vec3 aceleracao){float r=max(raio.r,1.002);float seno=max(sin(raio.theta),.0001);float lapse=max(1.0-1.0/r,.001);float tempo=raio.energia/lapse;"
    "velocidade=vec3(raio.dr,raio.dtheta,raio.dphi);"
    "aceleracao.x=-(1.0/(2.0*r*r))*lapse*tempo*tempo+raio.dr*raio.dr/(2.0*r*r*lapse)+r*(raio.dtheta*raio.dtheta+seno*seno*raio.dphi*raio.dphi);"
    "aceleracao.y=-2.0*raio.dr*raio.dtheta/r+sin(raio.theta)*cos(raio.theta)*raio.dphi*raio.dphi;"
    "aceleracao.z=-2.0*raio.dr*raio.dphi/r-2.0*cos(raio.theta)/seno*raio.dtheta*raio.dphi;}"
    "Raio avancar(Raio raio,float passo){vec3 velocidade,aceleracao;derivadas(raio,velocidade,aceleracao);Raio meio=raio;"
    "meio.r+=velocidade.x*passo*.5;meio.theta+=velocidade.y*passo*.5;meio.phi+=velocidade.z*passo*.5;meio.dr+=aceleracao.x*passo*.5;meio.dtheta+=aceleracao.y*passo*.5;meio.dphi+=aceleracao.z*passo*.5;"
    "derivadas(meio,velocidade,aceleracao);raio.r+=velocidade.x*passo;raio.theta+=velocidade.y*passo;raio.phi+=velocidade.z*passo;raio.dr+=aceleracao.x*passo;raio.dtheta+=aceleracao.y*passo;raio.dphi+=aceleracao.z*passo;"
    "raio.theta=clamp(raio.theta,.002,3.139);return raio;}"
    "void main(){"
    "vec2 screen=(vUv-.5)*2.0;screen.x*=uResolution.x/uResolution.y;"
    "vec3 observer=uDistance*vec3(sin(uAzimuth)*cos(uElevation),sin(uElevation),cos(uAzimuth)*cos(uElevation));"
    "vec3 forward=normalize(-observer);vec3 right=normalize(cross(forward,vec3(0.0,1.0,0.0)));vec3 up=normalize(cross(right,forward));"
    "vec3 direction=normalize(forward+right*screen.x*.58+up*screen.y*.58);Raio raio=criarRaio(observer,direction);"
    "bool atingiuHorizonte=false;bool atingiuDisco=false;vec3 pontoDisco=vec3(0.0);vec3 anterior=posicao(raio);"
    "for(int iteracao=0;iteracao<420;iteracao++){if(raio.r<=1.001){atingiuHorizonte=true;break;}"
    "float passo=.032*clamp(raio.r/4.0,.30,1.35);raio=avancar(raio,passo);vec3 atual=posicao(raio);"
    "if(anterior.y*atual.y<=0.0){float fracao=abs(anterior.y)/max(abs(anterior.y)+abs(atual.y),.0001);vec3 cruzamento=mix(anterior,atual,fracao);float raioDisco=length(cruzamento.xz);"
    "if(raioDisco>=1.35&&raioDisco<=3.40){atingiuDisco=true;pontoDisco=cruzamento;break;}}"
    "anterior=atual;if(raio.r>22.0)break;}if(abs(screen.x)<.007&&abs(screen.y)>.23){atingiuDisco=false;atingiuHorizonte=false;}"
    "vec3 color=estrelas(direction);float ground=1.0-smoothstep(.10,.34,screen.y);float perspective=1.0/max(.045,.37-screen.y);"
    "float grid=linhaGrade(vec2(screen.x*perspective*7.0,perspective*1.9))*ground;color+=grid*vec3(.028);"
    "if(atingiuDisco){float raioDisco=length(pontoDisco.xz);float normalizado=clamp((raioDisco-1.35)/(3.40-1.35),0.0,1.0);"
    "float angulo=atan(pontoDisco.z,pontoDisco.x);float faixas=.77+.23*sin(angulo*12.0-normalizado*28.0-uTime*1.2);"
    "vec3 velocidade=normalize(vec3(-pontoDisco.z,0.0,pontoDisco.x));float beta=sqrt(.5/max(raioDisco,1.35));float doppler=clamp(1.0/(sqrt(1.0-beta*beta)*(1.0-dot(velocidade,-direction)*beta)),.55,2.0);"
    "float calor=1.0-smoothstep(.04,.72,normalizado);vec3 quente=vec3(1.0,.80,.34);vec3 medio=vec3(1.0,.16,.006);vec3 frio=vec3(.36,.001,.0);"
    "vec3 emissao=mix(frio,medio,smoothstep(.18,.72,1.0-normalizado));emissao=mix(emissao,quente,smoothstep(.55,1.0,calor));color=emissao*faixas*pow(doppler,2.2)*1.15;}"
    "if(atingiuHorizonte)color=vec3(0.0);if(abs(screen.x)<.010&&abs(screen.y)>.23)color=estrelas(direction)+grid*vec3(.028);"
    "color=1.0-exp(-color);FragColor=vec4(color,1.0);}";

int main(int argc, char **argv)
{
    bool smoke = argc > 1 && strcmp(argv[1], "--smoke") == 0;
    LumeAppConfig configuracao = lume_app_config_default();
    LumeShaderConfig configuracao_shader = {shader_vertice, shader_fragmento, NULL, NULL};
    LumePipelineConfig configuracao_pipeline = lume_pipeline_config_default();
    LumeMaterialConfig configuracao_material = lume_material_config_default(LUME_MATERIAL_CUSTOM);
    LumeApp *aplicativo = NULL;
    LumeScene *cena = NULL;
    LumeNode *camera = NULL, *tela = NULL;
    LumeGeometry *plano = NULL;
    LumeShader *shader = NULL;
    LumePipeline *pipeline = NULL;
    LumeMaterial *material = NULL;
    float tempo = 0.0f;
    float azimute = 0.0f, elevacao = 0.55f, distancia = 11.2f;
    int largura = 1280, altura = 720, quadros = 0;

    configuracao.title = "Lume3D - Shader black hole";
    configuracao.visible = !smoke;
    configuracao.vsync = !smoke;
    configuracao.clear_color = (LumeColor){0.0f, 0.0f, 0.0f, 1.0f};

    if (lume_app_create(&configuracao, &aplicativo) != LUME_SUCCESS ||
        lume_scene_create(aplicativo, &cena) != LUME_SUCCESS ||
        lume_camera_create_perspective(cena, NULL, &camera) != LUME_SUCCESS ||
        lume_geometry_create_plane(aplicativo, 2.0f, 2.0f, &plano) != LUME_SUCCESS ||
        lume_shader_create(aplicativo, &configuracao_shader, &shader) != LUME_SUCCESS)
        goto falha;

    configuracao_pipeline.shader = shader;
    configuracao_pipeline.depth_test = false;
    configuracao_pipeline.depth_write = false;
    configuracao_pipeline.cull_back_faces = false;
    if (lume_pipeline_create(aplicativo, &configuracao_pipeline, &pipeline) != LUME_SUCCESS)
        goto falha;
    configuracao_material.custom_pipeline = pipeline;
    configuracao_material.double_sided = true;
    if (lume_material_create(aplicativo, &configuracao_material, &material) != LUME_SUCCESS ||
        lume_mesh_create(cena, plano, material, &tela) != LUME_SUCCESS)
        goto falha;

    /* O plano preenche a visão; o shader faz o traçado geodésico em unidades normalizadas. */
    lume_node_set_position(camera, (LumeVec3){0.0f, 0.0f, 1.0f});
    lume_geometry_release(plano);
    lume_material_release(material);
    lume_pipeline_release(pipeline);
    lume_shader_release(shader);

    while (!lume_app_should_close(aplicativo))
    {
        float delta = lume_app_begin_frame(aplicativo);
        tempo += delta;
        if (lume_key_was_pressed(aplicativo, LUME_KEY_ESCAPE))
            lume_app_request_close(aplicativo);
        /* Os controles orbitais mantêm a câmera e o integrador do exemplo de referência sincronizados. */
        if (lume_key_is_down(aplicativo, LUME_KEY_LEFT))
            azimute -= delta * 0.72f;
        if (lume_key_is_down(aplicativo, LUME_KEY_RIGHT))
            azimute += delta * 0.72f;
        if (lume_key_is_down(aplicativo, LUME_KEY_UP))
            elevacao += delta * 0.45f;
        if (lume_key_is_down(aplicativo, LUME_KEY_DOWN))
            elevacao -= delta * 0.45f;
        if (lume_key_is_down(aplicativo, LUME_KEY_W))
            distancia -= delta * 2.8f;
        if (lume_key_is_down(aplicativo, LUME_KEY_S))
            distancia += delta * 2.8f;
        if (lume_key_was_pressed(aplicativo, LUME_KEY_R))
        {
            azimute = 0.0f;
            elevacao = 0.55f;
            distancia = 11.2f;
        }
        elevacao = fmaxf(0.12f, fminf(elevacao, 1.08f));
        distancia = fmaxf(6.5f, fminf(distancia, 15.0f));
        lume_app_get_framebuffer_size(aplicativo, &largura, &altura);
        lume_shader_set_float(shader, "uTime", tempo);
        lume_shader_set_vec2(shader, "uResolution", (LumeVec2){(float)largura, (float)altura});
        lume_shader_set_float(shader, "uAzimuth", azimute);
        lume_shader_set_float(shader, "uElevation", elevacao);
        lume_shader_set_float(shader, "uDistance", distancia);
        if (lume_app_render(aplicativo, cena, camera) != LUME_SUCCESS)
            goto falha;
        lume_app_end_frame(aplicativo);
        if (smoke && ++quadros >= 2)
            lume_app_request_close(aplicativo);
    }

    if (!smoke)
        puts("Black hole simulation finished.");
    lume_app_destroy(aplicativo);
    return 0;

falha:
    fprintf(stderr, "Black hole example failed: %s\n", lume_error_last()->message);
    lume_app_destroy(aplicativo);
    return 1;
}
