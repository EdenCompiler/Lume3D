# Arquitetura do Lume3D

## Objetivo

Lume3D é um renderizador de cenas acessível, não uma engine completa de jogos. A API pública favorece chamadas curtas de criação e configuração, enquanto a implementação C mantém ownership explícito e uma fronteira OpenGL pequena.

## Organização dos módulos

A superfície pública fica em `include/lume/lume.h`. A implementação divide-se em aplicativo/input, matemática, grafo de cena, geometria, recursos e renderizador. Tipos e contratos privados compartilhados ficam em `src/lume_interno.h`.

GLFW cuida da integração nativa de janela. GLAD fornece declarações OpenGL 3.3 geradas. stb_image decodifica imagens. Vetores, matrizes, transformações, cenas e geometrias são código próprio do Lume3D.

## Fluxo de dados do frame

```text
processar input → atualizar nós → atualizar matrizes mundiais → coletar luzes
                 → configurar câmera → configurar malha/material → desenhar → apresentar
```

Dados de geometria na CPU são copiados na criação e enviados a VAO/VBO/EBO no primeiro uso. Materiais referenciam texturas compartilhadas. Cada render percorre o registro plano de nós da cena; transformações hierárquicas são atualizadas recursivamente pelas raízes.

## Convenções de coordenadas e matrizes

Lume3D usa coordenadas destras, +Y para cima e −Z local como frente da câmera. Matrizes são column-major para OpenGL. A composição é translação × rotação Z × rotação Y × rotação X × escala. Rotações Euler públicas usam radianos.

A matriz de visão é a inversa da transformação mundial da câmera. A projeção perspectiva usa profundidade de clip OpenGL −1..1. Proporção perspectiva zero seleciona a proporção do framebuffer a cada frame.

## Ownership

O aplicativo possui estado do renderizador e todos os recursos registrados. Uma cena pertence a exatamente um aplicativo e possui todo nó criado por ela. Um nó pode ter um pai e vários filhos na mesma cena.

Destruir um nó destrói recursivamente os descendentes. Destruir uma cena destrói seus nós. Destruir o aplicativo destrói primeiro cenas, depois recursos de GPU, renderizador e janela.

## Fronteira do renderizador

Chamadas OpenGL diretas ficam restritas à inicialização/destruição do aplicativo, upload de geometria, upload de textura e renderização. Isso mantém cena e matemática testáveis de forma independente e prepara um futuro backend sem alterar ownership da cena.

O shader 1.0 implementa transformações, composição de textura/cor, materiais basic sem luz e iluminação difusa Lambert. Transparência, sombras, pipelines personalizados e filas de render estão fora desta fronteira.

## Convenção de idioma

Símbolos públicos, diagnósticos de runtime, identificadores de shader, targets de build, saída de testes e exemplos voltados ao usuário usam inglês. Identificadores C privados e comentários do fonte usam português do Brasil. A documentação é espelhada nos dois idiomas.
