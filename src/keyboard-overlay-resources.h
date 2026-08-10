#pragma once

#include "keyboard-overlay-internal.h"

void keyboard_resources_init(struct keyboard_overlay_gg_data *keyboard);
void keyboard_resources_free(struct keyboard_overlay_gg_data *keyboard);
void keyboard_resources_update_labels(struct keyboard_overlay_gg_data *keyboard, obs_data_t *settings);
void keyboard_resources_render(struct keyboard_overlay_gg_data *keyboard);
