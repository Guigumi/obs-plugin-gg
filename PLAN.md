# Mouse Overlay GG - Plano de melhorias

## Objetivo atual

Melhorar exclusivamente a fonte de mouse antes de retomar o desenvolvimento do teclado.

Este documento contem apenas trabalho ainda pendente. Funcionalidades e etapas ja concluidas foram removidas.

## Diretrizes

- Manter o canvas fixo em `1920x1080` neste momento.
- Manter o caminho normal de cada frame sem alocacoes de heap.
- Nao criar threads adicionais.
- Preservar compatibilidade com configuracoes ja salvas no OBS.
- Preferir funcoes pequenas com uma responsabilidade clara.
- Separar cursor, trilha, cliques e recursos em arquivos dedicados.
- Simplificar a interpolacao da trilha, priorizando previsibilidade e manutencao.

## Estrutura proposta

```text
src/
|-- mouse-overlay.c
|-- mouse-overlay.h
|-- mouse-overlay-internal.h
|-- mouse-overlay-cursor.c
|-- mouse-overlay-cursor.h
|-- mouse-overlay-trail.c
|-- mouse-overlay-trail.h
|-- mouse-overlay-click.c
|-- mouse-overlay-click.h
|-- mouse-overlay-resources.c
`-- mouse-overlay-resources.h
```

Responsabilidades:

- `mouse-overlay.c`: ciclo de vida da fonte, propriedades, configuracoes e encaminhamento de `tick`/`render`.
- `mouse-overlay-cursor.c`: posicao, visibilidade, limites e renderizacao do cursor principal.
- `mouse-overlay-trail.c`: buffer circular, espacamento, expiracao, fade e renderizacao da trilha.
- `mouse-overlay-click.c`: captura dos eventos, buffer circular, animacoes e renderizacao dos cliques.
- `mouse-overlay-resources.c`: imagens, efeitos, parametros, techniques e recarga de recursos.
- `mouse-overlay-internal.h`: estado privado compartilhado entre os modulos.
- `mouse-overlay.h`: somente declaracoes publicas necessarias para registrar a fonte.

## 1. Melhoria da logica

### Cursor

- Validar `monitor_index` antes da amostragem e usar todos os monitores quando o indice salvo nao existir mais.
- Centralizar conversao de coordenadas normalizadas para o canvas.
- Centralizar o clamp do centro do cursor para impedir corte nas bordas.
- Resetar continuidade da trilha quando o cursor sair do monitor, voltar a aparecer ou trocar de monitor.

### Trilha

- Separar a logica em funcoes para reset, expiracao, amostragem, insercao e renderizacao.
- Substituir o back-dating atual por interpolacao de distancia acumulada.
- Guardar a distancia restante entre frames para manter pontos igualmente espacados.
- Inserir pontos ao longo do segmento entre a posicao anterior e a atual.
- Atribuir `now_ns` aos novos pontos, removendo calculos artificiais de tempo dentro do frame.
- Limitar a quantidade gerada por tick pela capacidade do buffer.
- Evitar um risco longo ao reaparecer, trocar de monitor ou reativar a trilha.
- Concentrar operacoes do buffer circular em helpers `trail_reset`, `trail_prune` e `trail_push`.
- Manter tamanho minimo interno de renderizacao em `1px`, sem adicionar nova propriedade ao usuario.

### Cliques

- Separar deteccao de borda, insercao, expiracao e renderizacao.
- Criar funcoes puras para escala e opacidade das animacoes.
- Validar o tipo de animacao recebido das configuracoes.
- Concentrar operacoes do buffer circular em helpers `click_reset`, `click_prune` e `click_push`.
- Manter cliques pressionados e eventos animados como estados distintos durante a renderizacao.
- Limpar eventos ao desativar cliques ou quando o cursor sair do monitor selecionado.

## 2. Melhoria da estrutura

Usar subestruturas privadas semelhantes a:

```c
struct mouse_cursor_state;
struct mouse_trail_state;
struct mouse_click_state;
struct mouse_overlay_resources;
```

- Configuracoes persistentes devem ficar junto do recurso que as utiliza.
- Estado transitorio, como indices de buffer e timestamps, deve ficar separado das configuracoes.
- O arquivo principal nao deve conhecer detalhes de interpolacao ou animacao.
- Cada modulo deve expor somente as operacoes necessarias para criar, atualizar, resetar, executar tick e renderizar.
- Nao criar uma camada generica de abstracao para um unico uso; compartilhar apenas carregamento, tint e desenho de sprites.

## 3. Qualidade de vida das funcoes

- Criar helpers de clamp para `float`, inteiro e percentual.
- Criar helpers de leitura validada para configuracoes `double`, inteiro e enum.
- Usar `isfinite` antes de aceitar todos os valores de ponto flutuante.
- Armazenar internamente opacidades e escalas ja normalizadas em `0.0-1.0`.
- Precalcular duracoes em nanossegundos no `update`, evitando conversao a cada frame.
- Criar helper unico para carregar uma imagem por caminho e registrar erros.
- Criar helper unico para carregar um efeito e validar techniques e parametros obrigatorios.
- Criar helper de desenho que encapsule textura, opacidade, transformacao e tamanho.
- Criar helper de tint que aceite a cor OBS e configure o `vec4` correto.
- Retornar cedo de `tick` e `render` quando cursor, trilha e cliques estiverem todos inativos.
- Evitar macros de leitura de configuracoes; preferir funcoes tipadas e depuraveis.

## 4. Minimos, maximos e capacidades

### Valores de interface

| Configuracao | Minimo | Padrao | Maximo | Passo |
| --- | ---: | ---: | ---: | ---: |
| Tamanho do cursor | 4px | 25px | 128px | 1px |
| Opacidade do cursor | 0% | 100% | 100% | 1% |
| Pontos da trilha | 1 | 25 | 256 | 1 |
| Duracao da trilha | 0.05s | 0.25s | 30s | 0.05s |
| Espacamento da trilha | 1px | 20px | 256px | 1px |
| Tamanho da trilha | 10% | 100% | 200% | 1% |
| Opacidade da trilha | 0% | 50% | 100% | 1% |
| Duracao do clique | 0.05s | 0.25s | 1.50s | 0.05s |
| Opacidade do clique | 0% | 25% | 100% | 1% |

### Capacidades internas

- Capacidade do buffer da trilha: `256` pontos.
- Capacidade do buffer de cliques: `256` eventos.
- Tamanho minimo renderizavel: `1px`.
- Escala minima das animacoes de clique: `0.5`.
- Escala maxima das animacoes de clique: `1.0`.

As capacidades internas nao devem ser apresentadas como configuracoes separadas.

## 5. Ordem de implementacao

### Fase 1 - Estrutura e validacao

- Criar os novos arquivos e atualizar `CMakeLists.txt`.
- Separar configuracao de estado transitorio nos novos modulos.
- Aplicar os novos limites.
- Implementar leitura validada e clamp de todas as configuracoes.

### Fase 2 - Recursos e cursor

- Mover imagens, efeitos e helpers graficos para o modulo de recursos.
- Mover atualizacao e renderizacao do cursor para seu modulo.
- Validar monitor e resetar continuidade quando a visibilidade mudar.
- Preservar o comportamento visual atual antes de alterar trilha e cliques.

### Fase 3 - Trilha

- Mover o buffer circular para o modulo da trilha.
- Implementar interpolacao por distancia acumulada.
- Remover back-dating e estado antigo de interpolacao.
- Validar espacamento, expiracao, shrink e fades.

### Fase 4 - Cliques

- Mover o buffer e animacoes para o modulo de cliques.
- Extrair funcoes puras de escala e opacidade.
- Validar eventos simultaneos, clique segurado e tipos de animacao.

### Fase 5 - Verificacao

- Executar `clang-format` nos arquivos alterados.
- Compilar em `RelWithDebInfo` sem avisos novos.
- Instalar DLL, PDB e dados em ProgramData.
- Testar cursor nos limites de `4px` e `256px`.
- Testar duracoes minima e maxima da trilha e dos cliques.
- Testar movimentos lentos, rapidos e saltos entre monitores.
- Testar troca, desconexao e reconexao de monitor.
- Testar habilitar e desabilitar cursor, trilha e cliques durante uso.
- Confirmar ausencia de alocacoes no caminho normal de `video_tick` e `video_render`.
- Verificar logs por efeitos, imagens ou recursos nao liberados.

## Criterios de conclusao

- `mouse-overlay.c` fica limitado a orquestracao da fonte OBS.
- Cursor, trilha, cliques e recursos ficam em modulos separados.
- Todos os valores recebidos das configuracoes sao validados e limitados.
- A trilha usa distancia acumulada sem back-dating.
- O comportamento do cursor e dos cliques permanece funcional.
- Nao existem consultas de techniques nem alocacoes de heap por frame.
- Build, instalacao e carregamento no OBS concluem sem erros novos.
