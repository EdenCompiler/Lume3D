# Exemplos práticos do Lume3D

Lume3D 1.5 inclui dois exemplos focados. Ambos são cenas animadas completas, construídas apenas com a API pública e shaders GLSL customizados; nenhum depende de assets baixados.

Compile e execute:

    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build --parallel

    ./build/lume_example_ocean
    ./build/lume_example_black_hole

Pressione Escape para fechar. Passe `--smoke` para criar uma janela oculta, renderizar dois frames e sair; CTest usa esse modo nos dois exemplos.

## Oceano com shaders

Fonte: `examples/oceano.c`

O oceano é uma superfície contínua densa, deslocada por quatro trens direcionais de ondas Gerstner. O vertex shader altera os três componentes da posição, calcula uma normal suave por diferenças finitas e envia a energia da crista ao fragment shader. O fragment shader combina reflexão Fresnel, brilho solar, cor por altura, ruído em múltiplas escalas e faixas estreitas de espuma quebrando. Um céu procedural de tela inteira cria o horizonte baixo com nuvens.

A câmera segue o enquadramento amplo e próximo da superfície da referência. Use W/A/S/D para percorrer a superfície sem perder esse estilo de câmera.

O exemplo serve como modelo para:

- deslocamento animado de vértices;
- normais por diferenças finitas no shader;
- múltiplos materiais custom e uniformes de frame;
- superfícies procedurais grandes e composição de horizonte baixo.

## Buraco negro de Kerr em rotação

Fonte: `examples/buraco_negro.c`

O fragment shader é uma demonstração compacta de relatividade numérica, não um redemoinho em screen-space. Em unidades geométricas (`G = c = 1`), ele usa um integrador Kerr reduzido por planos de raios com spin `a = 0.82M`. O movimento radial usa o potencial separado de Kerr:

```text
Δ = r² − 2Mr + a²
R(r) = [r² + a² − aξ]² − Δ[η + (ξ − a)²]
```

Ele deriva o horizonte externo como `r₊ = M + √(M² − a²)`, usa o ISCO Kerr prógrado `r ≈ 2.8019M` e avalia a taxa física de frame dragging ZAMO ao reconstruir cada raio num plano orbital 3D contínuo. Raios atravessam o horizonte ou escapam ao campo de estrelas. Raios curvados encontram um disco fino; sua emissão usa velocidade angular de órbita circular Kerr, redshift gravitacional/Doppler, temperatura radial e o fator de intensidade invariante `g³`.

O solver reduzido preserva os principais efeitos do spin Kerr e imagens contínuas primária/secundária do disco, mas não é uma integração adaptativa completa das equações radial e polar de Carter. Ele não modela turbulência magnetohidrodinâmica nem transferência radiativa volumétrica.

O exemplo serve como modelo para:

- shaders custom de tela inteira;
- simulações numericamente integradas em GLSL;
- constantes e condições de parada com significado físico;
- uniformes que acompanham resize.
