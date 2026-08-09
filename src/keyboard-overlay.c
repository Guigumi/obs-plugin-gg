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
#include <util/platform.h>

#include "keyboard-capture.h"
#include "keyboard-overlay.h"

#include <math.h>
#include <stdint.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define KEY_SIZE_DEFAULT 64.0f
#define KEY_SIZE_MIN 24.0f
#define KEY_SIZE_MAX 256.0f
#define KEY_SPACING_DEFAULT 8.0f
#define KEY_SPACING_MIN 0.0f
#define KEY_SPACING_MAX 64.0f
#define KEY_IDLE_OPACITY_DEFAULT 35.0f
#define KEY_ACTIVE_OPACITY_DEFAULT 100.0f
#define KEY_PULSE_DURATION_DEFAULT 0.35f
#define KEY_PULSE_DURATION_MIN 0.1f
#define KEY_PULSE_DURATION_MAX 2.0f
#define KEY_PULSE_SCALE 0.15f

#define KEY_FEEDBACK_COLOR 0
#define KEY_FEEDBACK_PULSE 1
#define KEY_FEEDBACK_BOTH 2
#define KEY_FEEDBACK_DEFAULT KEY_FEEDBACK_BOTH

static const char *const idle_image_paths[KEYBOARD_KEY_COUNT] = {
	"images/key-W.png",
	"images/key-A.png",
	"images/key-S.png",
	"images/key-D.png",
};

static const char *const active_image_paths[KEYBOARD_KEY_COUNT] = {
	"images/key-W-active.png",
	"images/key-A-active.png",
	"images/key-S-active.png",
	"images/key-D-active.png",
};

struct keyboard_overlay_gg_data {
	gs_image_file_t idle_images[KEYBOARD_KEY_COUNT];
	gs_image_file_t active_images[KEYBOARD_KEY_COUNT];
	gs_effect_t *effect;
	gs_technique_t *effect_technique;
	gs_eparam_t *effect_image;
	gs_eparam_t *effect_opacity;

	bool enabled;
	bool arrow_aliases;
	float key_size;
	float spacing;
	float idle_opacity_pct;
	float active_opacity_pct;
	float pulse_duration;
	int feedback;
	bool pressed[KEYBOARD_KEY_COUNT];
	uint64_t press_time_ns[KEYBOARD_KEY_COUNT];
};

static const char *keyboard_overlay_get_name(void *type_data)
{
	UNUSED_PARAMETER(type_data);
	return obs_module_text("KeyboardSourceName");
}

static void keyboard_overlay_get_defaults(obs_data_t *settings)
{
	obs_data_set_default_bool(settings, "keyboard_enabled", true);
	obs_data_set_default_bool(settings, "keyboard_arrow_aliases", true);
	obs_data_set_default_double(settings, "keyboard_key_size", KEY_SIZE_DEFAULT);
	obs_data_set_default_double(settings, "keyboard_spacing", KEY_SPACING_DEFAULT);
	obs_data_set_default_double(settings, "keyboard_idle_opacity", KEY_IDLE_OPACITY_DEFAULT);
	obs_data_set_default_double(settings, "keyboard_active_opacity", KEY_ACTIVE_OPACITY_DEFAULT);
	obs_data_set_default_double(settings, "keyboard_pulse_duration", KEY_PULSE_DURATION_DEFAULT);
	obs_data_set_default_int(settings, "keyboard_feedback", KEY_FEEDBACK_DEFAULT);
}

static obs_properties_t *keyboard_overlay_get_properties(void *data)
{
	UNUSED_PARAMETER(data);

	obs_properties_t *props = obs_properties_create();
	obs_properties_t *keyboard = obs_properties_create();
	obs_properties_add_group(props, "keyboard_enabled", obs_module_text("Keyboard"), OBS_GROUP_CHECKABLE, keyboard);
	obs_properties_add_float_slider(keyboard, "keyboard_key_size", obs_module_text("KeyboardKeySize"), KEY_SIZE_MIN,
					KEY_SIZE_MAX, 1.0f);
	obs_properties_add_float_slider(keyboard, "keyboard_spacing", obs_module_text("KeyboardSpacing"),
					KEY_SPACING_MIN, KEY_SPACING_MAX, 1.0f);
	obs_properties_add_float_slider(keyboard, "keyboard_idle_opacity", obs_module_text("KeyboardIdleOpacity"), 0.0f,
					100.0f, 1.0f);
	obs_properties_add_float_slider(keyboard, "keyboard_active_opacity", obs_module_text("KeyboardActiveOpacity"),
					0.0f, 100.0f, 1.0f);
	obs_properties_add_float_slider(keyboard, "keyboard_pulse_duration", obs_module_text("KeyboardPulseDuration"),
					KEY_PULSE_DURATION_MIN, KEY_PULSE_DURATION_MAX, 0.05f);
	obs_properties_add_bool(keyboard, "keyboard_arrow_aliases", obs_module_text("KeyboardArrowAliases"));

	obs_property_t *feedback = obs_properties_add_list(keyboard, "keyboard_feedback",
							   obs_module_text("KeyboardFeedback"), OBS_COMBO_TYPE_LIST,
							   OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(feedback, obs_module_text("KeyboardFeedbackColor"), KEY_FEEDBACK_COLOR);
	obs_property_list_add_int(feedback, obs_module_text("KeyboardFeedbackPulse"), KEY_FEEDBACK_PULSE);
	obs_property_list_add_int(feedback, obs_module_text("KeyboardFeedbackBoth"), KEY_FEEDBACK_BOTH);
	return props;
}

static void keyboard_overlay_update(void *data, obs_data_t *settings)
{
	struct keyboard_overlay_gg_data *keyboard = data;
	keyboard->enabled = obs_data_get_bool(settings, "keyboard_enabled");
	keyboard->arrow_aliases = obs_data_get_bool(settings, "keyboard_arrow_aliases");
	keyboard->key_size = (float)obs_data_get_double(settings, "keyboard_key_size");
	keyboard->spacing = (float)obs_data_get_double(settings, "keyboard_spacing");
	keyboard->idle_opacity_pct = (float)obs_data_get_double(settings, "keyboard_idle_opacity");
	keyboard->active_opacity_pct = (float)obs_data_get_double(settings, "keyboard_active_opacity");
	keyboard->pulse_duration = (float)obs_data_get_double(settings, "keyboard_pulse_duration");
	keyboard->feedback = (int)obs_data_get_int(settings, "keyboard_feedback");

	if (!isfinite(keyboard->key_size))
		keyboard->key_size = KEY_SIZE_DEFAULT;
	keyboard->key_size = fminf(fmaxf(keyboard->key_size, KEY_SIZE_MIN), KEY_SIZE_MAX);
	if (!isfinite(keyboard->spacing))
		keyboard->spacing = KEY_SPACING_DEFAULT;
	keyboard->spacing = fminf(fmaxf(keyboard->spacing, KEY_SPACING_MIN), KEY_SPACING_MAX);
	if (!isfinite(keyboard->idle_opacity_pct))
		keyboard->idle_opacity_pct = KEY_IDLE_OPACITY_DEFAULT;
	keyboard->idle_opacity_pct = fminf(fmaxf(keyboard->idle_opacity_pct, 0.0f), 100.0f);
	if (!isfinite(keyboard->active_opacity_pct))
		keyboard->active_opacity_pct = KEY_ACTIVE_OPACITY_DEFAULT;
	keyboard->active_opacity_pct = fminf(fmaxf(keyboard->active_opacity_pct, 0.0f), 100.0f);
	if (!isfinite(keyboard->pulse_duration))
		keyboard->pulse_duration = KEY_PULSE_DURATION_DEFAULT;
	keyboard->pulse_duration =
		fminf(fmaxf(keyboard->pulse_duration, KEY_PULSE_DURATION_MIN), KEY_PULSE_DURATION_MAX);
	if (keyboard->feedback < KEY_FEEDBACK_COLOR || keyboard->feedback > KEY_FEEDBACK_BOTH)
		keyboard->feedback = KEY_FEEDBACK_DEFAULT;
	if (!keyboard->enabled) {
		memset(keyboard->pressed, 0, sizeof(keyboard->pressed));
		memset(keyboard->press_time_ns, 0, sizeof(keyboard->press_time_ns));
	}
}

static void *keyboard_overlay_create(obs_data_t *settings, obs_source_t *source)
{
	UNUSED_PARAMETER(source);
	struct keyboard_overlay_gg_data *data = bzalloc(sizeof(*data));
	keyboard_overlay_update(data, settings);

	for (size_t i = 0; i < KEYBOARD_KEY_COUNT; i++) {
		char *path = obs_module_file(idle_image_paths[i]);
		gs_image_file_init(&data->idle_images[i], path);
		bfree(path);

		path = obs_module_file(active_image_paths[i]);
		gs_image_file_init(&data->active_images[i], path);
		bfree(path);
	}

	char *path = obs_module_file("keyboard.effect");
	char *effect_errors = NULL;
	obs_enter_graphics();
	data->effect = gs_effect_create_from_file(path, &effect_errors);
	obs_leave_graphics();
	bfree(path);

	if (data->effect) {
		data->effect_technique = gs_effect_get_technique(data->effect, "DrawKeyboard");
		data->effect_image = gs_effect_get_param_by_name(data->effect, "image");
		data->effect_opacity = gs_effect_get_param_by_name(data->effect, "opacity");
		if (!data->effect_technique || !data->effect_image || !data->effect_opacity)
			blog(LOG_ERROR, "keyboard.effect is missing required entries");
	} else {
		blog(LOG_ERROR, "Failed to load keyboard.effect: %s", effect_errors ? effect_errors : "unknown error");
	}
	bfree(effect_errors);
	return data;
}

static void keyboard_overlay_destroy(void *data)
{
	struct keyboard_overlay_gg_data *keyboard = data;
	if (!keyboard)
		return;

	obs_enter_graphics();
	for (size_t i = 0; i < KEYBOARD_KEY_COUNT; i++) {
		gs_image_file_free(&keyboard->idle_images[i]);
		gs_image_file_free(&keyboard->active_images[i]);
	}
	gs_effect_destroy(keyboard->effect);
	obs_leave_graphics();
	bfree(keyboard);
}

static uint32_t keyboard_overlay_get_width(void *data)
{
	const struct keyboard_overlay_gg_data *keyboard = data;
	const float pulse_padding = keyboard->key_size * KEY_PULSE_SCALE / 2.0f;
	return (uint32_t)ceilf(keyboard->key_size * 3.0f + keyboard->spacing * 2.0f + pulse_padding * 2.0f);
}

static uint32_t keyboard_overlay_get_height(void *data)
{
	const struct keyboard_overlay_gg_data *keyboard = data;
	const float pulse_padding = keyboard->key_size * KEY_PULSE_SCALE / 2.0f;
	return (uint32_t)ceilf(keyboard->key_size * 2.0f + keyboard->spacing + pulse_padding * 2.0f);
}

static void keyboard_overlay_video_tick(void *data, float seconds)
{
	UNUSED_PARAMETER(seconds);
	struct keyboard_overlay_gg_data *keyboard = data;
	if (!keyboard->enabled)
		return;

	bool pressed[KEYBOARD_KEY_COUNT];
	keyboard_capture_sample_wasd(keyboard->arrow_aliases, pressed);
	const uint64_t now_ns = os_gettime_ns();
	for (size_t i = 0; i < KEYBOARD_KEY_COUNT; i++) {
		if (pressed[i] && !keyboard->pressed[i])
			keyboard->press_time_ns[i] = now_ns;
		keyboard->pressed[i] = pressed[i];
	}
}

static void keyboard_overlay_draw_key(gs_eparam_t *image_param, gs_eparam_t *opacity_param, gs_texture_t *texture,
				      float x, float y, float size, float opacity)
{
	gs_effect_set_texture_srgb(image_param, texture);
	gs_effect_set_float(opacity_param, opacity);
	gs_matrix_push();
	gs_matrix_translate3f(x - size / 2.0f, y - size / 2.0f, 0.0f);
	gs_draw_sprite(texture, 0, (uint32_t)size, (uint32_t)size);
	gs_matrix_pop();
}

static void keyboard_overlay_video_render(void *data, gs_effect_t *effect)
{
	UNUSED_PARAMETER(effect);
	struct keyboard_overlay_gg_data *keyboard = data;
	if (!keyboard->enabled || !keyboard->effect || !keyboard->effect_technique || !keyboard->effect_image ||
	    !keyboard->effect_opacity)
		return;

	for (size_t i = 0; i < KEYBOARD_KEY_COUNT; i++) {
		if (!keyboard->idle_images[i].texture && keyboard->idle_images[i].loaded)
			gs_image_file_init_texture(&keyboard->idle_images[i]);
		if (!keyboard->active_images[i].texture && keyboard->active_images[i].loaded)
			gs_image_file_init_texture(&keyboard->active_images[i]);
	}

	gs_technique_t *technique = keyboard->effect_technique;
	if (!technique)
		return;

	const float step = keyboard->key_size + keyboard->spacing;
	const float pulse_padding = keyboard->key_size * KEY_PULSE_SCALE / 2.0f;
	const float positions[KEYBOARD_KEY_COUNT][2] = {
		{pulse_padding + step + keyboard->key_size / 2.0f, pulse_padding + keyboard->key_size / 2.0f},
		{pulse_padding + keyboard->key_size / 2.0f, pulse_padding + step + keyboard->key_size / 2.0f},
		{pulse_padding + step + keyboard->key_size / 2.0f, pulse_padding + step + keyboard->key_size / 2.0f},
		{pulse_padding + step * 2.0f + keyboard->key_size / 2.0f,
		 pulse_padding + step + keyboard->key_size / 2.0f},
	};
	const float idle_opacity = keyboard->idle_opacity_pct / 100.0f;
	const float active_opacity = keyboard->active_opacity_pct / 100.0f;
	const uint64_t now_ns = os_gettime_ns();

	gs_blend_state_push();
	gs_blend_function(GS_BLEND_ONE, GS_BLEND_INVSRCALPHA);
	const size_t passes = gs_technique_begin(technique);
	for (size_t pass = 0; pass < passes; pass++) {
		gs_technique_begin_pass(technique, pass);
		for (size_t key = 0; key < KEYBOARD_KEY_COUNT; key++) {
			gs_texture_t *idle = keyboard->idle_images[key].texture;
			const bool use_color = keyboard->feedback != KEY_FEEDBACK_PULSE;
			const bool use_pulse = keyboard->feedback != KEY_FEEDBACK_COLOR;
			const float age = keyboard->press_time_ns[key]
						  ? (float)(now_ns - keyboard->press_time_ns[key]) * 1e-9f
						  : keyboard->pulse_duration;
			const bool pulse_active = use_pulse && age < keyboard->pulse_duration;
			const bool color_active = use_color && keyboard->pressed[key];
			float scale = 1.0f;
			if (pulse_active) {
				const float t = age / keyboard->pulse_duration;
				scale += KEY_PULSE_SCALE * sinf((float)M_PI * t);
			}

			const bool active = pulse_active || color_active;
			gs_texture_t *texture = active && use_color ? keyboard->active_images[key].texture : idle;
			if (!texture)
				texture = idle;
			const float opacity = active ? active_opacity : idle_opacity;
			if (texture && opacity > 0.0f)
				keyboard_overlay_draw_key(keyboard->effect_image, keyboard->effect_opacity, texture,
							  positions[key][0], positions[key][1],
							  keyboard->key_size * scale, opacity);
		}
		gs_technique_end_pass(technique);
	}
	gs_technique_end(technique);
	gs_effect_set_texture_srgb(keyboard->effect_image, NULL);
	gs_blend_state_pop();
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
};
