# Exemplos práticos do Lume3D

Lume3D 1.5.1 inclui cinco exemplos focados. Todas as cenas usam apenas a API pública e funcionam offline, sem assets baixados.

Compile e execute:

    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build --parallel

    ./build/lume_example_ocean
    ./build/lume_example_black_hole
    ./build/lume_example_solar_system
    ./build/lume_example_instanced_city
    ./build/lume_example_lighting_studio

Pressione Escape para fechar. Todos os exemplos aceitam `--smoke`, que cria uma janela oculta, renderiza dois frames e sai. Os três exemplos da 1.5.1 também aceitam `--low`, selecionando janela de 800×450, geometria mais leve, mapas de sombra de 512 pixels e bloom desligado. CTest usa o modo smoke nos cinco programas.

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

## Sistema solar procedural

Fonte: `examples/sistema_solar.c`

O sistema solar constrói planetas, luas e pivôs orbitais com nós comuns da cena. Uma textura equiretangular da Terra gerada em runtime demonstra criação de texturas, enquanto um campo estelar instanciado com distribuição de Fibonacci forma o fundo. Planetas PBR, sol unlit em HDR, luz pontual, ACES, bloom e órbitas desenhadas por debug completam a cena sem assets externos.

Controles: arrastar com o botão esquerdo orbita, botão do meio desloca, a roda aplica zoom, Espaço pausa, Cima/Baixo altera a velocidade da simulação, `O` alterna as órbitas, `R` restaura e Esc sai.

O exemplo serve como modelo para hierarquia de cena, transformações entre pai e filho, texturas procedurais, geometria compartilhada, tipos diferentes de material, linhas de debug e câmera orbital.

## Cidade procedural instanciada

Fonte: `examples/cidade_instanciada.c`

A cidade gera deterministicamente uma grade de ruas e milhares de transformações de prédios. Uma malha instanciada com material custom renderiza o horizonte urbano em uma chamada de desenho para os prédios, com janelas procedurais que respeitam a escala e névoa por distância; faixas instanciadas com teste de profundidade e o piso PBR recebem sombras direcionais em cascata. Um passe LDR sensível à profundidade preenche apenas pixels intocados com um céu procedural de crepúsculo, preservando geometria distante. Os limites agregados corrigidos mantêm todo o conjunto disponível para culling e consultas espaciais.

Controles: WASD move, Q/E altera a altura, arrastar com o botão esquerdo olha ao redor, a roda ajusta a velocidade base, Shift acelera, `I` alterna estatísticas periódicas, `R` restaura e Esc sai.

O exemplo serve como modelo para instancing em hardware, geração determinística, shaders custom sensíveis à escala, pós-processamento orientado por profundidade, câmera livre, sombras, bounds seguros para culling e `LumeFrameStats`.

## Estúdio interativo de iluminação PBR

Fonte: `examples/estudio_iluminacao.c`

O estúdio organiza uma tabela 7×6 de materiais: as colunas aumentam a resposta metálica e as linhas aumentam a rugosidade. Luzes ambiente, direcional, pontuais móveis e spot com sombra iluminam a tabela, o piso e o fundo pelo renderizador HDR integrado. A seleção com o botão direito constrói um raio da câmera, chama `lume_scene_raycast`, destaca a esfera escolhida e pode desenhar seu AABB e eixos.

Controles: arrastar com o botão esquerdo orbita, botão do meio desloca, a roda aplica zoom, botão direito seleciona, `B` alterna bloom, `F` alterna FXAA, `L` pausa as luzes móveis, `D` alterna o debug, `R` restaura e Esc sai.

O exemplo serve como modelo para estudo de parâmetros PBR, os quatro tipos de luz, sombras direcionais e spot, reconfiguração do renderizador em runtime, picking pelo mouse e primitivas de debug.
