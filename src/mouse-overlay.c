/*
Mouse Overlay GG
Copyright (C) 2026 GUI

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.
*/

#include <obs-module.h>
#include <util/platform.h>

#include "mouse-capture.h"
#include "mouse-overlay.h"
#include "mouse-overlay-click.h"
#include "mouse-overlay-cursor.h"
#include "mouse-overlay-internal.h"
#include "mouse-overlay-resources.h"
#include "mouse-overlay-trail.h"

#include <stdio.h>

static const char *mouse_overlay_get_name(void *type_data)
{
	UNUSED_PARAMETER(type_data);
	return obs_module_text("SourceName");
}

static const char *mouse_overlay_get_dark_icon(void *type_data)
{
	UNUSED_PARAMETER(type_data);
	return obs_module_file("icons/mouse-dark.svg");
}

static const char *mouse_overlay_get_light_icon(void *type_data)
{
	UNUSED_PARAMETER(type_data);
	return obs_module_file("icons/mouse-light.svg");
}

static void mouse_overlay_get_defaults(obs_data_t *settings)
{
	mouse_cursor_defaults(settings);
	mouse_trail_defaults(settings);
	mouse_click_defaults(settings);
}

static void mouse_overlay_add_monitor_property(obs_properties_t *props)
{
	obs_property_t *monitor = obs_properties_add_list(props, "monitor_index", obs_module_text("Monitor"),
							  OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(monitor, obs_module_text("MonitorAll"), MOUSE_CAPTURE_MONITOR_ALL);
	const size_t monitor_count = mouse_capture_get_monitor_count();
	for (size_t i = 0; i < monitor_count; i++) {
		char monitor_name[MOUSE_CAPTURE_MONITOR_NAME_MAX];
		char monitor_label[MOUSE_CAPTURE_MONITOR_NAME_MAX + 16];
		if (!mouse_capture_get_monitor_name(i, monitor_name, sizeof(monitor_name)))
			continue;
		snprintf(monitor_label, sizeof(monitor_label), "%zu: %s", i + 1, monitor_name);
		obs_property_list_add_int(monitor, monitor_label, (long long)i);
	}
}

static obs_properties_t *mouse_overlay_get_properties(void *data)
{
	obs_properties_t *props = obs_properties_create();
	mouse_overlay_add_monitor_property(props);

	obs_properties_t *cursor = obs_properties_create();
	obs_properties_add_group(props, "cursor_enabled", obs_module_text("Cursor"), OBS_GROUP_CHECKABLE, cursor);
	mouse_cursor_add_properties(cursor, data ? &((struct mouse_overlay_gg_data *)data)->cursor : NULL);

	obs_properties_t *trail = obs_properties_create();
	obs_properties_add_group(props, "trail_enabled", obs_module_text("Trail"), OBS_GROUP_CHECKABLE, trail);
	mouse_trail_add_properties(trail);

	obs_properties_t *click = obs_properties_create();
	obs_properties_add_group(props, "click_enabled", obs_module_text("Clicks"), OBS_GROUP_CHECKABLE, click);
	mouse_click_add_properties(click);

	obs_properties_add_button2(props, "reload_images", obs_module_text("ReloadImages"),
				   mouse_resources_reload_images, data);
	return props;
}

static void mouse_overlay_update(void *context, obs_data_t *settings)
{
	struct mouse_overlay_gg_data *data = context;
	mouse_cursor_update(&data->cursor, settings);
	mouse_trail_update(&data->trail, settings);
	mouse_click_update(&data->click, settings);
}

static void *mouse_overlay_create(obs_data_t *settings, obs_source_t *source)
{
	struct mouse_overlay_gg_data *data = bzalloc(sizeof(*data));
	data->source = source;
	mouse_overlay_update(data, settings);
	mouse_click_sync_sequences(&data->click);
	mouse_resources_init(&data->resources);
	return data;
}

static void mouse_overlay_destroy(void *context)
{
	struct mouse_overlay_gg_data *data = context;
	if (!data)
		return;
	obs_enter_graphics();
	mouse_resources_free(&data->resources);
	obs_leave_graphics();
	bfree(data);
}

static uint32_t mouse_overlay_get_width(void *data)
{
	UNUSED_PARAMETER(data);
	return MOUSE_OVERLAY_WIDTH;
}

static uint32_t mouse_overlay_get_height(void *data)
{
	UNUSED_PARAMETER(data);
	return MOUSE_OVERLAY_HEIGHT;
}

static void mouse_overlay_video_tick(void *context, float seconds)
{
	UNUSED_PARAMETER(seconds);
	struct mouse_overlay_gg_data *data = context;
	if (!data->cursor.enabled && !data->trail.enabled && !data->click.enabled) {
		data->cursor.visible = false;
		data->cursor.relative_initialized = false;
		data->cursor.wrapped = false;
		mouse_trail_reset(&data->trail);
		mouse_click_reset(&data->click);
		mouse_click_sync_sequences(&data->click);
		data->click.left_down = false;
		data->click.right_down = false;
		return;
	}

	const int previous_mode = data->cursor.active_mode;
	const bool tracking_initialized = data->cursor.tracking_initialized;
	if (mouse_cursor_tick(&data->cursor)) {
		const bool mode_changed = tracking_initialized && previous_mode != data->cursor.active_mode;
		if (mode_changed)
			mouse_trail_reset(&data->trail);
		else
			mouse_trail_reset_continuity(&data->trail);
		if (!tracking_initialized || mode_changed)
			obs_source_update_properties(data->source);
		mouse_click_reset(&data->click);
	}
	if (data->cursor.wrapped)
		mouse_trail_reset_continuity(&data->trail);

	const uint64_t now_ns = os_gettime_ns();
	mouse_click_tick(&data->click, &data->cursor, now_ns);
	mouse_trail_tick(&data->trail, &data->cursor, now_ns);
}

static void mouse_overlay_video_render(void *context, gs_effect_t *effect)
{
	UNUSED_PARAMETER(effect);
	struct mouse_overlay_gg_data *data = context;
	mouse_resources_prepare(&data->resources);

	const bool draw_trail = mouse_trail_can_render(&data->trail, &data->resources);
	const bool draw_cursor = mouse_cursor_can_render(&data->cursor, &data->resources);
	const bool draw_clicks = mouse_click_can_render(&data->click, &data->cursor, &data->resources);
	if (!draw_trail && !draw_cursor && !draw_clicks)
		return;

	const uint64_t now_ns = os_gettime_ns();
	gs_blend_state_push();
	gs_blend_function(GS_BLEND_ONE, GS_BLEND_INVSRCALPHA);
	mouse_trail_render(&data->trail, &data->cursor, &data->resources, now_ns);
	mouse_cursor_render(&data->cursor, &data->resources);
	mouse_click_render(&data->click, &data->cursor, &data->resources, now_ns);
	mouse_resources_clear_textures(&data->resources);
	gs_blend_state_pop();
}

struct obs_source_info mouse_overlay_gg_source_info = {
	.id = "mouse_overlay_gg",
	.type = OBS_SOURCE_TYPE_INPUT,
	.output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_CUSTOM_DRAW | OBS_SOURCE_SRGB,
	.get_name = mouse_overlay_get_name,
	.create = mouse_overlay_create,
	.destroy = mouse_overlay_destroy,
	.update = mouse_overlay_update,
	.get_defaults = mouse_overlay_get_defaults,
	.get_properties = mouse_overlay_get_properties,
	.get_width = mouse_overlay_get_width,
	.get_height = mouse_overlay_get_height,
	.video_tick = mouse_overlay_video_tick,
	.video_render = mouse_overlay_video_render,
	.icon_type = OBS_ICON_TYPE_CUSTOM,
	.get_dark_icon = mouse_overlay_get_dark_icon,
	.get_light_icon = mouse_overlay_get_light_icon,
};
