#pragma once

#include "mouse-overlay-internal.h"

void mouse_cursor_defaults(obs_data_t *settings);
void mouse_cursor_add_properties(obs_properties_t *props);
void mouse_cursor_update(struct mouse_cursor_state *cursor, obs_data_t *settings);
bool mouse_cursor_tick(struct mouse_cursor_state *cursor);
bool mouse_cursor_can_render(const struct mouse_cursor_state *cursor, const struct mouse_overlay_resources *resources);
void mouse_cursor_render(const struct mouse_cursor_state *cursor, struct mouse_overlay_resources *resources);
