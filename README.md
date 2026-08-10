# Mouse Overlay GG

A pair of sources for [OBS Studio](https://obsproject.com) that show your mouse and keyboard on screen the moment you use them. Built for gameplay streams, tutorial videos, and any broadcast where viewers like to see the buttons you press.

## What it does

Two sources, independent from each other:

- **Mouse Overlay GG** — the cursor on screen (always visible or game-only), a trail behind the mouse, and a highlight effect when you click. You can pick the monitor and tweak the size, opacity, and color of each part.
- **Keyboard** — an on-screen keyboard that lights up as you type. It ships with WASD, ESDF, Arrows, and Numpad 1–5 presets, plus a custom mode to change the label of each key.

Neither one is a filter. You add them as sources, the same way you'd add a browser or an image.

## Installation

1. Install OBS Studio (a recent version) from <https://obsproject.com> if you don't have it yet.
2. Grab the plugin from the [releases](https://github.com/Guigumi/obs-plugin-gg/releases) page.
3. Extract the archive and copy the plugin folder into the `plugins/` directory of your OBS install.
4. Start (or restart) OBS.

OBS is the only dependency. No extra programs or drivers required.

## Quick start

**Adding the mouse:**

1. In **Sources**, click `+` and pick **Mouse Overlay GG**.
2. Give it a name and confirm. The source shows up in your scene.
3. In the properties, tick and untick the **Cursor**, **Trail**, and **Clicks** checkboxes to turn each part on or off.
4. Move the source above your game capture in the scene list so the overlay sits on top.

**Adding the keyboard:**

1. Click `+` in **Sources** and pick **Keyboard**.
2. The **WASD** preset is already set up: the four keys appear dimmed and light up as you press them.
3. Under **Keyboard Keys**, you can change each key's text — for example, type `Space` for the space bar.
4. Resize and position the source freely; it only defines where the keyboard sits on screen.

When you're starting out, just touch **Key Size** and **Spacing**. Everything else has a default that works fine in most scenes.

## Going further

The basics above are enough to get going. For Game Mode, the custom layout, capture details, building from source, and more advanced settings, check out the [wiki](https://github.com/Guigumi/obs-plugin-gg/wiki).

Found a bug or have an idea? Open an issue on the project page.

## License

Licensed under the [GPL-2.0](LICENSE). You're free to copy, modify, and distribute it, as long as you keep the same license and copyright notices.