#pragma once

#include "mouse-overlay-internal.h"

void mouse_trail_defaults(obs_data_t *settings);
void mouse_trail_add_properties(obs_properties_t *props);
void mouse_trail_update(struct mouse_trail_state *trail, obs_data_t *settings);
void mouse_trail_reset(struct mouse_trail_state *trail);
void mouse_trail_reset_continuity(struct mouse_trail_state *trail);
void mouse_trail_tick(struct mouse_trail_state *trail, const struct mouse_cursor_state *cursor, uint64_t now_ns);
bool mouse_trail_can_render(const struct mouse_trail_state *trail, const struct mouse_overlay_resources *resources);
void mouse_trail_render(const struct mouse_trail_state *trail, const struct mouse_cursor_state *cursor,
			struct mouse_overlay_resources *resources, uint64_t now_ns);
