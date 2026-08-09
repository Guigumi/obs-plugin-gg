# Mouse Overlay GG - Validacao pendente

## Objetivo atual

Validar visualmente a implementacao modular do mouse e o novo modo de cursor para jogos antes de retomar melhorias
do teclado.

As etapas concluidas de limpeza, logica, helpers, limites e separacao em modulos foram removidas deste documento.

## Estrutura atual

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

## Validacao visual

- Testar cursor nos limites de `4px` e `256px`.
- Testar trilha com duracoes de `0.05s` e `30s`.
- Testar cliques com duracoes de `0.05s` e `5s`.
- Testar movimentos lentos, rapidos e saltos longos do cursor.
- Confirmar que a trilha mantem espacamento uniforme em diferentes taxas de frame.
- Confirmar que a trilha antiga desaparece suavemente ao sair do monitor, sem criar uma linha ao retornar.
- Testar troca, desconexao e reconexao de monitor.
- Confirmar que nenhuma animacao de clique ultrapassa o tamanho do `cursor-main`.
- Confirmar que o clique procedural permanece suave entre `4px` e `256px`, sem serrilhado dos antigos PNGs.
- Confirmar que um clique solto permanece na posicao em que ocorreu.
- Confirmar que a animacao de entrada acontece apenas uma vez e termina em escala `1.0`.
- Confirmar que o efeito permanece estatico e acompanha o cursor enquanto o botao estiver segurado.
- Confirmar que o fade-out inicia somente ao soltar e permanece na posicao de soltura.
- Confirmar que clique segurado nao duplica a animacao e o indicador estatico.
- Testar cliques esquerdo e direito simultaneos.
- Testar habilitar e desabilitar cursor, trilha e cliques durante o uso.
- Confirmar os modos `Automatico`, `Desktop` e `Jogo`, incluindo fallback para Desktop quando Raw Input nao estiver
  disponivel.
- Em modo Automatico, testar jogos em tela cheia e em janela com cursor oculto, incluindo pausas longas sem movimento.
- Confirmar que navegador, apresentacao, RDP e maquina virtual nao ativam o modo Jogo indevidamente.
- Em modo Jogo, validar sensibilidade entre `0.1` e `5.0` e wrap horizontal, vertical e nos quatro cantos.
- Confirmar que cursor, trilha e cliques sao recortados nos limites da fonte durante o wrap, sem vazamento na cena.
- Testar perda de foco, menus com cursor visivel e retorno ao jogo sem saltos ou movimento acumulado.

## Validacao tecnica

- Medir se `video_tick` e `video_render` continuam sem alocacoes no caminho normal.
- Verificar uso de CPU com a fonte ociosa e durante movimentos rapidos.
- Testar adicionar, duplicar, reconfigurar e remover varias fontes de mouse.
- Testar fechamento do OBS e troca repetida de cenas.
- Testar carregamento e fechamento do OBS quando outro plugin ja possui o registro Raw Input do mouse.
- Confirmar que varias fontes compartilham os mesmos totais relativos sem consumir movimento umas das outras.
- Verificar logs por imagens, efeitos ou recursos nao liberados.

## Criterios de conclusao

- Cursor, trilha e cliques mantem o comportamento esperado em todos os limites.
- A trilha usa distancia acumulada sem back-dating e sem saltos entre monitores.
- Eventos de clique permanecem na posicao de origem.
- Nao existem erros novos no log do OBS.
- Desempenho permanece adequado com duas ou mais fontes de mouse.
