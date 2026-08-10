/*
Mouse Overlay GG
Copyright (C) 2026 GUI

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.
*/

#pragma once

#include <obs-module.h>
#include <graphics/image-file.h>

#include <math.h>
#include <stdint.h>

#define MOUSE_OVERLAY_WIDTH 1920u
#define MOUSE_OVERLAY_HEIGHT 1080u
#define MOUSE_TRAIL_POINTS_CAPACITY 256u
#define MOUSE_CLICK_EVENTS_CAPACITY 256u
#define MOUSE_TINT_COLOR_DEFAULT 0xFFFFFFu
#define MOUSE_NANOSECONDS_PER_SECOND 1000000000ULL
#define MOUSE_CURSOR_MODE_AUTOMATIC 0
#define MOUSE_CURSOR_MODE_DESKTOP 1
#define MOUSE_CURSOR_MODE_GAME 2

struct mouse_trail_point {
	float x;
	float y;
	uint64_t time_ns;
};

struct mouse_click_event {
	float x;
	float y;
	uint64_t time_ns;
	uint64_t release_time_ns;
	bool left;
	bool tracks_hold;
};

struct mouse_overlay_resources {
	gs_image_file_t cursor_image;
	gs_image_file_t trail_image;
	gs_effect_t *trail_effect;
	gs_technique_t *trail_technique;
	gs_eparam_t *trail_effect_image;
	gs_eparam_t *trail_effect_opacity;
	gs_eparam_t *trail_effect_tint;
	gs_effect_t *click_effect;
	gs_technique_t *click_technique;
	gs_technique_t *cursor_technique;
	gs_eparam_t *click_effect_image;
	gs_eparam_t *click_effect_opacity;
	gs_eparam_t *click_effect_tint;
	gs_eparam_t *click_effect_side;
	gs_eparam_t *click_effect_size;
	volatile bool images_dirty;
};

struct mouse_cursor_state {
	float size;
	float opacity;
	uint32_t tint_color;
	bool enabled;
	bool visible;
	bool tracking_initialized;
	bool relative_initialized;
	bool wrapped;
	int monitor_index;
	int active_monitor_index;
	int configured_mode;
	int active_mode;
	volatile long reported_mode;
	float game_sensitivity;
	int64_t relative_total_x;
	int64_t relative_total_y;
	float x;
	float y;
};

struct mouse_trail_state {
	bool enabled;
	bool shrink;
	uint64_t duration_ns;
	float spacing;
	float size_scale;
	float opacity;
	uint32_t tint_color;
	int fade;
	uint32_t point_limit;
	bool dirty;
	struct mouse_trail_point points[MOUSE_TRAIL_POINTS_CAPACITY];
	size_t count;
	size_t start;
	float last_sample_x;
	float last_sample_y;
	float distance_since_point;
	bool has_sample;
};

struct mouse_click_state {
	bool enabled;
	bool left_enabled;
	bool right_enabled;
	uint64_t duration_ns;
	float opacity;
	uint32_t tint_color;
	int animation;
	size_t count;
	size_t start;
	long seen_left_click_sequence;
	long seen_right_click_sequence;
	bool left_down;
	bool right_down;
	struct mouse_click_event events[MOUSE_CLICK_EVENTS_CAPACITY];
};

struct mouse_overlay_gg_data {
	obs_source_t *source;
	struct mouse_overlay_resources resources;
	struct mouse_cursor_state cursor;
	struct mouse_trail_state trail;
	struct mouse_click_state click;
};

static inline float mouse_clampf(float value, float minimum, float maximum)
{
	return fminf(fmaxf(value, minimum), maximum);
}

static inline float mouse_settings_get_float(obs_data_t *settings, const char *name, float minimum, float maximum,
					     float fallback)
{
	const double value = obs_data_get_double(settings, name);
	return isfinite(value) ? mouse_clampf((float)value, minimum, maximum) : fallback;
}

static inline int mouse_settings_get_int(obs_data_t *settings, const char *name, int minimum, int maximum, int fallback)
{
	const long long value = obs_data_get_int(settings, name);
	if (value < INT32_MIN || value > INT32_MAX)
		return fallback;
	if (value < minimum)
		return minimum;
	if (value > maximum)
		return maximum;
	return (int)value;
}

static inline int mouse_settings_get_enum(obs_data_t *settings, const char *name, int minimum, int maximum,
					  int fallback)
{
	const long long value = obs_data_get_int(settings, name);
	return value >= minimum && value <= maximum ? (int)value : fallback;
}

static inline float mouse_settings_get_opacity(obs_data_t *settings, const char *name, int fallback_pct)
{
	return (float)mouse_settings_get_int(settings, name, 0, 100, fallback_pct) / 100.0f;
}

static inline uint64_t mouse_seconds_to_ns(float seconds)
{
	return (uint64_t)(seconds * (float)MOUSE_NANOSECONDS_PER_SECOND);
}

static inline uint32_t mouse_settings_get_color(obs_data_t *settings, const char *name)
{
	return (uint32_t)obs_data_get_int(settings, name) & 0xFFFFFFu;
}
