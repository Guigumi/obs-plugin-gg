#include "keyboard-overlay-layout.h"

#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

void keyboard_layout_update(struct keyboard_overlay_gg_data *keyboard)
{
	const float step = keyboard->key_size + keyboard->spacing;
	const float half_size = keyboard->key_size / 2.0f;
	for (size_t i = 0; i < KEYBOARD_KEY_COUNT; i++) {
		keyboard->keys[i].width = keyboard->key_size;
		keyboard->keys[i].height = keyboard->key_size;
	}
	keyboard->keys[KEYBOARD_KEY_W].x = step + half_size;
	keyboard->keys[KEYBOARD_KEY_W].y = half_size;
	keyboard->keys[KEYBOARD_KEY_A].x = half_size;
	keyboard->keys[KEYBOARD_KEY_A].y = step + half_size;
	keyboard->keys[KEYBOARD_KEY_S].x = step + half_size;
	keyboard->keys[KEYBOARD_KEY_S].y = step + half_size;
	keyboard->keys[KEYBOARD_KEY_D].x = step * 2.0f + half_size;
	keyboard->keys[KEYBOARD_KEY_D].y = step + half_size;
}

void keyboard_layout_get_base_dimensions(const struct keyboard_overlay_gg_data *keyboard, float *width, float *height)
{
	*width = keyboard->key_size * 3.0f + keyboard->spacing * 2.0f;
	*height = keyboard->key_size * 2.0f + keyboard->spacing;
}

void keyboard_layout_get_dimensions(const struct keyboard_overlay_gg_data *keyboard, float *width, float *height)
{
	float base_width;
	float base_height;
	keyboard_layout_get_base_dimensions(keyboard, &base_width, &base_height);
	const float radians = keyboard->rotation_deg * (float)M_PI / 180.0f;
	const float cosine = fabsf(cosf(radians));
	const float sine = fabsf(sinf(radians));
	*width = base_width * cosine + base_height * sine;
	*height = base_width * sine + base_height * cosine;
}
