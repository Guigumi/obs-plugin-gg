# Plano: Versão Linux do Input Overlay GG

## Objetivo

Criar uma versão Linux do plugin funcionando no OBS Studio nativo (e no OBS de Flatpak), mantendo todas as
funcionalidades atuais: overlay de mouse (cursor, trilha, cliques) e overlay de teclado com os layouts existentes.

## Decisões já tomadas

- **Captura de input:** evdev (`/dev/input/event*`) via libevdev/libinput — funciona em X11 **e** Wayland.
  Requer o usuário no grupo `input` ou uma udev rule (documentar no README/wiki).
- **Distribuição:** pacote **Flatpak** (além do build manual).

## O que reaproveitamos sem mudança

- Todo o código de overlay (`keyboard-overlay*.c`, `mouse-overlay*.c`): só usa libobs, é portátil.
- As interfaces de captura já abstraem a plataforma: `keyboard-capture.h` e `mouse-capture.h`.
- Assets (`data/`), efeitos (`.effect`), testes unitários com stubs.

## Etapas

### 1. Build Linux básico (sem captura)

- [ ] Criar stubs `keyboard-capture-linux.c` e `mouse-capture-linux.c` implementando as interfaces com respostas vazias.
- [ ] Ajustar `CMakeLists.txt` para selecionar arquivos por plataforma (`-win32` vs `-linux`).
- [ ] Revisar `cmake/linux/defaults.cmake` e `compilerconfig.cmake` (já vêm do plugin template).
- [ ] Compilar contra o `libobs` do sistema e carregar o plugin no OBS — validar que os overlays abrem, mesmo sem input.
- [ ] Rodar a suíte de testes existente no Linux (os stubs de teste devem compilar direto).

### 2. Captura evdev (núcleo do trabalho)

Implementar `keyboard-capture-linux.c` e `mouse-capture-linux.c`:

- [ ] Enumerar dispositivos em `/dev/input/event*` com libevdev, filtrando teclados e mouses (`EV_KEY`, `BTN_*`).
- [ ] Thread dedicada lendo eventos com `poll()`/`epoll()`, análoga à thread Raw Input do Windows.
- [ ] Tabela de mapeamento: **EV code → `enum keyboard_overlay_key`** (a maioria dos KEY_* do Linux mapeia 1:1
      para os códigos já usados no enum).
- [ ] Estado compartilhado com press sequences monotônicas, preservando a semântica: um press completo entre
      ticks não pode ser perdido (igual ao Raw Input no Windows).
- [ ] Botões do mouse: `BTN_LEFT`/`BTN_RIGHT` → sequências de clique + estado `*_down`.
- [ ] Posição do cursor:
  - **Posição absoluta normalizada por monitor:** obter geometria dos monitores via XCB/RandR (sessões X11);
    em Wayland puro, consultar via portal (`org.freedesktop.portal`) ou fallback: virtual desktop completo.
  - Como alternativa mais simples na v1: rastrear posição acumulando movimentos relativos (`EV_REL`) — pode
    dar drift; documentar a limitação ou preferir posição real do compositor quando disponível.
- [ ] Movimento relativo: `EV_REL` / `REL_X`, `REL_Y` → `mouse_capture_get_relative_totals()`.
- [ ] Game mode (`mouse_capture_detect_game_mode`): versão Linux simplificada — detectar janela fullscreen
      via XCB/EWMH (`_NET_WM_STATE_FULLSCREEN`) em X11; em Wayland, retornar "indisponível" inicialmente.
- [ ] Permissões: detectar falta de acesso a `/dev/input` e logar mensagem clara (grupo `input`/udev rule).

### 3. Propriedades e UI

- [ ] Ocultar/adequar opções Windows-only (ex.: parâmetros de confinamento de cursor, se houver).
- [ ] Garantir que a lista de monitores (`mouse_capture_get_monitor_name`) mostre nomes úteis no Linux
      (nome do conector via RandR, ex.: `DP-1`, `HDMI-1`).

### 4. Empacotamento Flatpak

- [ ] Criar manifest `com.obsproject.Studio.Plugin.InputOverlayGG` (ou namespace próprio), seguindo o padrão
      dos plugins no Flathub.
- [ ] **Permissão crítica:** o sandbox do Flatpak bloqueia `/dev/input` por padrão — adicionar
      `--device=input` no manifest e documentar.
- [ ] Build no CI (GitHub Actions) com o runtime do OBS; testar instalação local: `flatpak install --user`.
- [ ] Submissão ao Flathub (após v1 estável) — opcional; distribuir o `.flatpak` nas releases antes disso.

### 5. CI e qualidade

- [ ] Job Linux no `.github/workflows/build.yml`: compilação + testes unitários no Ubuntu.
- [ ] Estender os testes atuais para cobrir a tabela de mapeamento EV code → enum (usando os stubs).
- [ ] clang-format/gersemi já configurados — garantir conformidade nos arquivos novos.

### 6. Validação manual e release

- [ ] Testar em: GNOME (Wayland), KDE Plasma (X11 e Wayland), e uma sessão X11 simples.
- [ ] Verificar: posição do cursor/multimonitor, trilha, cliques, todos os layouts do teclado, teclas
      modificadoras, numpad, game mode.
- [ ] Atualizar README (instruções Flatpak + grupo `input`) e wiki.
- [ ] Publicar release com `.flatpak` e tarball do build manual.

## Riscos conhecidos

| Risco | Mitigação |
|---|---|
| Permissão de `/dev/input` confunde usuários | Mensagem de log clara + wiki com udev rule pronta |
| Posição do cursor no Wayland puro | v1 aceita limitação (rastreamento relativo) ou usa portal; documentar |
| Flatpak sandbox bloqueia input global | `--device=input` no manifest + teste de instalação real |
| Game mode não detectável no Wayland | Fallback "sempre mostrar cursor" + nota no README |

## Ordem sugerida de entrega

**Etapa 1 → 2** são o núcleo (plugin funcional). **4** (Flatpak) pode começar em paralelo assim que o build
manual funcionar. **5 e 6** fecham a primeira release Linux.
