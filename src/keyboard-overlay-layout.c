#include "keyboard-overlay-layout.h"

#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static void keyboard_layout_set_key(struct keyboard_overlay_gg_data *keyboard, enum keyboard_overlay_key key,
					float x, float y, float width, float height)
{
	const float step = keyboard->key_size + keyboard->spacing;
	const float key_width = keyboard->key_size * width;
	const float key_height = keyboard->key_size * height;
	keyboard->keys[key].width = key_width;
	keyboard->keys[key].height = key_height;
	keyboard->keys[key].x = x * step + key_width / 2.0f;
	keyboard->keys[key].y = y * step + key_height / 2.0f;
}

static void keyboard_layout_place_key(struct keyboard_overlay_gg_data *keyboard, enum keyboard_overlay_key key,
					     float *x, float y, float width, float height)
{
	const float key_width = keyboard->key_size * width;
	const float key_height = keyboard->key_size * height;
	keyboard->keys[key].width = key_width;
	keyboard->keys[key].height = key_height;
	keyboard->keys[key].x = *x + key_width / 2.0f;
	keyboard->keys[key].y = y * (keyboard->key_size + keyboard->spacing) + key_height / 2.0f;
	*x += key_width + keyboard->spacing;
}

static void keyboard_layout_place_vertical_key(struct keyboard_overlay_gg_data *keyboard,
						       enum keyboard_overlay_key key, float *x, float y)
{
	const float key_width = keyboard->key_size;
	const float key_height = keyboard->key_size * 2.0f + keyboard->spacing;
	keyboard->keys[key].width = key_width;
	keyboard->keys[key].height = key_height;
	keyboard->keys[key].x = *x + key_width / 2.0f;
	keyboard->keys[key].y = y * (keyboard->key_size + keyboard->spacing) + key_height / 2.0f;
	*x += key_width + keyboard->spacing;
}

static void keyboard_layout_set_wasd(struct keyboard_overlay_gg_data *keyboard)
{
	keyboard_layout_set_key(keyboard, KEYBOARD_KEY_W, 1.0f, 0.0f, 1.0f, 1.0f);
	keyboard_layout_set_key(keyboard, KEYBOARD_KEY_A, 0.0f, 1.0f, 1.0f, 1.0f);
	keyboard_layout_set_key(keyboard, KEYBOARD_KEY_S, 1.0f, 1.0f, 1.0f, 1.0f);
	keyboard_layout_set_key(keyboard, KEYBOARD_KEY_D, 2.0f, 1.0f, 1.0f, 1.0f);
}

static void keyboard_layout_set_full(struct keyboard_overlay_gg_data *keyboard)
{
	float x;
	const float main_width = keyboard->key_size * 15.0f + keyboard->spacing * 13.0f;
	const float standard_row_start =
		(main_width - (keyboard->key_size * 15.0f + keyboard->spacing * 13.0f)) / 2.0f;
	const float bottom_row_start =
		(main_width - (keyboard->key_size * 13.25f + keyboard->spacing * 7.0f)) / 2.0f;

	/* Function row. Group gaps are independent of key width. */
	x = 0.0f;
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_ESCAPE, &x, 0.0f, 1.0f, 1.0f);
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_F1, &x, 0.0f, 1.0f, 1.0f);
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_F2, &x, 0.0f, 1.0f, 1.0f);
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_F3, &x, 0.0f, 1.0f, 1.0f);
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_F4, &x, 0.0f, 1.0f, 1.0f);
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_F5, &x, 0.0f, 1.0f, 1.0f);
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_F6, &x, 0.0f, 1.0f, 1.0f);
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_F7, &x, 0.0f, 1.0f, 1.0f);
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_F8, &x, 0.0f, 1.0f, 1.0f);
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_F9, &x, 0.0f, 1.0f, 1.0f);
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_F10, &x, 0.0f, 1.0f, 1.0f);
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_F11, &x, 0.0f, 1.0f, 1.0f);
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_F12, &x, 0.0f, 1.0f, 1.0f);
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_PRINT_SCREEN, &x, 0.0f, 1.0f, 1.0f);
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_SCROLL_LOCK, &x, 0.0f, 1.0f, 1.0f);
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_PAUSE, &x, 0.0f, 1.0f, 1.0f);

	/* Main rows. */
	x = standard_row_start;
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_GRAVE, &x, 1.0f, 1.0f, 1.0f);
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_1, &x, 1.0f, 1.0f, 1.0f);
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_2, &x, 1.0f, 1.0f, 1.0f);
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_3, &x, 1.0f, 1.0f, 1.0f);
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_4, &x, 1.0f, 1.0f, 1.0f);
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_5, &x, 1.0f, 1.0f, 1.0f);
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_6, &x, 1.0f, 1.0f, 1.0f);
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_7, &x, 1.0f, 1.0f, 1.0f);
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_8, &x, 1.0f, 1.0f, 1.0f);
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_9, &x, 1.0f, 1.0f, 1.0f);
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_0, &x, 1.0f, 1.0f, 1.0f);
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_MINUS, &x, 1.0f, 1.0f, 1.0f);
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_EQUAL, &x, 1.0f, 1.0f, 1.0f);
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_BACKSPACE, &x, 1.0f, 2.0f, 1.0f);

	x = standard_row_start;
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_TAB, &x, 2.0f, 2.0f, 1.0f);
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_Q, &x, 2.0f, 1.0f, 1.0f);
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_W, &x, 2.0f, 1.0f, 1.0f);
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_E, &x, 2.0f, 1.0f, 1.0f);
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_R, &x, 2.0f, 1.0f, 1.0f);
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_T, &x, 2.0f, 1.0f, 1.0f);
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_Y, &x, 2.0f, 1.0f, 1.0f);
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_U, &x, 2.0f, 1.0f, 1.0f);
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_I, &x, 2.0f, 1.0f, 1.0f);
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_O, &x, 2.0f, 1.0f, 1.0f);
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_P, &x, 2.0f, 1.0f, 1.0f);
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_LEFT_BRACKET, &x, 2.0f, 1.0f, 1.0f);
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_RIGHT_BRACKET, &x, 2.0f, 1.0f, 1.0f);
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_BACKSLASH, &x, 2.0f, 1.0f, 1.0f);

	x = standard_row_start;
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_CAPS, &x, 3.0f, 2.0f, 1.0f);
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_A, &x, 3.0f, 1.0f, 1.0f);
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_S, &x, 3.0f, 1.0f, 1.0f);
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_D, &x, 3.0f, 1.0f, 1.0f);
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_F, &x, 3.0f, 1.0f, 1.0f);
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_G, &x, 3.0f, 1.0f, 1.0f);
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_H, &x, 3.0f, 1.0f, 1.0f);
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_J, &x, 3.0f, 1.0f, 1.0f);
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_K, &x, 3.0f, 1.0f, 1.0f);
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_L, &x, 3.0f, 1.0f, 1.0f);
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_SEMICOLON, &x, 3.0f, 1.0f, 1.0f);
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_APOSTROPHE, &x, 3.0f, 1.0f, 1.0f);
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_ENTER, &x, 3.0f, 2.0f, 1.0f);

	x = standard_row_start;
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_SHIFT, &x, 4.0f, 2.0f, 1.0f);
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_Z, &x, 4.0f, 1.0f, 1.0f);
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_X, &x, 4.0f, 1.0f, 1.0f);
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_C, &x, 4.0f, 1.0f, 1.0f);
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_V, &x, 4.0f, 1.0f, 1.0f);
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_B, &x, 4.0f, 1.0f, 1.0f);
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_N, &x, 4.0f, 1.0f, 1.0f);
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_M, &x, 4.0f, 1.0f, 1.0f);
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_COMMA, &x, 4.0f, 1.0f, 1.0f);
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_PERIOD, &x, 4.0f, 1.0f, 1.0f);
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_SLASH, &x, 4.0f, 1.0f, 1.0f);
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_RIGHT_SHIFT, &x, 4.0f, 2.0f, 1.0f);

	x = bottom_row_start;
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_CTRL, &x, 5.0f, 1.0f, 1.0f);
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_LEFT_WINDOWS, &x, 5.0f, 1.0f, 1.0f);
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_LEFT_ALT, &x, 5.0f, 1.0f, 1.0f);
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_SPACE, &x, 5.0f, 6.25f, 1.0f);
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_RIGHT_ALT, &x, 5.0f, 1.0f, 1.0f);
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_RIGHT_WINDOWS, &x, 5.0f, 1.0f, 1.0f);
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_MENU, &x, 5.0f, 1.0f, 1.0f);
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_RIGHT_CTRL, &x, 5.0f, 1.0f, 1.0f);

	/* Navigation cluster and numeric keypad. */
	const float nav_x = main_width + keyboard->spacing;
	x = nav_x;
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_INSERT, &x, 1.0f, 1.0f, 1.0f);
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_HOME, &x, 1.0f, 1.0f, 1.0f);
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_PAGE_UP, &x, 1.0f, 1.0f, 1.0f);
	x = nav_x;
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_DELETE, &x, 2.0f, 1.0f, 1.0f);
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_END, &x, 2.0f, 1.0f, 1.0f);
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_PAGE_DOWN, &x, 2.0f, 1.0f, 1.0f);
	x = nav_x + keyboard->key_size + keyboard->spacing;
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_UP, &x, 3.0f, 1.0f, 1.0f);
	x = nav_x;
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_LEFT, &x, 4.0f, 1.0f, 1.0f);
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_DOWN, &x, 4.0f, 1.0f, 1.0f);
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_RIGHT, &x, 4.0f, 1.0f, 1.0f);

	const float numpad_x = nav_x + keyboard->key_size * 3.0f + keyboard->spacing * 3.0f;
	x = numpad_x;
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_NUM_LOCK, &x, 1.0f, 1.0f, 1.0f);
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_NUMPAD_DIVIDE, &x, 1.0f, 1.0f, 1.0f);
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_NUMPAD_MULTIPLY, &x, 1.0f, 1.0f, 1.0f);
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_NUMPAD_SUBTRACT, &x, 1.0f, 1.0f, 1.0f);
	x = numpad_x;
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_NUMPAD_7, &x, 2.0f, 1.0f, 1.0f);
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_NUMPAD_8, &x, 2.0f, 1.0f, 1.0f);
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_NUMPAD_9, &x, 2.0f, 1.0f, 1.0f);
	keyboard_layout_place_vertical_key(keyboard, KEYBOARD_KEY_NUMPAD_ADD, &x, 2.0f);
	x = numpad_x;
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_NUMPAD_4, &x, 3.0f, 1.0f, 1.0f);
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_NUMPAD_5, &x, 3.0f, 1.0f, 1.0f);
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_NUMPAD_6, &x, 3.0f, 1.0f, 1.0f);
	x = numpad_x;
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_NUMPAD_1, &x, 4.0f, 1.0f, 1.0f);
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_NUMPAD_2, &x, 4.0f, 1.0f, 1.0f);
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_NUMPAD_3, &x, 4.0f, 1.0f, 1.0f);
	keyboard_layout_place_vertical_key(keyboard, KEYBOARD_KEY_NUMPAD_ENTER, &x, 4.0f);
	x = numpad_x;
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_NUMPAD_0, &x, 5.0f, 2.0f, 1.0f);
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_NUMPAD_DECIMAL, &x, 5.0f, 1.0f, 1.0f);
}

static void keyboard_layout_set_editing(struct keyboard_overlay_gg_data *keyboard)
{
	const float step = keyboard->key_size + keyboard->spacing;
	float x = 0.0f;
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_PRINT_SCREEN, &x, 0.0f, 1.0f, 1.0f);
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_SCROLL_LOCK, &x, 0.0f, 1.0f, 1.0f);
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_PAUSE, &x, 0.0f, 1.0f, 1.0f);
	x = 0.0f;
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_INSERT, &x, 1.0f, 1.0f, 1.0f);
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_HOME, &x, 1.0f, 1.0f, 1.0f);
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_PAGE_UP, &x, 1.0f, 1.0f, 1.0f);
	x = 0.0f;
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_DELETE, &x, 2.0f, 1.0f, 1.0f);
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_END, &x, 2.0f, 1.0f, 1.0f);
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_PAGE_DOWN, &x, 2.0f, 1.0f, 1.0f);
	x = step;
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_UP, &x, 3.0f, 1.0f, 1.0f);
	x = 0.0f;
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_LEFT, &x, 4.0f, 1.0f, 1.0f);
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_DOWN, &x, 4.0f, 1.0f, 1.0f);
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_RIGHT, &x, 4.0f, 1.0f, 1.0f);
}

static void keyboard_layout_set_numpad(struct keyboard_overlay_gg_data *keyboard)
{
	float x = 0.0f;
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_NUM_LOCK, &x, 0.0f, 1.0f, 1.0f);
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_NUMPAD_DIVIDE, &x, 0.0f, 1.0f, 1.0f);
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_NUMPAD_MULTIPLY, &x, 0.0f, 1.0f, 1.0f);
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_NUMPAD_SUBTRACT, &x, 0.0f, 1.0f, 1.0f);
	x = 0.0f;
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_NUMPAD_7, &x, 1.0f, 1.0f, 1.0f);
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_NUMPAD_8, &x, 1.0f, 1.0f, 1.0f);
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_NUMPAD_9, &x, 1.0f, 1.0f, 1.0f);
	keyboard_layout_place_vertical_key(keyboard, KEYBOARD_KEY_NUMPAD_ADD, &x, 1.0f);
	x = 0.0f;
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_NUMPAD_4, &x, 2.0f, 1.0f, 1.0f);
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_NUMPAD_5, &x, 2.0f, 1.0f, 1.0f);
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_NUMPAD_6, &x, 2.0f, 1.0f, 1.0f);
	x = 0.0f;
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_NUMPAD_1, &x, 3.0f, 1.0f, 1.0f);
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_NUMPAD_2, &x, 3.0f, 1.0f, 1.0f);
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_NUMPAD_3, &x, 3.0f, 1.0f, 1.0f);
	keyboard_layout_place_vertical_key(keyboard, KEYBOARD_KEY_NUMPAD_ENTER, &x, 3.0f);
	x = 0.0f;
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_NUMPAD_0, &x, 4.0f, 2.0f, 1.0f);
	keyboard_layout_place_key(keyboard, KEYBOARD_KEY_NUMPAD_DECIMAL, &x, 4.0f, 1.0f, 1.0f);
}

void keyboard_layout_update(struct keyboard_overlay_gg_data *keyboard)
{
	for (size_t i = 0; i < KEYBOARD_KEY_COUNT; i++) {
		keyboard->keys[i].width = keyboard->key_size;
		keyboard->keys[i].height = keyboard->key_size;
		keyboard->keys[i].x = keyboard->key_size / 2.0f;
		keyboard->keys[i].y = keyboard->key_size / 2.0f;
	}

	if (keyboard->layout_preset == KEYBOARD_LAYOUT_100)
		keyboard_layout_set_full(keyboard);
	else if (keyboard->layout_preset == KEYBOARD_LAYOUT_EDITING)
		keyboard_layout_set_editing(keyboard);
	else if (keyboard->layout_preset == KEYBOARD_LAYOUT_NUMPAD)
		keyboard_layout_set_numpad(keyboard);
	else
		keyboard_layout_set_wasd(keyboard);
}

void keyboard_layout_get_base_dimensions(const struct keyboard_overlay_gg_data *keyboard, float *width, float *height)
{
	if (keyboard->layout_preset == KEYBOARD_LAYOUT_100) {
		*width = keyboard->key_size * 15.0f + keyboard->spacing * 13.0f;
		*height = keyboard->key_size * 6.0f + keyboard->spacing * 5.0f;
	} else if (keyboard->layout_preset == KEYBOARD_LAYOUT_EDITING) {
		*width = keyboard->key_size * 3.0f + keyboard->spacing * 2.0f;
		*height = keyboard->key_size * 5.0f + keyboard->spacing * 4.0f;
	} else if (keyboard->layout_preset == KEYBOARD_LAYOUT_NUMPAD) {
		*width = keyboard->key_size * 4.0f + keyboard->spacing * 3.0f;
		*height = keyboard->key_size * 5.0f + keyboard->spacing * 4.0f;
	} else {
		*width = keyboard->key_size * 3.0f + keyboard->spacing * 2.0f;
		*height = keyboard->key_size * 2.0f + keyboard->spacing;
	}
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
