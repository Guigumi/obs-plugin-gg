/*
 Mouse
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

#include "keyboard-overlay.h"

#include "keyboard-overlay-internal.h"
#include "keyboard-overlay-keys.h"
#include "keyboard-overlay-layout.h"
#include "keyboard-overlay-resources.h"

#include <obs-module.h>

#include <math.h>

static const char *keyboard_overlay_get_name(void *type_data)
{
	UNUSED_PARAMETER(type_data);
	return obs_module_text("KeyboardSourceName");
}

static const char *keyboard_overlay_get_dark_icon(void *type_data)
{
	UNUSED_PARAMETER(type_data);
	return obs_module_file("icons/keyboard-dark.svg");
}

static const char *keyboard_overlay_get_light_icon(void *type_data)
{
	UNUSED_PARAMETER(type_data);
	return obs_module_file("icons/keyboard-light.svg");
}

static void keyboard_overlay_get_defaults(obs_data_t *settings)
{
	keyboard_keys_defaults(settings);
}

static obs_properties_t *keyboard_overlay_get_properties(void *data)
{
	obs_properties_t *props = obs_properties_create();
	const struct keyboard_overlay_gg_data *keyboard = data;
	keyboard_keys_add_properties(props, keyboard ? keyboard->layout_preset : KEYBOARD_LAYOUT_WASD);
	return props;
}

static void keyboard_overlay_update(void *data, obs_data_t *settings)
{
	struct keyboard_overlay_gg_data *keyboard = data;
	keyboard_keys_update(keyboard, settings);
	keyboard_layout_update(keyboard);
	keyboard_resources_update_labels(keyboard, settings);
}

static void *keyboard_overlay_create(obs_data_t *settings, obs_source_t *source)
{
	UNUSED_PARAMETER(source);
	struct keyboard_overlay_gg_data *keyboard = bzalloc(sizeof(*keyboard));
	keyboard_keys_initialize(keyboard);
	keyboard_overlay_update(keyboard, settings);
	keyboard_resources_init(keyboard);
	return keyboard;
}

static void keyboard_overlay_destroy(void *data)
{
	struct keyboard_overlay_gg_data *keyboard = data;
	if (!keyboard)
		return;
	keyboard_resources_free(keyboard);
	bfree(keyboard);
}

static uint32_t keyboard_overlay_get_width(void *data)
{
	const struct keyboard_overlay_gg_data *keyboard = data;
	float width;
	float height;
	keyboard_layout_get_dimensions(keyboard, &width, &height);
	UNUSED_PARAMETER(height);
	return (uint32_t)ceilf(width);
}

static uint32_t keyboard_overlay_get_height(void *data)
{
	const struct keyboard_overlay_gg_data *keyboard = data;
	float width;
	float height;
	keyboard_layout_get_dimensions(keyboard, &width, &height);
	UNUSED_PARAMETER(width);
	return (uint32_t)ceilf(height);
}

static void keyboard_overlay_video_tick(void *data, float seconds)
{
	keyboard_keys_tick(data, seconds);
}

static void keyboard_overlay_video_render(void *data, gs_effect_t *effect)
{
	UNUSED_PARAMETER(effect);
	/* Capture events that arrived after video_tick before drawing this frame. */
	keyboard_keys_tick(data, 0.0f);
	keyboard_resources_render(data);
}

struct obs_source_info keyboard_overlay_gg_source_info = {
	.id = "keyboard_overlay_gg",
	.type = OBS_SOURCE_TYPE_INPUT,
	.output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_CUSTOM_DRAW | OBS_SOURCE_SRGB,
	.get_name = keyboard_overlay_get_name,
	.create = keyboard_overlay_create,
	.destroy = keyboard_overlay_destroy,
	.update = keyboard_overlay_update,
	.get_defaults = keyboard_overlay_get_defaults,
	.get_properties = keyboard_overlay_get_properties,
	.get_width = keyboard_overlay_get_width,
	.get_height = keyboard_overlay_get_height,
	.video_tick = keyboard_overlay_video_tick,
	.video_render = keyboard_overlay_video_render,
	.icon_type = OBS_ICON_TYPE_CUSTOM,
	.get_dark_icon = keyboard_overlay_get_dark_icon,
	.get_light_icon = keyboard_overlay_get_light_icon,
};
