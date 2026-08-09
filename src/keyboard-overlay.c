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
#include <graphics/matrix4.h>
#include <graphics/vec4.h>
#include <util/platform.h>

#include <windows.h>

#include "keyboard-capture.h"
#include "keyboard-overlay.h"

#include <math.h>
#include <stdint.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define KEY_SIZE_DEFAULT 64.0f
#define KEY_SIZE_MIN 8.0f
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
#define KEY_ROTATION_DEFAULT 0.0f
#define KEY_ROTATION_MIN -180.0f
#define KEY_ROTATION_MAX 180.0f
#define KEY_TINT_COLOR_DEFAULT 0xFFFFFF
#define KEY_LABEL_INACTIVE_COLOR 0xFFFFFF
#define KEY_LABEL_ACTIVE_COLOR 0x202020
#define KEY_LABEL_FONT_SIZE_DEFAULT 36
#define KEY_LABEL_REFERENCE_SIZE 64.0f
#define KEY_LABEL_SUPERSAMPLE 4u

#define KEY_FEEDBACK_COLOR 0
#define KEY_FEEDBACK_PULSE 1
#define KEY_FEEDBACK_BOTH 2
#define KEY_FEEDBACK_DEFAULT KEY_FEEDBACK_BOTH

static const char *const key_character_settings[KEYBOARD_KEY_COUNT] = {
	"keyboard_character_w",
	"keyboard_character_a",
	"keyboard_character_s",
	"keyboard_character_d",
};

struct keyboard_overlay_gg_data {
	gs_image_file_t inactive_image;
	gs_image_file_t active_image;
	gs_texture_t *label_textures[KEYBOARD_KEY_COUNT];
	gs_effect_t *effect;
	gs_technique_t *effect_technique;
	gs_eparam_t *effect_image;
	gs_eparam_t *effect_opacity;
	gs_eparam_t *effect_tint;

	bool enabled;
	bool arrow_aliases;
	float key_size;
	float spacing;
	float idle_opacity_pct;
	float active_opacity_pct;
	float pulse_duration;
	float rotation_deg;
	bool tint_enabled;
	uint32_t tint_color;
	int feedback;
	bool pressed[KEYBOARD_KEY_COUNT];
	uint64_t press_time_ns[KEYBOARD_KEY_COUNT];
};

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
	obs_data_t *font = obs_data_create();
	obs_data_set_default_string(font, "face", "Arial");
	obs_data_set_default_int(font, "size", KEY_LABEL_FONT_SIZE_DEFAULT);
	obs_data_set_default_int(font, "flags", OBS_FONT_BOLD);
	obs_data_set_default_obj(settings, "keyboard_font", font);
	obs_data_release(font);

	obs_data_set_default_bool(settings, "keyboard_enabled", true);
	obs_data_set_default_bool(settings, "keyboard_arrow_aliases", true);
	obs_data_set_default_double(settings, "keyboard_key_size", KEY_SIZE_DEFAULT);
	obs_data_set_default_double(settings, "keyboard_spacing", KEY_SPACING_DEFAULT);
	obs_data_set_default_double(settings, "keyboard_idle_opacity", KEY_IDLE_OPACITY_DEFAULT);
	obs_data_set_default_double(settings, "keyboard_active_opacity", KEY_ACTIVE_OPACITY_DEFAULT);
	obs_data_set_default_double(settings, "keyboard_pulse_duration", KEY_PULSE_DURATION_DEFAULT);
	obs_data_set_default_double(settings, "keyboard_rotation", KEY_ROTATION_DEFAULT);
	obs_data_set_default_bool(settings, "keyboard_tint_enabled", false);
	obs_data_set_default_int(settings, "keyboard_tint_color", KEY_TINT_COLOR_DEFAULT);
	obs_data_set_default_int(settings, "keyboard_feedback", KEY_FEEDBACK_DEFAULT);
	obs_data_set_default_string(settings, "keyboard_character_w", "W");
	obs_data_set_default_string(settings, "keyboard_character_a", "A");
	obs_data_set_default_string(settings, "keyboard_character_s", "S");
	obs_data_set_default_string(settings, "keyboard_character_d", "D");
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
	obs_properties_add_float_slider(keyboard, "keyboard_rotation", obs_module_text("KeyboardRotation"),
					KEY_ROTATION_MIN, KEY_ROTATION_MAX, 1.0f);
	obs_properties_add_float_slider(keyboard, "keyboard_idle_opacity", obs_module_text("KeyboardIdleOpacity"), 0.0f,
					100.0f, 1.0f);
	obs_properties_add_float_slider(keyboard, "keyboard_active_opacity", obs_module_text("KeyboardActiveOpacity"),
					0.0f, 100.0f, 1.0f);
	obs_properties_add_float_slider(keyboard, "keyboard_pulse_duration", obs_module_text("KeyboardPulseDuration"),
					KEY_PULSE_DURATION_MIN, KEY_PULSE_DURATION_MAX, 0.05f);
	obs_properties_add_bool(keyboard, "keyboard_arrow_aliases", obs_module_text("KeyboardArrowAliases"));
	obs_properties_add_bool(keyboard, "keyboard_tint_enabled", obs_module_text("KeyboardTintEnable"));
	obs_properties_add_color(keyboard, "keyboard_tint_color", obs_module_text("KeyboardTintColor"));
	obs_properties_add_font(keyboard, "keyboard_font", obs_module_text("KeyboardFont"));
	obs_properties_add_text(keyboard, "keyboard_character_w", obs_module_text("KeyboardCharacterW"),
				OBS_TEXT_DEFAULT);
	obs_properties_add_text(keyboard, "keyboard_character_a", obs_module_text("KeyboardCharacterA"),
				OBS_TEXT_DEFAULT);
	obs_properties_add_text(keyboard, "keyboard_character_s", obs_module_text("KeyboardCharacterS"),
				OBS_TEXT_DEFAULT);
	obs_properties_add_text(keyboard, "keyboard_character_d", obs_module_text("KeyboardCharacterD"),
				OBS_TEXT_DEFAULT);

	obs_property_t *feedback = obs_properties_add_list(keyboard, "keyboard_feedback",
							   obs_module_text("KeyboardFeedback"), OBS_COMBO_TYPE_LIST,
							   OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(feedback, obs_module_text("KeyboardFeedbackColor"), KEY_FEEDBACK_COLOR);
	obs_property_list_add_int(feedback, obs_module_text("KeyboardFeedbackPulse"), KEY_FEEDBACK_PULSE);
	obs_property_list_add_int(feedback, obs_module_text("KeyboardFeedbackBoth"), KEY_FEEDBACK_BOTH);
	return props;
}

static gs_texture_t *keyboard_overlay_create_label_texture(const char *text, obs_data_t *font, float key_size)
{
	if (!text || !*text || !font || key_size <= 0.0f)
		return NULL;
	const uint32_t texture_size = (uint32_t)ceilf(key_size * KEY_LABEL_SUPERSAMPLE);

	wchar_t *wide_text = NULL;
	wchar_t *wide_face = NULL;
	const char *face = obs_data_get_string(font, "face");
	if (!os_utf8_to_wcs_ptr(text, 0, &wide_text) || !os_utf8_to_wcs_ptr(face, 0, &wide_face)) {
		bfree(wide_text);
		bfree(wide_face);
		return NULL;
	}

	BITMAPINFO bitmap_info = {0};
	bitmap_info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bitmap_info.bmiHeader.biWidth = (LONG)texture_size;
	bitmap_info.bmiHeader.biHeight = -(LONG)texture_size;
	bitmap_info.bmiHeader.biPlanes = 1;
	bitmap_info.bmiHeader.biBitCount = 32;
	bitmap_info.bmiHeader.biCompression = BI_RGB;

	void *pixels = NULL;
	HDC dc = CreateCompatibleDC(NULL);
	HBITMAP bitmap = dc ? CreateDIBSection(dc, &bitmap_info, DIB_RGB_COLORS, &pixels, NULL, 0) : NULL;
	const int configured_font_size = (int)obs_data_get_int(font, "size");
	const float relative_font_size =
		(configured_font_size > 0 ? configured_font_size : KEY_LABEL_FONT_SIZE_DEFAULT) * key_size /
		KEY_LABEL_REFERENCE_SIZE;
	const int font_size = (int)fmaxf(1.0f, roundf(relative_font_size * KEY_LABEL_SUPERSAMPLE));
	const int64_t font_flags = obs_data_get_int(font, "flags");
	HFONT gdi_font = CreateFontW(-font_size, 0, 0, 0, (font_flags & OBS_FONT_BOLD) ? FW_BOLD : FW_NORMAL,
				     (font_flags & OBS_FONT_ITALIC) != 0, (font_flags & OBS_FONT_UNDERLINE) != 0,
				     (font_flags & OBS_FONT_STRIKEOUT) != 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
				     CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, wide_face);
	gs_texture_t *texture = NULL;
	if (dc && bitmap && pixels && gdi_font) {
		HGDIOBJ old_bitmap = SelectObject(dc, bitmap);
		HGDIOBJ old_font = SelectObject(dc, gdi_font);
		PatBlt(dc, 0, 0, (int)texture_size, (int)texture_size, BLACKNESS);
		SetBkMode(dc, OPAQUE);
		SetBkColor(dc, RGB(0, 0, 0));
		SetTextColor(dc, RGB(255, 255, 255));
		RECT rect = {0, 0, (LONG)texture_size, (LONG)texture_size};
		DrawTextW(dc, wide_text, -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

		uint8_t *pixel = pixels;
		for (size_t i = 0; i < (size_t)texture_size * texture_size; i++, pixel += 4) {
			uint8_t coverage = pixel[0] > pixel[1] ? pixel[0] : pixel[1];
			if (pixel[2] > coverage)
				coverage = pixel[2];
			pixel[0] = 255;
			pixel[1] = 255;
			pixel[2] = 255;
			pixel[3] = coverage;
		}

		const uint8_t *levels[] = {pixels};
		obs_enter_graphics();
		texture = gs_texture_create(texture_size, texture_size, GS_BGRA, 1, levels, 0);
		obs_leave_graphics();
		SelectObject(dc, old_font);
		SelectObject(dc, old_bitmap);
	}

	if (gdi_font)
		DeleteObject(gdi_font);
	if (bitmap)
		DeleteObject(bitmap);
	if (dc)
		DeleteDC(dc);
	bfree(wide_text);
	bfree(wide_face);
	return texture;
}

static void keyboard_overlay_update_labels(struct keyboard_overlay_gg_data *keyboard, obs_data_t *settings)
{
	obs_data_t *font = obs_data_get_obj(settings, "keyboard_font");
	for (size_t i = 0; i < KEYBOARD_KEY_COUNT; i++) {
		const char *text = obs_data_get_string(settings, key_character_settings[i]);
		gs_texture_t *texture = keyboard_overlay_create_label_texture(text, font, keyboard->key_size);
		obs_enter_graphics();
		gs_texture_destroy(keyboard->label_textures[i]);
		keyboard->label_textures[i] = texture;
		obs_leave_graphics();
		if (*text && !texture)
			blog(LOG_WARNING, "Failed to render keyboard character: %s", text);
	}
	obs_data_release(font);
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
	keyboard->rotation_deg = (float)obs_data_get_double(settings, "keyboard_rotation");
	keyboard->tint_enabled = obs_data_get_bool(settings, "keyboard_tint_enabled");
	keyboard->tint_color = (uint32_t)obs_data_get_int(settings, "keyboard_tint_color");
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
	if (!isfinite(keyboard->rotation_deg))
		keyboard->rotation_deg = KEY_ROTATION_DEFAULT;
	keyboard->rotation_deg = fminf(fmaxf(keyboard->rotation_deg, KEY_ROTATION_MIN), KEY_ROTATION_MAX);
	if (keyboard->feedback < KEY_FEEDBACK_COLOR || keyboard->feedback > KEY_FEEDBACK_BOTH)
		keyboard->feedback = KEY_FEEDBACK_DEFAULT;
	keyboard_overlay_update_labels(keyboard, settings);
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

	char *path = obs_module_file("images/key-inactive.png");
	gs_image_file_init(&data->inactive_image, path);
	if (!data->inactive_image.loaded)
		blog(LOG_WARNING, "Failed to load image: %s", path);
	bfree(path);

	path = obs_module_file("images/key-active.png");
	gs_image_file_init(&data->active_image, path);
	if (!data->active_image.loaded)
		blog(LOG_WARNING, "Failed to load image: %s", path);
	bfree(path);

	path = obs_module_file("keyboard.effect");
	char *effect_errors = NULL;
	obs_enter_graphics();
	data->effect = gs_effect_create_from_file(path, &effect_errors);
	obs_leave_graphics();
	bfree(path);

	if (data->effect) {
		data->effect_technique = gs_effect_get_technique(data->effect, "DrawKeyboard");
		data->effect_image = gs_effect_get_param_by_name(data->effect, "image");
		data->effect_opacity = gs_effect_get_param_by_name(data->effect, "opacity");
		data->effect_tint = gs_effect_get_param_by_name(data->effect, "tint");
		if (!data->effect_technique || !data->effect_image || !data->effect_opacity || !data->effect_tint)
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
	gs_image_file_free(&keyboard->inactive_image);
	gs_image_file_free(&keyboard->active_image);
	for (size_t i = 0; i < KEYBOARD_KEY_COUNT; i++)
		gs_texture_destroy(keyboard->label_textures[i]);
	gs_effect_destroy(keyboard->effect);
	obs_leave_graphics();
	bfree(keyboard);
}

static void keyboard_overlay_get_dimensions(const struct keyboard_overlay_gg_data *keyboard, float *width,
					    float *height)
{
	const float pulse_padding = keyboard->key_size * KEY_PULSE_SCALE / 2.0f;
	const float base_width = keyboard->key_size * 3.0f + keyboard->spacing * 2.0f + pulse_padding * 2.0f;
	const float base_height = keyboard->key_size * 2.0f + keyboard->spacing + pulse_padding * 2.0f;
	const float radians = keyboard->rotation_deg * (float)M_PI / 180.0f;
	const float cosine = fabsf(cosf(radians));
	const float sine = fabsf(sinf(radians));
	*width = base_width * cosine + base_height * sine;
	*height = base_width * sine + base_height * cosine;
}

static uint32_t keyboard_overlay_get_width(void *data)
{
	const struct keyboard_overlay_gg_data *keyboard = data;
	float width;
	float height;
	keyboard_overlay_get_dimensions(keyboard, &width, &height);
	UNUSED_PARAMETER(height);
	return (uint32_t)ceilf(width);
}

static uint32_t keyboard_overlay_get_height(void *data)
{
	const struct keyboard_overlay_gg_data *keyboard = data;
	float width;
	float height;
	keyboard_overlay_get_dimensions(keyboard, &width, &height);
	UNUSED_PARAMETER(width);
	return (uint32_t)ceilf(height);
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
	    !keyboard->effect_opacity || !keyboard->effect_tint)
		return;

	if (!keyboard->inactive_image.texture && keyboard->inactive_image.loaded)
		gs_image_file_init_texture(&keyboard->inactive_image);
	if (!keyboard->active_image.texture && keyboard->active_image.loaded)
		gs_image_file_init_texture(&keyboard->active_image);

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
	float scales[KEYBOARD_KEY_COUNT];
	float opacities[KEYBOARD_KEY_COUNT];
	bool active_states[KEYBOARD_KEY_COUNT];
	for (size_t key = 0; key < KEYBOARD_KEY_COUNT; key++) {
		const bool use_color = keyboard->feedback != KEY_FEEDBACK_PULSE;
		const bool use_pulse = keyboard->feedback != KEY_FEEDBACK_COLOR;
		const float age = keyboard->press_time_ns[key] ? (float)(now_ns - keyboard->press_time_ns[key]) * 1e-9f
							       : keyboard->pulse_duration;
		const bool pulse_active = use_pulse && age < keyboard->pulse_duration;
		const bool color_active = use_color && keyboard->pressed[key];
		scales[key] = 1.0f;
		if (pulse_active) {
			const float t = age / keyboard->pulse_duration;
			scales[key] += KEY_PULSE_SCALE * sinf((float)M_PI * t);
		}
		active_states[key] = use_color && (pulse_active || color_active);
		opacities[key] = (pulse_active || color_active) ? active_opacity : idle_opacity;
	}

	struct vec4 tint;
	vec4_from_rgba(&tint, (keyboard->tint_enabled ? keyboard->tint_color : KEY_TINT_COLOR_DEFAULT) | 0xFF000000);
	gs_effect_set_vec4(keyboard->effect_tint, &tint);

	const float base_width = keyboard->key_size * 3.0f + keyboard->spacing * 2.0f + pulse_padding * 2.0f;
	const float base_height = keyboard->key_size * 2.0f + keyboard->spacing + pulse_padding * 2.0f;
	float output_width;
	float output_height;
	keyboard_overlay_get_dimensions(keyboard, &output_width, &output_height);
	struct matrix4 identity;
	struct matrix4 rotation;
	matrix4_identity(&identity);
	matrix4_rotate_aa4f(&rotation, &identity, 0.0f, 0.0f, 1.0f, keyboard->rotation_deg * (float)M_PI / 180.0f);

	gs_blend_state_push();
	gs_blend_function(GS_BLEND_ONE, GS_BLEND_INVSRCALPHA);
	gs_matrix_push();
	gs_matrix_translate3f(output_width / 2.0f, output_height / 2.0f, 0.0f);
	gs_matrix_mul(&rotation);
	gs_matrix_translate3f(-base_width / 2.0f, -base_height / 2.0f, 0.0f);
	const size_t passes = gs_technique_begin(technique);
	for (size_t pass = 0; pass < passes; pass++) {
		gs_technique_begin_pass(technique, pass);
		for (size_t key = 0; key < KEYBOARD_KEY_COUNT; key++) {
			gs_texture_t *texture = active_states[key] ? keyboard->active_image.texture
								   : keyboard->inactive_image.texture;
			if (!texture)
				texture = keyboard->inactive_image.texture;
			if (texture && opacities[key] > 0.0f)
				keyboard_overlay_draw_key(keyboard->effect_image, keyboard->effect_opacity, texture,
							  positions[key][0], positions[key][1],
							  keyboard->key_size * scales[key], opacities[key]);
		}
		gs_technique_end_pass(technique);
	}
	gs_technique_end(technique);

	const size_t label_passes = gs_technique_begin(technique);
	for (size_t pass = 0; pass < label_passes; pass++) {
		gs_technique_begin_pass(technique, pass);
		for (size_t key = 0; key < KEYBOARD_KEY_COUNT; key++) {
			if (!keyboard->label_textures[key] || opacities[key] <= 0.0f)
				continue;
			vec4_from_rgba(&tint, (active_states[key] ? KEY_LABEL_ACTIVE_COLOR : KEY_LABEL_INACTIVE_COLOR) |
						      0xFF000000);
			gs_effect_set_vec4(keyboard->effect_tint, &tint);
			keyboard_overlay_draw_key(keyboard->effect_image, keyboard->effect_opacity,
						  keyboard->label_textures[key], positions[key][0], positions[key][1],
						  keyboard->key_size * scales[key], opacities[key]);
		}
		gs_technique_end_pass(technique);
	}
	gs_technique_end(technique);
	gs_matrix_pop();
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
	.icon_type = OBS_ICON_TYPE_CUSTOM,
	.get_dark_icon = keyboard_overlay_get_dark_icon,
	.get_light_icon = keyboard_overlay_get_light_icon,
};
