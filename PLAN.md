# Keyboard Overlay GG - Plano de desenvolvimento

## Objetivo

Preparar uma primeira versão estável, simples de configurar e com bom desempenho. Depois do lançamento, novas funções
e opções de personalização serão adicionadas conforme a necessidade e o impacto que causarem no desempenho.

## Escopo do lançamento inicial

- Quatro layouts: `W A S D`, `Teclado 100%`, `Área de edição` e `Teclado numérico`.
- Opções de teclas separadas por layout.
- Tamanho, espaçamento, rotação, opacidades, cores e fonte compartilhados.
- Teclas normais, largas e verticais usando os assets atuais.
- Captura com Raw Input em uma thread dedicada, usando `GetAsyncKeyState` como fallback.
- Feedback atual por cor, círculo e borda, sem criar novas opções de animação nesta etapa.
- Validação manual no OBS antes da publicação.

## Prioridades

### 1. Encontrar e corrigir problemas

Esta etapa bloqueia o lançamento. Antes de publicar, precisamos confirmar que os layouts e a captura funcionam de forma
previsível no uso real.

- Testar visualmente os quatro layouts com diferentes tamanhos e espaçamentos.
- Corrigir teclas ausentes, duplicadas, sobrepostas ou posicionadas fora da fonte.
- Confirmar que cada layout exibe apenas suas próprias teclas e opções.
- Testar a troca de layout sem deixar teclas presas ou reiniciar animações de forma incorreta.
- Testar pressionamentos rápidos, teclas mantidas e várias teclas ao mesmo tempo.
- Confirmar as teclas modificadoras, o numpad e `Print`, `Scroll` e `Pause`.
- Validar os aliases de setas no layout `W A S D`.
- Testar com o OBS em foco e fora de foco, em tela cheia e em janela sem borda.
- Testar habilitar, desabilitar, duplicar, reconfigurar e remover fontes.
- Verificar assets, locales, fontes ausentes, textos vazios, textos longos e Unicode.
- Investigar qualquer crash no carregamento, na troca de cena ou no encerramento do OBS.

### 2. Melhorar o desempenho

Depois de corrigir os problemas funcionais, vamos medir o custo do plugin e remover o que for desnecessário antes do
lançamento.

- Medir o uso de CPU em repouso e com todas as teclas animando.
- Medir o custo de `video_tick` e `video_render`.
- Confirmar que o caminho normal não faz alocações recorrentes.
- Recriar texturas de rótulo somente quando fonte, texto ou tamanho realmente mudarem.
- Reduzir chamadas GDI e operações gráficas repetidas.
- Manter o custo proporcional ao número de teclas visíveis.
- Testar duas ou mais fontes com layouts diferentes.
- Confirmar que a captura não interfere no Raw Input do mouse ou em outros plugins.
- Testar várias inicializações e encerramentos do OBS.
- Revisar os logs em busca de erros de textura, shader, fonte, thread, handle ou recurso não liberado.

### 3. Decidir o equilíbrio entre desempenho e personalização

Após medir o comportamento do plugin, vamos decidir quais recursos valem o custo de configuração e renderização.

- Priorizar opções aplicadas quando a fonte é atualizada, não durante `video_tick` ou `video_render`.
- Avaliar cor individual por tecla somente se a interface continuar simples e o custo permanecer baixo.
- Avaliar controles detalhados de fade, pulso e escala depois de medir o impacto.
- Avaliar sombra, contorno e outros efeitos de rótulo conforme o custo de renderização.
- Evitar opções que deixem a configuração extensa sem resolver uma necessidade real.
- Registrar as decisões de compatibilidade e desempenho antes de adicionar cada nova opção.

## Estado atual

- A estrutura foi separada em módulos.
- A captura Raw Input, o fallback e o compartilhamento de estado estão implementados.
- O Enter normal e o Enter do teclado numérico compartilham o mesmo input por decisão de compatibilidade.
- Os quatro layouts e as propriedades dinâmicas estão implementados.
- Teclas normais, horizontais, barra de espaço e teclas verticais estão implementadas.
- Texturas de rótulo são reutilizadas quando texto, fonte, tamanho e dimensões não mudam.
- O build `RelWithDebInfo` e os testes automatizados estão passando.
- A validação visual e funcional no OBS ainda está pendente e bloqueia o lançamento.

## Pós-lançamento

### Aparência e animação

- Avaliar a substituição dos assets por teclas desenhadas de forma procedural, com cantos arredondados.
- Adicionar uma pequena animação de escala ao pressionar a tecla, sem alterar o tamanho do rótulo.
- Reavaliar os efeitos de cor, círculo, borda e suas combinações.
- Adicionar sombra ou contorno configurável aos rótulos.

### Personalização

- Avaliar cores individuais por tecla.
- Avaliar configurações detalhadas de feedback, fade, pulso e escala.
- Avaliar novos presets somente depois de validar os quatro layouts iniciais.

### Dados e recursos avançados

- Avaliar importação e exportação de layouts em JSON.
- Avaliar configurações persistentes por layout quando houver uma necessidade real.
- Reavaliar a edição individual de posição e tamanho depois de medir a complexidade que isso traria para a interface.

## Critérios para o lançamento

- Os quatro layouts exibem corretamente todas as teclas previstas.
- Cada layout exibe apenas suas próprias opções de tecla.
- Nenhuma tecla fica presa durante o uso normal ou a troca de layout.
- Rótulos e efeitos permanecem dentro dos limites da fonte.
- Não existem crashes conhecidos, erros novos no log ou recursos sem liberação.
- O uso de CPU e memória permanece adequado com uma ou mais fontes.
- DLL, assets e locales são instalados corretamente nas pastas do OBS.
- Build `RelWithDebInfo`, testes automatizados e validação manual foram executados.

## Validação técnica

- Executar os quatro testes de teclado e mouse diretamente quando `ctest` não estiver disponível.
- Executar `git diff --check` antes da instalação.
- Comparar o hash da DLL compilada com a DLL em `bin\64bit` do OBS.
- Sincronizar os locales em `locale` e `data/locale`.
- Fechar o OBS antes de substituir a DLL.
