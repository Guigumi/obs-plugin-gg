#include "mouse-overlay-cursor.h"

#include "mouse-capture.h"
#include "mouse-overlay-resources.h"

#include <util/threading.h>

#define CURSOR_SIZE_DEFAULT 25.0f
#define CURSOR_SIZE_MIN 4.0f
#define CURSOR_SIZE_MAX 256.0f
#define CURSOR_OPACITY_DEFAULT_PCT 100
#define GAME_SENSITIVITY_DEFAULT 1.0f
#define GAME_SENSITIVITY_MIN 0.1f
#define GAME_SENSITIVITY_MAX 5.0f

void mouse_cursor_defaults(obs_data_t *settings)
{
	obs_data_set_default_double(settings, "cursor_size", CURSOR_SIZE_DEFAULT);
	obs_data_set_default_int(settings, "cursor_opacity", CURSOR_OPACITY_DEFAULT_PCT);
	obs_data_set_default_int(settings, "cursor_tint_color", MOUSE_TINT_COLOR_DEFAULT);
	obs_data_set_default_bool(settings, "cursor_enabled", true);
	obs_data_set_default_int(settings, "monitor_index", MOUSE_CAPTURE_MONITOR_ALL);
	obs_data_set_default_int(settings, "cursor_mode", MOUSE_CURSOR_MODE_AUTOMATIC);
	obs_data_set_default_double(settings, "game_sensitivity", GAME_SENSITIVITY_DEFAULT);
}

void mouse_cursor_add_properties(obs_properties_t *props, const struct mouse_cursor_state *cursor)
{
	obs_property_t *mode = obs_properties_add_list(props, "cursor_mode", obs_module_text("CursorMode"),
						       OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(mode, obs_module_text("CursorModeAutomatic"), MOUSE_CURSOR_MODE_AUTOMATIC);
	obs_property_list_add_int(mode, obs_module_text("CursorModeDesktop"), MOUSE_CURSOR_MODE_DESKTOP);
	obs_property_list_add_int(mode, obs_module_text("CursorModeGame"), MOUSE_CURSOR_MODE_GAME);
	const char *status_key = "CursorModeStatusUnknown";
	if (cursor) {
		const long reported_mode = os_atomic_load_long(&cursor->reported_mode);
		if (reported_mode == MOUSE_CURSOR_MODE_DESKTOP)
			status_key = "CursorModeStatusDesktop";
		else if (reported_mode == MOUSE_CURSOR_MODE_GAME)
			status_key = "CursorModeStatusGame";
	}
	obs_properties_add_text(props, "cursor_mode_status", obs_module_text(status_key), OBS_TEXT_INFO);
	obs_properties_add_float_slider(props, "game_sensitivity", obs_module_text("GameSensitivity"),
					GAME_SENSITIVITY_MIN, GAME_SENSITIVITY_MAX, 0.05f);
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
	cursor->configured_mode = mouse_settings_get_enum(settings, "cursor_mode", MOUSE_CURSOR_MODE_AUTOMATIC,
							  MOUSE_CURSOR_MODE_GAME, MOUSE_CURSOR_MODE_AUTOMATIC);
	cursor->game_sensitivity = mouse_settings_get_float(settings, "game_sensitivity", GAME_SENSITIVITY_MIN,
							    GAME_SENSITIVITY_MAX, GAME_SENSITIVITY_DEFAULT);
}

static int mouse_cursor_valid_monitor_index(int monitor_index)
{
	if (monitor_index == MOUSE_CAPTURE_MONITOR_ALL)
		return monitor_index;
	const size_t monitor_count = mouse_capture_get_monitor_count();
	return monitor_index >= 0 && (size_t)monitor_index < monitor_count ? monitor_index : MOUSE_CAPTURE_MONITOR_ALL;
}

static void mouse_cursor_to_canvas(struct mouse_cursor_state *cursor, float normalized_x, float normalized_y,
				   bool clamp_edges)
{
	if (clamp_edges) {
		const float half = cursor->size / 2.0f;
		cursor->x = mouse_clampf(normalized_x * MOUSE_OVERLAY_WIDTH, half, MOUSE_OVERLAY_WIDTH - half);
		cursor->y = mouse_clampf(normalized_y * MOUSE_OVERLAY_HEIGHT, half, MOUSE_OVERLAY_HEIGHT - half);
	} else {
		cursor->x = mouse_clampf(normalized_x, 0.0f, 1.0f) * MOUSE_OVERLAY_WIDTH;
		cursor->y = mouse_clampf(normalized_y, 0.0f, 1.0f) * MOUSE_OVERLAY_HEIGHT;
	}
}

static float mouse_cursor_wrap(float value, float extent, bool *wrapped)
{
	if (value >= 0.0f && value < extent)
		return value;
	*wrapped = true;
	value = fmodf(value, extent);
	return value < 0.0f ? value + extent : value;
}

static void mouse_cursor_begin_game_mode(struct mouse_cursor_state *cursor, int monitor_index)
{
	float normalized_x;
	float normalized_y;
	if (!cursor->visible && mouse_capture_sample_position(monitor_index, &normalized_x, &normalized_y))
		mouse_cursor_to_canvas(cursor, normalized_x, normalized_y, false);
	else if (!cursor->visible) {
		cursor->x = MOUSE_OVERLAY_WIDTH / 2.0f;
		cursor->y = MOUSE_OVERLAY_HEIGHT / 2.0f;
	}
	mouse_capture_get_relative_totals(&cursor->relative_total_x, &cursor->relative_total_y);
	cursor->relative_initialized = true;
	cursor->visible = true;
}

static void mouse_cursor_tick_game(struct mouse_cursor_state *cursor, int monitor_index, bool mode_changed)
{
	if (mode_changed || !cursor->relative_initialized)
		mouse_cursor_begin_game_mode(cursor, monitor_index);

	int64_t total_x;
	int64_t total_y;
	mouse_capture_get_relative_totals(&total_x, &total_y);
	const int64_t delta_x = total_x - cursor->relative_total_x;
	const int64_t delta_y = total_y - cursor->relative_total_y;
	cursor->relative_total_x = total_x;
	cursor->relative_total_y = total_y;
	cursor->wrapped = false;
	cursor->x = mouse_cursor_wrap(cursor->x + (float)delta_x * cursor->game_sensitivity, MOUSE_OVERLAY_WIDTH,
				      &cursor->wrapped);
	cursor->y = mouse_cursor_wrap(cursor->y + (float)delta_y * cursor->game_sensitivity, MOUSE_OVERLAY_HEIGHT,
				      &cursor->wrapped);
	cursor->visible = true;
}

bool mouse_cursor_tick(struct mouse_cursor_state *cursor)
{
	const bool tracking_started = !cursor->tracking_initialized;
	const bool was_visible = cursor->visible;
	const int monitor_index = mouse_cursor_valid_monitor_index(cursor->monitor_index);
	const bool relative_available = mouse_capture_relative_available();
	const int active_mode = cursor->configured_mode == MOUSE_CURSOR_MODE_AUTOMATIC
					? (relative_available && mouse_capture_detect_game_mode()
						   ? MOUSE_CURSOR_MODE_GAME
						   : MOUSE_CURSOR_MODE_DESKTOP)
					: (cursor->configured_mode == MOUSE_CURSOR_MODE_GAME && !relative_available
						   ? MOUSE_CURSOR_MODE_DESKTOP
						   : cursor->configured_mode);
	const bool mode_changed = cursor->tracking_initialized && cursor->active_mode != active_mode;
	const bool monitor_changed = cursor->tracking_initialized && cursor->active_monitor_index != monitor_index;

	if (active_mode == MOUSE_CURSOR_MODE_GAME) {
		mouse_cursor_tick_game(cursor, monitor_index, mode_changed);
	} else {
		float normalized_x;
		float normalized_y;
		cursor->relative_initialized = false;
		cursor->wrapped = false;
		cursor->visible = mouse_capture_sample_position(monitor_index, &normalized_x, &normalized_y);
		if (cursor->visible)
			mouse_cursor_to_canvas(cursor, normalized_x, normalized_y, true);
	}

	const bool visibility_changed = cursor->tracking_initialized && cursor->visible != was_visible;
	cursor->active_monitor_index = monitor_index;
	cursor->active_mode = active_mode;
	os_atomic_exchange_long(&cursor->reported_mode, active_mode);
	cursor->tracking_initialized = true;
	return tracking_started || mode_changed || monitor_changed || visibility_changed;
}

size_t mouse_cursor_draw_positions(const struct mouse_cursor_state *cursor, float x, float y, float size,
				   float positions[4][2])
{
	positions[0][0] = x;
	positions[0][1] = y;
	if (cursor->active_mode != MOUSE_CURSOR_MODE_GAME)
		return 1;

	float x_positions[2] = {x, x};
	float y_positions[2] = {y, y};
	size_t x_count = 1;
	size_t y_count = 1;
	const float half = size / 2.0f;
	if (x < half)
		x_positions[x_count++] = x + MOUSE_OVERLAY_WIDTH;
	else if (x > MOUSE_OVERLAY_WIDTH - half)
		x_positions[x_count++] = x - MOUSE_OVERLAY_WIDTH;
	if (y < half)
		y_positions[y_count++] = y + MOUSE_OVERLAY_HEIGHT;
	else if (y > MOUSE_OVERLAY_HEIGHT - half)
		y_positions[y_count++] = y - MOUSE_OVERLAY_HEIGHT;

	size_t count = 0;
	for (size_t yi = 0; yi < y_count; yi++) {
		for (size_t xi = 0; xi < x_count; xi++) {
			positions[count][0] = x_positions[xi];
			positions[count][1] = y_positions[yi];
			count++;
		}
	}
	return count;
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
	float positions[4][2];
	const size_t position_count =
		mouse_cursor_draw_positions(cursor, cursor->x, cursor->y, cursor->size, positions);
	gs_effect_set_float(resources->click_effect_side, 0.0f);
	gs_effect_set_float(resources->click_effect_size, cursor->size);
	gs_effect_set_texture(resources->click_effect_image, resources->cursor_image.texture);
	gs_effect_set_float(resources->click_effect_opacity, cursor->opacity);
	const size_t passes = gs_technique_begin(resources->cursor_technique);
	for (size_t i = 0; i < passes; i++) {
		gs_technique_begin_pass(resources->cursor_technique, i);
		for (size_t j = 0; j < position_count; j++)
			mouse_resources_draw_sprite(resources->click_effect_image, resources->click_effect_opacity,
						    resources->cursor_image.texture, positions[j][0], positions[j][1],
						    cursor->size, cursor->opacity);
		gs_technique_end_pass(resources->cursor_technique);
	}
	gs_technique_end(resources->cursor_technique);
}
