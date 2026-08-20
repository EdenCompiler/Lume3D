#include <lume/lume.h>

#include <math.h>
#include <stdio.h>
#include <string.h>

static const char *shader_vertice =
    "#version 330 core\n"
    "layout(location=0)in vec3 aPosition;layout(location=2)in vec2 aUv;out vec2 vUv;"
    "void main(){vUv=aUv;gl_Position=vec4(aPosition.xy,0.0,1.0);}";

static const char *shader_fragmento =
    /* Este é o traçador de raios da referência, reduzido para unidades do raio de Schwarzschild. */
    "#version 330 core\n"
    "in vec2 vUv;out vec4 FragColor;uniform vec2 uResolution;uniform float uTime;uniform float uAzimuth;uniform float uElevation;uniform float uDistance;uniform float uGridVisible;uniform float uMoving;uniform vec3 uTarget;"
    "const float RAIO_SCHWARZSCHILD=1.0;const float DISCO_INTERNO=2.2;const float DISCO_EXTERNO=5.2;"
    "float linhaGrade(vec2 p){vec2 largura=max(fwidth(p),vec2(.001));vec2 celula=abs(fract(p-.5)-.5)/largura;return 1.0-min(min(celula.x,celula.y),1.0);}"
    "struct Raio{float r,theta,phi;float dr,dtheta,dphi;float energia;};"
    "vec3 posicao(Raio raio){return vec3(raio.r*sin(raio.theta)*cos(raio.phi),raio.r*sin(raio.theta)*sin(raio.phi),raio.r*cos(raio.theta));}"
    "Raio iniciar(vec3 origem,vec3 direcao){Raio raio;raio.r=length(origem);raio.theta=acos(clamp(origem.z/raio.r,-1.0,1.0));raio.phi=atan(origem.y,origem.x);"
    "float seno=max(sin(raio.theta),.0001);raio.dr=sin(raio.theta)*cos(raio.phi)*direcao.x+sin(raio.theta)*sin(raio.phi)*direcao.y+cos(raio.theta)*direcao.z;"
    "raio.dtheta=(cos(raio.theta)*cos(raio.phi)*direcao.x+cos(raio.theta)*sin(raio.phi)*direcao.y-seno*direcao.z)/raio.r;"
    "raio.dphi=(-sin(raio.phi)*direcao.x+cos(raio.phi)*direcao.y)/(raio.r*seno);"
    "float lapse=max(1.0-RAIO_SCHWARZSCHILD/raio.r,.002);float tempo=sqrt(raio.dr*raio.dr/lapse+raio.r*raio.r*(raio.dtheta*raio.dtheta+seno*seno*raio.dphi*raio.dphi));raio.energia=lapse*tempo;return raio;}"
    "void derivadas(Raio raio,out vec3 d1,out vec3 d2){float r=max(raio.r,1.002);float seno=max(sin(raio.theta),.0001);float lapse=max(1.0-RAIO_SCHWARZSCHILD/r,.002);float tempo=raio.energia/lapse;"
    "d1=vec3(raio.dr,raio.dtheta,raio.dphi);d2.x=-(RAIO_SCHWARZSCHILD/(2.0*r*r))*lapse*tempo*tempo+RAIO_SCHWARZSCHILD*raio.dr*raio.dr/(2.0*r*r*lapse)+r*(raio.dtheta*raio.dtheta+seno*seno*raio.dphi*raio.dphi);"
    "d2.y=-2.0*raio.dr*raio.dtheta/r+sin(raio.theta)*cos(raio.theta)*raio.dphi*raio.dphi;d2.z=-2.0*raio.dr*raio.dphi/r-2.0*cos(raio.theta)/seno*raio.dtheta*raio.dphi;}"
    "Raio avancar(Raio raio,float passo){vec3 d1,d2;derivadas(raio,d1,d2);Raio meio=raio;meio.r+=d1.x*passo*.5;meio.theta+=d1.y*passo*.5;meio.phi+=d1.z*passo*.5;meio.dr+=d2.x*passo*.5;meio.dtheta+=d2.y*passo*.5;meio.dphi+=d2.z*passo*.5;derivadas(meio,d1,d2);raio.r+=d1.x*passo;raio.theta+=d1.y*passo;raio.phi+=d1.z*passo;raio.dr+=d2.x*passo;raio.dtheta+=d2.y*passo;raio.dphi+=d2.z*passo;raio.theta=clamp(raio.theta,.002,3.139);return raio;}"
    "bool corpoDireto(vec3 origem,vec3 direcao,vec3 centro,float raio){vec3 relativo=origem-centro;float b=dot(relativo,direcao);float discriminante=b*b-dot(relativo,relativo)+raio*raio;return discriminante>=0.0&&-b-sqrt(discriminante)>0.0;}"
    "void main(){vec2 pixel=gl_FragCoord.xy;float aspecto=uResolution.x/uResolution.y;vec2 tela=(vUv-.5)*2.0;tela.x*=aspecto;float u=(2.0*(pixel.x+.5)/uResolution.x-1.0)*aspecto*.57735;float v=(1.0-2.0*(pixel.y+.5)/uResolution.y)*.57735;"
    "vec3 camera=uTarget+uDistance*vec3(cos(uElevation)*cos(uAzimuth),sin(uElevation),cos(uElevation)*sin(uAzimuth));vec3 frente=normalize(uTarget-camera);vec3 direita=normalize(cross(frente,vec3(0.0,1.0,0.0)));vec3 cima=cross(direita,frente);"
    "vec3 direcao=normalize(u*direita-v*cima+frente);Raio raio=iniciar(camera,direcao);vec3 anterior=posicao(raio);bool horizonte=false,disco=false;bool corpo_azul=corpoDireto(camera,direcao,vec3(18.1,0.0,0.0),3.15);bool corpo_vermelho=corpoDireto(camera,direcao,vec3(-12.6,0.0,-4.0),3.15);vec3 ponto=vec3(0.0);"
    "int limite=uMoving>.5?420:900;float passo_base=uMoving>.5?.060:.030;for(int passo=0;passo<900;passo++){if(passo>=limite)break;if(raio.r<=RAIO_SCHWARZSCHILD){horizonte=true;break;}float tamanho=passo_base*clamp(raio.r/6.0,.38,2.0);raio=avancar(raio,tamanho);vec3 atual=posicao(raio);"
    "if(anterior.y*atual.y<0.0){float fracao=abs(anterior.y)/max(abs(anterior.y)+abs(atual.y),.0001);vec3 cruzamento=mix(anterior,atual,fracao);float raio_disco=length(cruzamento.xz);if(raio_disco>=DISCO_INTERNO&&raio_disco<=DISCO_EXTERNO){disco=true;ponto=cruzamento;break;}}"
    "anterior=atual;if(raio.r>36.0)break;}"
    "if(disco){float normalizado=clamp((length(ponto)-DISCO_INTERNO)/(DISCO_EXTERNO-DISCO_INTERNO),0.0,1.0);float angulo=atan(ponto.y,ponto.x);vec3 quente=vec3(1.0,1.0,.78),medio=vec3(1.0,.34,.003),frio=vec3(.78,.001,.0);vec3 cor=mix(medio,quente,smoothstep(.0,.32,1.0-normalizado));cor=mix(frio,cor,smoothstep(.28,1.0,1.0-normalizado));cor*=.78+.28*sin(angulo*10.0-normalizado*20.0-uTime*.10);FragColor=vec4(cor,1.0);return;}"
    "if(horizonte){FragColor=vec4(0.0,0.0,0.0,1.0);return;}if(corpo_azul){FragColor=vec4(.24,.48,1.0,1.0);return;}if(corpo_vermelho){FragColor=vec4(.38,.035,.010,1.0);return;}"
    "float mascara=1.0-smoothstep(.02,.26,tela.y);float poco=exp(-dot(tela*vec2(1.0,1.45),tela*vec2(1.0,1.45))*4.2);vec2 deformada=vec2(tela.x*(1.0-.16*poco),tela.y+.10*poco);"
    "float distancia_grade=1.0/max(.045,.30-deformada.y);vec2 coordenada=vec2(deformada.x*distancia_grade*5.4,distancia_grade*1.55);float grade=linhaGrade(coordenada)*mascara*uGridVisible;FragColor=vec4(vec3(grade*.28),1.0);}";

int main(int argc, char **argv)
{
    bool smoke = argc > 1 && strcmp(argv[1], "--smoke") == 0;
    LumeAppConfig configuracao = lume_app_config_default();
    LumePerspectiveCameraConfig configuracao_camera = lume_perspective_camera_config_default();
    LumeRendererConfig configuracao_renderizador = lume_renderer_config_default();
    LumeRenderTargetConfig configuracao_alvo = lume_render_target_config_default();
    LumeShaderConfig configuracao_shader = {shader_vertice, shader_fragmento, NULL, NULL};
    LumePipelineConfig configuracao_pipeline = lume_pipeline_config_default();
    LumeMaterialConfig configuracao_raio = lume_material_config_default(LUME_MATERIAL_CUSTOM);
    LumeApp *aplicativo = NULL;
    LumeScene *cena = NULL;
    LumeNode *camera = NULL, *tela = NULL;
    LumeGeometry *plano = NULL;
    LumeMaterial *material_raio = NULL;
    LumeShader *shader = NULL;
    LumePipeline *pipeline = NULL;
    LumeRenderTarget *alvo_baixa_resolucao = NULL;
    float azimute = 0.0f, elevacao = 0.31f, distancia = 13.4f, tempo = 0.0f;
    float alvo_x = 0.0f, alvo_y = -0.20f, alvo_z = 0.0f;
    bool pausado = false, grade_visivel = true;
    int quadros = 0;

    configuracao.title = "Lume3D - Black hole reference scene";
    configuracao.width = 500;
    configuracao.height = 300;
    configuracao.resizable = false;
    configuracao.visible = !smoke;
    configuracao.vsync = !smoke;
    configuracao.clear_color = (LumeColor){0.0f, 0.0f, 0.0f, 1.0f};
    configuracao_camera.field_of_view_radians = 1.0471975512f;
    configuracao_camera.near_plane = .02f;
    configuracao_camera.far_plane = 100.0f;
    configuracao_renderizador.hdr = false;
    configuracao_renderizador.fxaa = false;
    configuracao_renderizador.bloom = false;
    configuracao_renderizador.frustum_culling = false;
    /* A referência calcula 1/7 da resolução da janela e amplia o resultado com filtro linear. */
    configuracao_alvo.width = 72;
    configuracao_alvo.height = 43;
    configuracao_alvo.hdr = false;
    configuracao_alvo.depth = false;
    if (lume_app_create(&configuracao, &aplicativo) != LUME_SUCCESS ||
        lume_scene_create(aplicativo, &cena) != LUME_SUCCESS ||
        lume_camera_create_perspective(cena, &configuracao_camera, &camera) != LUME_SUCCESS ||
        lume_geometry_create_plane(aplicativo, 2.0f, 2.0f, &plano) != LUME_SUCCESS ||
        lume_shader_create(aplicativo, &configuracao_shader, &shader) != LUME_SUCCESS ||
        lume_render_target_create(aplicativo, &configuracao_alvo, &alvo_baixa_resolucao) != LUME_SUCCESS)
        goto falha;
    if (lume_renderer_configure(lume_app_renderer(aplicativo), &configuracao_renderizador) != LUME_SUCCESS)
        goto falha;
    configuracao_pipeline.shader = shader;
    configuracao_pipeline.depth_test = false;
    configuracao_pipeline.depth_write = false;
    configuracao_pipeline.blending = true;
    configuracao_pipeline.cull_back_faces = false;
    if (lume_pipeline_create(aplicativo, &configuracao_pipeline, &pipeline) != LUME_SUCCESS)
        goto falha;
    configuracao_raio.custom_pipeline = pipeline;
    configuracao_raio.double_sided = true;
    if (lume_material_create(aplicativo, &configuracao_raio, &material_raio) != LUME_SUCCESS ||
        lume_mesh_create(cena, plano, material_raio, &tela) != LUME_SUCCESS)
        goto falha;
    lume_geometry_release(plano);
    lume_material_release(material_raio);
    lume_pipeline_release(pipeline);
    lume_shader_release(shader);
    if (!smoke)
    {
        puts("Controls:");
        puts("  Left mouse + drag: orbit camera");
        puts("  Middle mouse + drag: pan camera");
        puts("  Mouse wheel: zoom");
        puts("  R: reset camera");
        puts("  P: pause or resume disk motion");
        puts("  G: toggle gravity grid");
        puts("  Esc: exit");
    }

    while (!lume_app_should_close(aplicativo))
    {
        float delta = lume_app_begin_frame(aplicativo);
        float coseno, delta_mouse_x, delta_mouse_y, rolagem_y;
        bool movendo_camera;

        if (lume_key_was_pressed(aplicativo, LUME_KEY_ESCAPE))
            lume_app_request_close(aplicativo);
        if (lume_key_was_pressed(aplicativo, LUME_KEY_P))
            pausado = !pausado;
        if (lume_key_was_pressed(aplicativo, LUME_KEY_G))
            grade_visivel = !grade_visivel;
        lume_mouse_get_delta(aplicativo, &delta_mouse_x, &delta_mouse_y);
        lume_mouse_get_scroll(aplicativo, NULL, &rolagem_y);
        movendo_camera = lume_mouse_button_is_down(aplicativo, LUME_MOUSE_BUTTON_LEFT) ||
                         lume_mouse_button_is_down(aplicativo, LUME_MOUSE_BUTTON_MIDDLE) || rolagem_y != 0.0f ||
                         lume_key_is_down(aplicativo, LUME_KEY_LEFT) ||
                         lume_key_is_down(aplicativo, LUME_KEY_RIGHT) || lume_key_is_down(aplicativo, LUME_KEY_UP) ||
                         lume_key_is_down(aplicativo, LUME_KEY_DOWN) || lume_key_is_down(aplicativo, LUME_KEY_W) ||
                         lume_key_is_down(aplicativo, LUME_KEY_S);
        if (lume_mouse_button_is_down(aplicativo, LUME_MOUSE_BUTTON_LEFT))
        {
            azimute += delta_mouse_x * 0.010f;
            elevacao -= delta_mouse_y * 0.010f;
        }
        if (lume_mouse_button_is_down(aplicativo, LUME_MOUSE_BUTTON_MIDDLE))
        {
            float escala = distancia * 0.0025f;
            float direita_x = -sinf(azimute), direita_z = cosf(azimute);
            float cima_x = -sinf(elevacao) * cosf(azimute);
            float cima_y = cosf(elevacao);
            float cima_z = -sinf(elevacao) * sinf(azimute);

            alvo_x += -delta_mouse_x * escala * direita_x + delta_mouse_y * escala * cima_x;
            alvo_y += delta_mouse_y * escala * cima_y;
            alvo_z += -delta_mouse_x * escala * direita_z + delta_mouse_y * escala * cima_z;
        }
        distancia -= rolagem_y * 0.72f;
        if (lume_key_is_down(aplicativo, LUME_KEY_LEFT))
            azimute -= delta * .72f;
        if (lume_key_is_down(aplicativo, LUME_KEY_RIGHT))
            azimute += delta * .72f;
        if (lume_key_is_down(aplicativo, LUME_KEY_UP))
            elevacao += delta * .42f;
        if (lume_key_is_down(aplicativo, LUME_KEY_DOWN))
            elevacao -= delta * .42f;
        if (lume_key_is_down(aplicativo, LUME_KEY_W))
            distancia -= delta * 2.6f;
        if (lume_key_is_down(aplicativo, LUME_KEY_S))
            distancia += delta * 2.6f;
        if (lume_key_was_pressed(aplicativo, LUME_KEY_R))
        {
            azimute = 0.0f;
            elevacao = .31f;
            distancia = 13.4f;
            alvo_x = 0.0f;
            alvo_y = -0.20f;
            alvo_z = 0.0f;
        }
        elevacao = fmaxf(.10f, fminf(elevacao, 1.02f));
        distancia = fmaxf(7.0f, fminf(distancia, 20.0f));
        coseno = cosf(elevacao);
        if (!pausado)
            tempo += delta;
        lume_node_set_position(camera, (LumeVec3){alvo_x + coseno * cosf(azimute) * distancia,
                                                   alvo_y + sinf(elevacao) * distancia,
                                                   alvo_z + coseno * sinf(azimute) * distancia});
        lume_node_look_at(camera, (LumeVec3){alvo_x, alvo_y, alvo_z});
        lume_shader_set_float(shader, "uTime", tempo);
        lume_shader_set_vec2(shader, "uResolution",
                             (LumeVec2){(float)configuracao_alvo.width, (float)configuracao_alvo.height});
        lume_shader_set_float(shader, "uAzimuth", azimute);
        lume_shader_set_float(shader, "uElevation", elevacao);
        lume_shader_set_float(shader, "uDistance", distancia);
        lume_shader_set_float(shader, "uGridVisible", grade_visivel ? 1.0f : 0.0f);
        lume_shader_set_float(shader, "uMoving", movendo_camera ? 1.0f : 0.0f);
        lume_shader_set_vec3(shader, "uTarget", (LumeVec3){alvo_x, alvo_y, alvo_z});
        if (lume_renderer_render(lume_app_renderer(aplicativo), cena, camera, alvo_baixa_resolucao) != LUME_SUCCESS ||
            lume_renderer_present_target(lume_app_renderer(aplicativo), alvo_baixa_resolucao) != LUME_SUCCESS)
            goto falha;
        lume_app_end_frame(aplicativo);
        if (smoke && ++quadros >= 2)
            lume_app_request_close(aplicativo);
    }
    if (!smoke)
        puts("Black hole reference scene finished.");
    lume_render_target_release(alvo_baixa_resolucao);
    lume_app_destroy(aplicativo);
    return 0;
falha:
    fprintf(stderr, "Black hole example failed: %s\n", lume_error_last()->message);
    lume_render_target_release(alvo_baixa_resolucao);
    lume_app_destroy(aplicativo);
    return 1;
}
