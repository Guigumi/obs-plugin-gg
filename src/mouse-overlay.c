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
#include <graphics/graphics.h>
#include <graphics/image-file.h>
#include <graphics/vec4.h>
#include <util/platform.h>
#include <util/threading.h>

#include "mouse-capture.h"
#include "mouse-overlay.h"

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define OVERLAY_WIDTH 1920u
#define OVERLAY_HEIGHT 1080u
#define CURSOR_SIZE_DEFAULT 64.0f
#define CURSOR_SIZE_MIN 8.0f
#define CURSOR_SIZE_MAX 256.0f
#define CURSOR_OPACITY_DEFAULT_PCT 100
#define CURSOR_OPACITY_MIN_PCT 0
#define CURSOR_OPACITY_MAX_PCT 100

#define TRAIL_POINTS_CAPACITY 256u
#define TRAIL_POINTS_MIN 1u
#define TRAIL_POINTS_MAX TRAIL_POINTS_CAPACITY
#define TRAIL_POINTS_DEFAULT 15u
#define TRAIL_DURATION_DEFAULT 0.25f
#define TRAIL_DURATION_MIN 0.1f
#define TRAIL_DURATION_MAX 10.0f
#define TRAIL_SPACING_DEFAULT 20.0f
#define TRAIL_SPACING_MIN 0.1f
#define TRAIL_SPACING_MAX 256.0f
#define TRAIL_SIZE_DEFAULT_PCT 100
#define TRAIL_SIZE_MIN_PCT 10
#define TRAIL_SIZE_MAX_PCT 200
#define TRAIL_OPACITY_DEFAULT_PCT 50
#define TRAIL_OPACITY_MAX_PCT 100
#define TRAIL_FADE_LINEAR 0
#define TRAIL_FADE_SMOOTH 1
#define TRAIL_FADE_EXPONENTIAL 2
#define TRAIL_FADE_DEFAULT TRAIL_FADE_SMOOTH
#define TINT_COLOR_DEFAULT 0xFFFFFF

#define CLICK_EVENTS_CAPACITY 256u
#define CLICK_DURATION_DEFAULT 0.50f
#define CLICK_DURATION_MIN 0.1f
#define CLICK_DURATION_MAX 2.0f
#define CLICK_OPACITY_MIN_PCT 0
#define CLICK_OPACITY_DEFAULT_PCT 25
#define CLICK_OPACITY_MAX_PCT 100
#define CLICK_ANIM_EXPAND 0
#define CLICK_ANIM_CONTRACT 1
#define CLICK_ANIM_PULSE 2
#define CLICK_ANIM_DEFAULT CLICK_ANIM_PULSE

struct trail_point {
	float x;
	float y;
	uint64_t time_ns;
};

struct click_event {
	uint64_t time_ns;
	bool left;
};

struct mouse_overlay_resources {
	gs_image_file_t cursor_image;
	gs_image_file_t trail_image;
	gs_image_file_t click_lmb_image;
	gs_image_file_t click_rmb_image;
	gs_effect_t *trail_effect;
	gs_technique_t *trail_technique;
	gs_eparam_t *trail_effect_image;
	gs_eparam_t *trail_effect_opacity;
	gs_eparam_t *trail_effect_tint;
	gs_effect_t *click_effect;
	gs_technique_t *cursor_technique;
	gs_technique_t *click_technique;
	gs_eparam_t *click_effect_image;
	gs_eparam_t *click_effect_opacity;
	gs_eparam_t *click_effect_tint;
	volatile bool images_dirty;
};

struct mouse_cursor_state {
	float size;
	float opacity_pct;
	uint32_t tint_color;
	bool enabled;
	bool visible;
	int monitor_index;
	float x;
	float y;
};

struct mouse_trail_state {
	bool enabled;
	bool shrink;
	float duration;
	float spacing;
	float size_pct;
	float opacity_pct;
	uint32_t tint_color;
	int fade;
	uint32_t point_limit;
	bool dirty;
	struct trail_point points[TRAIL_POINTS_CAPACITY];
	size_t count;
	size_t start;
	float last_pushed_x;
	float last_pushed_y;
	bool has_last_pushed;
};

struct mouse_click_state {
	bool enabled;
	bool left_enabled;
	bool right_enabled;
	float duration;
	float opacity_pct;
	uint32_t tint_color;
	int animation;
	size_t count;
	size_t start;
	long seen_left_click_sequence;
	long seen_right_click_sequence;
	bool left_down;
	bool right_down;
	struct click_event events[CLICK_EVENTS_CAPACITY];
};

struct mouse_overlay_gg_data {
	struct mouse_overlay_resources resources;
	struct mouse_cursor_state cursor;
	struct mouse_trail_state trail;
	struct mouse_click_state click;
};

struct mouse_image_entry {
	gs_image_file_t *image;
	const char *path;
};

static void mouse_overlay_load_image(gs_image_file_t *image, const char *relative_path)
{
	char *path = obs_module_file(relative_path);
	if (!path) {
		blog(LOG_WARNING, "Failed to resolve image path: %s", relative_path);
		return;
	}
	gs_image_file_init(image, path);
	if (!image->loaded)
		blog(LOG_WARNING, "Failed to load image: %s", path);
	bfree(path);
}

static void mouse_overlay_init_images(struct mouse_overlay_resources *resources)
{
	const struct mouse_image_entry images[] = {
		{&resources->cursor_image, "images/cursor-main.png"},
		{&resources->trail_image, "images/cursor-trail.png"},
		{&resources->click_lmb_image, "images/cursor-LMB.png"},
		{&resources->click_rmb_image, "images/cursor-RMB.png"},
	};
	for (size_t i = 0; i < sizeof(images) / sizeof(images[0]); i++)
		mouse_overlay_load_image(images[i].image, images[i].path);
}

static void mouse_overlay_free_images(struct mouse_overlay_resources *resources)
{
	gs_image_file_t *images[] = {
		&resources->cursor_image,
		&resources->trail_image,
		&resources->click_lmb_image,
		&resources->click_rmb_image,
	};
	for (size_t i = 0; i < sizeof(images) / sizeof(images[0]); i++)
		gs_image_file_free(images[i]);
}

static bool mouse_overlay_reload_images(obs_properties_t *props, obs_property_t *property, void *data)
{
	UNUSED_PARAMETER(props);
	UNUSED_PARAMETER(property);
	if (data)
		os_atomic_set_bool(&((struct mouse_overlay_gg_data *)data)->resources.images_dirty, true);
	return true;
}

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
	obs_data_set_default_double(settings, "cursor_size", CURSOR_SIZE_DEFAULT);
	obs_data_set_default_int(settings, "cursor_opacity", CURSOR_OPACITY_DEFAULT_PCT);
	obs_data_set_default_int(settings, "cursor_tint_color", TINT_COLOR_DEFAULT);
	obs_data_set_default_bool(settings, "cursor_enabled", true);
	obs_data_set_default_int(settings, "monitor_index", MOUSE_CAPTURE_MONITOR_ALL);
	obs_data_set_default_int(settings, "trail_points", (int)TRAIL_POINTS_DEFAULT);
	obs_data_set_default_double(settings, "trail_duration", TRAIL_DURATION_DEFAULT);
	obs_data_set_default_double(settings, "trail_spacing", TRAIL_SPACING_DEFAULT);
	obs_data_set_default_bool(settings, "trail_enabled", true);
	obs_data_set_default_bool(settings, "trail_shrink", true);
	obs_data_set_default_int(settings, "trail_size", TRAIL_SIZE_DEFAULT_PCT);
	obs_data_set_default_int(settings, "trail_opacity", TRAIL_OPACITY_DEFAULT_PCT);
	obs_data_set_default_int(settings, "trail_tint_color", TINT_COLOR_DEFAULT);
	obs_data_set_default_int(settings, "trail_fade", TRAIL_FADE_DEFAULT);
	obs_data_set_default_bool(settings, "click_enabled", true);
	obs_data_set_default_bool(settings, "click_left", true);
	obs_data_set_default_bool(settings, "click_right", true);
	obs_data_set_default_double(settings, "click_duration", CLICK_DURATION_DEFAULT);
	obs_data_set_default_int(settings, "click_opacity", CLICK_OPACITY_DEFAULT_PCT);
	obs_data_set_default_int(settings, "click_tint_color", TINT_COLOR_DEFAULT);
	obs_data_set_default_int(settings, "click_anim", CLICK_ANIM_DEFAULT);
}

static obs_properties_t *mouse_overlay_get_properties(void *data)
{
	obs_properties_t *props = obs_properties_create();
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

	obs_properties_t *cursor = obs_properties_create();
	obs_properties_add_group(props, "cursor_enabled", obs_module_text("Cursor"), OBS_GROUP_CHECKABLE, cursor);
	obs_properties_add_int_slider(cursor, "cursor_size", obs_module_text("CursorSize"), (int)CURSOR_SIZE_MIN,
				      (int)CURSOR_SIZE_MAX, 1);
	obs_properties_add_int_slider(cursor, "cursor_opacity", obs_module_text("CursorOpacity"),
				      CURSOR_OPACITY_MIN_PCT, CURSOR_OPACITY_MAX_PCT, 1);
	obs_properties_add_color(cursor, "cursor_tint_color", obs_module_text("CursorTintColor"));

	obs_properties_t *const trail = obs_properties_create();
	obs_properties_add_group(props, "trail_enabled", obs_module_text("Trail"), OBS_GROUP_CHECKABLE, trail);
	obs_properties_add_int_slider(trail, "trail_points", obs_module_text("TrailPoints"), (int)TRAIL_POINTS_MIN,
				      (int)TRAIL_POINTS_MAX, 1);
	obs_properties_add_int_slider(trail, "trail_size", obs_module_text("TrailSize"), TRAIL_SIZE_MIN_PCT,
				      TRAIL_SIZE_MAX_PCT, 1);
	obs_properties_add_int_slider(trail, "trail_opacity", obs_module_text("TrailOpacity"), 0, TRAIL_OPACITY_MAX_PCT,
				      1);
	obs_properties_add_float_slider(trail, "trail_duration", obs_module_text("TrailDuration"), TRAIL_DURATION_MIN,
					TRAIL_DURATION_MAX, 0.1f);
	obs_properties_add_float_slider(trail, "trail_spacing", obs_module_text("TrailSpacing"), TRAIL_SPACING_MIN,
					TRAIL_SPACING_MAX, 0.1f);
	obs_properties_add_bool(trail, "trail_shrink", obs_module_text("TrailShrink"));
	obs_properties_add_color(trail, "trail_tint_color", obs_module_text("TrailTintColor"));
	obs_property_t *fade = obs_properties_add_list(trail, "trail_fade", obs_module_text("TrailFade"),
						       OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(fade, obs_module_text("TrailFadeLinear"), TRAIL_FADE_LINEAR);
	obs_property_list_add_int(fade, obs_module_text("TrailFadeSmooth"), TRAIL_FADE_SMOOTH);
	obs_property_list_add_int(fade, obs_module_text("TrailFadeExponential"), TRAIL_FADE_EXPONENTIAL);

	obs_properties_t *const click = obs_properties_create();
	obs_properties_add_group(props, "click_enabled", obs_module_text("Clicks"), OBS_GROUP_CHECKABLE, click);
	obs_properties_add_bool(click, "click_left", obs_module_text("ClickLeft"));
	obs_properties_add_bool(click, "click_right", obs_module_text("ClickRight"));
	obs_properties_add_int_slider(click, "click_opacity", obs_module_text("ClickOpacity"), CLICK_OPACITY_MIN_PCT,
				      CLICK_OPACITY_MAX_PCT, 1);
	obs_properties_add_float_slider(click, "click_duration", obs_module_text("ClickDuration"), CLICK_DURATION_MIN,
					CLICK_DURATION_MAX, 0.05f);
	obs_property_t *click_anim = obs_properties_add_list(click, "click_anim", obs_module_text("ClickAnim"),
							     OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(click_anim, obs_module_text("ClickAnimExpand"), CLICK_ANIM_EXPAND);
	obs_property_list_add_int(click_anim, obs_module_text("ClickAnimContract"), CLICK_ANIM_CONTRACT);
	obs_property_list_add_int(click_anim, obs_module_text("ClickAnimPulse"), CLICK_ANIM_PULSE);
	obs_properties_add_color(click, "click_tint_color", obs_module_text("ClickTintColor"));
	obs_properties_add_button2(props, "reload_images", obs_module_text("ReloadImages"), mouse_overlay_reload_images,
				   data);
	return props;
}

static void mouse_overlay_update(void *context, obs_data_t *settings)
{
	struct mouse_overlay_gg_data *data = context;
	struct mouse_cursor_state *cursor = &data->cursor;
	struct mouse_trail_state *trail = &data->trail;
	struct mouse_click_state *click = &data->click;

	cursor->size = (float)obs_data_get_double(settings, "cursor_size");
	if (!cursor->size)
		cursor->size = CURSOR_SIZE_DEFAULT;
	cursor->opacity_pct = (float)obs_data_get_int(settings, "cursor_opacity");
	cursor->opacity_pct = fminf(fmaxf(cursor->opacity_pct, CURSOR_OPACITY_MIN_PCT), CURSOR_OPACITY_MAX_PCT);
	cursor->tint_color = (uint32_t)obs_data_get_int(settings, "cursor_tint_color");
	cursor->enabled = obs_data_get_bool(settings, "cursor_enabled");
	cursor->monitor_index = (int)obs_data_get_int(settings, "monitor_index");

	trail->enabled = obs_data_get_bool(settings, "trail_enabled");
	trail->shrink = obs_data_get_bool(settings, "trail_shrink");
	trail->point_limit = (uint32_t)obs_data_get_int(settings, "trail_points");
	if (trail->point_limit < TRAIL_POINTS_MIN)
		trail->point_limit = TRAIL_POINTS_MIN;
	if (trail->point_limit > TRAIL_POINTS_MAX)
		trail->point_limit = TRAIL_POINTS_MAX;
	trail->size_pct = (float)obs_data_get_int(settings, "trail_size");
	trail->opacity_pct = (float)obs_data_get_int(settings, "trail_opacity");
	trail->tint_color = (uint32_t)obs_data_get_int(settings, "trail_tint_color");
	trail->fade = (int)obs_data_get_int(settings, "trail_fade");
	trail->duration = (float)obs_data_get_double(settings, "trail_duration");
	if (trail->duration <= 0.0f)
		trail->duration = TRAIL_DURATION_DEFAULT;
	trail->spacing = (float)obs_data_get_double(settings, "trail_spacing");
	if (trail->spacing <= 0.0f)
		trail->spacing = TRAIL_SPACING_DEFAULT;

	click->enabled = obs_data_get_bool(settings, "click_enabled");
	click->left_enabled = obs_data_get_bool(settings, "click_left");
	click->right_enabled = obs_data_get_bool(settings, "click_right");
	click->duration = (float)obs_data_get_double(settings, "click_duration");
	if (click->duration <= 0.0f)
		click->duration = CLICK_DURATION_DEFAULT;
	click->opacity_pct = (float)obs_data_get_int(settings, "click_opacity");
	click->tint_color = (uint32_t)obs_data_get_int(settings, "click_tint_color");
	click->animation = (int)obs_data_get_int(settings, "click_anim");
	if (!click->enabled) {
		click->count = 0;
		click->start = 0;
	}

	trail->dirty = true;
}

static gs_effect_t *mouse_overlay_load_effect(const char *relative_path)
{
	char *path = obs_module_file(relative_path);
	if (!path) {
		blog(LOG_ERROR, "Failed to resolve effect path: %s", relative_path);
		return NULL;
	}
	char *errors = NULL;
	obs_enter_graphics();
	gs_effect_t *effect = gs_effect_create_from_file(path, &errors);
	obs_leave_graphics();
	if (!effect)
		blog(LOG_ERROR, "Failed to load effect %s: %s", path, errors ? errors : "unknown error");
	bfree(errors);
	bfree(path);
	return effect;
}

static bool mouse_overlay_validate_effect_entry(const char *effect_name, const char *entry_name, const void *entry)
{
	if (entry)
		return true;
	blog(LOG_ERROR, "%s is missing required entry: %s", effect_name, entry_name);
	return false;
}

static void mouse_overlay_init_effects(struct mouse_overlay_resources *resources)
{
	resources->trail_effect = mouse_overlay_load_effect("trail.effect");
	if (resources->trail_effect) {
		resources->trail_technique = gs_effect_get_technique(resources->trail_effect, "DrawTrail");
		resources->trail_effect_image = gs_effect_get_param_by_name(resources->trail_effect, "image");
		resources->trail_effect_opacity = gs_effect_get_param_by_name(resources->trail_effect, "opacity");
		resources->trail_effect_tint = gs_effect_get_param_by_name(resources->trail_effect, "tint");
		mouse_overlay_validate_effect_entry("trail.effect", "DrawTrail", resources->trail_technique);
		mouse_overlay_validate_effect_entry("trail.effect", "image", resources->trail_effect_image);
		mouse_overlay_validate_effect_entry("trail.effect", "opacity", resources->trail_effect_opacity);
		mouse_overlay_validate_effect_entry("trail.effect", "tint", resources->trail_effect_tint);
	}

	resources->click_effect = mouse_overlay_load_effect("click.effect");
	if (resources->click_effect) {
		resources->cursor_technique = gs_effect_get_technique(resources->click_effect, "DrawCursor");
		resources->click_technique = gs_effect_get_technique(resources->click_effect, "DrawClick");
		resources->click_effect_image = gs_effect_get_param_by_name(resources->click_effect, "image");
		resources->click_effect_opacity = gs_effect_get_param_by_name(resources->click_effect, "opacity");
		resources->click_effect_tint = gs_effect_get_param_by_name(resources->click_effect, "tint");
		mouse_overlay_validate_effect_entry("click.effect", "DrawCursor", resources->cursor_technique);
		mouse_overlay_validate_effect_entry("click.effect", "DrawClick", resources->click_technique);
		mouse_overlay_validate_effect_entry("click.effect", "image", resources->click_effect_image);
		mouse_overlay_validate_effect_entry("click.effect", "opacity", resources->click_effect_opacity);
		mouse_overlay_validate_effect_entry("click.effect", "tint", resources->click_effect_tint);
	}
}

static void mouse_overlay_free_resources(struct mouse_overlay_resources *resources)
{
	mouse_overlay_free_images(resources);
	gs_effect_destroy(resources->trail_effect);
	gs_effect_destroy(resources->click_effect);
}

static void *mouse_overlay_create(obs_data_t *settings, obs_source_t *source)
{
	UNUSED_PARAMETER(source);

	struct mouse_overlay_gg_data *data = bzalloc(sizeof(*data));
	mouse_capture_get_button_sequences(&data->click.seen_left_click_sequence,
					   &data->click.seen_right_click_sequence);
	mouse_overlay_update(data, settings);
	mouse_overlay_init_images(&data->resources);
	mouse_overlay_init_effects(&data->resources);

	return data;
}

static void mouse_overlay_destroy(void *context)
{
	struct mouse_overlay_gg_data *data = context;
	if (!data)
		return;

	obs_enter_graphics();
	mouse_overlay_free_resources(&data->resources);
	obs_leave_graphics();

	bfree(data);
}

static uint32_t mouse_overlay_get_width(void *data)
{
	UNUSED_PARAMETER(data);
	return OVERLAY_WIDTH;
}

static uint32_t mouse_overlay_get_height(void *data)
{
	UNUSED_PARAMETER(data);
	return OVERLAY_HEIGHT;
}

static void mouse_overlay_push_click(struct mouse_click_state *click, bool left, uint64_t now_ns);

static void mouse_overlay_video_tick(void *context, float seconds)
{
	struct mouse_overlay_gg_data *data = context;
	struct mouse_cursor_state *cursor = &data->cursor;
	struct mouse_trail_state *trail = &data->trail;
	struct mouse_click_state *click = &data->click;

	float nx;
	float ny;
	cursor->visible = mouse_capture_sample_position(cursor->monitor_index, &nx, &ny);

	const uint64_t now_ns = os_gettime_ns();
	const float half = cursor->size / 2.0f;
	const float min = half;
	const float max_x = OVERLAY_WIDTH - half;
	const float max_y = OVERLAY_HEIGHT - half;

	if (cursor->visible) {
		cursor->x = fminf(fmaxf(nx * OVERLAY_WIDTH, min), max_x);
		cursor->y = fminf(fmaxf(ny * OVERLAY_HEIGHT, min), max_y);
	}

	/* Detect each button edge once globally, then let every source instance
	 * consume the resulting sequence exactly once. */
	long left_sequence;
	long right_sequence;
	mouse_capture_sample_button_sequences(&left_sequence, &right_sequence, &click->left_down, &click->right_down);
	if (click->seen_left_click_sequence != left_sequence) {
		click->seen_left_click_sequence = left_sequence;
		if (cursor->visible && click->enabled && click->left_enabled)
			mouse_overlay_push_click(click, true, now_ns);
	}
	if (click->seen_right_click_sequence != right_sequence) {
		click->seen_right_click_sequence = right_sequence;
		if (cursor->visible && click->enabled && click->right_enabled)
			mouse_overlay_push_click(click, false, now_ns);
	}

	const uint64_t click_duration_ns = (uint64_t)(click->duration * 1000000000.0);
	while (click->count > 0) {
		const struct click_event *const ev = &click->events[click->start];
		if (now_ns - ev->time_ns >= click_duration_ns) {
			click->start = (click->start + 1) % CLICK_EVENTS_CAPACITY;
			click->count--;
			continue;
		}
		break;
	}
	if (!cursor->visible) {
		click->count = 0;
		click->start = 0;
		trail->has_last_pushed = false;
	}

	if (trail->dirty) {
		trail->dirty = false;
		if (!trail->enabled || !trail->point_limit) {
			trail->count = 0;
			trail->start = 0;
			trail->has_last_pushed = false;
			return;
		}
		if (trail->count > trail->point_limit) {
			const size_t drop = trail->count - trail->point_limit;
			trail->start = (trail->start + drop) % TRAIL_POINTS_CAPACITY;
			trail->count = trail->point_limit;
		}
	}

	const int64_t duration_ns = (int64_t)(trail->duration * 1000000000.0);
	uint64_t layer_span_ns = (uint64_t)(seconds * 1e9f);
	if (layer_span_ns > (uint64_t)duration_ns)
		layer_span_ns = (uint64_t)duration_ns;

	while (trail->count > 0) {
		const struct trail_point *const pt = &trail->points[trail->start];
		if (now_ns - pt->time_ns >= (uint64_t)duration_ns) {
			trail->start = (trail->start + 1) % TRAIL_POINTS_CAPACITY;
			trail->count--;
			continue;
		}
		break;
	}

	if (!cursor->visible || !trail->enabled || !trail->point_limit)
		return;

	/* Real-distance spacing: place a point every `spacing` pixels walked.
	 * Interpolate so a fast frame fills the gap instead of leaving holes. */
	const float dx = cursor->x - trail->last_pushed_x;
	const float dy = cursor->y - trail->last_pushed_y;
	const float dist = sqrtf(dx * dx + dy * dy);
	const float spacing = trail->spacing;

	if (trail->has_last_pushed && dist < spacing)
		return;

	int steps = 1;
	if (trail->has_last_pushed && spacing > 0.0f)
		steps = (int)(dist / spacing);
	if (steps < 1)
		steps = 1;
	if (steps > (int)TRAIL_POINTS_CAPACITY)
		steps = (int)TRAIL_POINTS_CAPACITY;

	for (int i = 0; i < steps; i++) {
		const float f = (float)(i + 1) / (float)steps;
		const float px = trail->last_pushed_x + dx * f;
		const float py = trail->last_pushed_y + dy * f;

		/* Back-date each interpolated point so the whole jump fades out
		 * as if it happened across the frame. */
		const uint64_t point_time = now_ns - (uint64_t)((float)layer_span_ns * (1.0f - f));

		const size_t next = (trail->start + trail->count) % TRAIL_POINTS_CAPACITY;
		struct trail_point *point = &trail->points[next];
		point->x = px;
		point->y = py;
		point->time_ns = point_time;

		if (trail->count >= trail->point_limit && trail->point_limit > 0) {
			trail->start = (trail->start + 1) % TRAIL_POINTS_CAPACITY;
		} else if (trail->count < TRAIL_POINTS_CAPACITY) {
			trail->count++;
		}
	}

	trail->last_pushed_x = cursor->x;
	trail->last_pushed_y = cursor->y;
	trail->has_last_pushed = true;
}

static void mouse_overlay_push_click(struct mouse_click_state *click, bool left, uint64_t now_ns)
{
	if (click->count < CLICK_EVENTS_CAPACITY) {
		click->count++;
	} else {
		click->start = (click->start + 1) % CLICK_EVENTS_CAPACITY;
	}

	const size_t next = (click->start + click->count - 1) % CLICK_EVENTS_CAPACITY;
	struct click_event *ev = &click->events[next];
	ev->time_ns = now_ns;
	ev->left = left;
}

static void mouse_overlay_draw_sprite(gs_eparam_t *image_param, gs_eparam_t *opacity_param, gs_texture_t *texture,
				      float x, float y, float size, float opacity)
{
	if (opacity_param)
		gs_effect_set_float(opacity_param, opacity);

	gs_effect_set_texture(image_param, texture);

	gs_matrix_push();
	gs_matrix_translate3f(x - size / 2.0f, y - size / 2.0f, 0.0f);
	gs_draw_sprite(texture, 0, (uint32_t)size, (uint32_t)size);
	gs_matrix_pop();
}

static void mouse_overlay_set_tint(gs_eparam_t *param, uint32_t color)
{
	struct vec4 tint;
	vec4_from_rgba(&tint, color | 0xFF000000);
	gs_effect_set_vec4(param, &tint);
}

static void mouse_overlay_init_image_textures(struct mouse_overlay_resources *resources)
{
	gs_image_file_t *images[] = {
		&resources->cursor_image,
		&resources->trail_image,
		&resources->click_lmb_image,
		&resources->click_rmb_image,
	};
	for (size_t i = 0; i < sizeof(images) / sizeof(images[0]); i++) {
		if (!images[i]->texture && images[i]->loaded)
			gs_image_file_init_texture(images[i]);
	}
}

static void mouse_overlay_video_render(void *context, gs_effect_t *effect)
{
	UNUSED_PARAMETER(effect);
	struct mouse_overlay_gg_data *data = context;
	struct mouse_overlay_resources *resources = &data->resources;
	const struct mouse_cursor_state *cursor = &data->cursor;
	const struct mouse_trail_state *trail = &data->trail;
	const struct mouse_click_state *click = &data->click;

	if (os_atomic_exchange_bool(&resources->images_dirty, false)) {
		mouse_overlay_free_images(resources);
		mouse_overlay_init_images(resources);
	}
	mouse_overlay_init_image_textures(resources);

	const bool click_effect_ready = resources->click_technique && resources->click_effect_image &&
					resources->click_effect_opacity && resources->click_effect_tint;
	const bool cursor_effect_ready = resources->cursor_technique && resources->click_effect_image &&
					 resources->click_effect_opacity && resources->click_effect_tint;
	const bool trail_effect_ready = resources->trail_technique && resources->trail_effect_image &&
					resources->trail_effect_opacity && resources->trail_effect_tint;
	const bool draw_cursor = cursor->visible && cursor->enabled && cursor->opacity_pct > 0.0f &&
				 resources->cursor_image.texture && cursor_effect_ready;
	const bool draw_trail = trail_effect_ready && resources->trail_image.texture && trail->count > 0 &&
				trail->point_limit > 0;
	const bool left_held = cursor->visible && click->enabled && click->left_enabled && click->left_down;
	const bool right_held = cursor->visible && click->enabled && click->right_enabled && click->right_down;
	const bool draw_clicks = click_effect_ready && (click->count > 0 || left_held || right_held);
	if (!draw_cursor && !draw_trail && !draw_clicks)
		return;

	gs_blend_state_push();
	gs_blend_function(GS_BLEND_ONE, GS_BLEND_INVSRCALPHA);

	if (draw_trail) {
		mouse_overlay_set_tint(resources->trail_effect_tint, trail->tint_color);
		const uint64_t now_ns = os_gettime_ns();
		const size_t trail_passes = gs_technique_begin(resources->trail_technique);
		for (size_t i = 0; i < trail_passes; i++) {
			gs_technique_begin_pass(resources->trail_technique, i);
			for (size_t j = 0; j < trail->count; j++) {
				const size_t idx = (trail->start + j) % TRAIL_POINTS_CAPACITY;
				const struct trail_point *const point = &trail->points[idx];
				const float raw_age = (float)(now_ns - point->time_ns) * 1e-9f;
				float t = 1.0f - raw_age / trail->duration;
				if (t <= 0.0f)
					continue;
				if (t > 1.0f)
					t = 1.0f;
				float fade = 0.0f;
				if (trail->fade == TRAIL_FADE_SMOOTH)
					fade = t * t * (3.0f - 2.0f * t);
				else if (trail->fade == TRAIL_FADE_EXPONENTIAL)
					fade = t * t;
				else
					fade = t;
				const float opacity = fade * (trail->opacity_pct / 100.0f);
				float trail_size = cursor->size * (trail->size_pct / 100.0f);
				if (trail->shrink)
					trail_size *= t;
				if (trail_size < 1.0f)
					continue;
				mouse_overlay_draw_sprite(resources->trail_effect_image,
							  resources->trail_effect_opacity,
							  resources->trail_image.texture, point->x, point->y,
							  trail_size, opacity);
			}
			gs_technique_end_pass(resources->trail_technique);
		}
		gs_technique_end(resources->trail_technique);
	}

	if (draw_cursor) {
		mouse_overlay_set_tint(resources->click_effect_tint, cursor->tint_color);
		const size_t cursor_passes = gs_technique_begin(resources->cursor_technique);
		for (size_t i = 0; i < cursor_passes; i++) {
			gs_technique_begin_pass(resources->cursor_technique, i);
			mouse_overlay_draw_sprite(resources->click_effect_image, resources->click_effect_opacity,
						  resources->cursor_image.texture, cursor->x, cursor->y, cursor->size,
						  cursor->opacity_pct / 100.0f);
			gs_technique_end_pass(resources->cursor_technique);
		}
		gs_technique_end(resources->cursor_technique);
	}

	if (draw_clicks) {
		const uint64_t now_ns = os_gettime_ns();
		const float base_size = cursor->size;
		mouse_overlay_set_tint(resources->click_effect_tint, click->tint_color);

		const size_t click_passes = gs_technique_begin(resources->click_technique);
		for (size_t i = 0; i < click_passes; i++) {
			gs_technique_begin_pass(resources->click_technique, i);
			const float held_opacity = click->opacity_pct / 100.0f;
			if (left_held && resources->click_lmb_image.texture)
				mouse_overlay_draw_sprite(resources->click_effect_image,
							  resources->click_effect_opacity,
							  resources->click_lmb_image.texture, cursor->x, cursor->y,
							  base_size, held_opacity);
			if (right_held && resources->click_rmb_image.texture)
				mouse_overlay_draw_sprite(resources->click_effect_image,
							  resources->click_effect_opacity,
							  resources->click_rmb_image.texture, cursor->x, cursor->y,
							  base_size, held_opacity);

			for (size_t j = 0; j < click->count; j++) {
				const size_t idx = (click->start + j) % CLICK_EVENTS_CAPACITY;
				const struct click_event *const event = &click->events[idx];
				if ((event->left && left_held) || (!event->left && right_held))
					continue;
				const float raw_age = (float)(now_ns - event->time_ns) * 1e-9f;
				float t = raw_age / click->duration;
				if (t >= 1.0f)
					continue;
				if (t < 0.0f)
					t = 0.0f;

				float scale = 0.5f + t;
				if (click->animation == CLICK_ANIM_CONTRACT)
					scale = 1.5f - t;
				else if (click->animation == CLICK_ANIM_PULSE)
					scale = 1.0f + 0.5f * sinf((float)M_PI * t);

				const float opacity = (1.0f - t) * (click->opacity_pct / 100.0f);
				gs_texture_t *texture = event->left ? resources->click_lmb_image.texture
								    : resources->click_rmb_image.texture;
				if (!texture)
					continue;
				mouse_overlay_draw_sprite(resources->click_effect_image,
							  resources->click_effect_opacity, texture, cursor->x,
							  cursor->y, base_size * scale, opacity);
			}
			gs_technique_end_pass(resources->click_technique);
		}
		gs_technique_end(resources->click_technique);
	}
	if (resources->trail_effect_image)
		gs_effect_set_texture(resources->trail_effect_image, NULL);
	if (resources->click_effect_image)
		gs_effect_set_texture(resources->click_effect_image, NULL);

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
