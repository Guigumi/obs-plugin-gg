# OBS Plugin GG - Plano de implementacao

## Objetivo

Criar um plugin extremamente leve para OBS Studio que exiba entradas do mouse em uma fonte de overlay. O primeiro MVP sera exclusivo para Windows e mostrara a posicao do cursor e o estado dos botoes, com consumo minimo de CPU e memoria.

O plugin sera desenvolvido de forma independente do repositorio principal do OBS, facilitando manutencao, compilacao e distribuicao.

## Escopo do MVP

- Fonte de video transparente adicionada pelo usuario a uma cena do OBS.
- Captura global do mouse no Windows, mesmo quando o OBS nao estiver em foco.
- Posicao atual do cursor.
- Indicacao visual dos botoes esquerdo, direito e central.
- Cor e tamanho do cursor configuraveis.
- Cor ou efeito de destaque configuravel durante cliques.
- Opcao futura para usar uma imagem PNG personalizada.
- Sem captura de teclado nesta primeira versao.
- Sem texto e, portanto, sem dependencia inicial do FreeType2.

## Arquitetura

### Plugin independente

O projeto deve usar o template e as dependencias oficiais para plugins externos do OBS, em vez de ser inserido diretamente na arvore de fontes do OBS Studio.

Nome do plugin:

```text
input-overlay-gg
```

ID da fonte:

```text
input_overlay_gg_mouse
```

### Captura de entrada

A captura sera feita uma vez por `video_tick`, usando a API Win32:

- `GetCursorPos` para obter a posicao global do cursor.
- `GetAsyncKeyState(VK_LBUTTON)` para o botao esquerdo.
- `GetAsyncKeyState(VK_RBUTTON)` para o botao direito.
- `GetAsyncKeyState(VK_MBUTTON)` para o botao central.

Nao sera usado `SetWindowsHookEx`. O MVP tambem nao criara threads, timers ou filas proprias. Isso reduz complexidade, sincronizacao e custo em segundo plano.

### Fonte OBS

A fonte usara:

```c
OBS_SOURCE_VIDEO | OBS_SOURCE_CUSTOM_DRAW
```

Callbacks previstos:

- `get_name`
- `create`
- `destroy`
- `get_width`
- `get_height`
- `get_defaults`
- `get_properties`
- `update`
- `video_tick`
- `video_render`

### Renderizacao

- Renderizar diretamente com a API grafica do OBS.
- Usar `gs_draw_sprite` para texturas e elementos do overlay.
- Manter texturas e outros recursos em cache.
- Nao alocar memoria a cada frame.
- Retornar cedo quando nao houver nada visivel para atualizar ou desenhar.
- Respeitar o contexto grafico do OBS ao criar ou destruir recursos de GPU.
- Manter fundo transparente para composicao sobre qualquer fonte.

## Estrutura inicial

```text
obs-plugin-gg/
|-- CMakeLists.txt
|-- CMakePresets.json
|-- cmake/
|-- data/
|   `-- locale/
|       |-- en-US.ini
|       `-- pt-BR.ini
|-- src/
|   |-- input-overlay-gg.c
|   |-- mouse-capture-win.c
|   `-- mouse-capture.h
`-- PLAN.md
```

A estrutura exata pode ser ajustada ao template oficial vigente do OBS no momento da implementacao.

## Etapas

### 1. Preparar o projeto

- Inicializar o repositorio Git.
- Adotar o template oficial de plugin do OBS.
- Configurar CMake para Windows x64.
- Vincular `OBS::libobs` e `user32`.
- Confirmar que um modulo vazio carrega no OBS sem erros.

**Observacao (validada no OBS 32.2.1 Windows):** plugins de usuario
sao escaneados de `%ProgramData%\obs-studio\plugins\<modulo>` (via
`GetProgramDataPath`/`CSIDL_COMMON_APPDATA`), nao de `%APPDATA%`. O
caminho binario e `...\bin\64bit\`, os dados de locale ficam em
`...\data\locale\`, e o OBS precisa ser iniciado com o diretorio de
trabalho em `C:\Program Files\obs-studio\bin\64bit` para localizar o
resto dos seus proprios dados.

**Recursos de GPU:** criar texturas em `create` falha (sem contexto
grafico ativo). Criar sob demanda no primeiro `video_render`. Ao chamar
`gs_texture_create` com dados embutidos, passe um ponteiro para a
variavel de dados (`const uint8_t *data = tex_data; ... &data`); passar
`(const uint8_t **)&array` faz o D3D11 ler os primeiros bytes como
endereco e falhar com `E_INVALIDARG` (0x80070057).

### 2. Registrar a fonte

- Implementar a entrada do modulo com `OBS_DECLARE_MODULE`.
- Registrar `input_overlay_gg_mouse` com `obs_register_source`.
- Criar e destruir o estado da fonte corretamente.
- Definir dimensoes configuraveis para o canvas transparente.

### 3. Capturar o mouse

- Isolar chamadas Win32 em `mouse-capture-win.c`.
- Ler posicao e botoes uma vez por `video_tick`.
- Converter coordenadas globais para o espaco visual escolhido pelo overlay.
- Guardar apenas o estado mais recente, sem historico ou fila de eventos.

### 4. Renderizar o overlay

- Desenhar o cursor com uma textura pequena ou forma simples.
- Aplicar destaque visual enquanto cada botao estiver pressionado.
- Evitar recriar texturas, efeitos ou buffers durante a renderizacao.
- Validar transparencia e escala em diferentes resolucoes.

### 5. Adicionar configuracoes

- Largura e altura da fonte.
- Tamanho do cursor.
- Cor normal.
- Cor de clique.
- Ativacao individual dos botoes esquerdo, direito e central.
- Opcionalmente, caminho para PNG personalizado depois que o desenho padrao estiver estavel.

### 6. Validar desempenho e estabilidade

- Medir uso de CPU com a fonte ociosa e em movimento.
- Confirmar ausencia de alocacoes continuas por frame.
- Confirmar que o plugin nao cria threads.
- Testar adicionar, remover, duplicar e reconfigurar a fonte repetidamente.
- Testar fechamento do OBS, troca de cena e perda de dispositivo grafico.
- Verificar logs do OBS quanto a erros e recursos nao liberados.

### 7. Empacotar

- Gerar artefatos para Windows x64.
- Incluir DLL, dados de locale e recursos visuais necessarios.
- Documentar instalacao manual e versoes do OBS suportadas.
- Adicionar licenca e atribuicoes das dependencias utilizadas.

## Metas de desempenho

- Nenhuma thread criada pelo plugin.
- Nenhuma alocacao de heap no caminho normal de cada frame.
- Uso de CPU ocioso desprezivel.
- Memoria propria do plugin abaixo de aproximadamente 2 MB, sem contar recursos compartilhados do OBS.
- Uma consulta de cursor e tres consultas de botoes por `video_tick`.

As metas devem ser confirmadas por medicao; nao devem ser tratadas como garantias antes dos testes.

## Evolucao posterior

Depois que o MVP do mouse estiver estavel:

1. Adicionar overlay de teclado com uma lista configuravel de teclas.
2. Avaliar captura por polling ou Raw Input conforme os requisitos reais.
3. Adicionar labels usando FreeType2 somente quando texto for necessario.
4. Criar temas e layouts reutilizaveis.
5. Adicionar animacoes curtas de clique sem manter trabalho ativo quando o overlay estiver ocioso.
6. Avaliar suporte a Linux e macOS com implementacoes de captura separadas por plataforma.

## Criterios de conclusao do MVP

- O plugin compila e carrega em uma versao suportada do OBS Studio para Windows x64.
- A fonte aparece na lista de fontes do OBS.
- O cursor acompanha a posicao global corretamente.
- Os tres botoes exibem feedback visual sem travamentos.
- As configuracoes persistem ao salvar e reabrir a cena.
- Adicionar e remover a fonte nao gera erros no log.
- O plugin permanece dentro das metas de desempenho apos medicao.
