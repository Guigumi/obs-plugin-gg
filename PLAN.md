# Mouse Overlay GG - Validacao pendente

## Objetivo atual

Validar visualmente a nova implementacao modular do mouse antes de retomar melhorias do teclado.

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
- Confirmar que um clique solto permanece na posicao em que ocorreu.
- Confirmar que a animacao continua em loop e acompanha o cursor enquanto o botao estiver segurado.
- Confirmar que clique segurado nao duplica a animacao e o indicador estatico.
- Testar cliques esquerdo e direito simultaneos.
- Testar habilitar e desabilitar cursor, trilha e cliques durante o uso.

## Validacao tecnica

- Medir se `video_tick` e `video_render` continuam sem alocacoes no caminho normal.
- Verificar uso de CPU com a fonte ociosa e durante movimentos rapidos.
- Testar adicionar, duplicar, reconfigurar e remover varias fontes de mouse.
- Testar fechamento do OBS e troca repetida de cenas.
- Verificar logs por imagens, efeitos ou recursos nao liberados.

## Criterios de conclusao

- Cursor, trilha e cliques mantem o comportamento esperado em todos os limites.
- A trilha usa distancia acumulada sem back-dating e sem saltos entre monitores.
- Eventos de clique permanecem na posicao de origem.
- Nao existem erros novos no log do OBS.
- Desempenho permanece adequado com duas ou mais fontes de mouse.
