# Keyboard Overlay GG - Plano de desenvolvimento

## Objetivo

Preparar uma primeira versão estável, simples de configurar e com bom desempenho. Depois do lançamento, novas funções
e opções de personalização serão adicionadas conforme a necessidade e o impacto que causarem no desempenho.

## Escopo da versão atual

- Layouts `W A S D`, `Teclado 100%`, `Extras`, `OSU! mania`, `OSU!standard`, `CS2` e `Valorant`.
- Opções de teclas separadas por layout.
- Tamanho, espaçamento, rotação, opacidades, cores e fonte compartilhados.
- Teclas normais, largas e verticais usando os assets atuais.
- Captura com Raw Input em uma thread dedicada, usando `GetAsyncKeyState` como fallback.
- Aparência por imagem ou procedural e animações básica ou completa.
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

## Lançamento 1.1.0

- Distribuir Mouse e Teclado juntos no instalador `Input_overlay_gg_v1.1.0_Setup.exe`.
- Instalar a DLL em `bin\64bit` e os assets/locales nas estruturas usadas pelo OBS.
- Fechar o OBS antes de substituir arquivos em uso.
- Registrar um desinstalador no Windows para remover somente os arquivos do plugin.
- Gerar o instalador com `build-release.ps1` a partir de um build `RelWithDebInfo` validado.
- Atualizar README, versão do plugin, hash do instalador e notas da versão antes de publicar.
- Testar a instalação em uma pasta limpa e confirmar o carregamento das fontes Mouse e Teclado no OBS.

## Estado atual

- A estrutura foi separada em módulos.
- A captura Raw Input, o fallback e o compartilhamento de estado estão implementados.
- O Enter normal e o Enter do teclado numérico compartilham o mesmo input por decisão de compatibilidade.
- Os layouts e as propriedades dinâmicas estão implementados.
- Teclas normais, horizontais, barra de espaço e teclas verticais estão implementadas.
- Texturas de rótulo são reutilizadas quando texto, fonte, tamanho e dimensões não mudam.
- A tecla procedural e os estilos de animação estão implementados.
- O build `RelWithDebInfo` e os testes automatizados estão passando.
- A validação visual e funcional no OBS foi concluída sem erros de carregamento do plugin.

## Pós-lançamento

### Aparência e animação

- Validar o desenho procedural de teclas com diferentes tamanhos, proporções e cantos arredondados.
- Medir o impacto do desenho procedural na CPU, GPU, VRAM e tempo de inicialização.
- Manter a animação `Básica` como padrão, com mudança de cor e uma leve redução de escala.
- Manter `Ondulação`, `Pulso` e `Completa` como opções extras para quem aceitar maior custo de renderização.
- Reavaliar os passes de círculo e borda depois das medições.
- Não adicionar sombra ou contorno aos rótulos nesta etapa.

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
