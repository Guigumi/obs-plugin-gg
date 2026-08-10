#pragma once

#include "keyboard-capture.h"

#include <obs-module.h>
#include <graphics/image-file.h>

#include <stdbool.h>
#include <stdint.h>

#define KEYBOARD_FEEDBACK_COLOR 0
#define KEYBOARD_FEEDBACK_PULSE 1
#define KEYBOARD_FEEDBACK_BOTH 2
#define KEYBOARD_ANIMATION_BASIC 0
#define KEYBOARD_ANIMATION_FULL 1
#define KEYBOARD_RENDER_IMAGE 0
#define KEYBOARD_RENDER_PROCEDURAL 1

struct keyboard_overlay_key_data {
	enum keyboard_overlay_key capture_key;
	const char *character_setting;
	gs_texture_t *label_texture;
	uint64_t label_cache_key;
	bool label_cache_valid;
	float x;
	float y;
	float width;
	float height;
	float normalized_x;
	float normalized_y;
	bool visible;
	bool pressed;
	long press_sequence;
	float color_level;
	uint64_t press_time_ns;
};

struct keyboard_overlay_gg_data {
	gs_image_file_t main_image;
	gs_image_file_t wide_image;
	gs_image_file_t space_image;
	gs_image_file_t vertical_image;
	gs_texture_t *procedural_texture;
	gs_effect_t *effect;
	gs_technique_t *effect_technique;
	gs_technique_t *effect_circle_technique;
	gs_technique_t *effect_border_technique;
	gs_technique_t *effect_procedural_technique;
	gs_eparam_t *effect_image;
	gs_eparam_t *effect_opacity;
	gs_eparam_t *effect_tint;
	gs_eparam_t *effect_corner_radius;

	bool enabled;
	bool arrow_aliases;
	enum keyboard_capture_layout layout_preset;
	float key_size;
	float spacing;
	bool font_auto_size;
	float idle_opacity_pct;
	float active_opacity_pct;
	float fade_duration;
	float pulse_duration;
	float rotation_deg;
	uint32_t tint_color;
	uint32_t font_color;
	int feedback;
	int animation_style;
	int render_style;
	struct keyboard_overlay_key_data keys[KEYBOARD_KEY_COUNT];
	bool capture_snapshot_initialized;
};
