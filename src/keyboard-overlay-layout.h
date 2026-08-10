#pragma once

#include "keyboard-overlay-internal.h"

void keyboard_layout_update(struct keyboard_overlay_gg_data *keyboard);
void keyboard_layout_get_base_dimensions(const struct keyboard_overlay_gg_data *keyboard, float *width, float *height);
void keyboard_layout_get_dimensions(const struct keyboard_overlay_gg_data *keyboard, float *width, float *height);
