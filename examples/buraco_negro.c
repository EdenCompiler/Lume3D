#include <lume/lume.h>

#include <stdio.h>
#include <string.h>

static const char *shader_vertice =
    "#version 330 core\n"
    "layout(location=0)in vec3 aPosition;layout(location=2)in vec2 aUv;"
    "out vec2 vUv;void main(){vUv=aUv;gl_Position=vec4(aPosition.xy,0.0,1.0);}";

static const char *shader_fragmento =
    /* As equações usam unidades geométricas, nas quais G e c valem 1. */
    "#version 330 core\n"
    "in vec2 vUv;out vec4 FragColor;uniform float uTime;uniform vec2 uResolution;"
    "float hash(vec2 p){return fract(sin(dot(p,vec2(127.1,311.7)))*43758.5453123);}"
    "float stars(vec2 p){vec2 cell=floor(p);vec2 local=fract(p)-.5;float n=hash(cell);"
    "float star=1.0-smoothstep(0.0,.055,length(local));return star*step(.985,n)*(1.2+.8*sin(n*50.0));}"
    "vec3 blackbody(float t){float x=clamp((t-1000.0)/11000.0,0.0,1.0);"
    "return mix(vec3(1.0,.035,.002),mix(vec3(1.0,.34,.035),vec3(.62,.78,1.0),smoothstep(.35,1.0,x)),smoothstep(0.0,.62,x));}"
    "void main(){"
    "const float M=1.0;const float SPIN=.82*M;const float HORIZON=M+sqrt(M*M-SPIN*SPIN);"
    "const float ISCO=2.8019*M;const float OBSERVER_RADIUS=30.0*M;"
    "vec2 screen=(vUv-.5)*vec2(uResolution.x/uResolution.y,1.0);float screenRadius=length(screen);"
    "float psi=atan(screenRadius,.94);float lapse=sqrt(1.0-2.0*M/OBSERVER_RADIUS);"
    "float impact=OBSERVER_RADIUS*sin(psi)/lapse;"
    "float xi=-impact*screen.x/max(screenRadius,.0001);float eta=max(impact*impact-xi*xi,0.0);"
    "float radius=OBSERVER_RADIUS;float radialSign=-1.0;float orbitAngle=0.0;float dragAngle=0.0;"
    "bool captured=false;int diskHits=0;vec3 diskColor=vec3(0.0);float stepSize=.18;"
    "vec3 observerAxis=normalize(vec3(0.0,.18,1.0));vec3 right=vec3(1.0,0.0,0.0);"
    "vec3 up=normalize(cross(observerAxis,right));"
    "vec3 transverse=screenRadius>.0001?normalize(right*screen.x+up*screen.y):right;"
    "vec3 previousPosition=observerAxis*OBSERVER_RADIUS;"
    "for(int iteration=0;iteration<380;iteration++){"
    "float delta=radius*radius-2.0*M*radius+SPIN*SPIN;"
    "float p=radius*radius+SPIN*SPIN-SPIN*xi;"
    "float radialPotential=p*p-delta*(eta+(xi-SPIN)*(xi-SPIN));"
    "if(radialPotential<0.0){radialSign=1.0;radialPotential=0.0;}"
    "float sigma=radius*radius+SPIN*SPIN*.18;"
    "radius+=radialSign*sqrt(radialPotential)/sigma*stepSize;"
    "orbitAngle+=impact/max(radius*radius+SPIN*SPIN, HORIZON*HORIZON)*stepSize;"
    "float area=pow(radius*radius+SPIN*SPIN,2.0)-SPIN*SPIN*max(delta,0.0);"
    "float frameDragging=2.0*M*SPIN*radius/max(area,.001);dragAngle+=frameDragging*stepSize;"
    "if(radius<=HORIZON*1.002){captured=true;break;}"
    "vec3 base=observerAxis*radius*cos(orbitAngle)+transverse*radius*sin(orbitAngle);"
    "float cd=cos(dragAngle),sd=sin(dragAngle);"
    "vec3 position=vec3(cd*base.x+sd*base.z,base.y,-sd*base.x+cd*base.z);"
    "if(diskHits<2&&iteration>2&&previousPosition.y*position.y<=0.0){"
    "float amount=abs(previousPosition.y)/(abs(previousPosition.y)+abs(position.y)+.00001);"
    "vec3 crossing=mix(previousPosition,position,amount);float diskRadius=length(crossing.xz);"
    "if(diskRadius>=ISCO&&diskRadius<=12.0*M){"
    "float diskAzimuth=atan(crossing.z,crossing.x);float orbitalOmega=1.0/(pow(diskRadius,1.5)+SPIN);"
    "float ut=(pow(diskRadius,1.5)+SPIN)/sqrt(max(pow(diskRadius,3.0)-3.0*diskRadius*diskRadius+2.0*SPIN*pow(diskRadius,1.5),.001));"
    "float redshift=clamp(1.0/(ut*(1.0-orbitalOmega*xi)),.08,2.6);"
    "float temperature=9800.0*pow(ISCO/diskRadius,.75)*pow(max(1.0-sqrt(ISCO/diskRadius),.018),.25);"
    "float spiral=.68+.32*sin(diskRadius*10.0-diskAzimuth*3.0-uTime*.72);"
    "float fineRings=.80+.20*sin(diskRadius*29.0+diskAzimuth*8.0);"
    "float edge=smoothstep(ISCO,ISCO+.34,diskRadius)*(1.0-smoothstep(11.1,12.0,diskRadius));"
    "float imageWeight=diskHits==0?1.0:.38;"
    "diskColor+=blackbody(temperature*redshift)*clamp(pow(redshift,3.0),.05,4.8)*spiral*fineRings*edge*.15*imageWeight;"
    "diskHits++;}}previousPosition=position;"
    "if(radialSign>0.0&&radius>OBSERVER_RADIUS*1.08)break;"
    "}"
    "float skyAngle=atan(screen.y,screen.x)+dragAngle;"
    "vec2 sky=vec2(cos(skyAngle),sin(skyAngle))*orbitAngle*8.0;"
    "float background=stars(sky*13.0)+.5*stars(sky*23.0+11.7);"
    "vec3 color=vec3(.0003,.0006,.002)+background*vec3(.32,.50,1.0);"
    "if(captured)color=vec3(0.0);if(diskHits>0)color+=diskColor;"
    "float frameGlow=exp(-abs(radius-HORIZON)*2.0);"
    "color+=vec3(.65,.09,.006)*frameGlow*.025*float(!captured);"
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

    /* O plano preenche a visão; toda a lente gravitacional é calculada no fragment shader. */
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
        lume_app_get_framebuffer_size(aplicativo, &largura, &altura);
        lume_shader_set_float(shader, "uTime", tempo);
        lume_shader_set_vec2(shader, "uResolution", (LumeVec2){(float)largura, (float)altura});
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
