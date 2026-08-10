# Keyboard Overlay GG - Plano de melhorias

## Objetivo atual

Evoluir o overlay de teclado de um conjunto fixo de quatro teclas para layouts de jogo mais flexiveis, com captura
confiavel, desenho procedural e configuracao clara, sem regredir o comportamento atual de WASD.

## Estado atual

- Layout fixo de `W`, `A`, `S` e `D`.
- Setas podem funcionar como aliases de WASD.
- Tamanho, espacamento, rotacao, opacidades, cor, fonte e caracteres sao configuraveis.
- Feedback disponivel por circulo suave, borda pulsante ou ambos.
- Rotulos sao renderizados em texturas com supersampling.
- Captura atual usa `GetAsyncKeyState` a cada `video_tick`.
- A aparencia principal depende de `data/images/key-main.png`.

## Decisoes iniciais

- Definir o primeiro conjunto expandido de teclas: `WASD`, `Espaco`, `Shift`, `Ctrl`, `Q`, `E`, `R`, `F`, `Tab`,
  `Caps Lock` e numeros de `1` a `5`.
- Definir os presets iniciais: `WASD`, `ESDF`, `Setas`, `Numpad` e `Personalizado`.
- Escolher entre Raw Input de teclado e `WH_KEYBOARD_LL`, priorizando confiabilidade, compatibilidade com jogos e
  ausencia de interferencia com outros plugins.
- Decisao: usar Raw Input em thread e janela dedicadas, sem suprimir mensagens legadas e sem substituir um registro
  de teclado pertencente a outro componente. Usar `GetAsyncKeyState` somente como fallback degradado.
- Definir se a primeira iteracao permite apenas presets ou tambem posicionamento individual das teclas.

## Fase 0 - Limpeza de codigo e revisao de estrutura

Status: implementacao estrutural concluida; validacao visual do layout WASD ainda pendente.

- Remover codigo morto, comentarios obsoletos e includes nao utilizados em `keyboard-overlay.c`.
- Padronizar nomes internos e substituir valores magicos por constantes nomeadas no modulo responsavel.
- Aplicar `.clang-format` antes de iniciar mudancas funcionais.
- Separar o codigo atual sem alterar comportamento:
  - `keyboard-overlay.c`: ciclo de vida e integracao com OBS.
  - `keyboard-overlay-internal.h`: estado compartilhado da fonte.
  - `keyboard-overlay-layout.c/h`: posicoes e dimensoes do layout WASD atual.
  - `keyboard-overlay-keys.c/h`: configuracoes, captura e animacao das teclas.
  - `keyboard-overlay-resources.c/h`: shader, imagens, rotulos e renderizacao.
  - `keyboard-capture-win32.c`: manter temporariamente a captura atual com `GetAsyncKeyState`.
- Atualizar `CMakeLists.txt` para compilar os novos modulos.
- Executar build e revisar o diff para confirmar que o layout e o comportamento WASD permanecem inalterados.

## Fase 1 - Arquitetura e captura

Status: implementacao concluida; validacao funcional em jogos e com multiplas fontes ainda pendente.

- Separar responsabilidades hoje concentradas em `keyboard-overlay.c`:
  - `keyboard-overlay.c`: ciclo de vida e integracao com OBS.
  - `keyboard-overlay-internal.h`: estado compartilhado da fonte.
  - `keyboard-overlay-layout.c/h`: presets, dimensoes e posicoes.
  - `keyboard-overlay-keys.c/h`: estado e animacao das teclas.
  - `keyboard-overlay-resources.c/h`: shader, rotulos e recursos graficos.
  - `keyboard-capture-win32.c`: captura de entrada do Windows.
- Manter `video_tick` e `video_render` sem alocacoes no caminho normal.
- Substituir `GetAsyncKeyState` pelo metodo de captura escolhido.
- Compartilhar eventos entre varias fontes sem uma fonte consumir a entrada das demais.
- Tratar pressionamentos e solturas rapidos, teclas simultaneas, auto-repeat e perda de foco.
- Garantir inicializacao e encerramento seguros, sem hooks, handles ou threads remanescentes.

## Fase 2 - Layouts e configuracao

Status: em andamento; modelo interno por tecla concluido para o layout WASD atual.

- Representar cada tecla com codigo de captura, rotulo, posicao, tamanho e estado de visibilidade.
- Adicionar presets para os layouts definidos na fase inicial.
- Preservar o layout WASD atual como padrao e manter aliases de setas quando aplicavel.
- Permitir mostrar ou ocultar teclas individualmente.
- Permitir caracteres personalizados para todas as teclas visiveis.
- Avaliar offsets ou coordenadas individuais para o modo `Personalizado`.
- Atualizar automaticamente as dimensoes da fonte conforme layout, tamanho, espacamento e rotacao.
- Organizar as propriedades do OBS para evitar uma lista extensa e confusa de campos.
- Avaliar importacao e exportacao do layout personalizado em JSON somente se o editor justificar essa complexidade.

## Fase 3 - Aparencia procedural

- Substituir `key-main.png` por uma tecla procedural com cantos arredondados.
- Preservar escala suave entre os tamanhos minimo e maximo.
- Adicionar animacao curta de pressionamento com escala e retorno, sem alterar as dimensoes da fonte.
- Manter os efeitos de circulo, borda e combinacao, ajustando antialiasing pelo tamanho renderizado.
- Adicionar sombra ou contorno configuravel aos rotulos para leitura em fundos claros e escuros.
- Avaliar cor individual por tecla sem prejudicar a configuracao simples de cor global.
- Eliminar texturas e parametros que deixarem de ser necessarios depois da migracao procedural.

## Fase 4 - Desempenho e robustez

- Recriar texturas de rotulo apenas quando fonte, texto ou tamanho efetivamente mudarem.
- Evitar chamadas GDI e operacoes graficas redundantes durante atualizacoes de propriedades.
- Validar limites e valores nao finitos de todas as novas configuracoes.
- Garantir que recursos parcialmente carregados sejam liberados corretamente.
- Manter o custo proporcional ao numero de teclas visiveis.
- Verificar comportamento com duas ou mais fontes usando layouts diferentes.

## Validacao funcional

- Confirmar pressionamento e soltura de cada tecla suportada dentro e fora do foco do OBS.
- Testar toques muito rapidos, teclas mantidas e varias teclas simultaneas.
- Confirmar que auto-repeat nao reinicia indevidamente a animacao de pressionamento.
- Testar aliases, presets e transicao entre layouts sem estados presos.
- Testar caracteres vazios, longos, Unicode e fontes ausentes.
- Testar tamanhos e espacamentos minimos e maximos, incluindo rotacoes de `-180` a `180` graus.
- Confirmar que os efeitos e rotulos permanecem dentro dos limites da fonte.
- Testar habilitar, desabilitar, duplicar, reconfigurar e remover fontes durante o uso.
- Testar jogos em tela cheia, janela sem borda, UAC, RDP e maquina virtual quando aplicavel ao metodo de captura.

## Validacao tecnica

- Medir alocacoes em `video_tick` e `video_render`.
- Comparar uso de CPU ocioso e com todas as teclas animando.
- Testar carregamento e encerramento repetido do OBS.
- Verificar logs por erros de shader, fonte, textura, hook, handle ou recurso nao liberado.
- Confirmar que a captura do teclado nao interfere no Raw Input do mouse ou em outros plugins.
- Revisar o diff completo e executar build `RelWithDebInfo` antes da instalacao.

## Criterios de conclusao

- O layout WASD atual continua funcional e compativel com configuracoes existentes.
- Os novos layouts representam corretamente todas as teclas configuradas.
- Nenhum pressionamento ou soltura fica preso durante uso normal, perda de foco ou troca de cena.
- Animacoes, rotulos e formas permanecem suaves em todos os limites.
- Nao existem alocacoes recorrentes desnecessarias nem erros novos no log do OBS.
- O desempenho permanece adequado com duas ou mais fontes e o maior preset suportado.
