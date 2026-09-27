# Especificação Técnica: Suporte a Linux Nativo e Flatpak

**Projeto:** Input Overlay GG  
**Data:** 2026-09-27  
**Status:** Aprovado  
**Alvo:** OBS Studio (Linux nativo e Flatpak)  

---

## 1. Contexto e Objetivo

O [Input Overlay GG](https://github.com/Guigumi/obs-plugin-gg) é um plugin para OBS Studio que renderiza overlays dinâmicos de teclado e mouse diretamente como fontes de vídeo. Atualmente, a captura de eventos de hardware utiliza APIs exclusivas do Windows (Raw Input e Win32).

O objetivo desta especificação é definir a implementação da versão Linux (ambientes nativos e empacotamento Flatpak), mantendo compatibilidade total com o pipeline de renderização existente e preservando as garantias de desempenho e integridade de eventos (detecção sem perda de pressionamentos rápidos entre quadros de vídeo).

---

## 2. Arquitetura e Estrutura de Arquivos

A interface de captura é desacoplada da renderização pelos cabeçalhos `src/keyboard-capture.h` e `src/mouse-capture.h`. No Linux, a implementação Win32 é substituída por módulos equivalentes baseados no subsistema `evdev` e XCB:

```
src/
├── platform-linux-input.h      # Gerenciamento de libudev, epoll e ciclo de vida
├── platform-linux-input.c      # Thread de polling unificada para teclado e mouse
├── keyboard-capture-linux.c    # Implementação da API keyboard-capture.h
├── mouse-capture-linux.c       # Implementação da API mouse-capture.h
tests/
└── test_keyboard_mapping_linux.c # Teste da matriz de conversão EV_KEY -> enum
```

### 2.1 Modelo de Threading e Concorrência

1. **Thread dedicada de polling:** Uma única thread de background criada na inicialização do plugin atende simultaneamente aos eventos de teclado e mouse via `epoll`.
2. **Descritores no `epoll`:**
   * Monitor `udev` via netlink para detecção em tempo real de conexões e desconexões (hotplug).
   * Arquivos descritores de `/dev/input/event*` correspondentes a teclados e mouses ativos.
   * `eventfd` de sinalização interna para encerramento imediato e síncrono durante `keyboard_capture_shutdown()` ou `mouse_capture_shutdown()`.
3. **Sincronização:** Estados compartilhados (`snapshot`, contadores relativos e sequências de cliques) são protegidos por `pthread_mutex_t` com tempo de bloqueio mínimo.

---

## 3. Detecção de Dispositivos e Hotplug (`platform-linux-input`)

Para evitar dependência de caminhos fixos ou conflito com interfaces virtuais, o gerenciamento de dispositivos utiliza `libudev`:

* **Filtros de dispositivo:**
  * Teclados: subsistema `input`, propriedade `ID_INPUT_KEYBOARD=1`, com validação adicional via `libevdev_has_event_type(dev, EV_KEY)` e checagem de suporte a teclas alfanuméricas padrão (`KEY_A` a `KEY_Z`).
  * Mouses: subsistema `input`, propriedade `ID_INPUT_MOUSE=1`, com checagem de `EV_REL` (`REL_X`, `REL_Y`) e `EV_KEY` (`BTN_LEFT`, `BTN_RIGHT`).
* **Hotplug dinâmico:**
  * O monitor `udev` recebe eventos `add` e `remove`.
  * Novos periféricos conectados são validados, configurados no `libevdev` e adicionados ao laço do `epoll`.
  * Periféricos removidos têm seus descritores fechados e desvinculados do `epoll` sem interromper a captura dos demais dispositivos.

---

## 4. Captura de Teclado (`keyboard-capture-linux.c`)

### 4.1 Mapeamento de Códigos de Eventos
Uma tabela de conversão estática traduz os códigos `KEY_*` definidos em `<linux/input-event-codes.h>` para o `enum keyboard_overlay_key`:

* **Alfanuméricos:** `KEY_A` a `KEY_Z`, `KEY_0` a `KEY_9`.
* **Controles e Navegação:** `KEY_SPACE`, `KEY_TAB`, `KEY_CAPSLOCK`, `KEY_BACKSPACE`, `KEY_ENTER`, `KEY_ESC`, `KEY_INSERT`, `KEY_DELETE`, `KEY_HOME`, `KEY_END`, `KEY_PAGEUP`, `KEY_PAGEDOWN`, `KEY_UP`, `KEY_LEFT`, `KEY_DOWN`, `KEY_RIGHT`.
* **Modificadores:** `KEY_LEFTSHIFT`, `KEY_RIGHTSHIFT`, `KEY_LEFTCTRL`, `KEY_RIGHTCTRL`, `KEY_LEFTALT`, `KEY_RIGHTALT`, `KEY_LEFTMETA`, `KEY_RIGHTMETA`.
* **Teclado Numérico (Numpad):** `KEY_KP0` a `KEY_KP9`, `KEY_KPSLASH`, `KEY_KPASTERISK`, `KEY_KPMINUS`, `KEY_KPPLUS`, `KEY_KPDOT`, `KEY_KPENTER`.
* **Teclas do Sistema:** `KEY_SYSRQ` (Print Screen), `KEY_SCROLLLOCK`, `KEY_PAUSE`, `KEY_GRAVE`.
* **Enter unificado:** Seguindo o padrão de compatibilidade estabelecido na v1.1.0, o Enter do numpad e o Enter convencional alimentam o mesmo identificador lógico no overlay.

### 4.2 Múltiplos Dispositivos e Sequências Monotônicas
* Para suportar múltiplos teclados simultâneos sem inconsistências, cada tecla possui um contador de referências ativas (`key_down_count[KEYBOARD_KEY_COUNT]`). O estado `pressed[key]` permanece verdadeiro enquanto o contador for maior que zero.
* A cada evento de pressionamento (`value == 1`), `press_sequences[key]` é incrementado atomicamente, garantindo que ativações rápidas ocorridas entre dois ciclos de renderização (`video_tick`) sejam detectadas.
* Quando o parâmetro `arrow_aliases` for verdadeiro para layouts como WASD, os eventos de setas direcionais acionam paralelamente as teclas correspondentes (`KEYBOARD_KEY_W`, `KEYBOARD_KEY_A`, `KEYBOARD_KEY_S`, `KEYBOARD_KEY_D`).

---

## 5. Captura de Mouse, Telas e Cursor (`mouse-capture-linux.c`)

### 5.1 Cliques e Movimento Relativo
* **Botões:** Eventos de `BTN_LEFT` e `BTN_RIGHT` controlam os estados instantâneos `left_down` e `right_down`. Cada transição para pressionado incrementa a sequência correspondente (`left_click_sequence`, `right_click_sequence`).
* **Movimento relativo:** Eventos `REL_X` e `REL_Y` alimentam contadores acumuladores globais (`int64_t relative_total_x` e `relative_total_y`), utilizados pelo overlay para desenhar a trilha e medir velocidade.

### 5.2 Enumeração de Monitores via XCB / RandR
* Conexão X11 inicializada com `xcb_connect()`.
* Através da extensão `xcb_randr`, o plugin consulta telas e CRTCs ativos para listar:
  * Coordenadas absolutas na tela virtual (`x`, `y`, `width`, `height`).
  * Nome do conector físico da saída de vídeo (ex.: `DP-1`, `HDMI-1`, `eDP-1`).
* Em sessões Wayland puras (sem conexão X11 disponível), o plugin expõe uma única tela virtual representativa.

### 5.3 Posição Absoluta do Cursor e Game Mode
* **Ambiente X11:**
  * **Posição:** Coordenadas absolutas obtidas periodicamente com `xcb_query_pointer()`. As coordenadas são normalizadas para a faixa `[0.0, 1.0]` com base no monitor selecionado na interface do OBS.
  * **Game Mode:** Consulta periódica ao átomo `_NET_ACTIVE_WINDOW` na janela raiz. Se a janela com foco apresentar o átomo `_NET_WM_STATE_FULLSCREEN` em `_NET_WM_STATE`, o modo de jogo é sinalizado como ativo.
* **Ambiente Wayland (Fallback Gracioso):**
  * **Posição:** Como o protocolo de segurança do Wayland bloqueia consultas de ponteiro global por janelas comuns, o plugin sintetiza a posição do cursor acumulando os eventos relativos `REL_X` e `REL_Y` com contenção (clamp) nas bordas da área de trabalho virtual.
  * **Game Mode:** Retorna sempre visível por padrão, assegurando que o overlay não fique oculto durante o jogo.

---

## 6. Permissões e Diagnóstico

* Ao tentar abrir qualquer nó `/dev/input/event*`:
  * Em caso de falha com `EACCES` ou `EPERM`:
    * Registra um aviso no log do OBS (`LOG_WARNING`):
      `[obs-plugin-gg] Permissão negada para /dev/input/event*. O usuário precisa estar no grupo 'input' (ex.: 'sudo usermod -aG input $USER') ou ter uma regra udev apropriada configurada.`
    * O módulo desativa a leitura daquele periférico sem gerar falhas de segmentação ou tentativas em loop de alta CPU.
  * Ao detectar execução dentro de sandbox Flatpak, exibe no log a orientação para uso de permissão de dispositivo.

---

## 7. Sistema de Build e Empacotamento

### 7.1 CMakeLists.txt
No arquivo raiz `CMakeLists.txt`, o suporte a Linux utiliza `PkgConfig` para localizar as dependências e alternar os arquivos-fonte da plataforma:

```cmake
if(UNIX AND NOT APPLE)
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(LIBEVDEV REQUIRED IMPORTED_TARGET libevdev)
  pkg_check_modules(LIBUDEV REQUIRED IMPORTED_TARGET libudev)
  pkg_check_modules(XCB REQUIRED IMPORTED_TARGET xcb xcb-randr)

  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE
    PkgConfig::LIBEVDEV
    PkgConfig::LIBUDEV
    PkgConfig::XCB
    pthread
  )
endif()
```

As fontes do projeto são compiladas condicionalmente:
* Windows: `src/keyboard-capture-win32.c`, `src/mouse-capture-win32.c`.
* Linux: `src/platform-linux-input.c`, `src/keyboard-capture-linux.c`, `src/mouse-capture-linux.c`.

### 7.2 Empacotamento Flatpak
* **Manifest:** `com.obsproject.Studio.Plugin.InputOverlayGG.json`, declarando-se como extensão de `com.obsproject.Studio`.
* **Permissões do sandbox:**
  * `--device=input` (acesso aos nós `/dev/input/event*`).
  * `--socket=x11`, `--socket=wayland`, `--share=ipc`.

---

## 8. Testes Automatizados e CI

1. **Testes Unitários:**
   * Os testes atuais em `tests/` (`test_keyboard_layout`, `test_keyboard_keys`, `test_mouse_trail`, `test_mouse_click`) continuam operando via `test_stubs.c`.
   * Criação de `tests/test_keyboard_mapping_linux.c`: validação exaustiva do mapa estático de teclas para verificar se todos os códigos de tecla esperados convertem para o enum correspondente sem sobreposições ou lacunas.
2. **Integração Contínua (GitHub Actions):**
   * Adição de job `build-linux` no workflow `.github/workflows/build.yml` executando em `ubuntu-latest`.
   * Instalação de pacotes `libobs-dev`, `libevdev-dev`, `libudev-dev`, `libxcb1-dev`, `libxcb-randr0-dev`.
   * Compilação com `RelWithDebInfo` e execução da suíte `ctest`.

---

## 9. Critérios de Aceite

* [ ] Compilação limpa no Linux com GCC e Clang sem avisos adicionais.
* [ ] Detecção e resposta a todas as teclas do layout 100% e WASD via `evdev`.
* [ ] Suporte a hotplug verificado ao desconectar e reconectar periféricos USB.
* [ ] Mouse captura cliques esquerdo e direito e trilha de movimento em X11 e Wayland.
* [ ] Nomes reais de telas exibidos nas propriedades do mouse em sessões X11 com RandR.
* [ ] Fallback de permissão com aviso informativo no log do OBS sem travamento ou crash.
* [ ] Todos os testes unitários passando no Linux.
