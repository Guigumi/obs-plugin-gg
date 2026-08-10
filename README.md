# OBS Plugins GG

A pair of sources for [OBS Studio](https://obsproject.com) that show your mouse and keyboard. Made for gameplay streams, tutorial videos, or any broadcast where people like seeing which buttons you press.

## What it does

Two sources, each one on its own:

- **Mouse** shows the cursor (always, or only while in a game), a trail behind it, and a highlight effect on clicks. You pick the monitor, then set the size, opacity, and color of each piece.
- **Keyboard** is an on-screen keyboard that lights up as you type. It comes with WASD, ESDF, Arrows, and Numpad 1-5 presets, and a custom mode where you can change the label of any key.

Neither is a filter. You add them as sources, the same way you'd add a browser or an image.

## Installation

1. Install OBS Studio (a recent version) from <https://obsproject.com> if you don't have it yet.
2. Grab the plugin from the [releases](https://github.com/Guigumi/obs-plugin-gg/releases) page.
3. Extract the archive and copy the plugin folder into the `plugins/` directory of your OBS install.
4. Start OBS, or restart it if it's already open.

OBS is the only dependency. No extra programs or drivers required.

## Quick start

**Adding the mouse:**

1. In **Sources**, click `+` and pick **Mouse**.
2. Give it a name and confirm. The source shows up in your scene.
3. In the properties, use the **Cursor**, **Trail**, and **Clicks** checkboxes to turn each part on or off.
4. Move the source above your game capture in the scene list so the overlay sits on top.

**Adding the keyboard:**

1. Click `+` in **Sources** and pick **Keyboard**.
2. The **WASD** preset is already set up: the four keys appear dimmed and light up as you press them.
3. Under **Keyboard Keys**, you can change each key's text. For example, type `Space` for the space bar.
4. Resize and move the source freely; it only decides where the keyboard sits on screen.

When you're starting out, just adjust **Key Size** and **Spacing**. Everything else has a default that works fine in most scenes.

## Going further

The steps above are enough to get you started. For Game Mode, the custom layout, capture details, building from source, and other advanced settings, check out the [wiki](https://github.com/Guigumi/obs-plugin-gg/wiki).

Found a bug or have an idea? Open an issue on the project page.

## License

Licensed under the [GPL-2.0](LICENSE). You're free to copy, modify, and distribute it, as long as you keep the same license and copyright notices.
