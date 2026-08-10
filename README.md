# Input Overlay GG

Mouse and keyboard sources for [OBS Studio](https://obsproject.com), made for gameplay streams, tutorials and other
videos where showing the pressed buttons is useful.

## What it does

The plugin adds two sources:

- **Mouse** shows the cursor (always, or only while in a game), a trail behind it, and a highlight effect on clicks. You pick the monitor, then set the size, opacity, and color of each piece.
- **Keyboard** is an on-screen keyboard that lights up as you type. It includes `W A S D`, `Keyboard 100%`,
  `Editing area` and `Numeric keyboard` layouts.

Neither is a filter. You add them as sources, the same way you'd add a browser or an image.

## Installation

1. Install OBS Studio from <https://obsproject.com> if you don't have it yet.
2. Download `Input_overlay_gg_v1.0.0_Setup.exe` from the [releases](https://github.com/Guigumi/obs-plugin-gg/releases) page.
3. Close OBS Studio and run the installer as administrator.
4. Start OBS again after the installation finishes.

OBS is the only dependency. No extra programs or drivers required.

## Quick start

**Adding the mouse:**

1. In **Sources**, click `+` and pick **Mouse**.
2. Give it a name and confirm. The source shows up in your scene.
3. In the properties, use the **Cursor**, **Trail**, and **Clicks** checkboxes to turn each part on or off.
4. Move the source above your game capture in the scene list so the overlay sits on top.

**Adding the keyboard:**

1. Click `+` in **Sources** and pick **Keyboard**.
2. The **W A S D** layout is already set up: the four keys appear dimmed and light up as you press them.
3. Choose another layout to show the main keyboard, the editing area or the numeric keyboard.
4. Use the layout-specific `Key: ...` fields to change the displayed labels.
5. Resize and move the source freely; it only decides where the keyboard sits on screen.

When you're starting out, just adjust **Key Size** and **Spacing**. Everything else has a default that works fine in most scenes.

## Going further

The steps above are enough to get you started. For Game Mode, the custom layout, capture details, building from source, and other advanced settings, check out the [wiki](https://github.com/Guigumi/obs-plugin-gg/wiki).

Found a bug or have an idea? Open an issue on the project page.

## License

Licensed under the [GPL-2.0](LICENSE). You're free to copy, modify, and distribute it, as long as you keep the same license and copyright notices.
