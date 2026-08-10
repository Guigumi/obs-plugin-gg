#include "keyboard-overlay-resources.h"

#include "keyboard-overlay-keys.h"
#include "keyboard-overlay-layout.h"

#include <graphics/matrix4.h>
#include <graphics/vec4.h>
#include <util/platform.h>

#include <windows.h>

#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define KEY_EFFECT_SIZE_RATIO (54.0f / 64.0f)
#define KEY_LABEL_COLOR_DARK 0x202020
#define KEY_LABEL_COLOR_LIGHT 0xFFFFFF
#define KEY_LABEL_FONT_SIZE_DEFAULT 36
#define KEY_LABEL_REFERENCE_SIZE 64.0f
#define KEY_LABEL_SUPERSAMPLE_MAX 8u
#define KEY_LABEL_TEXTURE_MAX 512u

static gs_texture_t *keyboard_resources_create_label_texture(const char *text, obs_data_t *font, float key_size)
{
	if (!text || !*text || !font || key_size <= 0.0f)
		return NULL;
	uint32_t supersample = 1u;
	while (supersample < KEY_LABEL_SUPERSAMPLE_MAX &&
	       (uint32_t)ceilf(key_size * (float)(supersample + 1)) <= KEY_LABEL_TEXTURE_MAX)
		supersample++;
	const uint32_t texture_size = (uint32_t)ceilf(key_size * supersample);

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
	const int font_size = (int)fmaxf(1.0f, roundf(relative_font_size * (float)supersample));
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

void keyboard_resources_update_labels(struct keyboard_overlay_gg_data *keyboard, obs_data_t *settings)
{
	obs_data_t *font = obs_data_get_obj(settings, "keyboard_font");
	for (size_t i = 0; i < KEYBOARD_KEY_COUNT; i++) {
		struct keyboard_overlay_key_data *key = &keyboard->keys[i];
		const char *text = obs_data_get_string(settings, key->character_setting);
		gs_texture_t *texture = keyboard_resources_create_label_texture(text, font, keyboard->key_size);
		obs_enter_graphics();
		gs_texture_destroy(key->label_texture);
		key->label_texture = texture;
		obs_leave_graphics();
		if (*text && !texture)
			blog(LOG_WARNING, "Failed to render keyboard character: %s", text);
	}
	obs_data_release(font);
}

void keyboard_resources_init(struct keyboard_overlay_gg_data *keyboard)
{
	char *path = obs_module_file("images/key-main.png");
	gs_image_file_init(&keyboard->main_image, path);
	if (!keyboard->main_image.loaded)
		blog(LOG_WARNING, "Failed to load image: %s", path);
	bfree(path);

	path = obs_module_file("keyboard.effect");
	char *effect_errors = NULL;
	obs_enter_graphics();
	keyboard->effect = gs_effect_create_from_file(path, &effect_errors);
	obs_leave_graphics();
	bfree(path);

	if (keyboard->effect) {
		keyboard->effect_technique = gs_effect_get_technique(keyboard->effect, "DrawKeyboard");
		keyboard->effect_circle_technique = gs_effect_get_technique(keyboard->effect, "DrawKeyboardCircle");
		keyboard->effect_border_technique = gs_effect_get_technique(keyboard->effect, "DrawKeyboardBorder");
		keyboard->effect_image = gs_effect_get_param_by_name(keyboard->effect, "image");
		keyboard->effect_opacity = gs_effect_get_param_by_name(keyboard->effect, "opacity");
		keyboard->effect_tint = gs_effect_get_param_by_name(keyboard->effect, "tint");
		if (!keyboard->effect_technique || !keyboard->effect_circle_technique ||
		    !keyboard->effect_border_technique || !keyboard->effect_image || !keyboard->effect_opacity ||
		    !keyboard->effect_tint)
			blog(LOG_ERROR, "keyboard.effect is missing required entries");
	} else {
		blog(LOG_ERROR, "Failed to load keyboard.effect: %s", effect_errors ? effect_errors : "unknown error");
	}
	bfree(effect_errors);
}

void keyboard_resources_free(struct keyboard_overlay_gg_data *keyboard)
{
	obs_enter_graphics();
	gs_image_file_free(&keyboard->main_image);
	for (size_t i = 0; i < KEYBOARD_KEY_COUNT; i++)
		gs_texture_destroy(keyboard->keys[i].label_texture);
	gs_effect_destroy(keyboard->effect);
	obs_leave_graphics();
}

static void keyboard_resources_draw_key(gs_eparam_t *image_param, gs_eparam_t *opacity_param, gs_texture_t *texture,
					float x, float y, float width, float height, float opacity)
{
	gs_effect_set_texture(image_param, texture);
	gs_effect_set_float(opacity_param, opacity);
	gs_matrix_push();
	gs_matrix_translate3f(x - width / 2.0f, y - height / 2.0f, 0.0f);
	gs_draw_sprite(texture, 0, (uint32_t)width, (uint32_t)height);
	gs_matrix_pop();
}

void keyboard_resources_render(struct keyboard_overlay_gg_data *keyboard)
{
	if (!keyboard->enabled || !keyboard->effect || !keyboard->effect_technique ||
	    !keyboard->effect_circle_technique || !keyboard->effect_border_technique || !keyboard->effect_image ||
	    !keyboard->effect_opacity || !keyboard->effect_tint)
		return;

	if (!keyboard->main_image.texture && keyboard->main_image.loaded)
		gs_image_file_init_texture(&keyboard->main_image);

	const float idle_opacity = keyboard->idle_opacity_pct / 100.0f;
	const float active_opacity = keyboard->active_opacity_pct / 100.0f;
	const uint64_t now_ns = os_gettime_ns();
	float key_opacities[KEYBOARD_KEY_COUNT];
	float circle_opacities[KEYBOARD_KEY_COUNT];
	float border_opacities[KEYBOARD_KEY_COUNT];

	const float tint_red = (float)(keyboard->tint_color & 0xFF) / 255.0f;
	const float tint_green = (float)((keyboard->tint_color >> 8) & 0xFF) / 255.0f;
	const float tint_blue = (float)((keyboard->tint_color >> 16) & 0xFF) / 255.0f;
	const float tint_luminance = tint_red * 0.2126f + tint_green * 0.7152f + tint_blue * 0.0722f;
	const uint32_t label_color = tint_luminance > 0.55f ? KEY_LABEL_COLOR_DARK : KEY_LABEL_COLOR_LIGHT;

	for (size_t key = 0; key < KEYBOARD_KEY_COUNT; key++) {
		const struct keyboard_overlay_key_data *key_data = &keyboard->keys[key];
		const bool use_color = keyboard->feedback != KEYBOARD_FEEDBACK_PULSE;
		const bool use_pulse = keyboard->feedback != KEYBOARD_FEEDBACK_COLOR;
		const float age = key_data->press_time_ns ? (float)(now_ns - key_data->press_time_ns) * 1e-9f
							  : keyboard->pulse_duration;
		float pulse_level = 0.0f;
		if (use_pulse && age < keyboard->pulse_duration) {
			const float t = age / keyboard->pulse_duration;
			pulse_level = sinf((float)M_PI * t);
		}
		const float color_level = use_color ? key_data->color_level : 0.0f;
		const float key_color_level = use_color ? color_level : 0.0f;
		key_opacities[key] = idle_opacity + (active_opacity - idle_opacity) * key_color_level;
		circle_opacities[key] = active_opacity * key_color_level * 0.35f;
		border_opacities[key] = active_opacity * pulse_level * 0.22f;
	}

	struct vec4 tint;
	vec4_from_rgba(&tint, keyboard->tint_color | 0xFF000000);
	gs_effect_set_vec4(keyboard->effect_tint, &tint);

	float base_width;
	float base_height;
	float output_width;
	float output_height;
	keyboard_layout_get_base_dimensions(keyboard, &base_width, &base_height);
	keyboard_layout_get_dimensions(keyboard, &output_width, &output_height);
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

	const size_t passes = gs_technique_begin(keyboard->effect_technique);
	for (size_t pass = 0; pass < passes; pass++) {
		gs_technique_begin_pass(keyboard->effect_technique, pass);
		for (size_t key = 0; key < KEYBOARD_KEY_COUNT; key++) {
			const struct keyboard_overlay_key_data *key_data = &keyboard->keys[key];
			if (key_data->visible && keyboard->main_image.texture && key_opacities[key] > 0.0f)
				keyboard_resources_draw_key(keyboard->effect_image, keyboard->effect_opacity,
							    keyboard->main_image.texture, key_data->x, key_data->y,
							    key_data->width, key_data->height, key_opacities[key]);
		}
		gs_technique_end_pass(keyboard->effect_technique);
	}
	gs_technique_end(keyboard->effect_technique);

	const size_t circle_passes = gs_technique_begin(keyboard->effect_circle_technique);
	for (size_t pass = 0; pass < circle_passes; pass++) {
		gs_technique_begin_pass(keyboard->effect_circle_technique, pass);
		for (size_t key = 0; key < KEYBOARD_KEY_COUNT; key++) {
			const struct keyboard_overlay_key_data *key_data = &keyboard->keys[key];
			const float effect_size = keyboard->key_size * KEY_EFFECT_SIZE_RATIO;
			if (key_data->visible && circle_opacities[key] > 0.001f)
				keyboard_resources_draw_key(keyboard->effect_image, keyboard->effect_opacity,
							    keyboard->main_image.texture, key_data->x, key_data->y,
							    effect_size, effect_size, circle_opacities[key]);
		}
		gs_technique_end_pass(keyboard->effect_circle_technique);
	}
	gs_technique_end(keyboard->effect_circle_technique);

	const size_t border_passes = gs_technique_begin(keyboard->effect_border_technique);
	for (size_t pass = 0; pass < border_passes; pass++) {
		gs_technique_begin_pass(keyboard->effect_border_technique, pass);
		for (size_t key = 0; key < KEYBOARD_KEY_COUNT; key++) {
			const struct keyboard_overlay_key_data *key_data = &keyboard->keys[key];
			if (key_data->visible && border_opacities[key] > 0.001f)
				keyboard_resources_draw_key(keyboard->effect_image, keyboard->effect_opacity,
							    keyboard->main_image.texture, key_data->x, key_data->y,
							    key_data->width, key_data->height, border_opacities[key]);
		}
		gs_technique_end_pass(keyboard->effect_border_technique);
	}
	gs_technique_end(keyboard->effect_border_technique);

	vec4_from_rgba(&tint, label_color | 0xFF000000);
	gs_effect_set_vec4(keyboard->effect_tint, &tint);
	const size_t label_passes = gs_technique_begin(keyboard->effect_technique);
	for (size_t pass = 0; pass < label_passes; pass++) {
		gs_technique_begin_pass(keyboard->effect_technique, pass);
		for (size_t key = 0; key < KEYBOARD_KEY_COUNT; key++) {
			const struct keyboard_overlay_key_data *key_data = &keyboard->keys[key];
			if (!key_data->visible || !key_data->label_texture || key_opacities[key] <= 0.0f)
				continue;
			keyboard_resources_draw_key(keyboard->effect_image, keyboard->effect_opacity,
						    key_data->label_texture, key_data->x, key_data->y, key_data->width,
						    key_data->height, key_opacities[key]);
		}
		gs_technique_end_pass(keyboard->effect_technique);
	}
	gs_technique_end(keyboard->effect_technique);

	gs_matrix_pop();
	gs_effect_set_texture(keyboard->effect_image, NULL);
	gs_blend_state_pop();
}
