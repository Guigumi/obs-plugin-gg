#include "mouse-overlay-cursor.h"

#include "mouse-capture.h"
#include "mouse-overlay-resources.h"

#define CURSOR_SIZE_DEFAULT 25.0f
#define CURSOR_SIZE_MIN 4.0f
#define CURSOR_SIZE_MAX 256.0f
#define CURSOR_OPACITY_DEFAULT_PCT 100

void mouse_cursor_defaults(obs_data_t *settings)
{
	obs_data_set_default_double(settings, "cursor_size", CURSOR_SIZE_DEFAULT);
	obs_data_set_default_int(settings, "cursor_opacity", CURSOR_OPACITY_DEFAULT_PCT);
	obs_data_set_default_int(settings, "cursor_tint_color", MOUSE_TINT_COLOR_DEFAULT);
	obs_data_set_default_bool(settings, "cursor_enabled", true);
	obs_data_set_default_int(settings, "monitor_index", MOUSE_CAPTURE_MONITOR_ALL);
}

void mouse_cursor_add_properties(obs_properties_t *props)
{
	obs_properties_add_int_slider(props, "cursor_size", obs_module_text("CursorSize"), (int)CURSOR_SIZE_MIN,
				      (int)CURSOR_SIZE_MAX, 1);
	obs_properties_add_int_slider(props, "cursor_opacity", obs_module_text("CursorOpacity"), 0, 100, 1);
	obs_properties_add_color(props, "cursor_tint_color", obs_module_text("CursorTintColor"));
}

void mouse_cursor_update(struct mouse_cursor_state *cursor, obs_data_t *settings)
{
	cursor->size = mouse_settings_get_float(settings, "cursor_size", CURSOR_SIZE_MIN, CURSOR_SIZE_MAX,
						CURSOR_SIZE_DEFAULT);
	cursor->opacity = mouse_settings_get_opacity(settings, "cursor_opacity", CURSOR_OPACITY_DEFAULT_PCT);
	cursor->tint_color = mouse_settings_get_color(settings, "cursor_tint_color");
	cursor->enabled = obs_data_get_bool(settings, "cursor_enabled");
	cursor->monitor_index = mouse_settings_get_int(settings, "monitor_index", MOUSE_CAPTURE_MONITOR_ALL, INT32_MAX,
						       MOUSE_CAPTURE_MONITOR_ALL);
}

static int mouse_cursor_valid_monitor_index(int monitor_index)
{
	if (monitor_index == MOUSE_CAPTURE_MONITOR_ALL)
		return monitor_index;
	const size_t monitor_count = mouse_capture_get_monitor_count();
	return monitor_index >= 0 && (size_t)monitor_index < monitor_count ? monitor_index : MOUSE_CAPTURE_MONITOR_ALL;
}

bool mouse_cursor_tick(struct mouse_cursor_state *cursor)
{
	const bool was_visible = cursor->visible;
	const int monitor_index = mouse_cursor_valid_monitor_index(cursor->monitor_index);
	const bool monitor_changed = cursor->tracking_initialized && cursor->active_monitor_index != monitor_index;
	float normalized_x;
	float normalized_y;
	cursor->visible = mouse_capture_sample_position(monitor_index, &normalized_x, &normalized_y);
	if (cursor->visible) {
		const float half = cursor->size / 2.0f;
		cursor->x = mouse_clampf(normalized_x * MOUSE_OVERLAY_WIDTH, half, MOUSE_OVERLAY_WIDTH - half);
		cursor->y = mouse_clampf(normalized_y * MOUSE_OVERLAY_HEIGHT, half, MOUSE_OVERLAY_HEIGHT - half);
	}

	const bool visibility_changed = cursor->tracking_initialized && cursor->visible != was_visible;
	cursor->active_monitor_index = monitor_index;
	cursor->tracking_initialized = true;
	return monitor_changed || visibility_changed;
}

bool mouse_cursor_can_render(const struct mouse_cursor_state *cursor, const struct mouse_overlay_resources *resources)
{
	return cursor->visible && cursor->enabled && cursor->opacity > 0.0f && resources->cursor_image.texture &&
	       resources->cursor_technique && resources->click_effect_image && resources->click_effect_opacity &&
	       resources->click_effect_tint;
}

void mouse_cursor_render(const struct mouse_cursor_state *cursor, struct mouse_overlay_resources *resources)
{
	if (!mouse_cursor_can_render(cursor, resources))
		return;
	mouse_resources_set_tint(resources->click_effect_tint, cursor->tint_color);
	const size_t passes = gs_technique_begin(resources->cursor_technique);
	for (size_t i = 0; i < passes; i++) {
		gs_technique_begin_pass(resources->cursor_technique, i);
		mouse_resources_draw_sprite(resources->click_effect_image, resources->click_effect_opacity,
					    resources->cursor_image.texture, cursor->x, cursor->y, cursor->size,
					    cursor->opacity);
		gs_technique_end_pass(resources->cursor_technique);
	}
	gs_technique_end(resources->cursor_technique);
}
