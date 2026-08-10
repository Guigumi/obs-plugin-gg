/*
Mouse Overlay GG
Copyright (C) 2026 GUI

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program. If not, see <https://www.gnu.org/licenses/>
*/

#include <obs-module.h>
#include <plugin-support.h>

#include "keyboard-capture.h"
#include "keyboard-overlay.h"
#include "mouse-capture.h"
#include "mouse-overlay.h"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE(PLUGIN_NAME, "en-US")

bool obs_module_load(void)
{
	if (!mouse_capture_initialize())
		obs_log(LOG_WARNING, "Raw Input unavailable; game mode will not receive relative motion");
	if (!keyboard_capture_initialize())
		obs_log(LOG_WARNING, "Keyboard Raw Input unavailable; using GetAsyncKeyState fallback");
	obs_register_source(&mouse_overlay_gg_source_info);
	obs_register_source(&keyboard_overlay_gg_source_info);
	obs_log(LOG_INFO, "plugin loaded successfully (version %s)", PLUGIN_VERSION);
	return true;
}

void obs_module_unload(void)
{
	keyboard_capture_shutdown();
	mouse_capture_shutdown();
	obs_log(LOG_INFO, "plugin unloaded");
}
