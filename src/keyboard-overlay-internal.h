#pragma once

#include "keyboard-capture.h"

#include <obs-module.h>
#include <graphics/image-file.h>

#include <stdbool.h>
#include <stdint.h>

#define KEYBOARD_FEEDBACK_COLOR 0
#define KEYBOARD_FEEDBACK_PULSE 1
#define KEYBOARD_FEEDBACK_BOTH 2

struct keyboard_overlay_key_data {
	enum keyboard_overlay_key capture_key;
	const char *character_setting;
	gs_texture_t *label_texture;
	float x;
	float y;
	float width;
	float height;
	bool visible;
	bool pressed;
	long press_sequence;
	float color_level;
	uint64_t press_time_ns;
};

struct keyboard_overlay_gg_data {
	gs_image_file_t main_image;
	gs_effect_t *effect;
	gs_technique_t *effect_technique;
	gs_technique_t *effect_circle_technique;
	gs_technique_t *effect_border_technique;
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
	uint32_t tint_color;
	int feedback;
	struct keyboard_overlay_key_data keys[KEYBOARD_KEY_COUNT];
	bool capture_snapshot_initialized;
};
