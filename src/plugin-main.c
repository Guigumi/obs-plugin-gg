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
#include <plugin-support.h>
#include <util/platform.h>

#include "mouse-capture.h"

#include <math.h>
#include <stdint.h>
#include <string.h>

#define OVERLAY_WIDTH 1920u
#define OVERLAY_HEIGHT 1080u
#define CURSOR_SIZE_DEFAULT 64.0f
#define CURSOR_SIZE_MIN 8.0f
#define CURSOR_SIZE_MAX 256.0f
#define TRAIL_POINTS_MAX 256u
#define TRAIL_POINTS_DEFAULT 15u
#define TRAIL_DURATION_DEFAULT 0.25f
#define TRAIL_DURATION_MIN 0.1f
#define TRAIL_DURATION_MAX 10.0f
#define TRAIL_SPACING_DEFAULT 10.0f
#define TRAIL_SPACING_MIN 0.1f
#define TRAIL_SPACING_MAX 256.0f
#define TRAIL_SIZE_DEFAULT_PCT 100
#define TRAIL_SIZE_MIN_PCT 10
#define TRAIL_SIZE_MAX_PCT 200
#define TRAIL_OPACITY_DEFAULT_PCT 100
#define TRAIL_OPACITY_MAX_PCT 100
#define TRAIL_FADE_LINEAR 0
#define TRAIL_FADE_SMOOTH 1
#define TRAIL_FADE_EXPONENTIAL 2
#define TRAIL_FADE_DEFAULT TRAIL_FADE_SMOOTH

struct trail_point {
	float x;
	float y;
	uint64_t time_ns;
};

struct mouse_overlay_gg_data {
	gs_image_file_t cursor_image;
	gs_image_file_t trail_image;
	gs_effect_t *trail_effect;
	gs_eparam_t *trail_effect_image;
	gs_eparam_t *trail_effect_opacity;

	float cursor_size;
	bool cursor_enabled;
	bool trail_enabled;
	bool trail_shrink;
	float trail_duration;
	float trail_spacing;
	float trail_size_pct;
	float trail_opacity_pct;
	int trail_fade;
	uint32_t trail_points;
	float cursor_x;
	float cursor_y;
	bool trail_dirty;
	struct trail_point trail[TRAIL_POINTS_MAX];
	size_t trail_count;
	size_t trail_start;
	float last_pushed_x;
	float last_pushed_y;
	bool has_last_pushed;
};

static const char *mouse_overlay_get_name(void *type_data)
{
	UNUSED_PARAMETER(type_data);
	return obs_module_text("SourceName");
}

static void mouse_overlay_get_defaults(obs_data_t *settings)
{
	obs_data_set_default_double(settings, "cursor_size",
				   CURSOR_SIZE_DEFAULT);
	obs_data_set_default_bool(settings, "cursor_enabled", true);
	obs_data_set_default_int(settings, "trail_points",
				 (int)TRAIL_POINTS_DEFAULT);
	obs_data_set_default_double(settings, "trail_duration",
				   TRAIL_DURATION_DEFAULT);
	obs_data_set_default_double(settings, "trail_spacing",
				   TRAIL_SPACING_DEFAULT);
	obs_data_set_default_bool(settings, "trail_enabled", true);
	obs_data_set_default_bool(settings, "trail_shrink", true);
	obs_data_set_default_int(settings, "trail_size",
				 TRAIL_SIZE_DEFAULT_PCT);
	obs_data_set_default_int(settings, "trail_opacity",
				 TRAIL_OPACITY_DEFAULT_PCT);
	obs_data_set_default_int(settings, "trail_fade",
				 TRAIL_FADE_DEFAULT);
}

static obs_properties_t *mouse_overlay_get_properties(void *data)
{
	UNUSED_PARAMETER(data);

	obs_properties_t *props = obs_properties_create();
	obs_properties_t *cursor = obs_properties_create();
	obs_properties_add_group(props, "cursor_enabled",
				 obs_module_text("Cursor"),
				 OBS_GROUP_CHECKABLE, cursor);
	obs_properties_add_int(cursor, "cursor_size",
			       obs_module_text("CursorSize"),
			       (int)CURSOR_SIZE_MIN, (int)CURSOR_SIZE_MAX, 1);

	obs_properties_t *const trail = obs_properties_create();
	obs_properties_add_group(props, "trail_enabled",
				 obs_module_text("Trail"),
				 OBS_GROUP_CHECKABLE, trail);
	obs_properties_add_int(trail, "trail_points",
			       obs_module_text("TrailPoints"), 0,
			       (int)TRAIL_POINTS_MAX, 1);
	obs_properties_add_int(trail, "trail_size",
			       obs_module_text("TrailSize"),
			       TRAIL_SIZE_MIN_PCT, TRAIL_SIZE_MAX_PCT, 1);
	obs_properties_add_int(trail, "trail_opacity",
			       obs_module_text("TrailOpacity"), 0,
			       TRAIL_OPACITY_MAX_PCT, 1);
	obs_properties_add_float(trail, "trail_duration",
				obs_module_text("TrailDuration"),
				TRAIL_DURATION_MIN, TRAIL_DURATION_MAX, 0.1f);
	obs_properties_add_float(trail, "trail_spacing",
				obs_module_text("TrailSpacing"),
				TRAIL_SPACING_MIN, TRAIL_SPACING_MAX, 0.1f);
	obs_properties_add_bool(trail, "trail_shrink",
				obs_module_text("TrailShrink"));
	obs_property_t *fade = obs_properties_add_list(
		trail, "trail_fade", obs_module_text("TrailFade"),
		OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(fade, obs_module_text("TrailFadeLinear"),
				  TRAIL_FADE_LINEAR);
	obs_property_list_add_int(fade, obs_module_text("TrailFadeSmooth"),
				  TRAIL_FADE_SMOOTH);
	obs_property_list_add_int(
		fade, obs_module_text("TrailFadeExponential"),
		TRAIL_FADE_EXPONENTIAL);
	return props;
}

static void mouse_overlay_update(void *data, obs_data_t *settings)
{
	struct mouse_overlay_gg_data *shdata = data;
	shdata->cursor_size =
		(float)obs_data_get_double(settings, "cursor_size");
	if (!shdata->cursor_size)
		shdata->cursor_size = CURSOR_SIZE_DEFAULT;

	shdata->cursor_enabled = obs_data_get_bool(settings, "cursor_enabled");

	shdata->trail_enabled = obs_data_get_bool(settings, "trail_enabled");

	shdata->trail_shrink = obs_data_get_bool(settings, "trail_shrink");

	shdata->trail_points = (uint32_t)obs_data_get_int(settings, "trail_points");
	if (shdata->trail_points > TRAIL_POINTS_MAX)
		shdata->trail_points = TRAIL_POINTS_MAX;

	shdata->trail_size_pct = (float)obs_data_get_int(settings, "trail_size");
	shdata->trail_opacity_pct =
		(float)obs_data_get_int(settings, "trail_opacity");

	shdata->trail_fade = (int)obs_data_get_int(settings, "trail_fade");

	shdata->trail_duration = (float)obs_data_get_double(settings, "trail_duration");
	if (shdata->trail_duration <= 0.0f)
		shdata->trail_duration = TRAIL_DURATION_DEFAULT;

	shdata->trail_spacing =
		(float)obs_data_get_double(settings, "trail_spacing");
	if (shdata->trail_spacing <= 0.0f)
		shdata->trail_spacing = TRAIL_SPACING_DEFAULT;

	/* Apply buffer-affecting changes (enable/points) on the next tick. */
	shdata->trail_dirty = true;
}

static void *mouse_overlay_create(obs_data_t *settings, obs_source_t *source)
{
	UNUSED_PARAMETER(source);

	struct mouse_overlay_gg_data *data = bzalloc(sizeof(*data));

	/* Apply settings at creation so an old saved scene without the new
	 * keys works on first frame (defaults from mouse_overlay_update). */
	mouse_overlay_update(data, settings);

	char *path = obs_module_file("images/cursor.png");
	gs_image_file_init(&data->cursor_image, path);
	bfree(path);

	path = obs_module_file("images/curosr-trail.png");
	gs_image_file_init(&data->trail_image, path);
	bfree(path);

	path = obs_module_file("trail.effect");
	obs_enter_graphics();
	data->trail_effect = gs_effect_create_from_file(path, NULL);
	obs_leave_graphics();
	bfree(path);

	if (data->trail_effect) {
		data->trail_effect_image =
			gs_effect_get_param_by_name(data->trail_effect, "image");
		data->trail_effect_opacity =
			gs_effect_get_param_by_name(data->trail_effect, "opacity");
	} else {
		blog(LOG_ERROR, "Failed to load trail.effect");
	}

	return data;
}

static void mouse_overlay_destroy(void *data)
{
	struct mouse_overlay_gg_data *shdata = data;
	if (!shdata)
		return;

	obs_enter_graphics();
	gs_image_file_free(&shdata->cursor_image);
	gs_image_file_free(&shdata->trail_image);
	gs_effect_destroy(shdata->trail_effect);
	obs_leave_graphics();

	bfree(shdata);
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

static void mouse_overlay_video_tick(void *data, float seconds)
{
	UNUSED_PARAMETER(seconds);
	struct mouse_overlay_gg_data *shdata = data;

	/* Always track the cursor, even when the trail is disabled. */
	float nx;
	float ny;
	mouse_capture_sample_position(&nx, &ny);

	const float cursor_size = shdata->cursor_size;
	const float half = cursor_size / 2.0f;
	const float min = half;
	const float max_x = OVERLAY_WIDTH - half;
	const float max_y = OVERLAY_HEIGHT - half;

	shdata->cursor_x = fminf(fmaxf(nx * OVERLAY_WIDTH, min), max_x);
	shdata->cursor_y = fminf(fmaxf(ny * OVERLAY_HEIGHT, min), max_y);

	/* Settings changed since last tick: apply to the buffer now so the
	 * render thread never sees a half-updated ring. */
	if (shdata->trail_dirty) {
		shdata->trail_dirty = false;
		if (!shdata->trail_enabled || !shdata->trail_points) {
			shdata->trail_count = 0;
			shdata->trail_start = 0;
			shdata->has_last_pushed = false;
			return;
		}
		/* Reduce point limit immediately. */
		if (shdata->trail_count > shdata->trail_points) {
			const size_t drop = shdata->trail_count -
					    shdata->trail_points;
			shdata->trail_start =
				(shdata->trail_start + drop) %
				TRAIL_POINTS_MAX;
			shdata->trail_count = shdata->trail_points;
		}
	}

	/* Expire points older than the configured duration. */
	const uint64_t now_ns = os_gettime_ns();
	const int64_t duration_ns =
		(int64_t)(shdata->trail_duration * 1000000000.0);
	/* Back-date interpolated points across the whole frame time. */
	uint64_t layer_span_ns = (uint64_t)(seconds * 1e9f);
	if (layer_span_ns > (uint64_t)duration_ns)
		layer_span_ns = (uint64_t)duration_ns;

	while (shdata->trail_count > 0) {
		const struct trail_point *const pt =
			&shdata->trail[shdata->trail_start];
		if (now_ns - pt->time_ns >= (uint64_t)duration_ns) {
			shdata->trail_start =
				(shdata->trail_start + 1) % TRAIL_POINTS_MAX;
			shdata->trail_count--;
			continue;
		}
		break;
	}

	if (!shdata->trail_enabled || !shdata->trail_points)
		return;

	/* Real-distance spacing: place a point every `spacing` pixels walked.
	 * Interpolate so a fast frame fills the gap instead of leaving holes. */
	const float dx = shdata->cursor_x - shdata->last_pushed_x;
	const float dy = shdata->cursor_y - shdata->last_pushed_y;
	const float dist = sqrtf(dx * dx + dy * dy);
	const float spacing = shdata->trail_spacing;

	if (shdata->has_last_pushed && dist < spacing)
		return;

	int steps = 1;
	if (shdata->has_last_pushed && spacing > 0.0f)
		steps = (int)(dist / spacing);
	if (steps < 1)
		steps = 1;
	if (steps > (int)TRAIL_POINTS_MAX)
		steps = (int)TRAIL_POINTS_MAX;

	for (int i = 0; i < steps; i++) {
		const float f = (float)(i + 1) / (float)steps;
		const float px = shdata->last_pushed_x + dx * f;
		const float py = shdata->last_pushed_y + dy * f;

		/* Back-date each interpolated point so the whole jump fades out
		 * as if it happened across the frame. */
		const uint64_t point_time =
			now_ns - (uint64_t)((float)layer_span_ns * (1.0f - f));

		const size_t next =
			(shdata->trail_start + shdata->trail_count) %
			TRAIL_POINTS_MAX;
		struct trail_point *point = &shdata->trail[next];
		point->x = px;
		point->y = py;
		point->time_ns = point_time;

		if (shdata->trail_count >= shdata->trail_points &&
		    shdata->trail_points > 0) {
			shdata->trail_start =
				(shdata->trail_start + 1) % TRAIL_POINTS_MAX;
		} else if (shdata->trail_count < TRAIL_POINTS_MAX) {
			shdata->trail_count++;
		}
	}

	shdata->last_pushed_x = shdata->cursor_x;
	shdata->last_pushed_y = shdata->cursor_y;
	shdata->has_last_pushed = true;
}

static void mouse_overlay_draw_sprite(gs_eparam_t *image_param,
				      gs_eparam_t *opacity_param,
				      gs_texture_t *texture, float x, float y,
				      float cursor_size, float opacity)
{
	if (opacity_param)
		gs_effect_set_float(opacity_param, opacity);

	gs_effect_set_texture_srgb(image_param, texture);

	gs_matrix_push();
	gs_matrix_translate3f(x - cursor_size / 2.0f,
			      y - cursor_size / 2.0f, 0.0f);
	gs_draw_sprite(texture, 0, (uint32_t)cursor_size,
		       (uint32_t)cursor_size);
	gs_matrix_pop();
}

static void mouse_overlay_video_render(void *data, gs_effect_t *effect)
{
	UNUSED_PARAMETER(effect);
	struct mouse_overlay_gg_data *shdata = data;

	if (!shdata->cursor_image.texture && shdata->cursor_image.loaded)
		gs_image_file_init_texture(&shdata->cursor_image);
	if (!shdata->trail_image.texture && shdata->trail_image.loaded)
		gs_image_file_init_texture(&shdata->trail_image);

	gs_effect_t *default_effect = NULL;
	gs_technique_t *default_tech = NULL;
	gs_eparam_t *default_image_param = NULL;
	if (shdata->cursor_enabled && shdata->cursor_image.texture) {
		default_effect = obs_get_base_effect(OBS_EFFECT_DEFAULT);
		if (default_effect) {
			default_tech = gs_effect_get_technique(
				default_effect, "Draw");
			if (default_tech)
				default_image_param = gs_effect_get_param_by_name(
					default_effect, "image");
		}
	}

	if (!default_effect && !(shdata->trail_enabled && shdata->trail_image.texture))
		return;

	gs_blend_state_push();
	gs_blend_function(GS_BLEND_ONE, GS_BLEND_INVSRCALPHA);

	/* Draw the trail with per-point fade-out. */
	gs_effect_t *const trail_effect = shdata->trail_effect;
	if (trail_effect && shdata->trail_count > 0 &&
	    shdata->trail_points > 0) {
		gs_technique_t *const trail_tech =
			gs_effect_get_technique(trail_effect, "DrawTrail");
		if (trail_tech) {
			const uint64_t now_ns = os_gettime_ns();
			const float duration = shdata->trail_duration;

			const size_t trail_passes = gs_technique_begin(trail_tech);
			for (size_t i = 0; i < trail_passes; i++) {
				gs_technique_begin_pass(trail_tech, i);
				for (size_t j = 0; j < shdata->trail_count;
				     j++) {
					const size_t idx =
						(shdata->trail_start + j) %
						TRAIL_POINTS_MAX;
					const struct trail_point *const pt =
						&shdata->trail[idx];
					const float raw_age =
						(float)(now_ns - pt->time_ns) *
						1e-9f;
					float t = 1.0f - raw_age / duration;
					if (t <= 0.0f)
						continue;
					if (t > 1.0f)
						t = 1.0f;
					float fade = 0.0f;
					if (shdata->trail_fade ==
					    TRAIL_FADE_SMOOTH) {
						fade = t * t *
						       (3.0f - 2.0f * t);
					} else if (shdata->trail_fade ==
						   TRAIL_FADE_EXPONENTIAL) {
						fade = t * t;
					} else {
						fade = t;
					}
					const float opacity =
						fade * (shdata->trail_opacity_pct /
							100.0f);
					float trail_size = shdata->cursor_size *
					   (shdata->trail_size_pct / 100.0f);
					/* Shrink with age so the trail tapers
					 * out instead of ending abruptly. */
					if (shdata->trail_shrink)
						trail_size *= t;
					if (trail_size < 1.0f)
						continue;
					mouse_overlay_draw_sprite(
						shdata->trail_effect_image,
						shdata->trail_effect_opacity,
						shdata->trail_image.texture,
						pt->x, pt->y, trail_size,
						opacity);
				}
				gs_technique_end_pass(trail_tech);
			}
			gs_technique_end(trail_tech);
		}
	}

	if (shdata->cursor_enabled && default_tech) {
		/* Draw the cursor with full opacity. */
		const size_t default_passes = gs_technique_begin(default_tech);
		for (size_t i = 0; i < default_passes; i++) {
			gs_technique_begin_pass(default_tech, i);
			mouse_overlay_draw_sprite(default_image_param, NULL,
						  shdata->cursor_image.texture,
						  shdata->cursor_x,
						  shdata->cursor_y,
						  shdata->cursor_size, 1.0f);
			gs_technique_end_pass(default_tech);
		}
		gs_technique_end(default_tech);
		gs_effect_set_texture_srgb(default_image_param, NULL);
	}

	gs_blend_state_pop();
}

struct obs_source_info mouse_overlay_gg_source_info = {
	.id = "mouse_overlay_gg",
	.type = OBS_SOURCE_TYPE_INPUT,
	.output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_CUSTOM_DRAW |
			OBS_SOURCE_SRGB,
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
};

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE(PLUGIN_NAME, "en-US")

bool obs_module_load(void)
{
	obs_register_source(&mouse_overlay_gg_source_info);
	obs_log(LOG_INFO, "plugin loaded successfully (version %s)",
		PLUGIN_VERSION);
	return true;
}

void obs_module_unload(void)
{
	obs_log(LOG_INFO, "plugin unloaded");
}