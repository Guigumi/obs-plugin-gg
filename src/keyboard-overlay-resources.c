#include "keyboard-overlay-resources.h"

#include "keyboard-overlay-keys.h"
#include "keyboard-overlay-layout.h"

#include <graphics/matrix4.h>
#include <graphics/vec4.h>
#include <util/platform.h>

#include <windows.h>

#include <math.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define KEY_EFFECT_SIZE_RATIO (54.0f / 64.0f)
#define KEY_LABEL_FONT_SIZE_DEFAULT 36
#define KEY_LABEL_REFERENCE_SIZE 64.0f
#define KEY_LABEL_SUPERSAMPLE_MAX 8u
#define KEY_LABEL_TEXTURE_MAX 512u

static gs_texture_t *keyboard_resources_create_label_texture(const char *text, obs_data_t *font, float key_width,
									float key_height, float spacing, bool font_auto_size)
{
	if (!text || !*text || !font || key_width <= 0.0f || key_height <= 0.0f)
		return NULL;
	uint32_t supersample = 1u;
	const float max_key_dimension = fmaxf(key_width, key_height);
	while (supersample < KEY_LABEL_SUPERSAMPLE_MAX &&
	       (uint32_t)ceilf(max_key_dimension * (float)(supersample + 1)) <= KEY_LABEL_TEXTURE_MAX)
		supersample++;
	const uint32_t texture_width = (uint32_t)ceilf(key_width * supersample);
	const uint32_t texture_height = (uint32_t)ceilf(key_height * supersample);

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
	bitmap_info.bmiHeader.biWidth = (LONG)texture_width;
	bitmap_info.bmiHeader.biHeight = -(LONG)texture_height;
	bitmap_info.bmiHeader.biPlanes = 1;
	bitmap_info.bmiHeader.biBitCount = 32;
	bitmap_info.bmiHeader.biCompression = BI_RGB;

	void *pixels = NULL;
	HDC dc = CreateCompatibleDC(NULL);
	HBITMAP bitmap = dc ? CreateDIBSection(dc, &bitmap_info, DIB_RGB_COLORS, &pixels, NULL, 0) : NULL;
	const int configured_font_size = (int)obs_data_get_int(font, "size");
	float relative_font_size;
	if (font_auto_size) {
		float ratio = 0.55f;
		if (spacing < 4.0f)
			ratio *= 0.9f;
		const size_t text_length = strlen(text);
		if (text_length > 3)
			ratio = fminf(ratio, 0.42f);
		if (text_length > 5)
			ratio = fminf(ratio, 0.35f);
		relative_font_size = fminf(key_width, key_height) * ratio;
	} else {
		relative_font_size =
			(configured_font_size > 0 ? configured_font_size : KEY_LABEL_FONT_SIZE_DEFAULT) * fminf(key_width, key_height) /
			KEY_LABEL_REFERENCE_SIZE;
	}
	int font_size = (int)fmaxf(1.0f, roundf(relative_font_size * (float)supersample));
	const int64_t font_flags = obs_data_get_int(font, "flags");
	HFONT gdi_font = CreateFontW(-font_size, 0, 0, 0, (font_flags & OBS_FONT_BOLD) ? FW_BOLD : FW_NORMAL,
				     (font_flags & OBS_FONT_ITALIC) != 0, (font_flags & OBS_FONT_UNDERLINE) != 0,
				     (font_flags & OBS_FONT_STRIKEOUT) != 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
				     CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, wide_face);
	if (font_auto_size && dc && gdi_font) {
		HGDIOBJ old_font = SelectObject(dc, gdi_font);
		SIZE text_size = {0};
		if (GetTextExtentPoint32W(dc, wide_text, (int)wcslen(wide_text), &text_size) && text_size.cx > 0) {
			const float max_text_width = texture_width * 0.86f;
			const float max_text_height = texture_height * 0.86f;
			const float fit_scale = fminf(max_text_width / (float)text_size.cx,
						     max_text_height / (float)text_size.cy);
			if (fit_scale < 1.0f) {
				font_size = (int)fmaxf(1.0f, floorf(font_size * fit_scale));
				SelectObject(dc, old_font);
				DeleteObject(gdi_font);
				gdi_font = CreateFontW(-font_size, 0, 0, 0,
						       (font_flags & OBS_FONT_BOLD) ? FW_BOLD : FW_NORMAL,
						       (font_flags & OBS_FONT_ITALIC) != 0,
						       (font_flags & OBS_FONT_UNDERLINE) != 0,
						       (font_flags & OBS_FONT_STRIKEOUT) != 0, DEFAULT_CHARSET,
						       OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY,
						       DEFAULT_PITCH | FF_DONTCARE, wide_face);
				old_font = SelectObject(dc, gdi_font);
			}
		}
		SelectObject(dc, old_font);
	}
	gs_texture_t *texture = NULL;
	if (dc && bitmap && pixels && gdi_font) {
		HGDIOBJ old_bitmap = SelectObject(dc, bitmap);
		HGDIOBJ old_font = SelectObject(dc, gdi_font);
		PatBlt(dc, 0, 0, (int)texture_width, (int)texture_height, BLACKNESS);
		SetBkMode(dc, OPAQUE);
		SetBkColor(dc, RGB(0, 0, 0));
		SetTextColor(dc, RGB(255, 255, 255));
		RECT rect = {0, 0, (LONG)texture_width, (LONG)texture_height};
		DrawTextW(dc, wide_text, -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

		uint8_t *pixel = pixels;
		for (size_t i = 0; i < (size_t)texture_width * texture_height; i++, pixel += 4) {
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
		texture = gs_texture_create(texture_width, texture_height, GS_BGRA, 1, levels, 0);
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
		const char *text = keyboard_keys_get_character(keyboard, settings, i);
		gs_texture_t *texture = keyboard_resources_create_label_texture(text, font, key->width, key->height,
										keyboard->spacing, keyboard->font_auto_size);
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
	const char *image_paths[] = {"images/key-main.png", "images/h128.png", "images/400.png", "images/v128.png"};
	gs_image_file_t *images[] = {&keyboard->main_image, &keyboard->wide_image, &keyboard->space_image,
					      &keyboard->vertical_image};
	for (size_t i = 0; i < sizeof(images) / sizeof(images[0]); i++) {
		char *image_path = obs_module_file(image_paths[i]);
		gs_image_file_init(images[i], image_path);
		if (!images[i]->loaded)
			blog(LOG_WARNING, "Failed to load image: %s", image_path);
		bfree(image_path);
	}

	char *path = obs_module_file("keyboard.effect");
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
	gs_image_file_free(&keyboard->wide_image);
	gs_image_file_free(&keyboard->space_image);
	gs_image_file_free(&keyboard->vertical_image);
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

static void keyboard_resources_prepare_draw(gs_eparam_t *image_param, gs_eparam_t *opacity_param, gs_texture_t *texture)
{
	gs_effect_set_texture(image_param, texture);
	gs_effect_set_float(opacity_param, 1.0f);
}

static gs_texture_t *keyboard_resources_get_key_texture(struct keyboard_overlay_gg_data *keyboard,
								const struct keyboard_overlay_key_data *key)
{
	if (keyboard->layout_preset == KEYBOARD_LAYOUT_WASD || keyboard->layout_preset == KEYBOARD_LAYOUT_EDITING)
		return keyboard->main_image.texture;
	if (key->width > keyboard->key_size * 4.0f)
		return keyboard->space_image.texture ? keyboard->space_image.texture : keyboard->main_image.texture;
	if (key->height > keyboard->key_size * 1.5f)
		return keyboard->vertical_image.texture ? keyboard->vertical_image.texture : keyboard->main_image.texture;
	if (key->width > keyboard->key_size * 1.5f)
		return keyboard->wide_image.texture ? keyboard->wide_image.texture : keyboard->main_image.texture;
	return keyboard->main_image.texture;
}

void keyboard_resources_render(struct keyboard_overlay_gg_data *keyboard)
{
	if (!keyboard->enabled || !keyboard->effect || !keyboard->effect_technique ||
	    !keyboard->effect_circle_technique || !keyboard->effect_border_technique || !keyboard->effect_image ||
	    !keyboard->effect_opacity || !keyboard->effect_tint)
		return;

	if (!keyboard->main_image.texture && keyboard->main_image.loaded)
		gs_image_file_init_texture(&keyboard->main_image);
	if (!keyboard->wide_image.texture && keyboard->wide_image.loaded)
		gs_image_file_init_texture(&keyboard->wide_image);
	if (!keyboard->space_image.texture && keyboard->space_image.loaded)
		gs_image_file_init_texture(&keyboard->space_image);
	if (!keyboard->vertical_image.texture && keyboard->vertical_image.loaded)
		gs_image_file_init_texture(&keyboard->vertical_image);

	const float idle_opacity = keyboard->idle_opacity_pct / 100.0f;
	const float active_opacity = keyboard->active_opacity_pct / 100.0f;
	const uint64_t now_ns = os_gettime_ns();
	float key_opacities[KEYBOARD_KEY_COUNT];
	float circle_opacities[KEYBOARD_KEY_COUNT];
	float border_opacities[KEYBOARD_KEY_COUNT];
	bool has_circle = false;
	bool has_border = false;

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
		has_circle = has_circle || circle_opacities[key] > 0.001f;
		has_border = has_border || border_opacities[key] > 0.001f;
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

	keyboard_resources_prepare_draw(keyboard->effect_image, keyboard->effect_opacity, keyboard->main_image.texture);
	const size_t passes = gs_technique_begin(keyboard->effect_technique);
	for (size_t pass = 0; pass < passes; pass++) {
		gs_technique_begin_pass(keyboard->effect_technique, pass);
		for (size_t key = 0; key < KEYBOARD_KEY_COUNT; key++) {
			const struct keyboard_overlay_key_data *key_data = &keyboard->keys[key];
			gs_texture_t *key_texture = keyboard_resources_get_key_texture(keyboard, key_data);
			if (key_data->visible && key_texture && key_opacities[key] > 0.0f)
				keyboard_resources_draw_key(keyboard->effect_image, keyboard->effect_opacity,
								    key_texture, key_data->x, key_data->y,
								    key_data->width, key_data->height, key_opacities[key]);
		}
		gs_technique_end_pass(keyboard->effect_technique);
	}
	gs_technique_end(keyboard->effect_technique);

	if (has_circle) {
		keyboard_resources_prepare_draw(keyboard->effect_image, keyboard->effect_opacity,
						keyboard->main_image.texture);
		const size_t circle_passes = gs_technique_begin(keyboard->effect_circle_technique);
		for (size_t pass = 0; pass < circle_passes; pass++) {
			gs_technique_begin_pass(keyboard->effect_circle_technique, pass);
			for (size_t key = 0; key < KEYBOARD_KEY_COUNT; key++) {
				const struct keyboard_overlay_key_data *key_data = &keyboard->keys[key];
				gs_texture_t *key_texture = keyboard_resources_get_key_texture(keyboard, key_data);
				const float effect_width = key_data->width * KEY_EFFECT_SIZE_RATIO;
				const float effect_height = key_data->height * KEY_EFFECT_SIZE_RATIO;
				if (key_data->visible && key_texture && circle_opacities[key] > 0.001f)
					keyboard_resources_draw_key(keyboard->effect_image, keyboard->effect_opacity,
									    key_texture, key_data->x, key_data->y, effect_width, effect_height,
									    circle_opacities[key]);
			}
			gs_technique_end_pass(keyboard->effect_circle_technique);
		}
		gs_technique_end(keyboard->effect_circle_technique);
	}

	if (has_border) {
		keyboard_resources_prepare_draw(keyboard->effect_image, keyboard->effect_opacity,
						keyboard->main_image.texture);
		const size_t border_passes = gs_technique_begin(keyboard->effect_border_technique);
		for (size_t pass = 0; pass < border_passes; pass++) {
			gs_technique_begin_pass(keyboard->effect_border_technique, pass);
			for (size_t key = 0; key < KEYBOARD_KEY_COUNT; key++) {
				const struct keyboard_overlay_key_data *key_data = &keyboard->keys[key];
				gs_texture_t *key_texture = keyboard_resources_get_key_texture(keyboard, key_data);
				if (key_data->visible && key_texture && border_opacities[key] > 0.001f)
					keyboard_resources_draw_key(keyboard->effect_image, keyboard->effect_opacity,
									    key_texture, key_data->x,
								    key_data->y, key_data->width, key_data->height,
								    border_opacities[key]);
			}
			gs_technique_end_pass(keyboard->effect_border_technique);
		}
		gs_technique_end(keyboard->effect_border_technique);
	}

	vec4_from_rgba(&tint, keyboard->font_color | 0xFF000000);
	gs_effect_set_vec4(keyboard->effect_tint, &tint);
	keyboard_resources_prepare_draw(keyboard->effect_image, keyboard->effect_opacity,
					keyboard->keys[0].label_texture);
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
