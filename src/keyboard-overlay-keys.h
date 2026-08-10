#pragma once

#include "keyboard-overlay-internal.h"

void keyboard_keys_initialize(struct keyboard_overlay_gg_data *keyboard);
void keyboard_keys_defaults(obs_data_t *settings);
void keyboard_keys_add_properties(obs_properties_t *props, enum keyboard_capture_layout initial_layout);
void keyboard_keys_update(struct keyboard_overlay_gg_data *keyboard, obs_data_t *settings);
void keyboard_keys_tick(struct keyboard_overlay_gg_data *keyboard, float seconds);
const char *keyboard_keys_get_character(const struct keyboard_overlay_gg_data *keyboard, obs_data_t *settings,
					 size_t index);
