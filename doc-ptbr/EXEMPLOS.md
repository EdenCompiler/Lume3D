# Guia dos exemplos Lume3D

Compile os exemplos com a configuração padrão:

    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build --parallel

Pressione Escape para fechar qualquer exemplo.

Passe `--smoke` para ocultar a janela, desativar VSync, renderizar dois frames e sair. Esse é o modo usado pela suíte de testes.

## Exemplos disponíveis

| Target | Fonte | Demonstra |
| --- | --- | --- |
| `lume_example_cube` | `examples/cubo.c` | Fluxo mínimo app/cena/câmera, material basic e rotação independente de frame rate |
| `lume_example_solar_system` | `examples/sistema_solar.c` | Pivôs vazios, transformações aninhadas, geometria compartilhada e vários materiais |
| `lume_example_lighting` | `examples/iluminacao.c` | Textura RGBA procedural, material Lambert, luzes ambiente/direcional/pontual e câmera look-at |

Execute no Linux a partir da raiz:

    ./build/lume_example_cube
    ./build/lume_example_solar_system
    ./build/lume_example_lighting

    ./build/lume_example_cube --smoke

Geradores multi-configuração colocam executáveis em subdiretório como `build/Release`.

## Cubo giratório

O exemplo do cubo segue o menor ciclo útil. Cria primeiro o aplicativo porque cenas e recursos pertencem a ele. Depois cria cena, câmera perspectiva padrão, geometria de caixa, material basic e nó de malha.

A câmera move-se para +Z e mantém a direção local −Z padrão. Cada frame processa eventos, gira o cubo usando segundos decorridos, renderiza e troca buffers. Todo caminho de falha imprime `lume_get_last_error()` em inglês e libera o aplicativo.

## Hierarquia de cena

O sistema solar compartilha uma esfera entre três malhas. Nós vazios funcionam como pivôs de órbita. Girar o pivô da Terra move a Terra e o pivô da Lua; o pivô da Lua fornece sua órbita independente.

O exemplo usa materiais basic para mostrar cores sem luzes e manter o foco na relação entre transformações.

## Iluminação

O exemplo de iluminação cria uma textura checker 2×2 diretamente de bytes RGBA. Um material Lambert combina a textura com luz ambiente, direcional e pontual quente. A posição do nó de luz pontual controla sua fonte no mundo.
