#include "keyboard-capture.h"
#include "mouse-capture.h"
#include "mouse-overlay-resources.h"

#include <string.h>

long test_left_click_sequence;
long test_right_click_sequence;
bool test_left_button_down;
bool test_right_button_down;

const char *obs_module_text(const char *text)
{
	return text;
}

void keyboard_capture_sample(enum keyboard_capture_layout layout, bool arrow_aliases,
				     struct keyboard_capture_snapshot *snapshot)
{
	(void)layout;
	(void)arrow_aliases;
	memset(snapshot, 0, sizeof(*snapshot));
}

void mouse_capture_get_relative_totals(int64_t *x, int64_t *y)
{
	*x = 0;
	*y = 0;
}

bool mouse_capture_relative_available(void)
{
	return false;
}

bool mouse_capture_detect_game_mode(void)
{
	return false;
}

size_t mouse_capture_get_monitor_count(void)
{
	return 0;
}

bool mouse_capture_sample_position(int monitor_index, float *x, float *y)
{
	(void)monitor_index;
	*x = 0.5f;
	*y = 0.5f;
	return false;
}

void mouse_capture_get_button_sequences(long *left, long *right)
{
	*left = test_left_click_sequence;
	*right = test_right_click_sequence;
}

void mouse_capture_sample_button_sequences(long *left, long *right, bool *left_down, bool *right_down)
{
	*left = test_left_click_sequence;
	*right = test_right_click_sequence;
	*left_down = test_left_button_down;
	*right_down = test_right_button_down;
}

void mouse_resources_draw_sprite(gs_eparam_t *image_param, gs_eparam_t *opacity_param, gs_texture_t *texture,
					 float x, float y, float size, float opacity)
{
	(void)image_param;
	(void)opacity_param;
	(void)texture;
	(void)x;
	(void)y;
	(void)size;
	(void)opacity;
}

void mouse_resources_draw_procedural(gs_eparam_t *opacity_param, float x, float y, float size, float opacity)
{
	(void)opacity_param;
	(void)x;
	(void)y;
	(void)size;
	(void)opacity;
}

void mouse_resources_set_tint(gs_eparam_t *param, uint32_t color)
{
	(void)param;
	(void)color;
}
