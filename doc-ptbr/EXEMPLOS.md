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

## Buraco negro de Schwarzschild e disco em rotação

Fonte: `examples/buraco_negro.c`

O fragment shader é uma demonstração compacta de relatividade numérica, não um redemoinho em screen-space. Ele adapta a estrutura do ray tracer C fornecido para uma implementação GLSL em tempo real, em unidades normalizadas do raio de Schwarzschild. Um raio guarda posição e velocidade esféricas; o shader o avança com integração de ponto médio das equações diferenciais de geodésicas de Schwarzschild:

```text
f(r) = 1 − rₛ / r
d²r/dλ², d²θ/dλ², d²φ/dλ² = lado direito da geodésica de Schwarzschild
```

Em cada passo ele testa o horizonte de eventos (`r ≤ rₛ`), os dois corpos em órbita e a mudança de sinal na passagem pelo plano equatorial. Uma passagem entre os raios do disco emite as cores rotativas e aquecidas radialmente usadas pelo programa fornecido. Raios que escapam revelam uma grade gravitacional em perspectiva deformada ao redor da massa central.

Os controles espelham o programa fornecido: arrastar com o botão esquerdo orbita, arrastar com o botão do meio desloca, a roda aplica zoom, `R` restaura a câmera, `P` pausa ou retoma o movimento do disco, `G` alterna a grade e `Esc` sai. As setas e `W`/`S` continuam como alternativas convenientes para órbita e zoom.

O solver reduzido usa um número fixo de passos de ponto médio. Ele não é um renderizador adaptativo, um solver Kerr completo (métrica com spin), uma simulação magnetohidrodinâmica do disco ou transferência radiativa volumétrica. A rotação do disco é visual; frame dragging exige uma extensão Kerr, não esta métrica de Schwarzschild.

Em sistemas apenas com CPU, o passe geodésico segue o programa fornecido e renderiza em `72 × 43`; `lume_renderer_present_target` amplia o resultado para a janela de `500 × 300`. O movimento da câmera usa temporariamente 420 passos mais largos por raio; a imagem parada usa 900. Isso mantém o uso de memória modesto e reduz o trabalho de fragmentos em cerca de 49 vezes frente ao traçado em resolução completa.

O exemplo serve como modelo para:

- shaders custom de tela inteira;
- simulações numericamente integradas em GLSL;
- constantes e condições de parada com significado físico;
- uniformes que acompanham resize.
